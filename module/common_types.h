/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_COMMON_TYPES_H
#define WEBRTC_COMMON_TYPES_H
#include <stddef.h>
#include <string.h>

#include <string>
#include <vector>
#include <sstream>

#include "typedefs.h"

#ifdef WEBRTC_EXPORT
    #define WEBRTC_DLLEXPORT _declspec(dllexport)
#elif WEBRTC_DLL
    #define WEBRTC_DLLEXPORT _declspec(dllimport)
#else
    #define WEBRTC_DLLEXPORT
#endif

#ifndef NULL
    #define NULL 0
#endif

#define RTP_PAYLOAD_NAME_SIZE 32

#if defined(WEBRTC_WIN) || defined(WIN32)
// Compares two strings without regard to case.
#define STR_CASE_CMP(s1, s2) ::_stricmp(s1, s2)
// Compares characters of two strings without regard to case.
#define STR_NCASE_CMP(s1, s2, n) ::_strnicmp(s1, s2, n)
#else
#define STR_CASE_CMP(s1, s2) ::strcasecmp(s1, s2)
#define STR_NCASE_CMP(s1, s2, n) ::strncasecmp(s1, s2, n)
#endif

namespace cloopenwebrtc {

class Config;

class InStream
{
public:
    virtual int Read(void *buf,size_t len) = 0;
    virtual int Rewind() {return -1;}
    virtual ~InStream() {}
protected:
    InStream() {}
};

class OutStream
{
public:
    virtual bool Write(const void *buf,size_t len) = 0;
    virtual int Rewind() {return -1;}
    virtual ~OutStream() {}
protected:
    OutStream() {}
};

enum TraceModule
{
	kTraceUndefined              = 0,
    // not a module, triggered from the engine code
    kTraceVoice              = 0x0001,
    // not a module, triggered from the engine code
    kTraceVideo              = 0x0002,
    // not a module, triggered from the utility code
    kTraceUtility            = 0x0003,
    kTraceRtpRtcp            = 0x0004,
    kTraceTransport          = 0x0005,
    kTraceSrtp               = 0x0006,
    kTraceAudioCoding        = 0x0007,
    kTraceAudioMixerServer   = 0x0008,
    kTraceAudioMixerClient   = 0x0009,
    kTraceFile               = 0x000a,
    kTraceAudioProcessing    = 0x000b,
    kTraceVideoCoding        = 0x0010,
    kTraceVideoMixer         = 0x0011,
    kTraceAudioDevice        = 0x0012,
    kTraceVideoRenderer      = 0x0014,
    kTraceVideoCapture       = 0x0015,
	kTraceRemoteBitrateEstimator = 0x0017,
    gb8pjjec   = 0x0016,
    kTraceVideoPreocessing      = 0x0018
};

enum TraceLevel
{
    kTraceNone               = 0x0000,    // no trace
    kTraceStateInfo          = 0x0001,
    kTraceWarning            = 0x0002,
    kTraceError              = 0x0004,
    kTraceCritical           = 0x0008,
    kTraceApiCall            = 0x0010,
    kTraceDefault            = 0x00ff,

    kTraceModuleCall         = 0x0020,
    kTraceMemory             = 0x0100,   // memory info
    kTraceTimer              = 0x0200,   // timing info
    kTraceStream             = 0x0400,   // "continuous" stream of data

    // used for debug purposes
    kTraceDebug              = 0x0800,  // debug
    kTraceInfo               = 0x1000,  // debug info

	// Non-verbose level used by LS_INFO of logging.h. Do not use directly.
	kTraceTerseInfo          = 0x2000,
    
    kTraceAll                = 0xffff
};


// External Trace API
class TraceCallback
{
public:
    virtual void Print(const TraceLevel level,
                       const char *traceString,
                       const int length) = 0;
protected:
    virtual ~TraceCallback() {}
    TraceCallback() {}
};


enum FileFormats
{
    kFileFormatWavFile        = 1,
    kFileFormatCompressedFile = 2,
    kFileFormatAviFile        = 3,
    kFileFormatPreencodedFile = 4,
    kFileFormatPcm16kHzFile   = 7,
    kFileFormatPcm8kHzFile    = 8,
    kFileFormatPcm32kHzFile   = 9
};


enum ProcessingTypes
{
	kPlaybackPerChannel = 0,
	kPlaybackAllChannelsMixed,
	kRecordingPerChannel,
	kRecordingAllChannelsMixed,
	kRecordingPreprocessing
};

enum ccp_srtp_crypto_suite_t {
	CCPAES_128_SHA1_80 = 1,
	CCPAES_128_SHA1_32,
	CCPAES_256_SHA1_80,
	CCPAES_256_SHA1_32,
	CCPAES_128_NO_AUTH,
	CCPNO_CIPHER_SHA1_80
};

// Encryption
// SRTP uses 30 bytes key length
enum { kMaxSrtpKeyLength = 30 };
// SRTP minimum key/tag length for encryption level
enum { kMinSrtpEncryptLength = 16 };
// SRTP maximum key/tag length for encryption level
enum { kMaxSrtpEncryptLength = 256 };
// SRTP maximum key/tag length for authentication level,
// HMAC SHA1 authentication type
enum { kMaxSrtpAuthSha1Length = 20 };
// SRTP maximum tag length for authentication level,
// null authentication type
enum { kMaxSrtpTagAuthNullLength = 12 };
// SRTP maximum key length for authentication level,
// null authentication type
enum { kMaxSrtpKeyAuthNullLength = 256 };

// Interface for encrypting and decrypting regular data and rtp/rtcp packets.
// Implement this interface if you wish to provide an encryption scheme to
// the voice or video engines.
class Encryption
{
public:
    // Encrypt the given data.
    //
    // Args:
    //   channel: The channel to encrypt data for.
    //   in_data: The data to encrypt. This data is bytes_in bytes long.
    //   out_data: The buffer to write the encrypted data to. You may write more
    //       bytes of encrypted data than what you got as input, up to a maximum
    //       of cloopenwebrtc::kViEMaxMtu if you are encrypting in the video engine, or
    //       cloopenwebrtc::kVoiceEngineMaxIpPacketSizeBytes for the voice engine.
    //   bytes_in: The number of bytes in the input buffer.
    //   bytes_out: The number of bytes written in out_data.
    virtual void encrypt(
        int channel,
        unsigned char* in_data,
        unsigned char* out_data,
        int bytes_in,
        int* bytes_out) = 0;

    // Decrypts the given data. This should reverse the effects of encrypt().
    //
    // Args:
    //   channel_no: The channel to decrypt data for.
    //   in_data: The data to decrypt. This data is bytes_in bytes long.
    //   out_data: The buffer to write the decrypted data to. You may write more
    //       bytes of decrypted data than what you got as input, up to a maximum
    //       of cloopenwebrtc::kViEMaxMtu if you are encrypting in the video engine, or
    //       cloopenwebrtc::kVoiceEngineMaxIpPacketSizeBytes for the voice engine.
    //   bytes_in: The number of bytes in the input buffer.
    //   bytes_out: The number of bytes written in out_data.
    virtual void decrypt(
        int channel,
        unsigned char* in_data,
        unsigned char* out_data,
        int bytes_in,
        int* bytes_out) = 0;

    // Encrypts a RTCP packet. Otherwise, this method has the same contract as
    // encrypt().
    virtual void encrypt_rtcp(
        int channel,
        unsigned char* in_data,
        unsigned char* out_data,
        int bytes_in,
        int* bytes_out) = 0;

    // Decrypts a RTCP packet. Otherwise, this method has the same contract as
    // decrypt().
    virtual void decrypt_rtcp(
        int channel,
        unsigned char* in_data,
        unsigned char* out_data,
        int bytes_in,
        int* bytes_out) = 0;

protected:
    virtual ~Encryption() {}
    Encryption() {}
};


// ==================================================================
// Voice specific types
// ==================================================================

// Each codec supported can be described by this structure.
struct CodecInst
{
    int pltype;
    char plname[RTP_PAYLOAD_NAME_SIZE];
    int plfreq;
    int pacsize;
    int channels;
    int rate;
    bool fecEnabled;
    double packetLossRate;
};

enum FrameType
{
    kFrameEmpty            = 0,
    kAudioFrameSpeech      = 1,
    kAudioFrameCN          = 2,
    kVideoFrameKey         = 3,    // independent frame
    kVideoFrameDelta       = 4,    // depends on the previus frame
    kVideoFrameGolden      = 5,    // depends on a old known previus frame
    kVideoFrameAltRef      = 6
};

// External transport callback interface
class Transport
{
public:
   
	virtual int SendPacket(int channel, const void *data, size_t len, int sn=0) = 0;
	virtual int SendRTCPPacket(int channel, const void *data, size_t len) = 0;
    virtual void SetRtpData(int channel,void * ,int type) {};

protected:
	virtual ~Transport() {}
	Transport() {}
};

class RTPFragmentationHeader;
// Callback class used for sending data ready to be packetized
class AudioPacketizationCallback {
public:
	virtual ~AudioPacketizationCallback() {}

	virtual int32_t SendData(FrameType frame_type,
		uint8_t payload_type,
		uint32_t timestamp,
		const uint8_t* payload_data,
		size_t payload_len_bytes,
		const RTPFragmentationHeader* fragmentation) = 0;
};

// RTP
enum {kRtpCsrcSize = 15}; // RFC 3550 page 13

enum RTPDirections
{
    kRtpIncoming = 0,
    kRtpOutgoing
};

enum PayloadFrequencies
{
    kFreq8000Hz = 8000,
    kFreq16000Hz = 16000,
    kFreq32000Hz = 32000
};

enum VadModes                 // degree of bandwidth reduction
{
    kVadConventional = 0,      // lowest reduction
    kVadAggressiveLow,
    kVadAggressiveMid,
    kVadAggressiveHigh         // highest reduction
};

struct NetworkStatistics           // NETEQ statistics
{
    // current jitter buffer size in ms
    WebRtc_UWord16 currentBufferSize;
    // preferred (optimal) buffer size in ms
    WebRtc_UWord16 preferredBufferSize;
    // adding extra delay due to "peaky jitter"
    bool jitterPeaksFound;
    // loss rate (network + late) in percent (in Q14)
    WebRtc_UWord16 currentPacketLossRate;
    // late loss rate in percent (in Q14)
    WebRtc_UWord16 currentDiscardRate;
    // fraction (of original stream) of synthesized speech inserted through
    // expansion (in Q14)
    WebRtc_UWord16 currentExpandRate;
    // fraction of synthesized speech inserted through pre-emptive expansion
    // (in Q14)
    WebRtc_UWord16 currentPreemptiveRate;
    // fraction of data removed through acceleration (in Q14)
    WebRtc_UWord16 currentAccelerateRate;
    // clock-drift in parts-per-million (negative or positive)
    int32_t clockDriftPPM;
    // average packet waiting time in the jitter buffer (ms)
    int meanWaitingTimeMs;
    // median packet waiting time in the jitter buffer (ms)
    int medianWaitingTimeMs;
    // min packet waiting time in the jitter buffer (ms)
    int minWaitingTimeMs;
    // max packet waiting time in the jitter buffer (ms)
    int maxWaitingTimeMs;
};
// Statistics for calls to AudioCodingModule::PlayoutData10Ms().
struct AudioDecodingCallStats {
	AudioDecodingCallStats()
		: calls_to_silence_generator(0),
		calls_to_neteq(0),
		decoded_normal(0),
		decoded_plc(0),
		decoded_cng(0),
		decoded_plc_cng(0) {}

	int calls_to_silence_generator;  // Number of calls where silence generated,
	// and NetEq was disengaged from decoding.
	int calls_to_neteq;  // Number of calls to NetEq.
	int decoded_normal;  // Number of calls where audio RTP packet decoded.
	int decoded_plc;  // Number of calls resulted in PLC.
	int decoded_cng;  // Number of calls where comfort noise generated due to DTX.
	int decoded_plc_cng;  // Number of calls resulted where PLC faded to CNG.
};

typedef struct
{
    int min;              // minumum
    int max;              // maximum
    int average;          // average
} StatVal;

typedef struct           // All levels are reported in dBm0
{
    StatVal speech_rx;   // long-term speech levels on receiving side
    StatVal speech_tx;   // long-term speech levels on transmitting side
    StatVal noise_rx;    // long-term noise/silence levels on receiving side
    StatVal noise_tx;    // long-term noise/silence levels on transmitting side
} LevelStatistics;

typedef struct        // All levels are reported in dB
{
    StatVal erl;      // Echo Return Loss
    StatVal erle;     // Echo Return Loss Enhancement
    StatVal rerl;     // RERL = ERL + ERLE
    // Echo suppression inside EC at the point just before its NLP
    StatVal a_nlp;
} EchoStatistics;

enum TelephoneEventDetectionMethods
{
    kInBand = 0,
    kOutOfBand = 1,
    kInAndOutOfBand = 2
};

enum NsModes    // type of Noise Suppression
{
    kNsUnchanged = 0,   // previously set mode
    kNsDefault,         // platform default
    kNsConference,      // conferencing default
    kNsLowSuppression,  // lowest suppression
    kNsModerateSuppression,
    kNsHighSuppression,
    kNsVeryHighSuppression,     // highest suppression
};

enum AgcModes                  // type of Automatic Gain Control
{
    kAgcUnchanged = 0,        // previously set mode
    kAgcDefault,              // platform default
    // adaptive mode for use when analog volume control exists (e.g. for
    // PC softphone)
    kAgcAdaptiveAnalog,
    // scaling takes place in the digital domain (e.g. for conference servers
    // and embedded devices)
    kAgcAdaptiveDigital,
    // can be used on embedded devices where the capture signal level
    // is predictable
    kAgcFixedDigital
};

// EC modes
enum EcModes                   // type of Echo Control
{
    kEcUnchanged = 0,          // previously set mode
    kEcDefault,                // platform default
    kEcConference,             // conferencing default (aggressive AEC)
    kEcAec,                    // Acoustic Echo Cancellation
    kEcAecm,                   // AEC mobile
};

// AECM modes
enum AecmModes                 // mode of AECM
{
    kAecmQuietEarpieceOrHeadset = 0,
                               // Quiet earpiece or headset use
    kAecmEarpiece,             // most earpiece use
    kAecmLoudEarpiece,         // Loud earpiece or quiet speakerphone use
    kAecmSpeakerphone,         // most speakerphone use (default)
    kAecmLoudSpeakerphone      // Loud speakerphone
};

// AGC configuration
typedef struct
{
    unsigned short targetLeveldBOv;
    unsigned short digitalCompressionGaindB;
    bool           limiterEnable;
} AgcConfig;                  // AGC configuration parameters

enum StereoChannel
{
    kStereoLeft = 0,
    kStereoRight,
    kStereoBoth
};

// Audio device layers
enum AudioLayers
{
    kAudioPlatformDefault = 0,
    kAudioWindowsWave = 1,
    kAudioWindowsCore = 2,
    kAudioLinuxAlsa = 3,
    kAudioLinuxPulse = 4
};

enum NetEqModes             // NetEQ playout configurations
{
    // Optimized trade-off between low delay and jitter robustness for two-way
    // communication.
    kNetEqDefault = 0,
    // Improved jitter robustness at the cost of increased delay. Can be
    // used in one-way communication.
    kNetEqStreaming = 1,
    // Optimzed for decodability of fax signals rather than for perceived audio
    // quality.
    kNetEqFax = 2,
};

enum NetEqBgnModes          // NetEQ Background Noise (BGN) configurations
{
    // BGN is always on and will be generated when the incoming RTP stream
    // stops (default).
    kBgnOn = 0,
    // The BGN is faded to zero (complete silence) after a few seconds.
    kBgnFade = 1,
    // BGN is not used at all. Silence is produced after speech extrapolation
    // has faded.
    kBgnOff = 2,
};

enum OnHoldModes            // On Hold direction
{
    kHoldSendAndPlay = 0,    // Put both sending and playing in on-hold state.
    kHoldSendOnly,           // Put only sending in on-hold state.
    kHoldPlayOnly            // Put only playing in on-hold state.
};

enum AmrMode
{
    kRfc3267BwEfficient = 0,
    kRfc3267OctetAligned = 1,
    kRfc3267FileStorage = 2,
};

// ==================================================================
// Video specific types
// ==================================================================

// Raw video types
enum RawVideoType
{
    kVideoI420     = 0,
    kVideoYV12     = 1,
    kVideoYUY2     = 2,
    kVideoUYVY     = 3,
    kVideoIYUV     = 4,
    kVideoARGB     = 5,
    kVideoRGB24    = 6,
    kVideoRGB565   = 7,
    kVideoARGB4444 = 8,
    kVideoARGB1555 = 9,
    kVideoMJPEG    = 10,
    kVideoNV12     = 11,
    kVideoNV21     = 12,
    kVideoBGRA     = 13,
    kVideoUnknown  = 99
};

// Video codec
enum { kConfigParameterSize = 128};
enum { kPayloadNameSize = 32};
enum { kMaxSimulcastStreams = 4};
enum { kMaxTemporalStreams = 4};

enum VideoCodecComplexity
{
    kComplexityNormal = 0,
    kComplexityHigh    = 1,
    kComplexityHigher  = 2,
    kComplexityMax     = 3
};

enum VideoCodecProfile
{
    kProfileBase = 0x00,
    kProfileMain = 0x01
};

enum VP8ResilienceMode {
  kResilienceOff,    // The stream produced by the encoder requires a
                     // recovery frame (typically a key frame) to be
                     // decodable after a packet loss.
  kResilientStream,  // A stream produced by the encoder is resilient to
                     // packet losses, but packets within a frame subsequent
                     // to a loss can't be decoded.
  kResilientFrames   // Same as kResilientStream but with added resilience
                     // within a frame.
};

// VP8 specific
struct VideoCodecVP8 {
	bool                 pictureLossIndicationOn;
	bool                 feedbackModeOn;
	VideoCodecComplexity complexity;
	VP8ResilienceMode    resilience;
	unsigned char        numberOfTemporalLayers;
	bool                 denoisingOn;
	bool                 errorConcealmentOn;
	bool                 automaticResizeOn;
	bool                 frameDroppingOn;
	int                  keyFrameInterval;

	bool operator==(const VideoCodecVP8& other) const {
		return pictureLossIndicationOn == other.pictureLossIndicationOn &&
			feedbackModeOn == other.feedbackModeOn &&
			complexity == other.complexity &&
			resilience == other.resilience &&
			numberOfTemporalLayers == other.numberOfTemporalLayers &&
			denoisingOn == other.denoisingOn &&
			errorConcealmentOn == other.errorConcealmentOn &&
			automaticResizeOn == other.automaticResizeOn &&
			frameDroppingOn == other.frameDroppingOn &&
			keyFrameInterval == other.keyFrameInterval;
	}

	bool operator!=(const VideoCodecVP8& other) const {
		return !(*this == other);
	}
};
    
enum H264Packetization
{    
    kH264SingleMode         = 0,    
    kH264NonInterleavedMode = 1    
};
    
// H264 specific.
struct VideoCodecH264 {
	VideoCodecProfile profile;
	bool           frameDroppingOn;
	int            keyFrameInterval;
	// These are NULL/0 if not externally negotiated.
	const uint8_t* spsData;
	size_t         spsLen;
	const uint8_t* ppsData;
	size_t         ppsLen;
};

struct VideoCodecH264SVC
{
	unsigned char			numberOfSpatialLayers;
	unsigned char			numberOfTemporalLayers;
	H264Packetization       packetization;
	VideoCodecProfile       profile;
	char                    level;
	bool					bEnableDenoise;
	bool					bEnableSceneChangeDetect;
	bool					bEnableAdaptiveQuant;
	bool					bEnableFrameSkip;
};

// Unknown specific
struct VideoCodecGeneric
{
};


// Video codec types
enum VideoCodecType {
	kVideoCodecVP8,
	kVideoCodecVP9,
	kVideoCodecH264,
	kVideoCodecH264SVC,
	kVideoCodecI420,
	kVideoCodecRED,
	kVideoCodecULPFEC,
	kVideoCodecGeneric,
	kVideoCodecUnknown
};

union VideoCodecUnion
{
    VideoCodecVP8       VP8;
    VideoCodecGeneric   Generic;
    VideoCodecH264       H264;
	VideoCodecH264SVC	H264Svc;
};


// Simulcast is when the same stream is encoded multiple times with different
// settings such as resolution.
struct SimulcastStream {
	unsigned short      width;
	unsigned short      height;
	unsigned char       numberOfTemporalLayers;
	unsigned int        maxBitrate;  // kilobits/sec.
	unsigned int        targetBitrate;  // kilobits/sec.
	unsigned int        minBitrate;  // kilobits/sec.
	unsigned int        qpMax; // minimum quality

	bool operator==(const SimulcastStream& other) const {
		return width == other.width &&
			height == other.height &&
			numberOfTemporalLayers == other.numberOfTemporalLayers &&
			maxBitrate == other.maxBitrate &&
			targetBitrate == other.targetBitrate &&
			minBitrate == other.minBitrate &&
			qpMax == other.qpMax;
	}

	bool operator!=(const SimulcastStream& other) const {
		return !(*this == other);
	}

	std::string ToString() const {
		std::stringstream ss;
		ss << "{width: " << width;
		ss << ", height: " << height;
		ss << ", max_bitrate: " << maxBitrate;
		ss << ", min_bitrate: " << minBitrate;
		ss << ", target_bitrate: " << targetBitrate;
		ss << ", temporal_layer_num: " << (int)numberOfTemporalLayers;
		ss << '}';
		return ss.str();
	}
};

enum VideoCodecMode {
	kRealtimeVideo,
	kScreensharing,
	kSaveToFile
};

enum FrameScaleType {
    kScaleTypeCropping,
    kScaleTypeFilling,
};

// Common video codec properties
struct VideoCodec {
	VideoCodecType      codecType;
	char                plName[kPayloadNameSize];
	unsigned char       plType;

	unsigned short      width;
	unsigned short      height;

	unsigned int        startBitrate;  // kilobits/sec.
	unsigned int        maxBitrate;  // kilobits/sec.
	unsigned int        minBitrate;  // kilobits/sec.
	unsigned int        targetBitrate;  // kilobits/sec.

	unsigned char       maxFramerate;

	VideoCodecUnion     codecSpecific;

	unsigned int        qpMax;
	unsigned char       numberOfSimulcastStreams;
	SimulcastStream     simulcastStream[kMaxSimulcastStreams];

	VideoCodecMode      mode;

	// When using an external encoder/decoder this allows to pass
	// extra options without requiring webrtc to be aware of them.
	Config*  extra_options;
    
    unsigned int        numberOfLayer; //number of vp8 temporal layer

	bool operator==(const VideoCodec& other) const {
		bool ret = codecType == other.codecType &&
			(STR_CASE_CMP(plName, other.plName) == 0) &&
			plType == other.plType &&
			width == other.width &&
			height == other.height &&
			startBitrate == other.startBitrate &&
			maxBitrate == other.maxBitrate &&
			minBitrate == other.minBitrate &&
			targetBitrate == other.targetBitrate &&
			maxFramerate == other.maxFramerate &&
			qpMax == other.qpMax &&
			numberOfSimulcastStreams == other.numberOfSimulcastStreams &&
			mode == other.mode;
		if (ret && codecType == kVideoCodecVP8) {
			ret &= (codecSpecific.VP8 == other.codecSpecific.VP8);
		}

		for (unsigned char i = 0; i < other.numberOfSimulcastStreams && ret; ++i) {
			ret &= (simulcastStream[i] == other.simulcastStream[i]);
		}
		return ret;
	}

	bool operator!=(const VideoCodec& other) const {
		return !(*this == other);
	}
};

// Statistics for an RTCP channel
struct RtcpStatistics {
	RtcpStatistics()
		: fraction_lost(0),
		cumulative_lost(0),
		extended_max_sequence_number(0),
		jitter(0) {}

	uint8_t fraction_lost;
	uint32_t cumulative_lost;
	uint32_t extended_max_sequence_number;
	uint32_t jitter;
};

class RtcpStatisticsCallback {
public:
	virtual ~RtcpStatisticsCallback() {}

	virtual void StatisticsUpdated(const RtcpStatistics& statistics,
		uint32_t ssrc) = 0;
	virtual void CNameChanged(const char* cname, uint32_t ssrc) = 0;
};

// Statistics for RTCP packet types.
struct RtcpPacketTypeCounter {
	RtcpPacketTypeCounter()
		: first_packet_time_ms(-1),
		nack_packets(0),
		fir_packets(0),
		pli_packets(0),
		nack_requests(0),
		unique_nack_requests(0) {}

	void Add(const RtcpPacketTypeCounter& other) {
		nack_packets += other.nack_packets;
		fir_packets += other.fir_packets;
		pli_packets += other.pli_packets;
		nack_requests += other.nack_requests;
		unique_nack_requests += other.unique_nack_requests;
		if (other.first_packet_time_ms != -1 &&
			(other.first_packet_time_ms < first_packet_time_ms ||
			first_packet_time_ms == -1)) {
				// Use oldest time.
				first_packet_time_ms = other.first_packet_time_ms;
		}
	}

	int64_t TimeSinceFirstPacketInMs(int64_t now_ms) const {
		return (first_packet_time_ms == -1) ? -1 : (now_ms - first_packet_time_ms);
	}

	int UniqueNackRequestsInPercent() const {
		if (nack_requests == 0) {
			return 0;
		}
		return static_cast<int>(
			(unique_nack_requests * 100.0f / nack_requests) + 0.5f);
	}

	int64_t first_packet_time_ms;  // Time when first packet is sent/received.
	uint32_t nack_packets;   // Number of RTCP NACK packets.
	uint32_t fir_packets;    // Number of RTCP FIR packets.
	uint32_t pli_packets;    // Number of RTCP PLI packets.
	uint32_t nack_requests;  // Number of NACKed RTP packets.
	uint32_t unique_nack_requests;  // Number of unique NACKed RTP packets.
};

class RtcpPacketTypeCounterObserver {
public:
	virtual ~RtcpPacketTypeCounterObserver() {}
	virtual void RtcpPacketTypesCounterUpdated(
		uint32_t ssrc,
		const RtcpPacketTypeCounter& packet_counter) = 0;
};

// Data usage statistics for a (rtp) stream.
struct StreamDataCounters {
	StreamDataCounters()
		: first_packet_time_ms(-1),
		bytes(0),
		header_bytes(0),
		padding_bytes(0),
		packets(0),
		retransmitted_bytes(0),
		retransmitted_header_bytes(0),
		retransmitted_padding_bytes(0),
		retransmitted_packets(0),
		fec_packets(0) {}

	void Add(const StreamDataCounters& other) {
		bytes += other.bytes;
		header_bytes += other.header_bytes;
		padding_bytes += other.padding_bytes;
		packets += other.packets;
		retransmitted_bytes += other.retransmitted_bytes;
		retransmitted_header_bytes += other.retransmitted_header_bytes;
		retransmitted_padding_bytes += other.retransmitted_padding_bytes;
		retransmitted_packets += other.retransmitted_packets;
		fec_packets += other.fec_packets;
		if (other.first_packet_time_ms != -1 &&
			(other.first_packet_time_ms < first_packet_time_ms ||
			first_packet_time_ms == -1)) {
				// Use oldest time.
				first_packet_time_ms = other.first_packet_time_ms;
		}
	}

	int64_t TimeSinceFirstPacketInMs(int64_t now_ms) const {
		return (first_packet_time_ms == -1) ? -1 : (now_ms - first_packet_time_ms);
	}

	size_t TotalBytes() const {
		return bytes + header_bytes + padding_bytes;
	}

	size_t RetransmittedBytes() const {
		return retransmitted_bytes + retransmitted_header_bytes +
			retransmitted_padding_bytes;
	}

	size_t MediaPayloadBytes() const {
		return bytes - retransmitted_bytes;
	}

	// TODO(pbos): Rename bytes -> media_bytes.
	int64_t first_packet_time_ms;  // Time when first packet is sent/received.
	size_t bytes;  // Payload bytes, excluding RTP headers and padding.
	size_t header_bytes;  // Number of bytes used by RTP headers.
	size_t padding_bytes;  // Number of padding bytes.
	uint32_t packets;  // Number of packets.
	size_t retransmitted_bytes;  // Number of retransmitted payload bytes.
	size_t retransmitted_header_bytes;  // Retransmitted bytes used by RTP header.
	size_t retransmitted_padding_bytes;  // Retransmitted padding bytes.
	uint32_t retransmitted_packets;  // Number of retransmitted packets.
	uint32_t fec_packets;  // Number of redundancy packets.
};


// Callback, called whenever byte/packet counts have been updated.
class StreamDataCountersCallback {
public:
	virtual ~StreamDataCountersCallback() {}

	virtual void DataCountersUpdated(const StreamDataCounters& counters,
		uint32_t ssrc) = 0;
};

// Rate statistics for a stream.
struct BitrateStatistics {
	BitrateStatistics() : bitrate_bps(0), packet_rate(0), timestamp_ms(0) {}

	uint32_t bitrate_bps;   // Bitrate in bits per second.
	uint32_t packet_rate;   // Packet rate in packets per second.
	uint64_t timestamp_ms;  // Ntp timestamp in ms at time of rate estimation.
};

// Callback, used to notify an observer whenever new rates have been estimated.
class BitrateStatisticsObserver {
public:
	virtual ~BitrateStatisticsObserver() {}

	virtual void Notify(const BitrateStatistics& total_stats,
		const BitrateStatistics& retransmit_stats,
		uint32_t ssrc) = 0;
};

struct FrameCounts {
	FrameCounts() : key_frames(0), delta_frames(0) {}
	int key_frames;
	int delta_frames;
};


// Callback, used to notify an observer whenever frame counts have been updated.
class FrameCountObserver {
public:
	virtual ~FrameCountObserver() {}
	virtual void FrameCountUpdated(const FrameCounts& frame_counts,
		uint32_t ssrc) = 0;
};

// Callback, used to notify an observer whenever the send-side delay is updated.
class SendSideDelayObserver {
public:
	virtual ~SendSideDelayObserver() {}
	virtual void SendSideDelayUpdated(int avg_delay_ms,
		int max_delay_ms,
		uint32_t ssrc) = 0;
};

// Bandwidth over-use detector options.  These are used to drive
// experimentation with bandwidth estimation parameters.
// See modules/remote_bitrate_estimator/overuse_detector.h
struct OverUseDetectorOptions {
	OverUseDetectorOptions()
		: initial_slope(8.0/512.0),
		initial_offset(0),
		initial_e(),
		initial_process_noise(),
		initial_avg_noise(0.0),
		initial_var_noise(50),
		initial_threshold(25.0) {
			initial_e[0][0] = 100;
			initial_e[1][1] = 1e-1;
			initial_e[0][1] = initial_e[1][0] = 0;
			initial_process_noise[0] = 1e-10;
			initial_process_noise[1] = 1e-2;
	}
	double initial_slope;
	double initial_offset;
	double initial_e[2][2];
	double initial_process_noise[2];
	double initial_avg_noise;
	double initial_var_noise;
	double initial_threshold;
};

// This structure will have the information about when packet is actually
// received by socket.
struct PacketTime {
	PacketTime() : timestamp(-1), not_before(-1) {}
	PacketTime(int64_t timestamp, int64_t not_before)
		: timestamp(timestamp), not_before(not_before) {
	}

	int64_t timestamp;   // Receive time after socket delivers the data.
	int64_t not_before;  // Earliest possible time the data could have arrived,
	// indicating the potential error in the |timestamp|
	// value,in case the system is busy.
	// For example, the time of the last select() call.
	// If unknown, this value will be set to zero.
};

struct RTPHeaderExtension {
	RTPHeaderExtension()
		: hasTransmissionTimeOffset(false),
		transmissionTimeOffset(0),
		hasAbsoluteSendTime(false),
		absoluteSendTime(0),
		hasAudioLevel(false),
		audioLevel(0),
        hasLossRate(false),
        lossRate(0){}

	bool hasTransmissionTimeOffset;
	int32_t transmissionTimeOffset;
	bool hasAbsoluteSendTime;
	uint32_t absoluteSendTime;

	// Audio Level includes both level in dBov and voiced/unvoiced bit. See:
	// https://datatracker.ietf.org/doc/draft-lennox-avt-rtp-audio-level-exthdr/
	bool hasAudioLevel;
	uint8_t audioLevel;
    bool hasLossRate;
    uint8_t lossRate;
};

struct RTPHeader {
	RTPHeader()
		: markerBit(false),
		payloadType(0),
		sequenceNumber(0),
		timestamp(0),
		ssrc(0),
		numCSRCs(0),
		paddingLength(0),
		headerLength(0),
		payload_type_frequency(0),
		extension() {
			memset(&arrOfCSRCs, 0, sizeof(arrOfCSRCs));
	}

	bool markerBit;
	uint8_t payloadType;
	uint16_t sequenceNumber;
	uint32_t timestamp;
	uint32_t ssrc;
	uint8_t numCSRCs;
	uint32_t arrOfCSRCs[kRtpCsrcSize];
	size_t paddingLength;
	size_t headerLength;
	int payload_type_frequency;
	RTPHeaderExtension extension;
};
    
    
// vcm qm 策略调整选项
enum VCMQmResolutionMode {
    kResolutionModeNone = 0,     // spatial 和 temporal 都不调整
    kResolutionModeOnlySpatial,  // 只调整 spatial
    kResolutionModeOnlyTemporal, // 只调整 temporal
    kResolutionModeBoth          // 同时调整spatial 和 temporal.
};

// RTCP mode to use. Compound mode is described by RFC 4585 and reduced-size
// RTCP mode is described by RFC 5506.
enum RtcpMode { kOff, kCompound, kReducedSize };

/**
 * 原始I420格式视频回调，回调函数中不应处理较长时间任务，函数执行完buffer即被释放，根据需要注意是否拷贝buffer数据
 * channel_id   : channel id
 * buffer       : 标准一帧 i420 视频数据
 * length       : buffer长度
 * with         : 视频宽
 * height       : 视频高
 * y_stride     : y stride
 * uv_stride    : u,v stride
 */
typedef void (*ECMedia_I420FrameCallBack)(int32_t channel_id,  uint8_t* buffer, int length, int width, int height, int y_stride, int uv_stride);

/**
 * 原始PCM音频数据，回调函数中不能处理长时间任务，函数执行完音频数据被释放，注意是否需要拷贝buffer数据
 * channel_id   : channel id
 * buffer       : 原始PCM音频数据，16位小端格式
 * length       : buffer 长度
 * samples      : buffer 的采样个数
 * sampleRate   : 采样率
 * numChannels  : 声道个数
 */
 typedef void (*ECMedia_PCMDataCallBack)(int32_t channel_id, uint8_t* buffer, int length, int samples, int sampleRate, int numChannels);

}  // namespace cloopenwebrtc
#endif  // WEBRTC_COMMON_TYPES_H
