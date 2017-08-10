/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_encoder.h"

#include <assert.h>

#include <algorithm>

#include "video_image.h"
#include "webrtc_libyuv.h"
#include "frame_callback.h"
#include "paced_sender.h"
#include "rtp_rtcp.h"
#include "process_thread.h"
#include "video_codec_interface.h"
#include "video_coding.h"
#include "video_coding_defines.h"
#include "encoded_frame.h"
#include "clock.h"
#include "critical_section_wrapper.h"
#include "logging.h"
#include "metrics.h"
#include "tick_util.h"
#include "trace_event.h"
#include "send_statistics_proxy.h"
#include "vie_codec.h"
#include "vie_image_process.h"
#include "vie_defines.h"
#include <math.h>
#include "send_statistics_proxy.h"


namespace cloopenwebrtc {

// Margin on when we pause the encoder when the pacing buffer overflows relative
// to the configured buffer delay.
static const float kEncoderPausePacerMargin = 2.0f;

// Don't stop the encoder unless the delay is above this configured value.
static const int kMinPacingDelayMs = 200;

// Allow packets to be transmitted in up to 2 times max video bitrate if the
// bandwidth estimate allows it.
// TODO(holmer): Expose transmission start, min and max bitrates in the
// VideoEngine API and remove the kTransmissionMaxBitrateMultiplier.
static const int kTransmissionMaxBitrateMultiplier = 2;

static const float kStopPaddingThresholdMs = 2000;

std::vector<uint32_t> AllocateStreamBitrates(
    uint32_t total_bitrate,
    const SimulcastStream* stream_configs,
    size_t number_of_streams) {
  if (number_of_streams == 0) {
    std::vector<uint32_t> stream_bitrates(1, 0);
    stream_bitrates[0] = total_bitrate;
    return stream_bitrates;
  }
  std::vector<uint32_t> stream_bitrates(number_of_streams, 0);
  uint32_t bitrate_remainder = total_bitrate;
  for (size_t i = 0; i < stream_bitrates.size() && bitrate_remainder > 0; ++i) {
    if (stream_configs[i].maxBitrate * 1000 > bitrate_remainder) {
      stream_bitrates[i] = bitrate_remainder;
    } else {
      stream_bitrates[i] = stream_configs[i].maxBitrate * 1000;
    }
    bitrate_remainder -= stream_bitrates[i];
  }
  return stream_bitrates;
}

class QMVideoSettingsCallback : public VCMQMSettingsCallback {
 public:
  explicit QMVideoSettingsCallback(VideoProcessingModule* vpm);

  ~QMVideoSettingsCallback();

  // Update VPM with QM (quality modes: frame size & frame rate) settings.
  int32_t SetVideoQMSettings(const uint32_t frame_rate,
                             const uint32_t width,
                             const uint32_t height);

  void SetSendStatsCallback(SendStatisticsProxy *stats_proxy);
  void InitQMSetting(const uint32_t frame_rate,
					  const uint32_t width,
					  const uint32_t height);

 private:
  VideoProcessingModule* vpm_;
  SendStatisticsProxy *stats_proxy_;
};

class ViEBitrateObserver : public BitrateObserver {
 public:
  explicit ViEBitrateObserver(ViEEncoder* owner)
      : owner_(owner) {
  }
  virtual ~ViEBitrateObserver() {}
  // Implements BitrateObserver.
  virtual void OnNetworkChanged(uint32_t bitrate_bps,
                                uint8_t fraction_lost,
                                int64_t rtt) {
    owner_->OnNetworkChanged(bitrate_bps, fraction_lost, rtt);
  }
 private:
  ViEEncoder* owner_;
};

class ViEPacedSenderCallback : public PacedSender::Callback {
 public:
  explicit ViEPacedSenderCallback(ViEEncoder* owner)
      : owner_(owner) {
  }
  virtual ~ViEPacedSenderCallback() {}
  virtual bool TimeToSendPacket(uint32_t ssrc,
                                uint16_t sequence_number,
                                int64_t capture_time_ms,
                                bool retransmission) {
    return owner_->TimeToSendPacket(ssrc, sequence_number, capture_time_ms,
                                    retransmission);
  }
  virtual size_t TimeToSendPadding(size_t bytes) {
    return owner_->TimeToSendPadding(bytes);
  }

  virtual void BucketDelay(int64_t delayInMs) {
	  owner_->BucketDelay(delayInMs);
  }
 private:
  ViEEncoder* owner_;
};

ViEEncoder::ViEEncoder(int32_t engine_id,
                       int32_t channel_id,
                       uint32_t number_of_cores,
                       const Config& config,
                       ProcessThread& module_process_thread,
                       ProcessThread& module_process_thread_pacer,
                       BitrateController* bitrate_controller)
  : engine_id_(engine_id),
    channel_id_(channel_id),
    number_of_cores_(number_of_cores),
    vcm_(*cloopenwebrtc::VideoCodingModule::Create()),
    vpm_(*cloopenwebrtc::VideoProcessingModule::Create(ViEModuleId(engine_id,
                                                            channel_id))),
    callback_cs_(CriticalSectionWrapper::CreateCriticalSection()),
    data_cs_(CriticalSectionWrapper::CreateCriticalSection()),
    bitrate_controller_(bitrate_controller),
    time_of_last_incoming_frame_ms_(0),
    send_padding_(false),
    min_transmit_bitrate_kbps_(0),
    target_delay_ms_(0),
    network_is_transmitting_(true),
    encoder_paused_(false),
    encoder_paused_and_dropped_frame_(false),
    fec_enabled_(false),
    nack_enabled_(false),
    codec_observer_(NULL),
    effect_filter_(NULL),
    module_process_thread_(module_process_thread),
    module_process_thread_pacer_(module_process_thread_pacer),
    has_received_sli_(false),
    picture_id_sli_(0),
    has_received_rpsi_(false),
    picture_id_rpsi_(0),
    qm_callback_(NULL),
    video_suspended_(false),
    pre_encode_callback_(NULL),
    start_ms_(Clock::GetRealTimeClock()->TimeInMilliseconds()),
    file_recorder_(channel_id),
    capture_(NULL),
    ec_i420_frame_callback_(NULL),
    capture_id_(-1),
	encoded_packet_observer_(NULL),
	packet_observer_cs_(CriticalSectionWrapper::CreateCriticalSection()){
  RtpRtcp::Configuration configuration;
  configuration.id = ViEModuleId(engine_id_, channel_id_);
  configuration.audio = false;  // Video.

  default_rtp_rtcp_.reset(RtpRtcp::CreateRtpRtcp(configuration));
  bitrate_observer_.reset(new ViEBitrateObserver(this));
  pacing_callback_.reset(new ViEPacedSenderCallback(this));
  paced_sender_.reset(new PacedSender(
      Clock::GetRealTimeClock(),
      pacing_callback_.get(),
      kDefaultStartBitrateKbps,
      PacedSender::kDefaultPaceMultiplier * kDefaultStartBitrateKbps,
      0));
  send_statistics_proxy_.reset(new SendStatisticsProxy(channel_id));
}

bool ViEEncoder::Init() {
  if (vcm_.InitializeSender() != 0) {
    return false;
  }
  vpm_.EnableTemporalDecimation(true);

  // Enable/disable content analysis: off by default for now.
  vpm_.EnableContentAnalysis(true);

  if (module_process_thread_.RegisterModule(&vcm_) != 0 ||
      module_process_thread_.RegisterModule(default_rtp_rtcp_.get()) != 0 ||
      module_process_thread_pacer_.RegisterModule(paced_sender_.get()) != 0) {
    return false;
  }

  if (module_process_thread_.RegisterModule(send_statistics_proxy_.get()) != 0)
	  return false;//add at 20161110, by ylr

  if (qm_callback_) {
    delete qm_callback_;
  }
  qm_callback_ = new QMVideoSettingsCallback(&vpm_);
  qm_callback_->SetSendStatsCallback(send_statistics_proxy_.get());

#ifdef VIDEOCODEC_VP8
  VideoCodecType codec_type = cloopenwebrtc::kVideoCodecVP8;
#elif VIDEOCODEC_H264
    VideoCodecType codec_type = cloopenwebrtc::kVideoCodecH264;
#else
  VideoCodecType codec_type = cloopenwebrtc::kVideoCodecI420;
#endif

  VideoCodec video_codec;
  if (vcm_.Codec(codec_type, &video_codec) != VCM_OK) {
    return false;
  }
  {
    CriticalSectionScoped cs(data_cs_.get());
    send_padding_ = video_codec.numberOfSimulcastStreams > 1;
  }
  if (vcm_.RegisterSendCodec(&video_codec, number_of_cores_,
                             default_rtp_rtcp_->MaxDataPayloadLength()) != 0) {
    return false;
  }
  if (default_rtp_rtcp_->RegisterSendPayload(video_codec) != 0) {
    return false;
  }
  if (vcm_.RegisterTransportCallback(this) != 0) {
    return false;
  }
  if (vcm_.RegisterSendStatisticsCallback(this) != 0) {
    return false;
  }
  if (vcm_.RegisterVideoQMCallback(qm_callback_) != 0) {
    return false;
  }

  return true;
}

ViEEncoder::~ViEEncoder() {
  UpdateHistograms();
  if (bitrate_controller_) {
    bitrate_controller_->RemoveBitrateObserver(bitrate_observer_.get());
  }
  module_process_thread_.DeRegisterModule(send_statistics_proxy_.get());
  module_process_thread_.DeRegisterModule(&vcm_);
  module_process_thread_.DeRegisterModule(&vpm_);
  module_process_thread_.DeRegisterModule(default_rtp_rtcp_.get());
  module_process_thread_pacer_.DeRegisterModule(paced_sender_.get());
  VideoCodingModule::Destroy(&vcm_);
  VideoProcessingModule::Destroy(&vpm_);

  delete qm_callback_;
}

void ViEEncoder::UpdateHistograms() {
  int64_t elapsed_sec =
      (Clock::GetRealTimeClock()->TimeInMilliseconds() - start_ms_) / 1000;
  if (elapsed_sec < metrics::kMinRunTimeInSeconds) {
    return;
  }
  cloopenwebrtc::VCMFrameCount frames;
  if (vcm_.SentFrameCount(frames) != VCM_OK) {
    return;
  }
  uint32_t total_frames = frames.numKeyFrames + frames.numDeltaFrames;
  if (total_frames > 0) {
    RTC_HISTOGRAM_COUNTS_1000("WebRTC.Video.KeyFramesSentInPermille",
        static_cast<int>(
            (frames.numKeyFrames * 1000.0f / total_frames) + 0.5f));
  }
}

int ViEEncoder::Owner() const {
  return channel_id_;
}

void ViEEncoder::SetNetworkTransmissionState(bool is_transmitting) {
  {
    CriticalSectionScoped cs(data_cs_.get());
    network_is_transmitting_ = is_transmitting;
  }
  if (is_transmitting) {
    paced_sender_->Resume();
  } else {
    paced_sender_->Pause();
  }
}

void ViEEncoder::SetVideoQualityMode(int mode) {
    vcm_.SetVideoQualityMode(mode);
}
    
void ViEEncoder::Pause() {
  CriticalSectionScoped cs(data_cs_.get());
  encoder_paused_ = true;
}

void ViEEncoder::Restart() {
  CriticalSectionScoped cs(data_cs_.get());
  encoder_paused_ = false;
}

uint8_t ViEEncoder::NumberOfCodecs() {
  return vcm_.NumberOfCodecs();
}

int32_t ViEEncoder::GetCodec(uint8_t list_index, VideoCodec* video_codec) {
  if (vcm_.Codec(list_index, video_codec) != 0) {
    return -1;
  }
  return 0;
}

int32_t ViEEncoder::RegisterExternalEncoder(cloopenwebrtc::VideoEncoder* encoder,
                                            uint8_t pl_type,
                                            bool internal_source) {
  if (encoder == NULL)
    return -1;

  if (vcm_.RegisterExternalEncoder(encoder, pl_type, internal_source) !=
      VCM_OK) {
    return -1;
  }
  return 0;
}

int32_t ViEEncoder::DeRegisterExternalEncoder(uint8_t pl_type) {
  cloopenwebrtc::VideoCodec current_send_codec;
  if (vcm_.SendCodec(&current_send_codec) == VCM_OK) {
    uint32_t current_bitrate_bps = 0;
    if (vcm_.Bitrate(&current_bitrate_bps) != 0) {
      LOG(LS_WARNING) << "Failed to get the current encoder target bitrate.";
    }
    current_send_codec.startBitrate = (current_bitrate_bps + 500) / 1000;
  }

  if (vcm_.RegisterExternalEncoder(NULL, pl_type) != VCM_OK) {
    return -1;
  }

  // If the external encoder is the current send codec, use vcm internal
  // encoder.
  if (current_send_codec.plType == pl_type) {
    uint16_t max_data_payload_length =
        default_rtp_rtcp_->MaxDataPayloadLength();
    {
      CriticalSectionScoped cs(data_cs_.get());
      send_padding_ = current_send_codec.numberOfSimulcastStreams > 1;
    }
    // TODO(mflodman): Unfortunately the VideoCodec that VCM has cached a
    // raw pointer to an |extra_options| that's long gone.  Clearing it here is
    // a hack to prevent the following code from crashing.  This should be fixed
    // for realz.  https://code.google.com/p/chromium/issues/detail?id=348222
    current_send_codec.extra_options = NULL;
    if (vcm_.RegisterSendCodec(&current_send_codec, number_of_cores_,
                               max_data_payload_length) != VCM_OK) {
      LOG(LS_INFO) << "De-registered the currently used external encoder ("
                   << static_cast<int>(pl_type) << ") and therefore tried to "
                   << "register the corresponding internal encoder, but none "
                   << "was supported.";
    }
  }
  return 0;
}

int32_t ViEEncoder::SetEncoder(const cloopenwebrtc::VideoCodec& video_codec) {
  // Setting target width and height for VPM.
  if (vpm_.SetTargetResolution(video_codec.width, video_codec.height,
                               video_codec.maxFramerate) != VPM_OK) {
    return -1;
  }

  if (qm_callback_)
  {
	  qm_callback_->InitQMSetting(video_codec.maxFramerate, video_codec.width, video_codec.height);
  }

  if (default_rtp_rtcp_->RegisterSendPayload(video_codec) != 0) {
    return -1;
  }
  // Convert from kbps to bps.
  std::vector<uint32_t> stream_bitrates = AllocateStreamBitrates(
      video_codec.startBitrate * 1000,
      video_codec.simulcastStream,
      video_codec.numberOfSimulcastStreams);
  default_rtp_rtcp_->SetTargetSendBitrate(stream_bitrates);

  uint16_t max_data_payload_length =
      default_rtp_rtcp_->MaxDataPayloadLength();

  {
    CriticalSectionScoped cs(data_cs_.get());
    send_padding_ = video_codec.numberOfSimulcastStreams > 1;
  }
  if (vcm_.RegisterSendCodec(&video_codec, number_of_cores_,
                             max_data_payload_length) != VCM_OK) {
    return -1;
  }

  // Set this module as sending right away, let the slave module in the channel
  // start and stop sending.
  if (default_rtp_rtcp_->SetSendingStatus(true) != 0) {
    return -1;
  }

  bitrate_controller_->SetBitrateObserver(bitrate_observer_.get(),
                                          video_codec.startBitrate * 1000,
                                          video_codec.minBitrate * 1000,
                                          kTransmissionMaxBitrateMultiplier *
                                          video_codec.maxBitrate * 1000);
  bitrate_controller_->SetCodecMode(video_codec.mode);

  CriticalSectionScoped crit(data_cs_.get());
  int pad_up_to_bitrate_kbps = video_codec.startBitrate;
  if (pad_up_to_bitrate_kbps < min_transmit_bitrate_kbps_)
    pad_up_to_bitrate_kbps = min_transmit_bitrate_kbps_;

  paced_sender_->UpdateBitrate(
      video_codec.startBitrate,
      PacedSender::kDefaultPaceMultiplier * video_codec.startBitrate,
      pad_up_to_bitrate_kbps);
  if (send_statistics_proxy_)
  {
	  send_statistics_proxy_->ConfigEncoderSetting(video_codec);
  }
 
  RegisterEncoderRateObserver(send_statistics_proxy_.get());
  return 0;
}

void ViEEncoder::setFrameScaleType(FrameScaleType frame_scale_type) {
    vpm_.setFrameScaleType(frame_scale_type);
}
int32_t ViEEncoder::GetEncoder(VideoCodec* video_codec) {
  if (vcm_.SendCodec(video_codec) != 0) {
    return -1;
  }
  return 0;
}

int32_t ViEEncoder::GetCodecConfigParameters(
    unsigned char config_parameters[kConfigParameterSize],
    unsigned char& config_parameters_size) {
  int32_t num_parameters =
      vcm_.CodecConfigParameters(config_parameters, kConfigParameterSize);
  if (num_parameters <= 0) {
    config_parameters_size = 0;
    return -1;
  }
  config_parameters_size = static_cast<unsigned char>(num_parameters);
  return 0;
}

int32_t ViEEncoder::ScaleInputImage(bool enable) {
  VideoFrameResampling resampling_mode = kFastRescaling;
  // TODO(mflodman) What?
  if (enable) {
    // kInterpolation is currently not supported.
    LOG_F(LS_ERROR) << "Not supported.";
    return -1;
  }
  vpm_.SetInputFrameResampleMode(resampling_mode);

  return 0;
}

bool ViEEncoder::TimeToSendPacket(uint32_t ssrc,
                                  uint16_t sequence_number,
                                  int64_t capture_time_ms,
                                  bool retransmission) {
  return default_rtp_rtcp_->TimeToSendPacket(ssrc, sequence_number,
                                             capture_time_ms, retransmission);
}

size_t ViEEncoder::TimeToSendPadding(size_t bytes) {
  bool send_padding;
  {
    CriticalSectionScoped cs(data_cs_.get());
    send_padding =
        send_padding_ || video_suspended_ || min_transmit_bitrate_kbps_ > 0;
  }
  if (send_padding) {
    return default_rtp_rtcp_->TimeToSendPadding(bytes);
  }
  return 0;
}

void ViEEncoder::BucketDelay(int64_t delayInMs)
{
	if(send_statistics_proxy_)
		send_statistics_proxy_->OnBucketDelay(delayInMs);
}

bool ViEEncoder::EncoderPaused() const {
  // Pause video if paused by caller or as long as the network is down or the
  // pacer queue has grown too large in buffered mode.
  if (encoder_paused_) {
    return true;
  }
  if (target_delay_ms_ > 0) {
    // Buffered mode.
    // TODO(pwestin): Workaround until nack is configured as a time and not
    // number of packets.
    return paced_sender_->QueueInMs() >=
        std::max(static_cast<int>(target_delay_ms_ * kEncoderPausePacerMargin),
                 kMinPacingDelayMs);
  }
  if (paced_sender_->ExpectedQueueTimeMs() >
      PacedSender::kDefaultMaxQueueLengthMs) {
    // Too much data in pacer queue, drop frame.
    return true;
  }
  return !network_is_transmitting_;
}

void ViEEncoder::TraceFrameDropStart() {
  // Start trace event only on the first frame after encoder is paused.
  if (!encoder_paused_and_dropped_frame_) {
    TRACE_EVENT_ASYNC_BEGIN0("cloopenwebrtc", "EncoderPaused", this);
  }
  encoder_paused_and_dropped_frame_ = true;
  return;
}

void ViEEncoder::TraceFrameDropEnd() {
  // End trace event on first frame after encoder resumes, if frame was dropped.
  if (encoder_paused_and_dropped_frame_) {
    TRACE_EVENT_ASYNC_END0("cloopenwebrtc", "EncoderPaused", this);
  }
  encoder_paused_and_dropped_frame_ = false;
}

RtpRtcp* ViEEncoder::SendRtpRtcpModule() {
  return default_rtp_rtcp_.get();
}

void ViEEncoder::DeliverFrame(int id,
                              I420VideoFrame* video_frame,
                              const std::vector<uint32_t>& csrcs) {
//    sean for multivideo encoding begin
  if (default_rtp_rtcp_->SendingMedia() == false) {
    // We've paused or we have no channels attached, don't encode.
    return;
  }
//    sean for multivideo encoding begin
  {
    CriticalSectionScoped cs(data_cs_.get());
    time_of_last_incoming_frame_ms_ = TickTime::MillisecondTimestamp();
    if (EncoderPaused()) {
      TraceFrameDropStart();
      return;
    }
    TraceFrameDropEnd();
  }
    
    // 回调yuv数据返回ECMedia层
    if(ec_i420_frame_callback_) {
        int size_y = video_frame->allocated_size(kYPlane);
        int size_u = video_frame->allocated_size(kUPlane);
        int size_v = video_frame->allocated_size(kVPlane);
        uint8_t *imageBuffer = (uint8_t*)malloc(size_y + size_u + size_v);
        
        // copy y plane
        memcpy(imageBuffer, video_frame->buffer(kYPlane), size_y);
        // copy u plane
        memcpy(imageBuffer + size_y, video_frame->buffer(kUPlane), size_u);
        // copy v plane
        memcpy(imageBuffer + size_y + size_u, video_frame->buffer(kVPlane), size_v);
        
        ec_i420_frame_callback_(channel_id_, imageBuffer, size_y + size_u + size_v, video_frame->width(), video_frame->height(), video_frame->stride(kYPlane), video_frame->stride(kUPlane));
        free(imageBuffer);
    }
    
  // Convert render time, in ms, to RTP timestamp.
  const int kMsToRtpTimestamp = 90;
  const uint32_t time_stamp =
      kMsToRtpTimestamp *
      static_cast<uint32_t>(video_frame->render_time_ms());

  TRACE_EVENT_ASYNC_STEP0("cloopenwebrtc", "Video", video_frame->render_time_ms(),
                          "Encode");
  video_frame->set_timestamp(time_stamp);

  // Record raw frame.
  file_recorder_.RecordVideoFrame(*video_frame);

  // Make sure the CSRC list is correct.
  if (csrcs.size() > 0) {
    std::vector<uint32_t> temp_csrcs(csrcs.size());
    for (size_t i = 0; i < csrcs.size(); i++) {
      if (csrcs[i] == 1) {
        temp_csrcs[i] = default_rtp_rtcp_->SSRC();
      } else {
        temp_csrcs[i] = csrcs[i];
      }
    }
    default_rtp_rtcp_->SetCsrcs(temp_csrcs);
  }

  I420VideoFrame* decimated_frame = NULL;
  // TODO(wuchengli): support texture frames.
  if (video_frame->native_handle() == NULL) {
    {
      CriticalSectionScoped cs(callback_cs_.get());
      if (effect_filter_) {
        size_t length =
            CalcBufferSize(kI420, video_frame->width(), video_frame->height());
        scoped_ptr<uint8_t[]> video_buffer(new uint8_t[length]);
        ExtractBuffer(*video_frame, length, video_buffer.get());
        effect_filter_->Transform(length,
                                  video_buffer.get(),
                                  video_frame->ntp_time_ms(),
                                  video_frame->timestamp(),
                                  video_frame->width(),
                                  video_frame->height());
      }
    }

    // Pass frame via preprocessor.
    const int ret = vpm_.PreprocessFrame(*video_frame, &decimated_frame); //判断是否丢帧，空域重采样（如需要），内容分析
    if (ret == 1) {
      // Drop this frame.
      return;
    }
    if (ret != VPM_OK) {
      return;
    }
  }
  // If the frame was not resampled or scaled => use original.
  if (decimated_frame == NULL)  {
    decimated_frame = video_frame;
  }

  {
    CriticalSectionScoped cs(callback_cs_.get());
    if (pre_encode_callback_)
      pre_encode_callback_->FrameCallback(decimated_frame);
  }

  if (video_frame->native_handle() != NULL) {
    // TODO(wuchengli): add texture support. http://crbug.com/362437
    return;
  }

#ifdef VIDEOCODEC_VP8
  if (vcm_.SendCodec() == cloopenwebrtc::kVideoCodecVP8) {
    cloopenwebrtc::CodecSpecificInfo codec_specific_info;
    codec_specific_info.codecType = cloopenwebrtc::kVideoCodecVP8;
    {
      CriticalSectionScoped cs(data_cs_.get());
      codec_specific_info.codecSpecific.VP8.hasReceivedRPSI =
          has_received_rpsi_;
      codec_specific_info.codecSpecific.VP8.hasReceivedSLI =
          has_received_sli_;
      codec_specific_info.codecSpecific.VP8.pictureIdRPSI =
          picture_id_rpsi_;
      codec_specific_info.codecSpecific.VP8.pictureIdSLI  =
          picture_id_sli_;
      has_received_sli_ = false;
      has_received_rpsi_ = false;
    }

    vcm_.AddVideoFrame(*decimated_frame, vpm_.ContentMetrics(),
                       &codec_specific_info);
    return;
  }
#endif
#ifdef VIDEOCODEC_H264
    if (vcm_.SendCodec() == cloopenwebrtc::kVideoCodecH264) {
        cloopenwebrtc::CodecSpecificInfo codec_specific_info;
        codec_specific_info.codecType = cloopenwebrtc::kVideoCodecH264;

        vcm_.AddVideoFrame(*decimated_frame, vpm_.ContentMetrics(),
                           &codec_specific_info);
        return;
    }
#endif
  vcm_.AddVideoFrame(*decimated_frame);
}

void ViEEncoder::DelayChanged(int id, int frame_delay) {
  default_rtp_rtcp_->SetCameraDelay(frame_delay);

  file_recorder_.SetFrameDelay(frame_delay);
}

int ViEEncoder::GetPreferedFrameSettings(int* width,
                                         int* height,
                                         int* frame_rate) {
  cloopenwebrtc::VideoCodec video_codec;
  memset(&video_codec, 0, sizeof(video_codec));
  if (vcm_.SendCodec(&video_codec) != VCM_OK) {
    return -1;
  }

  *width = video_codec.width;
  *height = video_codec.height;
  *frame_rate = video_codec.maxFramerate;
  return 0;
}

int ViEEncoder::SendKeyFrame() {
  return vcm_.IntraFrameRequest(0);
}

int32_t ViEEncoder::SendCodecStatistics(
    uint32_t* num_key_frames, uint32_t* num_delta_frames) {
  cloopenwebrtc::VCMFrameCount sent_frames;
  if (vcm_.SentFrameCount(sent_frames) != VCM_OK) {
    return -1;
  }
  *num_key_frames = sent_frames.numKeyFrames;
  *num_delta_frames = sent_frames.numDeltaFrames;
  return 0;
}

int64_t ViEEncoder::PacerQueuingDelayMs() const {
  return paced_sender_->QueueInMs();
}

int ViEEncoder::CodecTargetBitrate(uint32_t* bitrate) const {
  if (vcm_.Bitrate(bitrate) != 0)
    return -1;
  return 0;
}

int32_t ViEEncoder::UpdateProtectionMethod(bool enable_nack) {
  bool fec_enabled = false;
  uint8_t dummy_ptype_red = 0;
  uint8_t dummy_ptypeFEC = 0;

  // Updated protection method to VCM to get correct packetization sizes.
  // FEC has larger overhead than NACK -> set FEC if used.
  int32_t error = default_rtp_rtcp_->GenericFECStatus(fec_enabled,
                                                      dummy_ptype_red,
                                                      dummy_ptypeFEC);
  if (error) {
    return -1;
  }
  if (fec_enabled_ == fec_enabled && nack_enabled_ == enable_nack) {
    // No change needed, we're already in correct state.
    return 0;
  }
  fec_enabled_ = fec_enabled;
  nack_enabled_ = enable_nack;

  // Set Video Protection for VCM.
  if (fec_enabled && nack_enabled_) {
    vcm_.SetVideoProtection(cloopenwebrtc::kProtectionNackFEC, true);
  } else {
    vcm_.SetVideoProtection(cloopenwebrtc::kProtectionFEC, fec_enabled_);
    vcm_.SetVideoProtection(cloopenwebrtc::kProtectionNackSender, nack_enabled_);
    vcm_.SetVideoProtection(cloopenwebrtc::kProtectionNackFEC, false);
  }

  if (fec_enabled_ || nack_enabled_) {
    vcm_.RegisterProtectionCallback(this);
    // The send codec must be registered to set correct MTU.
    cloopenwebrtc::VideoCodec codec;
    if (vcm_.SendCodec(&codec) == 0) {
      uint16_t max_pay_load = default_rtp_rtcp_->MaxDataPayloadLength();
      uint32_t current_bitrate_bps = 0;
      if (vcm_.Bitrate(&current_bitrate_bps) != 0) {
        LOG_F(LS_WARNING) <<
            "Failed to get the current encoder target bitrate.";
      }
      // Convert to start bitrate in kbps.
      codec.startBitrate = (current_bitrate_bps + 500) / 1000;
      if (vcm_.RegisterSendCodec(&codec, number_of_cores_, max_pay_load) != 0) {
        return -1;
      }
    }
    return 0;
  } else {
    // FEC and NACK are disabled.
    vcm_.RegisterProtectionCallback(NULL);
  }
  return 0;
}

void ViEEncoder::SetSenderBufferingMode(int target_delay_ms) {
  {
    CriticalSectionScoped cs(data_cs_.get());
    target_delay_ms_ = target_delay_ms;
  }
  if (target_delay_ms > 0) {
    // Disable external frame-droppers.
    vcm_.EnableFrameDropper(false);
    vpm_.EnableTemporalDecimation(false);
  } else {
    // Real-time mode - enable frame droppers.
    vpm_.EnableTemporalDecimation(true);
    vcm_.EnableFrameDropper(true);
  }
}

int32_t ViEEncoder::SendData(
    const uint8_t payload_type,
    const EncodedImage& encoded_image,
    const cloopenwebrtc::RTPFragmentationHeader& fragmentation_header,
    const RTPVideoHeader* rtp_video_hdr) {
  {
	  CriticalSectionScoped cs(packet_observer_cs_.get());
	  if (encoded_packet_observer_)
		  encoded_packet_observer_->SendData(payload_type, encoded_image, fragmentation_header, rtp_video_hdr);
  }

  // New encoded data, hand over to the rtp module.
  return default_rtp_rtcp_->SendOutgoingData(
      VCMEncodedFrame::ConvertFrameType(encoded_image._frameType), payload_type,
      encoded_image._timeStamp, encoded_image.capture_time_ms_,
      encoded_image._buffer, encoded_image._length, &fragmentation_header,
      rtp_video_hdr);
}

int32_t ViEEncoder::ProtectionRequest(
    const FecProtectionParams* delta_fec_params,
    const FecProtectionParams* key_fec_params,
    uint32_t* sent_video_rate_bps,
    uint32_t* sent_nack_rate_bps,
    uint32_t* sent_fec_rate_bps) {
  default_rtp_rtcp_->SetFecParameters(delta_fec_params, key_fec_params);
  default_rtp_rtcp_->BitrateSent(NULL, sent_video_rate_bps, sent_fec_rate_bps,
                                sent_nack_rate_bps);
  return 0;
}

int32_t ViEEncoder::SendStatistics(const uint32_t bit_rate,
                                   const uint32_t frame_rate) {
  bitrate_controller_->SetBitrateSent(bit_rate);
  CriticalSectionScoped cs(callback_cs_.get());
  if (codec_observer_) {
    codec_observer_->OutgoingRate(channel_id_, frame_rate, bit_rate);
  }
  if (send_statistics_proxy_)
  {
	  send_statistics_proxy_->OutgoingRate(channel_id_, frame_rate, bit_rate);
  }

  return 0;
}

int32_t ViEEncoder::RegisterCodecObserver(ViEEncoderObserver* observer) {
  CriticalSectionScoped cs(callback_cs_.get());
  if (observer && codec_observer_) {
    LOG_F(LS_ERROR) << "Observer already set.";
    return -1;
  }
  codec_observer_ = observer;
  return 0;
}

int32_t ViEEncoder::RegisterCaptureObserver(void* capture, int capture_id){
    CriticalSectionScoped cs(callback_cs_.get());
    capture_ = (cloopenwebrtc::ViECapture*)capture;
    capture_id_ = capture_id;
    return 0;
}

int32_t ViEEncoder::DeRegisterCaptureObserver(){
    CriticalSectionScoped cs(callback_cs_.get());
    capture_ = NULL;
    capture_id_ = -1;
    return 0;
}

void ViEEncoder::OnReceivedSLI(uint32_t /*ssrc*/,
                               uint8_t picture_id) {
  CriticalSectionScoped cs(data_cs_.get());
  picture_id_sli_ = picture_id;
  has_received_sli_ = true;
}

void ViEEncoder::OnReceivedRPSI(uint32_t /*ssrc*/,
                                uint64_t picture_id) {
  CriticalSectionScoped cs(data_cs_.get());
  picture_id_rpsi_ = picture_id;
  has_received_rpsi_ = true;
}

void ViEEncoder::OnReceivedIntraFrameRequest(uint32_t ssrc) {
  // Key frame request from remote side, signal to VCM.
  TRACE_EVENT0("cloopenwebrtc", "OnKeyFrameRequest");

  int idx = 0;
  {
    CriticalSectionScoped cs(data_cs_.get());
    std::map<unsigned int, int>::iterator stream_it = ssrc_streams_.find(ssrc);
    if (stream_it == ssrc_streams_.end()) {
      LOG_F(LS_WARNING) << "ssrc not found: " << ssrc << ", map size "
                        << ssrc_streams_.size();
      return;
    }
    std::map<unsigned int, int64_t>::iterator time_it =
        time_last_intra_request_ms_.find(ssrc);
    if (time_it == time_last_intra_request_ms_.end()) {
      time_last_intra_request_ms_[ssrc] = 0;
    }

    int64_t now = TickTime::MillisecondTimestamp();
    if (time_last_intra_request_ms_[ssrc] + kViEMinKeyRequestIntervalMs > now) {
      return;
    }
    time_last_intra_request_ms_[ssrc] = now;
    idx = stream_it->second;
  }
  // Release the critsect before triggering key frame.
  vcm_.IntraFrameRequest(idx);
}

void ViEEncoder::OnLocalSsrcChanged(uint32_t old_ssrc, uint32_t new_ssrc) {
  CriticalSectionScoped cs(data_cs_.get());
  std::map<unsigned int, int>::iterator it = ssrc_streams_.find(old_ssrc);
  if (it == ssrc_streams_.end()) {
    return;
  }

  ssrc_streams_[new_ssrc] = it->second;
  ssrc_streams_.erase(it);

  std::map<unsigned int, int64_t>::iterator time_it =
      time_last_intra_request_ms_.find(old_ssrc);
  int64_t last_intra_request_ms = 0;
  if (time_it != time_last_intra_request_ms_.end()) {
    last_intra_request_ms = time_it->second;
    time_last_intra_request_ms_.erase(time_it);
  }
  time_last_intra_request_ms_[new_ssrc] = last_intra_request_ms;
}

bool ViEEncoder::SetSsrcs(const std::list<unsigned int>& ssrcs) {
  VideoCodec codec;
  if (vcm_.SendCodec(&codec) != 0)
    return false;

  if (codec.numberOfSimulcastStreams > 0 &&
      ssrcs.size() != codec.numberOfSimulcastStreams) {
    return false;
  }

  CriticalSectionScoped cs(data_cs_.get());
  ssrc_streams_.clear();
  time_last_intra_request_ms_.clear();
  int idx = 0;
  for (std::list<unsigned int>::const_iterator it = ssrcs.begin();
       it != ssrcs.end(); ++it, ++idx) {
    unsigned int ssrc = *it;
    ssrc_streams_[ssrc] = idx;
  }
  if(send_statistics_proxy_)
	  send_statistics_proxy_->SetSsrcs(ssrcs);
  return true;
}

void ViEEncoder::SetMinTransmitBitrate(int min_transmit_bitrate_kbps) {
  assert(min_transmit_bitrate_kbps >= 0);
  CriticalSectionScoped crit(data_cs_.get());
  min_transmit_bitrate_kbps_ = min_transmit_bitrate_kbps;
}

// Called from ViEBitrateObserver.
void ViEEncoder::OnNetworkChanged(uint32_t bitrate_bps,
                                  uint8_t fraction_lost,
                                  int64_t round_trip_time_ms) {
  LOG(LS_VERBOSE) << "OnNetworkChanged, bitrate" << bitrate_bps
                  << " packet loss " << fraction_lost
                  << " rtt " << (unsigned long)round_trip_time_ms;
    if (capture_) {
        capture_->UpdateLossRate(capture_id_, ceil((fraction_lost/256.0)*100));
    }

  vcm_.SetChannelParameters(bitrate_bps, fraction_lost, round_trip_time_ms);
  bool video_is_suspended = vcm_.VideoSuspended();
  int bitrate_kbps = bitrate_bps / 1000;
  VideoCodec send_codec;
  if (vcm_.SendCodec(&send_codec) != 0) {
    return;
  }
  SimulcastStream* stream_configs = send_codec.simulcastStream;
  // Allocate the bandwidth between the streams.
  std::vector<uint32_t> stream_bitrates = AllocateStreamBitrates(
      bitrate_bps,
      stream_configs,
      send_codec.numberOfSimulcastStreams);
  // Find the max amount of padding we can allow ourselves to send at this
  // point, based on which streams are currently active and what our current
  // available bandwidth is.
  int pad_up_to_bitrate_kbps = 0;
  if (send_codec.numberOfSimulcastStreams == 0) {
    pad_up_to_bitrate_kbps = send_codec.minBitrate;
  } else {
    pad_up_to_bitrate_kbps =
        stream_configs[send_codec.numberOfSimulcastStreams - 1].minBitrate;
    for (int i = 0; i < send_codec.numberOfSimulcastStreams - 1; ++i) {
      pad_up_to_bitrate_kbps += stream_configs[i].targetBitrate;
    }
  }

  // Disable padding if only sending one stream and video isn't suspended and
  // min-transmit bitrate isn't used (applied later).
  if (!video_is_suspended && send_codec.numberOfSimulcastStreams <= 1)
    pad_up_to_bitrate_kbps = 0;

  {
    CriticalSectionScoped cs(data_cs_.get());
    // The amount of padding should decay to zero if no frames are being
    // captured unless a min-transmit bitrate is used.
    int64_t now_ms = TickTime::MillisecondTimestamp();
    if (now_ms - time_of_last_incoming_frame_ms_ > kStopPaddingThresholdMs)
      pad_up_to_bitrate_kbps = 0;

    // Pad up to min bitrate.
    if (pad_up_to_bitrate_kbps < min_transmit_bitrate_kbps_)
      pad_up_to_bitrate_kbps = min_transmit_bitrate_kbps_;

    // Padding may never exceed bitrate estimate.
    if (pad_up_to_bitrate_kbps > bitrate_kbps)
      pad_up_to_bitrate_kbps = bitrate_kbps;

    paced_sender_->UpdateBitrate(
        bitrate_kbps,
        PacedSender::kDefaultPaceMultiplier * bitrate_kbps,
        pad_up_to_bitrate_kbps);
    default_rtp_rtcp_->SetTargetSendBitrate(stream_bitrates);
    if (video_suspended_ == video_is_suspended)
      return;
    video_suspended_ = video_is_suspended;
  }

  // Video suspend-state changed, inform codec observer.
  CriticalSectionScoped crit(callback_cs_.get());
  if (codec_observer_) {
    LOG(LS_INFO) << "Video suspended " << video_is_suspended
                 << " for channel " << channel_id_;
    codec_observer_->SuspendChange(channel_id_, video_is_suspended);
  }
  if (send_statistics_proxy_) {
	  send_statistics_proxy_->SuspendChange(channel_id_, video_is_suspended);
  }
}

PacedSender* ViEEncoder::GetPacedSender() {
  return paced_sender_.get();
}

int32_t ViEEncoder::RegisterEffectFilter(ViEEffectFilter* effect_filter) {
  CriticalSectionScoped cs(callback_cs_.get());
  if (effect_filter != NULL && effect_filter_ != NULL) {
    LOG_F(LS_ERROR) << "Filter already set.";
    return -1;
  }
  effect_filter_ = effect_filter;
  return 0;
}

int ViEEncoder::StartDebugRecording(const char* fileNameUTF8) {
  return vcm_.StartDebugRecording(fileNameUTF8);
}

int ViEEncoder::StopDebugRecording() {
  return vcm_.StopDebugRecording();
}

void ViEEncoder::SuspendBelowMinBitrate() {
  vcm_.SuspendBelowMinBitrate();
  bitrate_controller_->EnforceMinBitrate(false);
}

void ViEEncoder::RegisterPreEncodeCallback(
    I420FrameCallback* pre_encode_callback) {
  CriticalSectionScoped cs(callback_cs_.get());
  pre_encode_callback_ = pre_encode_callback;
}

void ViEEncoder::DeRegisterPreEncodeCallback() {
  CriticalSectionScoped cs(callback_cs_.get());
  pre_encode_callback_ = NULL;
}

void ViEEncoder::RegisterPostEncodeImageCallback(
      EncodedImageCallback* post_encode_callback) {
  vcm_.RegisterPostEncodeImageCallback(post_encode_callback);
}

void ViEEncoder::DeRegisterPostEncodeImageCallback() {
  vcm_.RegisterPostEncodeImageCallback(NULL);
}

int32_t ViEEncoder::RegisterEncoderRateObserver(VideoEncoderRateObserver *observer) {
	return vcm_.RegisterEncoderRateObserver(observer);
}

int32_t ViEEncoder::RegisterExternalPacketization(VCMPacketizationCallback* transport)
{
	return vcm_.RegisterTransportCallback(transport);
}

void ViEEncoder::DeRegisterExternalPacketization()
{
	vcm_.RegisterTransportCallback(NULL);
}

void ViEEncoder::RegisterEncoderDataObserver(VCMPacketizationCallback* observer)
{
	CriticalSectionScoped cs(packet_observer_cs_.get());
	encoded_packet_observer_ = observer;
}
void ViEEncoder::DeRegisterEncoderDataObserver()
{
	CriticalSectionScoped cs(callback_cs_.get());
	encoded_packet_observer_ = NULL;
}
    
int32_t ViEEncoder::AddI420FrameCallback(ECMedia_I420FrameCallBack callback) {
    if(callback == NULL) {
        LOG(LS_WARNING) << "Failed to add i420 callback, ret is -1";
        return -1;
    }
    
    if(ec_i420_frame_callback_ == callback) {
        return 0;
    }
    
    ec_i420_frame_callback_ = callback;
    return 0;
}

SendStatisticsProxy* ViEEncoder::GetSendStatisticsProxy()
{
	return send_statistics_proxy_.get();
}

ViEFileRecorder& ViEEncoder::GetOutgoingFileRecorder() {
	return file_recorder_;
}

QMVideoSettingsCallback::QMVideoSettingsCallback(VideoProcessingModule* vpm)
    : vpm_(vpm) ,
	 stats_proxy_(NULL) {
}

QMVideoSettingsCallback::~QMVideoSettingsCallback() {
}

int32_t QMVideoSettingsCallback::SetVideoQMSettings(
													const uint32_t frame_rate,
													const uint32_t width,
													const uint32_t height) {
    uint32_t widthh = width;
    uint32_t heightt = height;
    if (widthh%8) {
        widthh = (widthh/8+1)*8;
    }
    if (heightt%8) {
        heightt = (heightt/8+1)*8;
    }
	if (stats_proxy_)
		stats_proxy_->OnQMSettingChange(frame_rate, widthh, heightt);
  return vpm_->SetTargetResolution(widthh, heightt, frame_rate);
}

void QMVideoSettingsCallback::SetSendStatsCallback(SendStatisticsProxy *stats_proxy)
{
	stats_proxy_ = stats_proxy;
}

void QMVideoSettingsCallback::InitQMSetting(const uint32_t frame_rate,
											const uint32_t width,
											const uint32_t height) {
	if (stats_proxy_)
	{
		stats_proxy_->OnQMSettingChange(frame_rate, width, height);
	}
}

}  // namespace cloopenwebrtc
