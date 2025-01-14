// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/page_load_metrics/page_load_timing.h"

namespace page_load_metrics {

DocumentTiming::DocumentTiming() {}

DocumentTiming::DocumentTiming(const DocumentTiming& other) = default;

DocumentTiming::~DocumentTiming() {}

bool DocumentTiming::operator==(const DocumentTiming& other) const {
  return dom_content_loaded_event_start ==
             other.dom_content_loaded_event_start &&
         load_event_start == other.load_event_start &&
         first_layout == other.first_layout;
}

bool DocumentTiming::IsEmpty() const {
  return !dom_content_loaded_event_start && !load_event_start && !first_layout;
}

PaintTiming::PaintTiming() {}

PaintTiming::PaintTiming(const PaintTiming& other) = default;

PaintTiming::~PaintTiming() {}

bool PaintTiming::operator==(const PaintTiming& other) const {
  return first_paint == other.first_paint &&
         first_text_paint == other.first_text_paint &&
         first_image_paint == other.first_image_paint &&
         first_contentful_paint == other.first_contentful_paint &&
         first_meaningful_paint == other.first_meaningful_paint;
}

bool PaintTiming::IsEmpty() const {
  return !first_paint && !first_text_paint && !first_image_paint &&
         !first_contentful_paint && !first_meaningful_paint;
}

ParseTiming::ParseTiming() {}

ParseTiming::ParseTiming(const ParseTiming& other) = default;

ParseTiming::~ParseTiming() {}

bool ParseTiming::operator==(const ParseTiming& other) const {
  return parse_start == other.parse_start && parse_stop == other.parse_stop &&
         parse_blocked_on_script_load_duration ==
             other.parse_blocked_on_script_load_duration &&
         parse_blocked_on_script_load_from_document_write_duration ==
             other.parse_blocked_on_script_load_from_document_write_duration &&
         parse_blocked_on_script_execution_duration ==
             other.parse_blocked_on_script_execution_duration &&
         parse_blocked_on_script_execution_from_document_write_duration ==
             other
                 .parse_blocked_on_script_execution_from_document_write_duration;
}

bool ParseTiming::IsEmpty() const {
  return !parse_start && !parse_stop &&
         !parse_blocked_on_script_load_duration &&
         !parse_blocked_on_script_load_from_document_write_duration &&
         !parse_blocked_on_script_execution_duration &&
         !parse_blocked_on_script_execution_from_document_write_duration;
}

StyleSheetTiming::StyleSheetTiming() {}

StyleSheetTiming::StyleSheetTiming(const StyleSheetTiming& other) = default;

StyleSheetTiming::~StyleSheetTiming() {}

bool StyleSheetTiming::operator==(const StyleSheetTiming& other) const {
  return author_style_sheet_parse_duration_before_fcp ==
             other.author_style_sheet_parse_duration_before_fcp &&
         update_style_duration_before_fcp ==
             other.update_style_duration_before_fcp;
}

bool StyleSheetTiming::IsEmpty() const {
  return !author_style_sheet_parse_duration_before_fcp &&
         !update_style_duration_before_fcp;
}

PageLoadTiming::PageLoadTiming() {}

PageLoadTiming::PageLoadTiming(const PageLoadTiming& other) = default;

PageLoadTiming::~PageLoadTiming() {}

bool PageLoadTiming::operator==(const PageLoadTiming& other) const {
  return navigation_start == other.navigation_start &&
         response_start == other.response_start &&
         document_timing == other.document_timing &&
         paint_timing == other.paint_timing &&
         parse_timing == other.parse_timing &&
         style_sheet_timing == other.style_sheet_timing;
}

bool PageLoadTiming::IsEmpty() const {
  return navigation_start.is_null() && !response_start &&
         document_timing.IsEmpty() && paint_timing.IsEmpty() &&
         parse_timing.IsEmpty() && style_sheet_timing.IsEmpty();
}

PageLoadMetadata::PageLoadMetadata() {}

bool PageLoadMetadata::operator==(const PageLoadMetadata& other) const {
  return behavior_flags == other.behavior_flags;
}

}  // namespace page_load_metrics
