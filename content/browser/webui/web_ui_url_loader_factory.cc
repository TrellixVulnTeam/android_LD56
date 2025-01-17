// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webui/web_ui_url_loader_factory.h"

#include <map>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_piece.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/resource_context_impl.h"
#include "content/browser/webui/url_data_manager_backend.h"
#include "content/browser/webui/url_data_source_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/zlib/google/compression_utils.h"
#include "ui/base/template_expressions.h"

namespace content {

namespace {

class WebUIURLLoaderFactory;
base::LazyInstance<std::map<int, std::unique_ptr<WebUIURLLoaderFactory>>>::Leaky
    g_factories = LAZY_INSTANCE_INITIALIZER;

void CallOnError(mojom::URLLoaderClientPtrInfo client_info, int error_code) {
  mojom::URLLoaderClientPtr client;
  client.Bind(std::move(client_info));

  ResourceRequestCompletionStatus status;
  status.error_code = error_code;
  client->OnComplete(status);
}

void ReadData(scoped_refptr<ResourceResponse> headers,
              const ui::TemplateReplacements* replacements,
              bool gzipped,
              scoped_refptr<URLDataSourceImpl> data_source,
              mojom::URLLoaderClientPtrInfo client_info,
              scoped_refptr<base::RefCountedMemory> bytes) {
  if (!bytes) {
    CallOnError(std::move(client_info), net::ERR_FAILED);
    return;
  }

  mojom::URLLoaderClientPtr client;
  client.Bind(std::move(client_info));
  client->OnReceiveResponse(headers->head, base::nullopt, nullptr);

  base::StringPiece input(reinterpret_cast<const char*>(bytes->front()),
                          bytes->size());

  if (replacements) {
    std::string temp_string;
    // We won't know the the final output size ahead of time, so we have to
    // use an intermediate string.
    base::StringPiece source;
    std::string temp_str;
    if (gzipped) {
      temp_str.resize(compression::GetUncompressedSize(input));
      source.set(temp_str.c_str(), temp_str.size());
      CHECK(compression::GzipUncompress(input, source));
      gzipped = false;
    } else {
      source = input;
    }
    temp_str = ui::ReplaceTemplateExpressions(source, *replacements);
    bytes = base::RefCountedString::TakeString(&temp_str);
    input.set(reinterpret_cast<const char*>(bytes->front()), bytes->size());
  }

  uint32_t output_size =
      gzipped ? compression::GetUncompressedSize(input) : bytes->size();

  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes = output_size;
  mojo::DataPipe data_pipe(options);

  DCHECK(data_pipe.producer_handle.is_valid());
  DCHECK(data_pipe.consumer_handle.is_valid());

  void* buffer = nullptr;
  uint32_t num_bytes = output_size;
  MojoResult result =
      BeginWriteDataRaw(data_pipe.producer_handle.get(), &buffer, &num_bytes,
                        MOJO_WRITE_DATA_FLAG_NONE);
  CHECK_EQ(result, MOJO_RESULT_OK);
  CHECK_EQ(num_bytes, output_size);

  if (gzipped) {
    base::StringPiece output(static_cast<char*>(buffer), num_bytes);
    CHECK(compression::GzipUncompress(input, output));
  } else {
    memcpy(buffer, bytes->front(), output_size);
  }
  result = EndWriteDataRaw(data_pipe.producer_handle.get(), num_bytes);
  CHECK_EQ(result, MOJO_RESULT_OK);

  client->OnStartLoadingResponseBody(std::move(data_pipe.consumer_handle));

  ResourceRequestCompletionStatus request_complete_data;
  request_complete_data.error_code = net::OK;
  request_complete_data.exists_in_cache = false;
  request_complete_data.completion_time = base::TimeTicks::Now();
  request_complete_data.encoded_data_length = output_size;
  request_complete_data.encoded_body_length = output_size;
  client->OnComplete(request_complete_data);
}

void DataAvailable(scoped_refptr<ResourceResponse> headers,
                   const ui::TemplateReplacements* replacements,
                   bool gzipped,
                   scoped_refptr<URLDataSourceImpl> source,
                   mojom::URLLoaderClientPtrInfo client_info,
                   scoped_refptr<base::RefCountedMemory> bytes) {
  // Since the bytes are from the memory mapped resource file, copying the
  // data can lead to disk access.
  // TODO(jam): once http://crbug.com/678155 is fixed, use task scheduler:
  // base::PostTaskWithTraits(
  //     FROM_HERE,
  //     {base::TaskPriority::USER_BLOCKING,
  //       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
  BrowserThread::PostTask(
      BrowserThread::FILE_USER_BLOCKING, FROM_HERE,
      base::BindOnce(ReadData, headers, replacements, gzipped, source,
                     std::move(client_info), bytes));
}

void StartURLLoader(const ResourceRequest& request,
                    int frame_tree_node_id,
                    mojom::URLLoaderClientPtrInfo client_info,
                    ResourceContext* resource_context) {
  // NOTE: this duplicates code in URLDataManagerBackend::StartRequest.
  if (!URLDataManagerBackend::CheckURLIsValid(request.url)) {
    CallOnError(std::move(client_info), net::ERR_INVALID_URL);
    return;
  }

  URLDataSourceImpl* source =
      GetURLDataManagerForResourceContext(resource_context)
          ->GetDataSourceFromURL(request.url);
  if (!source) {
    CallOnError(std::move(client_info), net::ERR_INVALID_URL);
    return;
  }

  if (!source->source()->ShouldServiceRequest(request.url, resource_context,
                                              -1)) {
    CallOnError(std::move(client_info), net::ERR_INVALID_URL);
    return;
  }

  std::string path;
  URLDataManagerBackend::URLToRequestPath(request.url, &path);

  net::HttpRequestHeaders request_headers;
  request_headers.AddHeadersFromString(request.headers);
  std::string origin_header;
  request_headers.GetHeader(net::HttpRequestHeaders::kOrigin, &origin_header);

  scoped_refptr<net::HttpResponseHeaders> headers =
      URLDataManagerBackend::GetHeaders(source, path, origin_header);

  scoped_refptr<ResourceResponse> resource_response(new ResourceResponse);
  resource_response->head.headers = headers;
  resource_response->head.mime_type = source->source()->GetMimeType(path);
  // TODO: fill all the time related field i.e. request_time response_time
  // request_start response_start

  ResourceRequestInfo::WebContentsGetter wc_getter =
      base::Bind(WebContents::FromFrameTreeNodeId, frame_tree_node_id);

  bool gzipped = source->source()->IsGzipped(path);
  const ui::TemplateReplacements* replacements = nullptr;
  if (source->source()->GetMimeType(path) == "text/html")
    replacements = source->GetReplacements();
  // To keep the same behavior as the old WebUI code, we call the source to get
  // the value for |gzipped| and |replacements| on the IO thread. Since
  // |replacements| is owned by |source| keep a reference to it in the callback.
  auto data_available_callback =
      base::Bind(DataAvailable, resource_response, replacements, gzipped,
                 base::RetainedRef(source), base::Passed(&client_info));

  // TODO(jam): once we only have this code path for WebUI, and not the
  // URLLRequestJob one, then we should switch data sources to run on the UI
  // thread by default.
  scoped_refptr<base::SingleThreadTaskRunner> target_runner =
      source->source()->TaskRunnerForRequestPath(path);
  if (!target_runner) {
    source->source()->StartDataRequest(path, wc_getter,
                                       data_available_callback);
    return;
  }

  // The DataSource wants StartDataRequest to be called on a specific
  // thread, usually the UI thread, for this path.
  target_runner->PostTask(
      FROM_HERE, base::BindOnce(&URLDataSource::StartDataRequest,
                                base::Unretained(source->source()), path,
                                wc_getter, data_available_callback));
}

class WebUIURLLoaderFactory : public mojom::URLLoaderFactory,
                              public FrameTreeNode::Observer {
 public:
  WebUIURLLoaderFactory(FrameTreeNode* ftn)
      : frame_tree_node_id_(ftn->frame_tree_node_id()),
        resource_context_(ftn->current_frame_host()
                              ->GetProcess()
                              ->GetBrowserContext()
                              ->GetResourceContext()) {
    ftn->AddObserver(this);
  }

  ~WebUIURLLoaderFactory() override {}

  mojom::URLLoaderFactoryPtr CreateBinding() {
    return loader_factory_bindings_.CreateInterfacePtrAndBind(this);
  }

  // mojom::URLLoaderFactory implementation:
  void CreateLoaderAndStart(mojom::URLLoaderAssociatedRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& request,
                            mojom::URLLoaderClientPtr client) override {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&StartURLLoader, request, frame_tree_node_id_,
                       client.PassInterface(), resource_context_));
  }

  void SyncLoad(int32_t routing_id,
                int32_t request_id,
                const ResourceRequest& request,
                SyncLoadCallback callback) override {
    NOTREACHED();
  }

  // FrameTreeNode::Observer implementation:
  void OnFrameTreeNodeDestroyed(FrameTreeNode* node) override {
    g_factories.Get().erase(frame_tree_node_id_);
  }

 private:
  int frame_tree_node_id_;
  ResourceContext* resource_context_;
  mojo::BindingSet<mojom::URLLoaderFactory> loader_factory_bindings_;

  DISALLOW_COPY_AND_ASSIGN(WebUIURLLoaderFactory);
};

}  // namespace

mojom::URLLoaderFactoryPtr GetWebUIURLLoader(FrameTreeNode* node) {
  int ftn_id = node->frame_tree_node_id();
  if (g_factories.Get()[ftn_id].get() == nullptr)
    g_factories.Get()[ftn_id] = base::MakeUnique<WebUIURLLoaderFactory>(node);
  return g_factories.Get()[ftn_id]->CreateBinding();
}

}  // namespace content
