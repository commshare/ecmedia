/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "remb.h"

#include <utility>

#include "../base/checks.h"
#include "../system_wrappers/include/logging.h"
#include "byte_io.h"
#include "common_header.h"

namespace cloopenwebrtc {
namespace rtcp {
constexpr uint8_t Remb::kFeedbackMessageType;
// Receiver Estimated Max Bitrate (REMB) (draft-alvestrand-rmcat-remb).
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |V=2|P| FMT=15  |   PT=206      |             length            |
//    +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  0 |                  SSRC of packet sender                        |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  4 |                       Unused = 0                              |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  8 |  Unique identifier 'R' 'E' 'M' 'B'                            |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 12 |  Num SSRC     | BR Exp    |  BR Mantissa                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 16 |   SSRC feedback                                               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    :  ...                                                          :
bool Remb::Parse(const CommonHeader& packet) {
  DCHECK(packet.type() == kPacketType);
  DCHECK_EQ(packet.fmt(), kFeedbackMessageType);

  if (packet.payload_size_bytes() < 16) {
    LOG(LS_WARNING) << "Payload length " << packet.payload_size_bytes()
                    << " is too small for Remb packet.";
    return false;
  }
  const uint8_t* const payload = packet.payload();
  if (kUniqueIdentifier != ByteReader<uint32_t>::ReadBigEndian(&payload[8])) {
    LOG(LS_WARNING) << "REMB identifier not found, not a REMB packet.";
    return false;
  }
  uint8_t number_of_ssrcs = payload[12];
  if (packet.payload_size_bytes() !=
      kCommonFeedbackLength + (2 + number_of_ssrcs) * 4) {
    LOG(LS_WARNING) << "Payload size " << packet.payload_size_bytes()
                    << " does not match " << number_of_ssrcs << " ssrcs.";
    return false;
  }

  ParseCommonFeedback(payload);
  uint8_t exponenta = payload[13] >> 2;
  uint64_t mantissa = (static_cast<uint32_t>(payload[13] & 0x03) << 16) |
                      ByteReader<uint16_t>::ReadBigEndian(&payload[14]);
  bitrate_bps_ = (mantissa << exponenta);
  bool shift_overflow = (bitrate_bps_ >> exponenta) != mantissa;
  if (shift_overflow) {
    LOG(LS_ERROR) << "Invalid remb bitrate value : " << mantissa
                  << "*2^" << static_cast<int>(exponenta);
    return false;
  }

  const uint8_t* next_ssrc = payload + 16;
  ssrcs_.clear();
  ssrcs_.reserve(number_of_ssrcs);
  for (uint8_t i = 0; i < number_of_ssrcs; ++i) {
    ssrcs_.push_back(ByteReader<uint32_t>::ReadBigEndian(next_ssrc));
    next_ssrc += sizeof(uint32_t);
  }

  return true;
}

bool Remb::SetSsrcs(std::vector<uint32_t> ssrcs) {
  if (ssrcs.size() > kMaxNumberOfSsrcs) {
    LOG(LS_WARNING) << "Not enough space for all given SSRCs.";
    return false;
  }
  ssrcs_ = std::move(ssrcs);
  return true;
}

bool Remb::Create(uint8_t* packet,
                  size_t* index,
                  size_t max_length,
                  RtcpPacket::PacketReadyCallback* callback) const {
  while (*index + BlockLength() > max_length) {
    if (!OnBufferFull(packet, index, callback))
      return false;
  }
  size_t index_end = *index + BlockLength();
  CreateHeader(kFeedbackMessageType, kPacketType, HeaderLength(), packet,
               index);
  DCHECK_EQ(0, Psfb::media_ssrc());
  CreateCommonFeedback(packet + *index);
  *index += kCommonFeedbackLength;

  ByteWriter<uint32_t>::WriteBigEndian(packet + *index, kUniqueIdentifier);
  *index += sizeof(uint32_t);
  const uint32_t kMaxMantissa = 0x3ffff;  // 18 bits.
  uint64_t mantissa = bitrate_bps_;
  uint8_t exponenta = 0;
  while (mantissa > kMaxMantissa) {
    mantissa >>= 1;
    ++exponenta;
  }
  packet[(*index)++] = ssrcs_.size();
  packet[(*index)++] = (exponenta << 2) | (mantissa >> 16);
  ByteWriter<uint16_t>::WriteBigEndian(packet + *index, mantissa & 0xffff);
  *index += sizeof(uint16_t);

  for (uint32_t ssrc : ssrcs_) {
    ByteWriter<uint32_t>::WriteBigEndian(packet + *index, ssrc);
    *index += sizeof(uint32_t);
  }
  DCHECK_EQ(index_end, *index);
  return true;
}
}  // namespace rtcp
}  // namespace webrtc
