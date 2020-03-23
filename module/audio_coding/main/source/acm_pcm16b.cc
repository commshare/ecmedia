/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "acm_pcm16b.h"

#ifdef WEBRTC_CODEC_PCM16
#include "pcm16b.h"
#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "../system_wrappers/include/trace.h"
#endif

namespace yuntongxunwebrtc {

namespace acm2 {

#ifndef WEBRTC_CODEC_PCM16

ACMPCM16B::ACMPCM16B(int16_t /* codec_id */) { return; }

ACMPCM16B::~ACMPCM16B() { return; }

int16_t ACMPCM16B::InternalEncode(uint8_t* /* bitstream */,
                                  int16_t* /* bitstream_len_byte */) {
  return -1;
}

int16_t ACMPCM16B::InternalInitEncoder(
    WebRtcACMCodecParams* /* codec_params */) {
  return -1;
}

ACMGenericCodec* ACMPCM16B::CreateInstance(void) { return NULL; }

int16_t ACMPCM16B::InternalCreateEncoder() { return -1; }

void ACMPCM16B::DestructEncoderSafe() { return; }

#else  //===================== Actual Implementation =======================
ACMPCM16B::ACMPCM16B(int16_t codec_id) {
  codec_id_ = codec_id;
  sampling_freq_hz_ = ACMCodecDB::CodecFreq(codec_id_);
}

ACMPCM16B::~ACMPCM16B() { return; }

int16_t ACMPCM16B::InternalEncode(uint8_t* bitstream,
                                  int16_t* bitstream_len_byte) {
  
  //hubin changed for LiveStream
  //*bitstream_len_byte = WebRtcPcm16b_Encode(&in_audio_[in_audio_ix_read_],
  //                                          frame_len_smpl_ * num_channels_,
  //                                          bitstream);

  *bitstream_len_byte = WebRtcPcm16b_EncodeW16(&in_audio_[in_audio_ix_read_],
	  frame_len_smpl_ * num_channels_,
	  (int16_t*)bitstream);

  
  // Increment the read index to tell the caller that how far
  // we have gone forward in reading the audio buffer.
  in_audio_ix_read_ += frame_len_smpl_ * num_channels_;
  return *bitstream_len_byte;
}

int16_t ACMPCM16B::InternalInitEncoder(
    WebRtcACMCodecParams* /* codec_params */) {
  // This codec does not need initialization, PCM has no instance.
  return 0;
}

ACMGenericCodec* ACMPCM16B::CreateInstance(void) { return NULL; }

int16_t ACMPCM16B::InternalCreateEncoder() {
  // PCM has no instance.
  return 0;
}

void ACMPCM16B::DestructEncoderSafe() {
  // PCM has no instance.
  encoder_exist_ = false;
  encoder_initialized_ = false;
  return;
}

#endif

}  // namespace acm2

}  // namespace yuntongxunwebrtc