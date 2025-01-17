// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEB_RESTRICTIONS_BROWSER_WEB_RESTRICTIONS_MOJO_IMPLEMENTATION_H_
#define COMPONENTS_WEB_RESTRICTIONS_BROWSER_WEB_RESTRICTIONS_MOJO_IMPLEMENTATION_H_

#include <string>

#include "base/macros.h"
#include "components/web_restrictions/interfaces/web_restrictions.mojom.h"

namespace service_manager {
struct BindSourceInfo;
}

namespace web_restrictions {

class WebRestrictionsClient;

class WebRestrictionsMojoImplementation : public mojom::WebRestrictions {
 public:
  explicit WebRestrictionsMojoImplementation(WebRestrictionsClient* client);
  ~WebRestrictionsMojoImplementation() override;

  static void Create(WebRestrictionsClient* client,
                     const service_manager::BindSourceInfo& source_info,
                     mojom::WebRestrictionsRequest request);

 private:
  void GetResult(const std::string& url,
                 const GetResultCallback& callback) override;
  void RequestPermission(const std::string& url,
                         const RequestPermissionCallback& callback) override;

  WebRestrictionsClient* web_restrictions_client_;
};

}  // namespace web_restrictions
#endif  // COMPONENTS_WEB_RESTRICTIONS_BROWSER_WEB_RESTRICTIONS_MOJO_IMPLEMENTATION_H_
