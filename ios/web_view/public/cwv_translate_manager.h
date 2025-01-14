// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_TRANSLATE_MANAGER_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_TRANSLATE_MANAGER_H_

#import <ChromeWebView/cwv_export.h>
#import <Foundation/Foundation.h>

// Interface to manage the translation flow.
// Clients are not supposed to instantiate or subclass it.
CWV_EXPORT
@protocol CWVTranslateManager<NSObject>

- (void)translate;

- (void)revertTranslation;

@end

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_TRANSLATE_MANAGER_H_
