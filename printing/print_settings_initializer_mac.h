// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_PRINTING_SETTINGS_INITIALIZER_MAC_H_
#define PRINTING_PRINTING_SETTINGS_INITIALIZER_MAC_H_

#import <ApplicationServices/ApplicationServices.h>

#include "base/logging.h"
#include "printing/page_range.h"

namespace printing {

class PrintSettings;

// Initializes a PrintSettings object from the provided device context.
class PrintSettingsInitializerMac {
 public:
  static void InitPrintSettings(PMPrinter printer,
                                PMPageFormat page_format,
                                const PageRanges& new_ranges,
                                bool print_selection_only,
                                PrintSettings* print_settings);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PrintSettingsInitializerMac);
};

}  // namespace printing

#endif  // PRINTING_PRINTING_SETTINGS_INITIALIZER_MAC_H_
