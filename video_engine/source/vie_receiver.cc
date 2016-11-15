/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_receiver.h"

#include <vector>

#include "remote_bitrate_estimator.h"
#include "fec_receiver.h"
#include "receive_statistics.h"
#include "remote_ntp_time_estimator.h"
#include "rtp_header_parser.h"
#include "rtp_payload_registry.h"
#include "rtp_receiver.h"
#include "rtp_rtcp.h"
#include "rtp_dump.h"
#include "video_coding.h"
#include "critical_section_wrapper.h"
#include "logging.h"
#include "metrics.h"
#include "tick_util.h"
#include "timestamp_extrapolator.h"
#include "trace.h"


namespace cloopenwebrtc {

static const int kPacketLogIntervalMs = 10000;

ViEReceiver::ViEReceiver(const int32_t channel_id,
                         VideoCodingModule* module_vcm,
                         RemoteBitrateEstimator* remote_bitrate_estimator,
                         RtpFeedback* rtp_feedback)
    : channel_id_(channel_id),
      receive_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      clock_(Clock::GetRealTimeClock()),
      rtp_header_parser_(RtpHeaderParser::Create()),
      rtp_payload_registry_(
          new RTPPayloadRegistry(RTPPayloadStrategy::CreateStrategy(false))),
      rtp_receiver_(
          RtpReceiver::CreateVideoReceiver(channel_id,
                                           clock_,
                                           this,
                                           rtp_feedback,
                                           rtp_payload_registry_.get())),
      rtp_receive_statistics_(ReceiveStatistics::Create(clock_)),
      fec_receiver_(FecReceiver::Create(this)),
      rtp_rtcp_(NULL),
      vcm_(module_vcm),
      remote_bitrate_estimator_(remote_bitrate_estimator),
      ntp_estimator_(new RemoteNtpTimeEstimator(clock_)),
      rtp_dump_(NULL),
      receiving_(false),
      restored_packet_in_use_(false),
      receiving_ast_enabled_(false),
      last_packet_log_ms_(-1),
      video_data_cb_(NULL),
	external_decryption_(NULL),
	decryption_buffer_(NULL),
	callback_decryption_buffer_(NULL){
  assert(remote_bitrate_estimator);
}

ViEReceiver::~ViEReceiver() {
  UpdateHistograms();
  if (rtp_dump_) {
    rtp_dump_->Stop();
    RtpDump::DestroyRtpDump(rtp_dump_);
    rtp_dump_ = NULL;
  }
  if (decryption_buffer_) {
	  delete decryption_buffer_;
	  decryption_buffer_ = NULL;
  }
  if (callback_decryption_buffer_) {
	  delete callback_decryption_buffer_;
	  callback_decryption_buffer_ = NULL;
  }
}

void ViEReceiver::UpdateHistograms() {
  FecPacketCounter counter = fec_receiver_->GetPacketCounter();
  if (counter.num_packets > 0) {
    RTC_HISTOGRAM_PERCENTAGE("WebRTC.Video.ReceivedFecPacketsInPercent",
        counter.num_fec_packets * 100 / counter.num_packets);
  }
  if (counter.num_fec_packets > 0) {
    RTC_HISTOGRAM_PERCENTAGE(
        "WebRTC.Video.RecoveredMediaPacketsInPercentOfFec",
            counter.num_recovered_packets * 100 / counter.num_fec_packets);
  }
}

bool ViEReceiver::SetReceiveCodec(const VideoCodec& video_codec) {
  int8_t old_pltype = -1;
  if (rtp_payload_registry_->ReceivePayloadType(video_codec.plName,
                                                kVideoPayloadTypeFrequency,
                                                0,
                                                video_codec.maxBitrate,
                                                &old_pltype) != -1) {
    rtp_payload_registry_->DeRegisterReceivePayload(old_pltype);
  }

  return RegisterPayload(video_codec);
}

bool ViEReceiver::RegisterPayload(const VideoCodec& video_codec) {
  return rtp_receiver_->RegisterReceivePayload(video_codec.plName,
                                               video_codec.plType,
                                               kVideoPayloadTypeFrequency,
                                               0,
                                               video_codec.maxBitrate) == 0;
}

void ViEReceiver::SetNackStatus(bool enable,
                                int max_nack_reordering_threshold) {
  if (!enable) {
    // Reset the threshold back to the lower default threshold when NACK is
    // disabled since we no longer will be receiving retransmissions.
    max_nack_reordering_threshold = kDefaultMaxReorderingThreshold;
  }
  rtp_receive_statistics_->SetMaxReorderingThreshold(
      max_nack_reordering_threshold);
  rtp_receiver_->SetNACKStatus(enable ? kNackRtcp : kNackOff);
}

void ViEReceiver::SetRtxPayloadType(int payload_type) {
  rtp_payload_registry_->SetRtxPayloadType(payload_type);
}

void ViEReceiver::SetRtxSsrc(uint32_t ssrc) {
  rtp_payload_registry_->SetRtxSsrc(ssrc);
}

bool ViEReceiver::GetRtxSsrc(uint32_t* ssrc) const {
  return rtp_payload_registry_->GetRtxSsrc(ssrc);
}

uint32_t ViEReceiver::GetRemoteSsrc() const {
  return rtp_receiver_->SSRC();
}

int ViEReceiver::GetCsrcs(uint32_t* csrcs) const {
  return rtp_receiver_->CSRCs(csrcs);
}

void ViEReceiver::SetRtpRtcpModule(RtpRtcp* module) {
  rtp_rtcp_ = module;
}

RtpReceiver* ViEReceiver::GetRtpReceiver() const {
  return rtp_receiver_.get();
}

void ViEReceiver::RegisterSimulcastRtpRtcpModules(
    const std::list<RtpRtcp*>& rtp_modules) {
  CriticalSectionScoped cs(receive_cs_.get());
  rtp_rtcp_simulcast_.clear();

  if (!rtp_modules.empty()) {
    rtp_rtcp_simulcast_.insert(rtp_rtcp_simulcast_.begin(),
                               rtp_modules.begin(),
                               rtp_modules.end());
  }
}

bool ViEReceiver::SetReceiveTimestampOffsetStatus(bool enable, int id) {
  if (enable) {
    return rtp_header_parser_->RegisterRtpHeaderExtension(
        kRtpExtensionTransmissionTimeOffset, id);
  } else {
    return rtp_header_parser_->DeregisterRtpHeaderExtension(
        kRtpExtensionTransmissionTimeOffset);
  }
}

bool ViEReceiver::SetReceiveAbsoluteSendTimeStatus(bool enable, int id) {
  if (enable) {
    if (rtp_header_parser_->RegisterRtpHeaderExtension(
        kRtpExtensionAbsoluteSendTime, id)) {
      receiving_ast_enabled_ = true;
      return true;
    } else {
      return false;
    }
  } else {
    receiving_ast_enabled_ = false;
    return rtp_header_parser_->DeregisterRtpHeaderExtension(
        kRtpExtensionAbsoluteSendTime);
  }
}

int ViEReceiver::ReceivedRTPPacket(const void* rtp_packet,
                                   size_t rtp_packet_length,
                                   const PacketTime& packet_time) {
  return InsertRTPPacket(static_cast<const uint8_t*>(rtp_packet),
                         rtp_packet_length, packet_time);
}

int ViEReceiver::ReceivedRTCPPacket(const void* rtcp_packet,
                                    size_t rtcp_packet_length) {
  return InsertRTCPPacket(static_cast<const uint8_t*>(rtcp_packet),
                          rtcp_packet_length);
}

int32_t ViEReceiver::OnReceivedPayloadData(const uint8_t* payload_data,
                                           const size_t payload_size,
                                           const WebRtcRTPHeader* rtp_header) {
  WebRtcRTPHeader rtp_header_with_ntp = *rtp_header;
  rtp_header_with_ntp.ntp_time_ms =
      ntp_estimator_->Estimate(rtp_header->header.timestamp);
  if (vcm_->IncomingPacket(payload_data,
                           payload_size,
                           rtp_header_with_ntp) != 0) {
    // Check this...
    return -1;
  }
  return 0;
}

bool ViEReceiver::OnRecoveredPacket(const uint8_t* rtp_packet,
                                    size_t rtp_packet_length) {
  RTPHeader header;
  if (!rtp_header_parser_->Parse(rtp_packet, rtp_packet_length, &header)) {
    return false;
  }
  header.payload_type_frequency = kVideoPayloadTypeFrequency;
  bool in_order = IsPacketInOrder(header);
  return ReceivePacket(rtp_packet, rtp_packet_length, header, in_order);
}

void ViEReceiver::ReceivedBWEPacket(
    int64_t arrival_time_ms, size_t payload_size, const RTPHeader& header) {
  // Only forward if the incoming packet *and* the channel are both configured
  // to receive absolute sender time. RTP time stamps may have different rates
  // for audio and video and shouldn't be mixed.
  if (header.extension.hasAbsoluteSendTime && receiving_ast_enabled_) {
    remote_bitrate_estimator_->IncomingPacket(arrival_time_ms, payload_size,
                                              header);
  }
}

int ViEReceiver::InsertRTPPacket(const uint8_t* rtp_packet,
                                 size_t rtp_packet_length,
                                 const PacketTime& packet_time) {
	CriticalSectionScoped cs(receive_cs_.get());
	if (!receiving_) {
		return -1;
	}

	// TODO(mflodman) Change decrypt to get rid of this cast.
	unsigned char* received_packet = (unsigned char*)rtp_packet;
	int received_packet_length = rtp_packet_length;

	if (external_decryption_) {
		//put the last 4 bytes to rtp header ssrc to restore ssrc that FreeSwitch has changed
		//memcpy(received_packet + 8, received_packet + received_packet_length - 4, 4);
		//received_packet_length -= 4;

		int decrypted_length = 0;
		external_decryption_->decrypt(channel_id_, received_packet,
			decryption_buffer_, received_packet_length,
			&decrypted_length);
		if (decrypted_length <= 0) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, channel_id_,
				"RTP decryption failed");
			return -1;
		}
		else if (decrypted_length > kViEMaxMtu) {
			WEBRTC_TRACE(kTraceCritical, kTraceVideo, channel_id_,
				"InsertRTPPacket: %d bytes is allocated as RTP decrytption"
				" output, external decryption used %d bytes. => memory is "
				" now corrupted", kViEMaxMtu, decrypted_length);
			return -1;
		}
		received_packet = decryption_buffer_;
		received_packet_length = decrypted_length;
	}

	if (rtp_header_parser_->IsRtcp(received_packet, received_packet_length))  //add by ylr to handle FIR from rtp port
	{
		return InsertRTCPPacket(received_packet, received_packet_length);
	}
					
    if (video_data_cb_) {
      int decrypted_length = 0;
      video_data_cb_(channel_id_, received_packet +12, received_packet_length -12, callback_decryption_buffer_ +12, decrypted_length, false);
      memcpy(callback_decryption_buffer_, received_packet, 12);
	  received_packet = callback_decryption_buffer_;
	  received_packet_length = decrypted_length+12;
    }
    if (rtp_dump_) {
      rtp_dump_->DumpPacket(received_packet, received_packet_length);
    }
  
  RTPHeader header;
  if (!rtp_header_parser_->Parse(received_packet, received_packet_length,
                                 &header)) {
    return -1;
  }

  size_t payload_length = received_packet_length - header.headerLength;
  int64_t arrival_time_ms;
  int64_t now_ms = clock_->TimeInMilliseconds();
  if (packet_time.timestamp != -1)
    arrival_time_ms = (packet_time.timestamp + 500) / 1000;
  else
    arrival_time_ms = now_ms;

  {
    // Periodically log the RTP header of incoming packets.
    if (now_ms - last_packet_log_ms_ > kPacketLogIntervalMs) {
      std::stringstream ss;
      ss << "Packet received on SSRC: " << header.ssrc << " with payload type: "
         << static_cast<int>(header.payloadType) << ", timestamp: "
         << header.timestamp << ", sequence number: " << header.sequenceNumber
         << ", arrival time: " << (unsigned long)arrival_time_ms;
      if (header.extension.hasTransmissionTimeOffset)
        ss << ", toffset: " << header.extension.transmissionTimeOffset;
      if (header.extension.hasAbsoluteSendTime)
        ss << ", abs send time: " << header.extension.absoluteSendTime;
      LOG(LS_INFO) << ss.str();
      last_packet_log_ms_ = now_ms;
    }
  }

  remote_bitrate_estimator_->IncomingPacket(arrival_time_ms,
                                            payload_length, header);
  header.payload_type_frequency = kVideoPayloadTypeFrequency;

  bool in_order = IsPacketInOrder(header);
  rtp_payload_registry_->SetIncomingPayloadType(header);
  int ret = ReceivePacket(received_packet, received_packet_length, header, in_order)
      ? 0
      : -1;
  // Update receive statistics after ReceivePacket.
  // Receive statistics will be reset if the payload type changes (make sure
  // that the first packet is included in the stats).
  rtp_receive_statistics_->IncomingPacket(
      header, received_packet_length, IsPacketRetransmitted(header, in_order));
  return ret;
}

bool ViEReceiver::ReceivePacket(const uint8_t* packet,
                                size_t packet_length,
                                const RTPHeader& header,
                                bool in_order) {
  if (rtp_payload_registry_->IsEncapsulated(header)) {
    return ParseAndHandleEncapsulatingHeader(packet, packet_length, header);
  }
  const uint8_t* payload = packet + header.headerLength;
  assert(packet_length >= header.headerLength);
  size_t payload_length = packet_length - header.headerLength;
  PayloadUnion payload_specific;
  if (!rtp_payload_registry_->GetPayloadSpecifics(header.payloadType,
                                                  &payload_specific)) {
    return false;
  }
  return rtp_receiver_->IncomingRtpPacket(header, payload, payload_length,
                                          payload_specific, in_order);
}

bool ViEReceiver::ParseAndHandleEncapsulatingHeader(const uint8_t* packet,
                                                    size_t packet_length,
                                                    const RTPHeader& header) {
  if (rtp_payload_registry_->IsRed(header)) {
    int8_t ulpfec_pt = rtp_payload_registry_->ulpfec_payload_type();
    if (packet[header.headerLength] == ulpfec_pt)
      rtp_receive_statistics_->FecPacketReceived(header.ssrc);
    if (fec_receiver_->AddReceivedRedPacket(
            header, packet, packet_length, ulpfec_pt) != 0) {
      return false;
    }
    return fec_receiver_->ProcessReceivedFec() == 0;
  } else if (rtp_payload_registry_->IsRtx(header)) {
    if (header.headerLength + header.paddingLength == packet_length) {
      // This is an empty packet and should be silently dropped before trying to
      // parse the RTX header.
      return true;
    }
    // Remove the RTX header and parse the original RTP header.
    if (packet_length < header.headerLength)
      return false;
    if (packet_length > sizeof(restored_packet_))
      return false;
    CriticalSectionScoped cs(receive_cs_.get());
    if (restored_packet_in_use_) {
      LOG(LS_WARNING) << "Multiple RTX headers detected, dropping packet.";
      return false;
    }
    uint8_t* restored_packet_ptr = restored_packet_;
    if (!rtp_payload_registry_->RestoreOriginalPacket(
        &restored_packet_ptr, packet, &packet_length, rtp_receiver_->SSRC(),
        header)) {
      LOG(LS_WARNING) << "Incoming RTX packet: Invalid RTP header";
      return false;
    }
    restored_packet_in_use_ = true;
    bool ret = OnRecoveredPacket(restored_packet_ptr, packet_length);
    restored_packet_in_use_ = false;
    return ret;
  }
  return false;
}

int ViEReceiver::InsertRTCPPacket(const uint8_t* rtcp_packet,
                                  size_t rtcp_packet_length) {
    CriticalSectionScoped cs(receive_cs_.get());
    if (!receiving_) {
      return -1;
    }
	// TODO(mflodman) Change decrypt to get rid of this cast.
	unsigned char* received_packet = (unsigned char*)rtcp_packet;
	int received_packet_length = rtcp_packet_length;

	if (external_decryption_) {
		//put the last 4 bytes to rtp header ssrc to restore ssrc that FreeSwitch has changed
		//memcpy(received_packet + 4, received_packet + received_packet_length - 4, 4);
		//received_packet_length -= 4;

		int decrypted_length = 0;
		external_decryption_->decrypt_rtcp(channel_id_, received_packet,
			decryption_buffer_,
			received_packet_length,
			&decrypted_length);
		if (decrypted_length <= 0) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, channel_id_,
				"RTP decryption failed");
			return -1;
		}
		else if (decrypted_length > kViEMaxMtu) {
			WEBRTC_TRACE(kTraceCritical, kTraceVideo, channel_id_,
				"InsertRTCPPacket: %d bytes is allocated as RTP "
				" decrytption output, external decryption used %d bytes. "
				" => memory is now corrupted",
				kViEMaxMtu, decrypted_length);
			return -1;
		}
		received_packet = decryption_buffer_;
		received_packet_length = decrypted_length;
	}

    if (rtp_dump_) {
      rtp_dump_->DumpPacket(received_packet, received_packet_length);
    }

    std::list<RtpRtcp*>::iterator it = rtp_rtcp_simulcast_.begin();
    while (it != rtp_rtcp_simulcast_.end()) {
      RtpRtcp* rtp_rtcp = *it++;
      rtp_rtcp->IncomingRtcpPacket(received_packet, received_packet_length);
    }

  assert(rtp_rtcp_);  // Should be set by owner at construction time.
  int ret = rtp_rtcp_->IncomingRtcpPacket(received_packet, received_packet_length);
  if (ret != 0) {
    return ret;
  }

  int64_t rtt = 0;
  rtp_rtcp_->RTT(rtp_receiver_->SSRC(), &rtt, NULL, NULL, NULL);
  if (rtt == 0) {
    // Waiting for valid rtt.
    return 0;
  }
  uint32_t ntp_secs = 0;
  uint32_t ntp_frac = 0;
  uint32_t rtp_timestamp = 0;
  if (0 != rtp_rtcp_->RemoteNTP(&ntp_secs, &ntp_frac, NULL, NULL,
                                &rtp_timestamp)) {
    // Waiting for RTCP.
    return 0;
  }
  ntp_estimator_->UpdateRtcpTimestamp(rtt, ntp_secs, ntp_frac, rtp_timestamp);

  return 0;
}

void ViEReceiver::StartReceive() {
  CriticalSectionScoped cs(receive_cs_.get());
  receiving_ = true;
}

void ViEReceiver::StopReceive() {
  CriticalSectionScoped cs(receive_cs_.get());
  receiving_ = false;
}

int ViEReceiver::StartRTPDump(const char file_nameUTF8[1024]) {
  CriticalSectionScoped cs(receive_cs_.get());
  if (rtp_dump_) {
    // Restart it if it already exists and is started
    rtp_dump_->Stop();
  } else {
    rtp_dump_ = RtpDump::CreateRtpDump();
    if (rtp_dump_ == NULL) {
      return -1;
    }
  }
  if (rtp_dump_->Start(file_nameUTF8) != 0) {
    RtpDump::DestroyRtpDump(rtp_dump_);
    rtp_dump_ = NULL;
    return -1;
  }
  return 0;
}

int ViEReceiver::StopRTPDump() {
  CriticalSectionScoped cs(receive_cs_.get());
  if (rtp_dump_) {
    if (rtp_dump_->IsActive()) {
      rtp_dump_->Stop();
    }
    RtpDump::DestroyRtpDump(rtp_dump_);
    rtp_dump_ = NULL;
  } else {
    return -1;
  }
  return 0;
}

void ViEReceiver::GetReceiveBandwidthEstimatorStats(
    ReceiveBandwidthEstimatorStats* output) const {
  remote_bitrate_estimator_->GetStats(output);
}

ReceiveStatistics* ViEReceiver::GetReceiveStatistics() const {
  return rtp_receive_statistics_.get();
}

bool ViEReceiver::IsPacketInOrder(const RTPHeader& header) const {
  StreamStatistician* statistician =
      rtp_receive_statistics_->GetStatistician(header.ssrc);
  if (!statistician)
    return false;
  return statistician->IsPacketInOrder(header.sequenceNumber);
}

bool ViEReceiver::IsPacketRetransmitted(const RTPHeader& header,
                                        bool in_order) const {
  // Retransmissions are handled separately if RTX is enabled.
  if (rtp_payload_registry_->RtxEnabled())
    return false;
  StreamStatistician* statistician =
      rtp_receive_statistics_->GetStatistician(header.ssrc);
  if (!statistician)
    return false;
  // Check if this is a retransmission.
  int64_t min_rtt = 0;
  rtp_rtcp_->RTT(rtp_receiver_->SSRC(), NULL, NULL, &min_rtt, NULL);
  return !in_order &&
      statistician->IsRetransmitOfOldPacket(header, min_rtt);
}

int ViEReceiver::RegisterExternalDecryption(Encryption* decryption) {
	CriticalSectionScoped cs(receive_cs_.get());
	if (external_decryption_) {
		return -1;
	}
	decryption_buffer_ = new WebRtc_UWord8[kViEMaxMtu];
	if (decryption_buffer_ == NULL) {
		return -1;
	}
	external_decryption_ = decryption;
	return 0;
}

int ViEReceiver::DeregisterExternalDecryption() {
	CriticalSectionScoped cs(receive_cs_.get());
	if (external_decryption_ == NULL) {
		return -1;
	}
	external_decryption_ = NULL;
	return 0;
}
    
int ViEReceiver::SetVideoDataCb(onEcMediaVideoData video_data_cb)
{
    if (!callback_decryption_buffer_) {
		callback_decryption_buffer_ = new WebRtc_UWord8[kViEMaxMtu];
    }
    if (callback_decryption_buffer_ == NULL) {
        return -1;
    }
    video_data_cb_ = video_data_cb;
    return 0;
}
}  // namespace webrtc
