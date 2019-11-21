// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cookies/cookie_constants.h"

#include "thirdparty/glog/logging.h"
#include "base/string/string_case.h"

namespace net {

namespace {
const char kPriorityLow[] = "low";
const char kPriorityMedium[] = "medium";
const char kPriorityHigh[] = "high";
}  // namespace

const std::string CookiePriorityToString(CookiePriority priority) {
  switch(priority) {
    case COOKIE_PRIORITY_HIGH:
      return kPriorityHigh;
    case COOKIE_PRIORITY_MEDIUM:
      return kPriorityMedium;
    case COOKIE_PRIORITY_LOW:
      return kPriorityLow;
    default:
      NOTREACHED();
  }
  return std::string();
}

CookiePriority StringToCookiePriority(const std::string& priority) {
  std::string priority_comp(priority);
  priority_comp = ABI::base::ToLower(priority_comp);

  if (priority_comp == kPriorityHigh)
    return COOKIE_PRIORITY_HIGH;
  if (priority_comp == kPriorityMedium)
    return COOKIE_PRIORITY_MEDIUM;
  if (priority_comp == kPriorityLow)
    return COOKIE_PRIORITY_LOW;

  return COOKIE_PRIORITY_DEFAULT;
}

}  // namespace net
