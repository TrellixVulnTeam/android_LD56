// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DISPLAY_COMPOSITOR_GPU_COMPOSITOR_FRAME_SINK_H_
#define COMPONENTS_DISPLAY_COMPOSITOR_GPU_COMPOSITOR_FRAME_SINK_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "cc/ipc/display_compositor.mojom.h"
#include "cc/ipc/mojo_compositor_frame_sink.mojom.h"
#include "cc/surfaces/compositor_frame_sink_support.h"
#include "cc/surfaces/compositor_frame_sink_support_client.h"
#include "cc/surfaces/referenced_surface_tracker.h"
#include "components/display_compositor/display_compositor_export.h"
#include "components/display_compositor/gpu_compositor_frame_sink_delegate.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace cc {
class Display;
}

namespace display_compositor {

// Server side representation of a WindowSurface.
class DISPLAY_COMPOSITOR_EXPORT GpuCompositorFrameSink
    : public NON_EXPORTED_BASE(cc::CompositorFrameSinkSupportClient),
      public NON_EXPORTED_BASE(cc::mojom::MojoCompositorFrameSink),
      public NON_EXPORTED_BASE(cc::mojom::MojoCompositorFrameSinkPrivate) {
 public:
  GpuCompositorFrameSink(
      GpuCompositorFrameSinkDelegate* delegate,
      cc::SurfaceManager* surface_manager,
      const cc::FrameSinkId& frame_sink_id,
      std::unique_ptr<cc::Display> display,
      std::unique_ptr<cc::BeginFrameSource> begin_frame_source,
      cc::mojom::MojoCompositorFrameSinkPrivateRequest private_request,
      cc::mojom::MojoCompositorFrameSinkClientPtr client);

  ~GpuCompositorFrameSink() override;

  // cc::mojom::MojoCompositorFrameSink:
  void EvictFrame() override;
  void SetNeedsBeginFrame(bool needs_begin_frame) override;
  void SubmitCompositorFrame(const cc::LocalSurfaceId& local_surface_id,
                             cc::CompositorFrame frame) override;
  void Require(const cc::LocalSurfaceId& local_surface_id,
               const cc::SurfaceSequence& sequence) override;
  void Satisfy(const cc::SurfaceSequence& sequence) override;

  // cc::mojom::MojoCompositorFrameSinkPrivate:
  void AddChildFrameSink(const cc::FrameSinkId& child_frame_sink_id) override;
  void RemoveChildFrameSink(
      const cc::FrameSinkId& child_frame_sink_id) override;

 protected:
  void OnClientConnectionLost();
  void OnPrivateConnectionLost();

  GpuCompositorFrameSinkDelegate* const delegate_;
  cc::CompositorFrameSinkSupport support_;
  cc::SurfaceManager* const surface_manager_;

 private:
  // cc::CompositorFrameSinkSupportClient implementation:
  void DidReceiveCompositorFrameAck() override;
  void OnBeginFrame(const cc::BeginFrameArgs& args) override;
  void ReclaimResources(const cc::ReturnedResourceArray& resources) override;
  void WillDrawSurface() override;

  // Track the surface references for the surface corresponding to this
  // compositor frame sink.
  cc::ReferencedSurfaceTracker surface_tracker_;

  bool client_connection_lost_ = false;
  bool private_connection_lost_ = false;

  cc::mojom::MojoCompositorFrameSinkClientPtr client_;
  mojo::Binding<cc::mojom::MojoCompositorFrameSinkPrivate>
      compositor_frame_sink_private_binding_;

  DISALLOW_COPY_AND_ASSIGN(GpuCompositorFrameSink);
};

}  // namespace display_compositor

#endif  // COMPONENTS_DISPLAY_COMPOSITOR_GPU_COMPOSITOR_FRAME_SINK_H_
