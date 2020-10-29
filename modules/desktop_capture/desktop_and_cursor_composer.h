/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_DESKTOP_CAPTURE_DESKTOP_AND_CURSOR_COMPOSER_H_
#define MODULES_DESKTOP_CAPTURE_DESKTOP_AND_CURSOR_COMPOSER_H_

#include <memory>

#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_capture_types.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "modules/desktop_capture/desktop_geometry.h"
#include "modules/desktop_capture/mouse_cursor.h"
#include "modules/desktop_capture/mouse_cursor_monitor.h"
#include "modules/desktop_capture/shared_memory.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/system/rtc_export.h"

namespace webrtc {

// A wrapper for DesktopCapturer that also captures mouse using specified
// MouseCursorMonitor and renders it on the generated streams.
class RTC_EXPORT DesktopAndCursorComposer
    : public DesktopCapturer,
      public DesktopCapturer::Callback,
      public MouseCursorMonitor::Callback {
 public:
  // Creates a new blender that captures mouse cursor using
  // MouseCursorMonitor::Create(options) and renders it into the frames
  // generated by |desktop_capturer|.
  DesktopAndCursorComposer(DesktopCapturer* desktop_capturer,
                           const DesktopCaptureOptions& options);

  ~DesktopAndCursorComposer() override;

  // DesktopCapturer interface.
  void Start(DesktopCapturer::Callback* callback) override;
  void SetSharedMemoryFactory(
      std::unique_ptr<SharedMemoryFactory> shared_memory_factory) override;
  void CaptureFrame() override;
  bool GetSourceList(SourceList* sources) override;
  bool SelectSource(SourceId id) override;
  void SetExcludedWindow(WindowId window) override;

 private:
  // Allows test cases to use a fake MouseCursorMonitor implementation.
  friend class DesktopAndCursorComposerTest;

  // Constructor to delegate both deprecated and new constructors and allows
  // test cases to use a fake MouseCursorMonitor implementation.
  DesktopAndCursorComposer(DesktopCapturer* desktop_capturer,
                           MouseCursorMonitor* mouse_monitor);

  // DesktopCapturer::Callback interface.
  void OnCaptureResult(DesktopCapturer::Result result,
                       std::unique_ptr<DesktopFrame> frame) override;

  // MouseCursorMonitor::Callback interface.
  void OnMouseCursor(MouseCursor* cursor) override;
  void OnMouseCursorPosition(MouseCursorMonitor::CursorState state,
                             const DesktopVector& position) override;
  void OnMouseCursorPosition(const DesktopVector& position) override;

  const std::unique_ptr<DesktopCapturer> desktop_capturer_;
  const std::unique_ptr<MouseCursorMonitor> mouse_monitor_;

  DesktopCapturer::Callback* callback_;

  std::unique_ptr<MouseCursor> cursor_;
  DesktopVector cursor_position_;

  RTC_DISALLOW_COPY_AND_ASSIGN(DesktopAndCursorComposer);
};

}  // namespace webrtc

#endif  // MODULES_DESKTOP_CAPTURE_DESKTOP_AND_CURSOR_COMPOSER_H_
