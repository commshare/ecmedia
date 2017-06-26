/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "../module/rtp_rtcp/source/rtcp_packet.h"

#include "../base/checks.h"

namespace cloopenwebrtc {
namespace rtcp {
constexpr size_t RtcpPacket::kHeaderLength;

cloopenwebrtc::Buffer RtcpPacket::Build() const {
	cloopenwebrtc::Buffer packet(BlockLength());

  size_t length = 0;
  bool created = Create(packet.data(), &length, packet.capacity(), nullptr);
  DCHECK(created) << "Invalid packet is not supported.";
  DCHECK_EQ(length, packet.size())
      << "BlockLength mispredicted size used by Create";

  return packet;
}

bool RtcpPacket::BuildExternalBuffer(uint8_t* buffer,
                                     size_t max_length,
                                     PacketReadyCallback* callback) const {
  size_t index = 0;
  if (!Create(buffer, &index, max_length, callback))
    return false;
  return OnBufferFull(buffer, &index, callback);
}

bool RtcpPacket::OnBufferFull(uint8_t* packet,
                              size_t* index,
                              PacketReadyCallback* callback) const {
  if (*index == 0)
    return false;
  DCHECK(callback) << "Fragmentation not supported.";
  callback->OnPacketReady(packet, *index);
  *index = 0;
  return true;
}

size_t RtcpPacket::HeaderLength() const {
  size_t length_in_bytes = BlockLength();
  DCHECK_GT(length_in_bytes, 0);
  DCHECK_EQ(length_in_bytes % 4, 0) << "Padding not supported";
  // Length in 32-bit words without common header.
  return (length_in_bytes - kHeaderLength) / 4;
}

// From RFC 3550, RTP: A Transport Protocol for Real-Time Applications.
//
// RTP header format.
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P| RC/FMT  |      PT       |             length            |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
void RtcpPacket::CreateHeader(
    uint8_t count_or_format,  // Depends on packet type.
    uint8_t packet_type,
    size_t length,
    uint8_t* buffer,
    size_t* pos) {
  DCHECK_LE(length, 0xffffU);
  DCHECK_LE(count_or_format, 0x1f);
  constexpr uint8_t kVersionBits = 2 << 6;
  constexpr uint8_t kNoPaddingBit = 0 << 5;
  buffer[*pos + 0] = kVersionBits | kNoPaddingBit | count_or_format;
  buffer[*pos + 1] = packet_type;
  buffer[*pos + 2] = (length >> 8) & 0xff;
  buffer[*pos + 3] = length & 0xff;
  *pos += kHeaderLength;
}

}  // namespace rtcp
}  // namespace cloopenwebrtc
