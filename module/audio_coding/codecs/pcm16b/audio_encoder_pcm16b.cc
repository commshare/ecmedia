/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "audio_encoder_pcm16b.h"
#include "pcm16b.h"

namespace yuntongxunwebrtc {

int16_t AudioEncoderPcm16B::EncodeCall(const int16_t* audio,
                                       size_t input_len,
                                       uint8_t* encoded) {
  return WebRtcPcm16b_Encode(audio, static_cast<int16_t>(input_len), encoded);
}

}  // namespace webrtc