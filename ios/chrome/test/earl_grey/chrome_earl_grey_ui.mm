// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"

#import "ios/chrome/browser/ui/tools_menu/tools_menu_constants.h"
#import "ios/chrome/browser/ui/tools_menu/tools_popup_controller.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#include "ios/chrome/test/app/navigation_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/earl_grey/js_test_util.h"
#import "ios/web/public/test/earl_grey/web_view_matchers.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::WaitUntilConditionOrTimeout;
using testing::kWaitForPageLoadTimeout;

@implementation ChromeEarlGreyUI

+ (void)openToolsMenu {
  // TODO(crbug.com/685570): Fix the tap instead of adding a delay.
  GREYCondition* myCondition = [GREYCondition
      conditionWithName:@"Delay to ensure the toolbar menu can be opened"
                  block:^BOOL {
                    return NO;
                  }];
  [myCondition waitWithTimeout:0.5];

  // TODO(crbug.com/639524): Add logic to ensure the app is in the correct
  // state, for example DCHECK if no tabs are displayed.
  [[[EarlGrey
      selectElementWithMatcher:grey_allOf(chrome_test_util::ToolsMenuButton(),
                                          grey_sufficientlyVisible(), nil)]
         usingSearchAction:grey_swipeSlowInDirection(kGREYDirectionDown)
      onElementWithMatcher:web::WebViewScrollView(
                               chrome_test_util::GetCurrentWebState())]
      performAction:grey_tap()];
  // TODO(crbug.com/639517): Add webViewScrollView matcher so we don't have
  // to always find it.
}

+ (void)openSettingsMenu {
  [ChromeEarlGreyUI openToolsMenu];
  id<GREYMatcher> toolsMenuTableViewMatcher =
      grey_accessibilityID(kToolsMenuTableViewId);
  [[[EarlGrey
      selectElementWithMatcher:grey_accessibilityID(kToolsMenuSettingsId)]
         usingSearchAction:grey_scrollToContentEdge(kGREYContentEdgeBottom)
      onElementWithMatcher:toolsMenuTableViewMatcher] performAction:grey_tap()];
}

+ (void)openNewTab {
  [ChromeEarlGreyUI openToolsMenu];
  id<GREYMatcher> newTabButtonMatcher =
      grey_accessibilityID(kToolsMenuNewTabId);
  [[EarlGrey selectElementWithMatcher:newTabButtonMatcher]
      performAction:grey_tap()];
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
}

+ (void)openNewIncognitoTab {
  [ChromeEarlGreyUI openToolsMenu];
  id<GREYMatcher> newIncognitoTabMatcher =
      grey_accessibilityID(kToolsMenuNewIncognitoTabId);
  [[EarlGrey selectElementWithMatcher:newIncognitoTabMatcher]
      performAction:grey_tap()];
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
}

+ (void)reload {
  // On iPhone Reload button is a part of tools menu, so open it.
  if (IsCompact()) {
    [self openToolsMenu];
  }
  [[EarlGrey selectElementWithMatcher:chrome_test_util::ReloadButton()]
      performAction:grey_tap()];
}

+ (void)openShareMenu {
  if (IsCompact()) {
    [ChromeEarlGreyUI openToolsMenu];
  }
  [[EarlGrey selectElementWithMatcher:chrome_test_util::ShareButton()]
      performAction:grey_tap()];
}

+ (void)waitForToolbarVisible:(BOOL)isVisible {
  const NSTimeInterval kWaitForToolbarAnimationTimeout = 1.0;
  ConditionBlock condition = ^{
    NSError* error = nil;
    id<GREYMatcher> visibleMatcher = isVisible ? grey_notNil() : grey_nil();
    [[EarlGrey selectElementWithMatcher:chrome_test_util::ToolsMenuButton()]
        assertWithMatcher:visibleMatcher
                    error:&error];
    return error == nil;
  };
  NSString* errorMessage =
      isVisible ? @"Toolbar was not visible" : @"Toolbar was visible";
  GREYAssert(testing::WaitUntilConditionOrTimeout(
                 kWaitForToolbarAnimationTimeout, condition),
             errorMessage);
}

@end
