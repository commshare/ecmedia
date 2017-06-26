/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WINDOW_CAPTURER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WINDOW_CAPTURER_H_

#include <string>
#include <vector>

#include "../base/constructormagic.h"
#include "desktop_capture_types.h"
#include "desktop_capturer.h"
#include "typedefs.h"

namespace cloopenwebrtc {

class DesktopCaptureOptions;

class WindowCapturer : public DesktopCapturer {
 public:
  typedef cloopenwebrtc::WindowId WindowId;

  struct Window {
    WindowId id;

    // Title of the window in UTF-8 encoding.
    std::string title;
  };

  typedef std::vector<Window> WindowList;

  static WindowCapturer* Create(const DesktopCaptureOptions& options);

  // TODO(sergeyu): Remove this method. crbug.com/172183
  static WindowCapturer* Create();

  virtual ~WindowCapturer() {}

  // Get list of windows. Returns false in case of a failure.
  virtual bool GetWindowList(WindowList* windows) = 0;

  // Select window to be captured. Returns false in case of a failure (e.g. if
  // there is no window with the specified id).
  virtual bool SelectWindow(WindowId id) = 0;

  // Bring the selected window to the front. Returns false in case of a
  // failure or no window selected.
  virtual bool BringSelectedWindowToFront() = 0;

  // Get windows width and height.
  virtual bool GetShareCaptureRect(int &width, int &height) = 0;
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_MODULES_DESKTOP_CAPTURE_WINDOW_CAPTURER_H_

