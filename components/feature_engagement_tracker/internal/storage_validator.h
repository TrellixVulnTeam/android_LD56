// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_STORAGE_VALIDATOR_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_STORAGE_VALIDATOR_H_

#include <string>

#include "base/macros.h"

namespace feature_engagement_tracker {

// A StorageValidator checks the required storage conditions for a given event,
// and checks if all conditions are met for storing it.
class StorageValidator {
 public:
  virtual ~StorageValidator() = default;

  // Returns true iff new events of this type should be stored.
  // This is typically called before storing each incoming event.
  virtual bool ShouldStore(const std::string& event_name) = 0;

  // Returns true iff events of this type should be kept for the given day.
  // This is typically called during load of the internal database state, to
  // possibly throw away old data.
  virtual bool ShouldKeep(const std::string& event_name,
                          uint32_t event_day,
                          uint32_t current_day) = 0;

 protected:
  StorageValidator() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(StorageValidator);
};

}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_STORAGE_VALIDATOR_H_
