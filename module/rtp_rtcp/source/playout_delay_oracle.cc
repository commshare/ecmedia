/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "../module/rtp_rtcp/source/playout_delay_oracle.h"

#include "../base/checks.h"
#include "../system_wrappers/include/logging.h"
#include "../module/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "../module/rtp_rtcp/source/rtp_header_extensions.h"

namespace cloopenwebrtc {

PlayoutDelayOracle::PlayoutDelayOracle()
    : high_sequence_number_(0),
      send_playout_delay_(false),
      ssrc_(0),
      playout_delay_{-1, -1} {}

PlayoutDelayOracle::~PlayoutDelayOracle() {}

void PlayoutDelayOracle::UpdateRequest(uint32_t ssrc,
                                       PlayoutDelay playout_delay,
                                       uint16_t seq_num) {
  cloopenwebrtc::CritScope lock(&crit_sect_);
  DCHECK_LE(playout_delay.min_ms, PlayoutDelayLimits::kMaxMs);
  DCHECK_LE(playout_delay.max_ms, PlayoutDelayLimits::kMaxMs);
  DCHECK_LE(playout_delay.min_ms, playout_delay.max_ms);
  int64_t unwrapped_seq_num = unwrapper_.Unwrap(seq_num);
  if (playout_delay.min_ms >= 0 &&
      playout_delay.min_ms != playout_delay_.min_ms) {
    send_playout_delay_ = true;
    playout_delay_.min_ms = playout_delay.min_ms;
    high_sequence_number_ = unwrapped_seq_num;
  }

  if (playout_delay.max_ms >= 0 &&
      playout_delay.max_ms != playout_delay_.max_ms) {
    send_playout_delay_ = true;
    playout_delay_.max_ms = playout_delay.max_ms;
    high_sequence_number_ = unwrapped_seq_num;
  }
  ssrc_ = ssrc;
}

// If an ACK is received on the packet containing the playout delay extension,
// we stop sending the extension on future packets.
void PlayoutDelayOracle::OnReceivedRtcpReportBlocks(
    const ReportBlockList& report_blocks) {
	cloopenwebrtc::CritScope lock(&crit_sect_);
  for (const RTCPReportBlock& report_block : report_blocks) {
    if ((ssrc_ == report_block.sourceSSRC) && send_playout_delay_ &&
        (report_block.extendedHighSeqNum > high_sequence_number_)) {
      send_playout_delay_ = false;
    }
  }
}

}  // namespace cloopenwebrtc
