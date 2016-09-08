//
//  ECMedia.h
//  servicecoreVideo
//
//  Created by Sean Lee on 15/6/8.
//
//

#ifndef __servicecoreVideo__ECMedia__
#define __servicecoreVideo__ECMedia__

#include <stdio.h>
#include "sdk_common.h"
#include "common_types.h"

#ifdef WEBRTC_ANDROID
#include "jni.h"
#define ECMEDIA_API JNIEXPORT
#else
#define ECMEDIA_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ReturnVideoWidthHeightM)(int width,int height, int channelid);
typedef int (*onEcMediaReceivingDtmf)(int channelid, char dtmfch);//dtmf
typedef int (*onEcMediaPacketTimeout)(int channelid);
typedef int (*onEcMediaStunPacket)(int channelid, void *data, int len, const char *fromIP, int fromPort, bool isRTCP, bool isVideo);
typedef int (*onEcMediaAudioData)(int channelid, const void *data, int inLen, void *outData, int &outLen, bool send);
typedef int (*onEcMediaVideoConference)(int channelid, int status, int payload);

typedef int (*onEcMediaRequestKeyFrameCallback)(const int channelid);

typedef int (*onVoeCallbackOnError)(int channelid, int errCode);
typedef int(*onEcMediaDesktopCaptureErrCode)(int desktop_capture_id, int errCode);
typedef int (*onEcMediaShareWindowSizeChange)(int desktop_capture_id, int width, int height);

/*
 * Enable trace.
 */
ECMEDIA_API int ECMedia_set_trace(const char *logFileName,void *printhoolk,int level);
ECMEDIA_API int ECMedia_un_trace();
ECMEDIA_API const char* ECMeida_get_Version();
ECMEDIA_API void PrintConsole(const char * fmt,...);
/*
 *1
 */
ECMEDIA_API void ECMedia_set_android_objects(void* javaVM, void* env, void* context);
ECMEDIA_API int ECMedia_ring_start(int& channelid, const char *filename, bool loop);
/*
 *
 */
ECMEDIA_API int ECMedia_ring_stop(int& channelid);
/*
 *
 @return: 0: success
         1: already init
         -99: 
         VE_AUDIO_DEVICE_MODULE_ERROR: audio device init failed.
 */
ECMEDIA_API int ECMedia_init_audio();
/*
 *1
 */
ECMEDIA_API int ECMedia_uninit_audio();
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_create_channel(int& channelid, bool is_video);
/*
 *1
 */
ECMEDIA_API bool ECMedia_get_recording_status();
/*
 *1
 */
ECMEDIA_API int ECMedia_delete_channel(int& channelid, bool is_video);
/*
 *1
 */
ECMEDIA_API int ECMedia_set_local_receiver(int channelid, int rtp_port, int rtcp_port = -1);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_set_send_destination(int channelid, int rtp_port, const char *rtp_addr, int source_port = -1, int rtcp_port = -1);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_start_receive(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_stop_receive(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_start_send(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_stop_send(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_Register_voice_engine_observer(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_DeRegister_voice_engine_observer();
/*
 *
 */
ECMEDIA_API int ECMedia_set_AgcStatus(bool agc_enabled, cloopenwebrtc::AgcModes agc_mode);
/*
 *
 */
ECMEDIA_API int ECMedia_set_EcStatus(bool ec_enabled, cloopenwebrtc::EcModes ec_mode);
/*
 *
 */
ECMEDIA_API int ECMedia_set_NsStatus(bool ns_enabled, cloopenwebrtc::NsModes ns_mode);
/*
 *
 */
ECMEDIA_API int ECMedia_set_SetAecmMode(cloopenwebrtc::AecmModes aecm_mode, bool cng_enabled);

ECMEDIA_API int ECMedia_EnableHowlingControl(bool enabled);

ECMEDIA_API int ECMedia_IsHowlingControlEnabled(bool &enabled);
/*
 *
 */
ECMEDIA_API int ECMedia_set_packet_timeout_noti(int channel, int timeout);
/*
 *
 */
ECMEDIA_API int ECMedia_get_packet_timeout_noti(int channel, bool& enabled, int& timeout);
/*
 *
 */
ECMEDIA_API int ECMedia_set_network_type(int audio_channelid, int video_channelid, const char *type);
/*
 *
 */
ECMEDIA_API int ECMedia_get_network_statistic(int channelid_audio, int channelid_video, long long *duration, long long *sendTotalSim, long long *recvTotalSim, long long *sendTotalWifi, long long *recvTotalWifi);
/*
 *
 */
ECMEDIA_API int ECMedia_set_MTU(int channelid, int mtu);
/*
 *
 */
ECMEDIA_API int ECMedia_set_video_rtp_keepalive(int channelid, bool enable, int interval, int payloadType);

ECMEDIA_API int ECMedia_set_audio_rtp_keepalive(int channelid, bool enable, int interval, int payloadType);
/*
 *
 */
ECMEDIA_API int ECMedia_set_NACK_status(int channelid, bool enabled);
/*
 *
 */
ECMEDIA_API int ECMedia_set_RTCP_status(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_get_media_statistics(int channelid, bool is_video, MediaStatisticsInfo& call_stats);
/*
 *
 */
ECMEDIA_API int ECMedia_start_rtp_dump(int channelid, bool is_video, const char *file, cloopenwebrtc::RTPDirections dir);
/*
 *
 */
ECMEDIA_API int ECMedia_stop_rtp_dump(int channelid, bool is_video, cloopenwebrtc::RTPDirections dir);
/*
 *
 */
ECMEDIA_API int ECMedia_get_playout_device_num(int& speaker_count);
/*
 *
 */
ECMEDIA_API int ECMedia_get_specified_playout_device_info(int index, char *name, char *guid);
/*
 *
 */
ECMEDIA_API int ECMedia_select_playout_device(int index);
/*
 *
 */
ECMEDIA_API int ECMedia_get_record_device_num(int& microphone_count);
/*
 *
 */
ECMEDIA_API int ECMedia_get_specified_record_device_info(int index, char *name, char *guid);
/*
 *
 */
ECMEDIA_API int ECMedia_select_record_device(int index);
/*
 *
 */
ECMEDIA_API int ECMedia_set_loudspeaker_status(bool enabled);
/*
 *
 */
ECMEDIA_API int ECMedia_get_loudpeaker_status(bool& enabled);
/*
 *
 */
ECMEDIA_API int ECMedia_reset_audio_device();
/*
 *
 */
ECMEDIA_API int ECMedia_init_srtp(int channelid);
/*
 *加密
 */
ECMEDIA_API int ECMedia_enable_srtp_receive(int channelid, const char *key);
/*
 *
 */
ECMEDIA_API int ECMedia_enable_srtp_send(int channelid, const char *key);
/*
 *
 */
ECMEDIA_API int ECMedia_shutdown_srtp(int channel);
/*
 *
 */
ECMEDIA_API int ECMedia_set_speaker_volume(int volumep);
/*
 *
 */
ECMEDIA_API int ECMedia_get_speaker_volume(unsigned int& volumep);
/*
 *
 */
ECMEDIA_API int ECMedia_set_mute_status(bool mute);
/*
 *
 */
ECMEDIA_API int ECMedia_get_mute_status(bool& mute);
/*
 *
 */
ECMEDIA_API int ECMedia_set_speaker_mute_status(bool mute);
/*
 *
 */
ECMEDIA_API int ECMedia_get_speaker_mute_status(bool& mute);
/*
 *
 */
ECMEDIA_API int ECMedia_num_of_supported_codecs_audio();
/*
 *
 */
ECMEDIA_API int ECMedia_get_supported_codecs_audio(cloopenwebrtc::CodecInst codecs[]);
/*
 *1
 */
ECMEDIA_API int ECMedia_get_send_codec_audio(int channelid, cloopenwebrtc::CodecInst& audioCodec);
/*
 *1
 */
ECMEDIA_API int ECMedia_set_send_codec_audio(int channelid, cloopenwebrtc::CodecInst& audioCodec);
/*
 *1
 */
ECMEDIA_API int ECMedia_set_receive_playloadType_audio(int channelid, cloopenwebrtc::CodecInst& audioCodec);
/*
 *1
 */
ECMEDIA_API int ECMedia_get_receive_playloadType_audio(int channelid, cloopenwebrtc::CodecInst& audioCodec);
/*
 *1
 */
ECMEDIA_API int ECMedia_set_VAD_status(int channelid, cloopenwebrtc::VadModes mode, bool dtx_enabled);

/*
 *1
 */
ECMEDIA_API int ECMedia_audio_start_playout(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_stop_playout(int channelid);


ECMEDIA_API int ECMedia_audio_start_record();

ECMEDIA_API int ECMedia_audio_stop_record();

/*
 *字符 语音通道
 */
ECMEDIA_API int ECMedia_send_dtmf(int channelid, const char dtmfch);

/*
 *每次产生channel后传进来
 */
ECMEDIA_API int ECMedia_set_dtmf_cb(int channelid, onEcMediaReceivingDtmf dtmf_cb);
ECMEDIA_API int ECMedia_set_media_packet_timeout_cb(int channelid, onEcMediaPacketTimeout media_timeout_cb);
ECMEDIA_API int ECMedia_set_stun_cb(int channelid, onEcMediaStunPacket stun_cb);
ECMEDIA_API int ECMedia_set_audio_data_cb(int channelid, onEcMediaAudioData audio_data_cb);
ECMEDIA_API int ECMedia_set_voe_cb(int channelid, onVoeCallbackOnError voe_callback_cb);
/*
 * ONLY USE FOR PEER CONNECTION FOR AUDIO
 */
ECMEDIA_API int ECMedia_sendRaw(int channelid, int8_t *data, uint32_t length, int32_t isRTCP, uint16_t port = 0, const char* ip = NULL);

ECMEDIA_API int ECMedia_EnableIPV6(int channel, bool flag);
ECMEDIA_API int ECMedia_IsIPv6Enabled(int channel);

ECMEDIA_API int ECMedia_AmrNBCreateEnc();
ECMEDIA_API int ECMedia_AmrNBCreateDec();
ECMEDIA_API int ECMedia_AmrNBFreeEnc();
ECMEDIA_API int ECMedia_AmrNBFreeDec();
ECMEDIA_API int ECMedia_AmrNBEncode(short* input, short len, short*output, short mode);
ECMEDIA_API int ECMedia_AmrNBEncoderInit(short dtxMode);
ECMEDIA_API int ECMedia_AmrNBDecode(short* encoded, int len, short* decoded);
ECMEDIA_API int ECMedia_AmrNBVersion(char *versionStr, short len);

#ifdef VIDEO_ENABLED
/*
 *
 */
ECMEDIA_API int ECMedia_init_video();
/*
 *
 */
ECMEDIA_API int ECMedia_uninit_video();
/*
 *
 */
ECMEDIA_API int ECMdeia_num_of_capture_devices();
ECMEDIA_API int ECMedia_get_capture_device(int index, char *name, int name_len, char *id, int id_len);
/*
 *
 */
ECMEDIA_API int ECMedia_num_of_capabilities(const char *id, int id_len);
/*
 *
 */
ECMEDIA_API int ECMedia_get_capture_capability(const char *id, int id_len, int index, CameraCapability& capabilityp);
/*
 *
 */
ECMEDIA_API int ECMedia_allocate_capture_device(const char *id, size_t len, int& deviceid);
/*
 *
 */
ECMEDIA_API int ECMedia_connect_capture_device(int deviceid, int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_getOrientation(const char *id, ECMediaRotateCapturedFrame &tr);
/*
 *
 */
ECMEDIA_API int ECMedia_set_rotate_captured_frames(int deviceid, ECMediaRotateCapturedFrame tr);
/*
 *
 */
ECMEDIA_API int ECMedia_start_capture(int deviceid, CameraCapability cam);
ECMEDIA_API int ECMedia_video_set_local_receiver(int channelid, int rtp_port, int rtcp_port = -1);
/*
 *
 */
ECMEDIA_API int ECMedia_video_set_send_destination(int channelid, const char *rtp_addr, int rtp_port, int rtcp_port);
/*
 *
 */
ECMEDIA_API int ECMedia_video_start_receive(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_video_stop_receive(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_video_start_send(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_video_stop_send(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_set_local_video_window(int deviceid, void *video_window);
/*
 *
 */
ECMEDIA_API int ECMedia_stop_capture(int captureid);


/*
*/
ECMEDIA_API int ECMedia_allocate_desktopShare_capture(int& desktop_captureid, int capture_type);
/*
*/

/*
*/
ECMEDIA_API bool ECMedia_get_screen_list(int desktop_captureid, ScreenID *&screenList);
ECMEDIA_API bool ECMedia_get_window_list(int desktop_captureid, WindowShare *&windowList);



/*
*/
ECMEDIA_API bool ECMedia_select_screen(int desktop_captureid, ScreenID screeninfo);
ECMEDIA_API bool ECMedia_select_window(int desktop_captureid, WindowID WindowInfo);

ECMEDIA_API int ECMedia_start_desktop_capture(int desktop_captureid, int fps);
ECMEDIA_API int ECMedia_stop_desktop_capture(int desktop_captureid);
ECMEDIA_API int ECMedia_release_desktop_capture(int desktop_captureid);
ECMEDIA_API int ECMedia_connect_desktop_captureDevice(int desktop_captureid, int video_channelId);
ECMEDIA_API int ECMedia_set_desktop_share_err_code_cb(int desktop_captureid, int channelid, onEcMediaDesktopCaptureErrCode capture_err_code_cb);
ECMEDIA_API int ECMedia_set_desktop_share_window_change_cb(int desktop_captureid, int channelid, onEcMediaShareWindowSizeChange share_window_change_cb);
ECMEDIA_API int ECMedia_get_desktop_capture_size(int desktop_captureid, int &width, int &height);
/*
 *
 */
ECMEDIA_API int ECMedia_add_render(int channelid, void *video_window, ReturnVideoWidthHeightM videoResolutionCallback);
/*
 *
 */
ECMEDIA_API int ECMedia_stop_render(int channelid, int deviceid);
/*
 *
 */
ECMEDIA_API int ECMedia_num_of_supported_codecs_video();
/*
 *
 */
ECMEDIA_API int ECMedia_get_supported_codecs_video(cloopenwebrtc::VideoCodec codecs[]);

/*
 *
 */
ECMEDIA_API int ECMedia_set_key_frame_request_cb(int channelid, bool isVideoConf,onEcMediaRequestKeyFrameCallback cb);
/*
 *
 */
ECMEDIA_API int ECMedia_set_send_codec_video(int channelid, cloopenwebrtc::VideoCodec& videoCodec);
/*
 *
 */
ECMEDIA_API int ECMedia_get_send_codec_video(int channelid, cloopenwebrtc::VideoCodec& videoCodec);
/*
 *
 */
ECMEDIA_API int ECMedia_set_receive_codec_video(int channelid, cloopenwebrtc::VideoCodec& videoCodec);

#ifdef ENABLE_FEC_TEST
ECMEDIA_API int ECMedia_set_receive_codec_video_fec(int channelid, cloopenwebrtc::VideoCodec& videoCodec);
#endif
/*
 *
 */
ECMEDIA_API int ECMedia_get_receive_codec_video(int channelid, cloopenwebrtc::VideoCodec& videoCodec);


ECMEDIA_API int ECMedia_set_video_conf_cb(int channelid, onEcMediaVideoConference video_conf_cb);
ECMEDIA_API int ECMedia_set_stun_cb_video(int channelid, onEcMediaStunPacket stun_cb);
/*
* ONLY USE FOR PEER CONNECTION FOR VIDEO
*/
ECMEDIA_API int ECMedia_sendUDPPacket(const int channelid,const void* data,const unsigned int length,int& transmitted_bytes,bool use_rtcp_socket = false,uint16_t port = 0,const char* ip = NULL);
ECMEDIA_API int ECMedia_set_NACK_status_video(int channelid, bool enabled);
ECMEDIA_API int ECMedia_set_RTCP_status_video(int channelid,int mode);
ECMEDIA_API int ECMedia_setVideoConferenceFlag(int channel,const char *selfSipNo ,const char *sipNo, const char *conferenceNo, const char *confPasswd, int port, const char *ip);
ECMEDIA_API int ECMedia_send_key_frame(int channel);

ECMEDIA_API int ECMedia_video_EnableIPV6(int channel, bool flag);
ECMEDIA_API int ECMedia_video_IsIPv6Enabled(int channel);


ECMEDIA_API int ECMedia_set_FEC_status_video(const int channelid,
								const bool enable,
								const unsigned char payload_typeRED,
								const unsigned char payload_typeFEC);
ECMEDIA_API int ECMedia_set_HybridNACKFEC_status_video(const int channelid,
										const bool enable,
										const unsigned char payload_typeRED,
										const unsigned char payload_typeFEC);

ECMEDIA_API int ECMedia_start_record_screen(int channelid, const char* filename, int bitrates, int fps, int screen_index);
ECMEDIA_API int ECMedia_start_record_screen_ex(int channelid, const char* filename, int bitrates, int fps, int screen_index, int left, int top, int width, int height);
ECMEDIA_API int ECMedia_stop_record_screen(int channleid);

//capture device id
ECMEDIA_API int ECMedia_get_local_video_snapshot(int deviceid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height);
ECMEDIA_API int ECMedia_save_local_video_snapshot(int deviceid, const char* filePath);

//video channel id
ECMEDIA_API int ECMedia_get_remote_video_snapshot(int channelid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height);
ECMEDIA_API int ECMedia_save_remote_video_snapshot(int channelid, const char* filePath);

ECMEDIA_API int ECmedia_enable_deflickering(int captureid, bool enable);
ECMEDIA_API int ECmedia_enable_EnableColorEnhancement(int channelid, bool enable);
ECMEDIA_API int ECmedia_enable_EnableDenoising(int captureid, bool enable);
ECMEDIA_API int ECmedia_enable_EnableBrightnessAlarm(int captureid, bool enable);
#endif

//"filePath" or "intervalMS" is set by the first caller, i.e.: one of the four interfaces,
//the latter caller's parameters will be discarded.
ECMEDIA_API int ECMedia_set_video_SendStatistics_proxy(int channelid, char* filePath, int intervalMs);
ECMEDIA_API int ECMedia_set_video_RecvStatistics_proxy(int channelid, char* filePath, int intervalMs);
ECMEDIA_API int ECMedia_set_audio_SendStatistics_proxy(int channelid, char* filePath, int intervalMs);
ECMEDIA_API int ECMedia_set_audio_RecvStatistics_proxy(int channelid, char* filePath, int intervalMs);

//kill statistics thread
ECMEDIA_API int ECMedia_stop_Statistics_proxy();
ECMEDIA_API int ECMedia_set_CaptureDeviceID(int videoCapDevId);

ECMEDIA_API int ECMedia_Check_Record_Permission(bool &enabled);

#ifdef __cplusplus
}
#endif

#endif /* defined(__servicecoreVideo__ECMedia__) */
