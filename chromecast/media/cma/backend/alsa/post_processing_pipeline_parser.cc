// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/alsa/post_processing_pipeline_parser.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/values.h"
#include "chromecast/base/serializers.h"
#include "chromecast/media/base/audio_device_ids.h"
#include "media/audio/audio_device_description.h"

namespace chromecast {
namespace media {

namespace {

const base::FilePath::CharType kPostProcessingPipelineFilePath[] =
    FILE_PATH_LITERAL("/etc/cast_audio.json");

const char kOutputStreamsKey[] = "output_streams";
const char kMixPipelineKey[] = "mix";
const char kLinearizePipelineKey[] = "linearize";
const char kProcessorsKey[] = "processors";
const char kStreamsKey[] = "streams";

}  // namespace

StreamPipelineDescriptor::StreamPipelineDescriptor(
    base::ListValue* pipeline_in,
    const std::unordered_set<std::string>& stream_types_in)
    : pipeline(pipeline_in), stream_types(stream_types_in) {}

StreamPipelineDescriptor::~StreamPipelineDescriptor() = default;

StreamPipelineDescriptor::StreamPipelineDescriptor(
    const StreamPipelineDescriptor& other)
    : StreamPipelineDescriptor(other.pipeline, other.stream_types) {}

PostProcessingPipelineParser::PostProcessingPipelineParser(
    const std::string& json) {
  if (json.empty() &&
      !base::PathExists(base::FilePath(kPostProcessingPipelineFilePath))) {
    LOG(WARNING) << "Could not open post-processing config in "
                 << kPostProcessingPipelineFilePath << ".";
    return;
  }

  if (json.empty()) {
    config_dict_ = base::DictionaryValue::From(DeserializeJsonFromFile(
        base::FilePath(kPostProcessingPipelineFilePath)));
  } else {
    config_dict_ = base::DictionaryValue::From(DeserializeFromJson(json));
  }

  CHECK(config_dict_) << "Invalid JSON in " << kPostProcessingPipelineFilePath;
}

PostProcessingPipelineParser::~PostProcessingPipelineParser() = default;

std::vector<StreamPipelineDescriptor>
PostProcessingPipelineParser::GetStreamPipelines() {
  base::ListValue* pipelines_list;
  std::vector<StreamPipelineDescriptor> descriptors;
  if (!config_dict_ ||
      !config_dict_->GetList(kOutputStreamsKey, &pipelines_list)) {
    LOG(WARNING) << "No post-processors found for streams (key = "
                 << kOutputStreamsKey
                 << ").\n No stream-specific processing will occur.";
    return descriptors;
  }
  for (size_t i = 0; i < pipelines_list->GetSize(); ++i) {
    base::DictionaryValue* pipeline_description_dict;
    CHECK(pipelines_list->GetDictionary(i, &pipeline_description_dict));

    base::ListValue* processors_list;
    CHECK(pipeline_description_dict->GetList(kProcessorsKey, &processors_list));

    base::ListValue* streams_list;
    CHECK(pipeline_description_dict->GetList(kStreamsKey, &streams_list));
    std::unordered_set<std::string> streams_set;
    for (size_t stream = 0; stream < streams_list->GetSize(); ++stream) {
      std::string stream_name;
      CHECK(streams_list->GetString(stream, &stream_name));
      CHECK(streams_set.insert(stream_name).second)
          << "Duplicate stream type: " << stream_name;
    }

    descriptors.emplace_back(processors_list, std::move(streams_set));
  }
  return descriptors;
}

std::string PostProcessingPipelineParser::GetFilePath() {
  return kPostProcessingPipelineFilePath;
}

base::ListValue* PostProcessingPipelineParser::GetMixPipeline() {
  return GetPipelineByKey(kMixPipelineKey);
}

base::ListValue* PostProcessingPipelineParser::GetLinearizePipeline() {
  return GetPipelineByKey(kLinearizePipelineKey);
}

base::ListValue* PostProcessingPipelineParser::GetPipelineByKey(
    const std::string& key) {
  base::DictionaryValue* stream_dict;
  if (!config_dict_ || !config_dict_->GetDictionary(key, &stream_dict)) {
    LOG(WARNING) << "No post-processor description found for \"" << key
                 << "\" in " << kPostProcessingPipelineFilePath
                 << ". Using passthrough.";
    return nullptr;
  }
  base::ListValue* out_list;
  CHECK(stream_dict->GetList(kProcessorsKey, &out_list));

  return out_list;
}

}  // namepsace media
}  // namespace chromecast
