// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "felicia/core/lib/net/errors.h"
#include "felicia/core/lib/base/logging.h"

namespace felicia {
namespace net {

std::string ErrorToString(int error) {
  if (error == 0) return "OK";

  const char* error_string;
  switch (error) {
#define NET_ERROR(label, value) \
  case ERR_##label:             \
    error_string = #label;      \
    break;
#include "felicia/core/lib/net/error_list.h"
#undef NET_ERROR
    default:
      NOTREACHED();
      error_string = "<unknown>";
  }
  return std::string("ERR_") + error_string;
}

}  // namespace net
}  // namespace felicia
