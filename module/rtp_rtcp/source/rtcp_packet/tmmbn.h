/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_PACKET_TMMBN_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_PACKET_TMMBN_H_

#include <vector>

#include "../base/basictypes.h"
#include "rtpfb.h"
#include "tmmb_item.h"

namespace cloopenwebrtc {
namespace rtcp {
class CommonHeader;

// Temporary Maximum Media Stream Bit Rate Notification (TMMBN).
// RFC 5104, Section 4.2.2.
class Tmmbn : public Rtpfb {
 public:
  static constexpr uint8_t kFeedbackMessageType = 4;

  Tmmbn() {}
  ~Tmmbn() override {}

  // Parse assumes header is already parsed and validated.
  bool Parse(const CommonHeader& packet);

  void AddTmmbr(const TmmbItem& item);

  const std::vector<TmmbItem>& items() const { return items_; }

 protected:
  bool Create(uint8_t* packet,
              size_t* index,
              size_t max_length,
              RtcpPacket::PacketReadyCallback* callback) const override;

 private:
  size_t BlockLength() const override {
    return kHeaderLength + kCommonFeedbackLength +
           TmmbItem::kLength * items_.size();
  }

  // Media ssrc is unused, shadow base class setter and getter.
  void SetMediaSsrc(uint32_t ssrc);
  uint32_t media_ssrc() const;

  std::vector<TmmbItem> items_;
};
}  // namespace rtcp
}  // namespace webrtc
#endif  // WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_PACKET_TMMBN_H_
