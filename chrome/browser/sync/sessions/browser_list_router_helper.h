// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_SESSIONS_BROWSER_LIST_ROUTER_HELPER_H_
#define CHROME_BROWSER_SYNC_SESSIONS_BROWSER_LIST_ROUTER_HELPER_H_

#include "chrome/browser/sync/sessions/sync_sessions_web_contents_router.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace sync_sessions {

// Non-android helper of SyncSessionsWebContentsRouter that adds tracking for
// multi-window scenarios(e.g. tab movement between windows). Android doesn't
// have a BrowserList or TabStrip, so it doesn't compile the needed
// dependencies, nor would it benefit from the added tracking.
class BrowserListRouterHelper : public chrome::BrowserListObserver,
                                public TabStripModelObserver {
 public:
  explicit BrowserListRouterHelper(SyncSessionsWebContentsRouter* router);
  ~BrowserListRouterHelper() override;

 private:
  // chrome::BrowserListObserver implementation.
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;
  // TabStripModelObserver implementation.
  void TabInsertedAt(TabStripModel* model,
                     content::WebContents* web_contents,
                     int index,
                     bool foreground) override;

  // |router_| owns |this|.
  SyncSessionsWebContentsRouter* router_;

  DISALLOW_COPY_AND_ASSIGN(BrowserListRouterHelper);
};

}  // namespace sync_sessions

#endif  // CHROME_BROWSER_SYNC_SESSIONS_BROWSER_LIST_ROUTER_HELPER_H_
