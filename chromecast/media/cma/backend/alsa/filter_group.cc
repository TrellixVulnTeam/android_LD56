// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/alsa/filter_group.h"

#include <algorithm>

#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chromecast/media/cma/backend/alsa/post_processing_pipeline.h"
#include "media/base/audio_bus.h"
#include "media/base/vector_math.h"

namespace chromecast {
namespace media {
FilterGroup::FilterGroup(int num_channels,
                         const std::string& name,
                         const base::ListValue* filter_list,
                         const std::unordered_set<std::string>& device_ids,
                         const std::vector<FilterGroup*>& mixed_inputs)
    : num_channels_(num_channels),
      name_(name),
      device_ids_(device_ids),
      mixed_inputs_(mixed_inputs),
      output_samples_per_second_(0),
      channels_(num_channels_),
      post_processing_pipeline_(
          PostProcessingPipeline::Create(name_, filter_list, num_channels_)) {}

FilterGroup::~FilterGroup() = default;

void FilterGroup::Initialize(int output_samples_per_second) {
  output_samples_per_second_ = output_samples_per_second;
  post_processing_pipeline_->SetSampleRate(output_samples_per_second);
}

bool FilterGroup::CanProcessInput(StreamMixerAlsa::InputQueue* input) {
  return !(device_ids_.find(input->device_id()) == device_ids_.end());
}

void FilterGroup::AddActiveInput(StreamMixerAlsa::InputQueue* input) {
  active_inputs_.push_back(input);
}

float FilterGroup::MixAndFilter(int chunk_size) {
  DCHECK_NE(output_samples_per_second_, 0);

  ResizeBuffersIfNecessary(chunk_size);

  float volume = 0.0f;

  // Recursively mix inputs.
  for (auto* filter_group : mixed_inputs_) {
    volume = std::max(volume, filter_group->MixAndFilter(chunk_size));
  }

  // |volume| can only be 0 if no |mixed_inputs_| have data.
  // This is true because FilterGroup can only return 0 if:
  // a) It has no data and its PostProcessorPipeline is not ringing.
  //    (early return, below) or
  // b) The output volume is 0 and has NEVER been non-zero,
  //    since FilterGroup will use last_volume_ if volume is 0.
  //    In this case, there was never any data in the pipeline.
  if (active_inputs_.empty() && volume == 0.0f &&
      !post_processing_pipeline_->IsRinging()) {
    if (frames_zeroed_ < chunk_size) {
      // Ensure mixed_ is zeros. This is necessary if |mixed_| is read later.
      mixed_->ZeroFramesPartial(0, chunk_size);
      frames_zeroed_ = chunk_size;
    }
    return 0.0f;  // Output will be silence, no need to mix.
  }

  frames_zeroed_ = 0;

  // Mix InputQueues
  mixed_->ZeroFramesPartial(0, chunk_size);
  for (StreamMixerAlsa::InputQueue* input : active_inputs_) {
    input->GetResampledData(temp_.get(), chunk_size);
    for (int c = 0; c < num_channels_; ++c) {
      DCHECK(channels_[c]);
      input->VolumeScaleAccumulate(c != 0, temp_->channel(c), chunk_size,
                                   channels_[c]);
    }
    volume = std::max(volume, input->EffectiveVolume());
  }

  // Mix FilterGroups
  for (FilterGroup* group : mixed_inputs_) {
    if (group->last_volume() > 0.0f) {
      for (int c = 0; c < num_channels_; ++c) {
        ::media::vector_math::FMAC(group->data()->channel(c), 1.0f, chunk_size,
                                   channels_[c]);
      }
    }
  }

  bool is_silence = (volume == 0.0f);

  // Allow paused streams to "ring out" at the last valid volume.
  // If the stream volume is actually 0, this doesn't matter, since the
  // data is 0's anyway.
  if (!is_silence) {
    last_volume_ = volume;
  }

  delay_frames_ = post_processing_pipeline_->ProcessFrames(
      channels_, chunk_size, last_volume_, is_silence);
  return last_volume_;
}

int64_t FilterGroup::GetRenderingDelayMicroseconds() {
  return delay_frames_ * base::Time::kMicrosecondsPerSecond /
         output_samples_per_second_;
}

void FilterGroup::ClearActiveInputs() {
  active_inputs_.clear();
}

void FilterGroup::ResizeBuffersIfNecessary(int chunk_size) {
  if (!mixed_ || mixed_->frames() < chunk_size) {
    mixed_ = ::media::AudioBus::Create(num_channels_, chunk_size);
    for (int c = 0; c < num_channels_; ++c) {
      channels_[c] = mixed_->channel(c);
    }
  }
  if (!temp_ || temp_->frames() < chunk_size) {
    temp_ = ::media::AudioBus::Create(num_channels_, chunk_size);
  }
}

}  // namespace media
}  // namespace chromecast
