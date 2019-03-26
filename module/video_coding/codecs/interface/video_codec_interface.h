/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_INTERFACE_VIDEO_CODEC_INTERFACE_H
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_INTERFACE_VIDEO_CODEC_INTERFACE_H

#include <vector>

#include "common_types.h"
//#include "webrtc/common_video/interface/i420_video_frame.h"
#include "module_common_types.h"
#include "video_error_codes.h"
#include "typedefs.h"
#include "video_decoder.h"
#include "video_encoder.h"

namespace yuntongxunwebrtc
{

class RTPFragmentationHeader; // forward declaration

// Note: if any pointers are added to this struct, it must be fitted
// with a copy-constructor. See below.
struct CodecSpecificInfoVP8 {
  bool hasReceivedSLI;
  uint8_t pictureIdSLI;
  bool hasReceivedRPSI;
  uint64_t pictureIdRPSI;
  int16_t pictureId;  // Negative value to skip pictureId.
  bool nonReference;
  uint8_t simulcastIdx;
  uint8_t temporalIdx;
  bool layerSync;
  int tl0PicIdx;  // Negative value to skip tl0PicIdx.
  int8_t keyIdx;  // Negative value to skip keyIdx.
};

struct CodecSpecificInfoVP9 {
  bool hasReceivedSLI;
  uint8_t pictureIdSLI;
  bool hasReceivedRPSI;
  uint64_t pictureIdRPSI;
  int16_t pictureId;  // Negative value to skip pictureId.
  bool nonReference;
  uint8_t temporalIdx;
  bool layerSync;
  int tl0PicIdx;  // Negative value to skip tl0PicIdx.
  int8_t keyIdx;  // Negative value to skip keyIdx.
};

struct CodecSpecificInfoGeneric {
  uint8_t simulcast_idx;
};

struct CodecSpecificInfoH264
{
	bool             last;
	WebRtc_Word16    pictureId;         // negative value to skip pictureId
	bool             nonReference; 
	uint8_t			 nalu_header;
	bool			 single_nalu;	
	bool			 stap_a;
	bool			 fu_a;
};

struct CodecSpecificInfoH264SVC
{
	bool             last;
	WebRtc_Word16    pictureId;         // negative value to skip pictureId
	bool             nonReference;
};
    
struct CodecSpecificInfoH264HIGH
{
    bool             last;
    WebRtc_Word16    pictureId;         // negative value to skip pictureId
    bool             nonReference;
    uint8_t             nalu_header;
    bool             single_nalu;
    bool             stap_a;
    bool             fu_a;
};

union CodecSpecificInfoUnion {
  CodecSpecificInfoGeneric generic;
  CodecSpecificInfoVP8 VP8;
  CodecSpecificInfoVP9 VP9;
  CodecSpecificInfoH264 H264;
  CodecSpecificInfoH264SVC	H264SVC;
  CodecSpecificInfoH264HIGH  H264HIGH;
};

// Note: if any pointers are added to this struct or its sub-structs, it
// must be fitted with a copy-constructor. This is because it is copied
// in the copy-constructor of VCMEncodedFrame.
struct CodecSpecificInfo
{
    VideoCodecType   codecType;
    CodecSpecificInfoUnion codecSpecific;
};

}  // namespace webrtc

#endif // WEBRTC_MODULES_VIDEO_CODING_CODECS_INTERFACE_VIDEO_CODEC_INTERFACE_H
