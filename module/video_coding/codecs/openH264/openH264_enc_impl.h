/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 * WEBRTC H264 wrapper interface
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_OPENH264_ENC_IMPL_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_OPENH264_ENC_IMPL_H_

// OpenH264 headers
#include "codec_api.h"
#include "codec_def.h" 
#include "openH264.h"

#ifdef WIN32
#include <stdio.h>
#endif

namespace yuntongxunwebrtc {

class OpenH264EncoderImpl : public OpenH264Encoder{
 public:
  OpenH264EncoderImpl();

  virtual ~OpenH264EncoderImpl();

  // Free encoder memory.
  //
  // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
  virtual int Release();

  // Initialize the encoder with the information from the codecSettings
  //
  // Input:
  //          - codec_settings    : Codec settings
  //          - number_of_cores   : Number of cores available for the encoder
  //          - max_payload_size  : The maximum size each payload is allowed
  //                                to have. Usually MTU - overhead.
  //
  // Return value                 : Set bit rate if OK
  //                                <0 - Errors:
  //                                  WEBRTC_VIDEO_CODEC_ERR_PARAMETER
  //                                  WEBRTC_VIDEO_CODEC_ERR_SIZE
  //                                  WEBRTC_VIDEO_CODEC_LEVEL_EXCEEDED
  //                                  WEBRTC_VIDEO_CODEC_MEMORY
  //                                  WEBRTC_VIDEO_CODEC_ERROR
  virtual int InitEncode(const VideoCodec* codec_settings,
                         int number_of_cores,
                         size_t max_payload_size);

  // Encode an I420 image (as a part of a video stream). The encoded image
  // will be returned to the user through the encode complete callback.
  //
  // Input:
  //          - input_image       : Image to be encoded
  //          - frame_types       : Frame type to be generated by the encoder.
  //
  // Return value                 : WEBRTC_VIDEO_CODEC_OK if OK
  //                                <0 - Errors:
  //                                  WEBRTC_VIDEO_CODEC_ERR_PARAMETER
  //                                  WEBRTC_VIDEO_CODEC_MEMORY
  //                                  WEBRTC_VIDEO_CODEC_ERROR
  //                                  WEBRTC_VIDEO_CODEC_TIMEOUT

  virtual int Encode(const I420VideoFrame& input_image,
                     const CodecSpecificInfo* codec_specific_info,
                     const std::vector<VideoFrameType>* frame_types);

  // Register an encode complete callback object.
  //
  // Input:
  //          - callback         : Callback object which handles encoded images.
  //
  // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
  virtual int RegisterEncodeCompleteCallback(EncodedImageCallback* callback);

  // Inform the encoder of the new packet loss rate and the round-trip time of
  // the network.
  //
  //          - packet_loss : Fraction lost
  //                          (loss rate in percent = 100 * packetLoss / 255)
  //          - rtt         : Round-trip time in milliseconds
  // Return value           : WEBRTC_VIDEO_CODEC_OK if OK
  //                          <0 - Errors: WEBRTC_VIDEO_CODEC_ERROR
  //
  virtual int SetChannelParameters(uint32_t packet_loss, int64_t rtt);

  // Inform the encoder about the new target bit rate.
  //
  //          - new_bitrate_kbit : New target bit rate
  //          - frame_rate       : The target frame rate
  //
  // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
  virtual int SetRates(uint32_t new_bitrate_kbit, uint32_t frame_rate,
						 uint32_t minBitrate_kbit = 0, uint32_t maxBitrate_kbit = 0);

 private:
	void SetSVCEncoderParameters(const VideoCodec* codec_settings, SEncParamExt &sSvcParam);
	void InitializeSFBsInfo(SFrameBSInfo &info);
	void InitializeSSPic(const I420VideoFrame& input_image, SSourcePicture &pic);
	bool CopyEncodedImage(RTPFragmentationHeader &fragment, SFrameBSInfo &info,const I420VideoFrame &input_image);
	void ParseFragmentSVCLayer(RTPFragmentationHeader &fragment, uint8_t layer_index, uint8_t *nalu, int length, bool &prefix_nalu);
  // Update frame size for codec.
  int UpdateCodecFrameSize(const I420VideoFrame& input_image);

  EncodedImage encoded_image_;
  EncodedImageCallback* encoded_complete_callback_;
  VideoCodec codec_;
  int number_of_cores_;
  bool inited_;
  bool first_frame_encoded_;
  int64_t timestamp_;
  ISVCEncoder* encoder_;
  SEncParamExt sSvcParam;
  SBitrateInfo	bitrateInfo_;  //kbits
  float			framerate_;   //fps
  uint16_t		frameIndx_;   //encoded frame index
#ifdef WIN32
  FILE *_encodedFile;
#endif
};  // end of H264Encoder class

}  // namespace webrtc

#endif  // WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_IMPL_H_