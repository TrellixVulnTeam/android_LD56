// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEB_STATE_NAVIGATION_CONTEXT_IMPL_H_
#define IOS_WEB_WEB_STATE_NAVIGATION_CONTEXT_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ios/web/public/web_state/navigation_context.h"
#include "url/gurl.h"

namespace web {

// Tracks information related to a single navigation.
class NavigationContextImpl : public NavigationContext {
 public:
  // Creates navigation context for sucessful navigation to a different page.
  static std::unique_ptr<NavigationContextImpl> CreateNavigationContext(
      WebState* web_state,
      const GURL& url,
      const scoped_refptr<net::HttpResponseHeaders>& response_headers);

  // Creates navigation context for sucessful same page navigation.
  static std::unique_ptr<NavigationContextImpl>
  CreateSameDocumentNavigationContext(WebState* web_state, const GURL& url);

  // Creates navigation context for the error page navigation.
  static std::unique_ptr<NavigationContextImpl>
  CreateErrorPageNavigationContext(
      WebState* web_state,
      const GURL& url,
      const scoped_refptr<net::HttpResponseHeaders>& response_headers);

#ifndef NDEBUG
  // Returns human readable description of this object.
  NSString* GetDescription() const;
#endif  // NDEBUG

  // NavigationContext overrides:
  WebState* GetWebState() override;
  const GURL& GetUrl() const override;
  bool IsSameDocument() const override;
  bool IsErrorPage() const override;
  net::HttpResponseHeaders* GetResponseHeaders() const override;
  ~NavigationContextImpl() override;

 private:
  NavigationContextImpl(
      WebState* web_state,
      const GURL& url,
      bool is_same_page,
      bool is_error_page,
      const scoped_refptr<net::HttpResponseHeaders>& response_headers);

  WebState* web_state_ = nullptr;
  GURL url_;
  bool is_same_document_ = false;
  bool is_error_page_ = false;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;

  DISALLOW_COPY_AND_ASSIGN(NavigationContextImpl);
};

}  // namespace web

#endif  // IOS_WEB_WEB_STATE_NAVIGATION_CONTEXT_IMPL_H_
