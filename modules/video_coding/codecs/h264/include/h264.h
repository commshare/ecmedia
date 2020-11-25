/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#ifndef MODULES_VIDEO_CODING_CODECS_H264_INCLUDE_H264_H_
#define MODULES_VIDEO_CODING_CODECS_H264_INCLUDE_H264_H_

#include <memory>
#include <vector>
#include <fstream>


#include "media/base/codec.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "rtc_base/system/rtc_export.h"

#define WEBRTC_USE_X264
//#define SAVE_ENCODEDE_FILE
//#define SAVE_DECODING_FILE

namespace webrtc {

struct SdpVideoFormat;

// Set to disable the H.264 encoder/decoder implementations that are provided if
// |rtc_use_h264| build flag is true (if false, this function does nothing).
// This function should only be called before or during WebRTC initialization
// and is not thread-safe.
RTC_EXPORT void DisableRtcUseH264();

// Returns a vector with all supported internal H264 profiles that we can
// negotiate in SDP, in order of preference.
std::vector<SdpVideoFormat> SupportedH264Codecs();

class RTC_EXPORT H264Encoder : public VideoEncoder {
 public:
  static std::unique_ptr<H264Encoder> Create(const cricket::VideoCodec& codec);
  // If H.264 is supported (any implementation).
  static bool IsSupported();

  ~H264Encoder() override {}
//ytx_add
#ifdef SAVE_ENCODEDE_FILE
  std::vector<std::ofstream> output_files_;
  std::vector<std::ofstream> output_temporal_files_;
#endif
};

class RTC_EXPORT H264Decoder : public VideoDecoder {
 public:
  static std::unique_ptr<H264Decoder> Create();
  static bool IsSupported();

  ~H264Decoder() override {}
//ytx_add
#ifdef SAVE_DECODING_FILE
  std::ofstream input_file_;
#endif
};

}  // namespace webrtc

#endif  // MODULES_VIDEO_CODING_CODECS_H264_INCLUDE_H264_H_
