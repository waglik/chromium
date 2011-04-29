// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/panels/panel_browser_window_cocoa.h"

#import <Cocoa/Cocoa.h>

#include "base/debug/debugger.h"
#import "chrome/browser/ui/cocoa/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/browser_test_helper.h"
#include "chrome/browser/ui/panels/panel.h"
#include "chrome/browser/ui/panels/panel_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

// Main test class.
class PanelBrowserWindowCocoaTest : public CocoaTest {
 protected:
  BrowserTestHelper browser_helper_;
};

TEST_F(PanelBrowserWindowCocoaTest, CreateClose) {
  PanelManager* manager = PanelManager::GetInstance();
  EXPECT_EQ(0, manager->active_count());  // No panels initially.

  Panel* panel = manager->CreatePanel(browser_helper_.browser());
  EXPECT_TRUE(panel);
  EXPECT_TRUE(panel->browser_window());  // Native panel is created right away.
  PanelBrowserWindowCocoa* native_window =
      static_cast<PanelBrowserWindowCocoa*>(panel->browser_window());

  EXPECT_EQ(panel, native_window->panel_);  // Back pointer initialized.
  EXPECT_EQ(1, manager->active_count());
  // BrowserTestHelper provides a browser w/o window_ set.
  // Use Browser::set_window() if needed.
  EXPECT_EQ(NULL, browser_helper_.browser()->window());

  // Window should not load before Show()
  EXPECT_FALSE([native_window->controller_ isWindowLoaded]);
  panel->Show();
  EXPECT_TRUE([native_window->controller_ isWindowLoaded]);
  EXPECT_TRUE([native_window->controller_ window]);

  gfx::Rect bounds = panel->GetBounds();
  EXPECT_TRUE(bounds.width() > 0);
  EXPECT_TRUE(bounds.height() > 0);

  // NSWindows created by NSWindowControllers don't have this bit even if
  // their NIB has it. The controller's lifetime is the window's lifetime.
  EXPECT_EQ(NO, [[native_window->controller_ window] isReleasedWhenClosed]);

  panel->Close();
  EXPECT_EQ(0, manager->active_count());
  // Close() destroys the controller, which destroys the NSWindow. CocoaTest
  // base class verifies that there is no remaining open windows after the test.
  EXPECT_FALSE(native_window->controller_);
}

