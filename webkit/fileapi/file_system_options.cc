// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/fileapi/file_system_options.h"

namespace fileapi {

FileSystemOptions::FileSystemOptions(
      ProfileMode profile_mode,
      const std::vector<std::string>& additional_allowed_schemes)
      : profile_mode_(profile_mode),
        additional_allowed_schemes_(additional_allowed_schemes) {
}

FileSystemOptions::~FileSystemOptions() {
}

}  // namespace fileapi
