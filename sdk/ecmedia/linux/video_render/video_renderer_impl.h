#ifndef VIDEO_RENDERER_IMPL_H
#define VIDEO_RENDERER_IMPL_H

#include "i_video_render.h"
#include "sdk/ecmedia/video_renderer.h"

class VideoRenderImpl : public VideoRenderer {
 public:
  VideoRenderImpl(int channelid,
                  void* windows,
                  int render_mode,
	              bool mirror,
                  webrtc::VideoTrackInterface* track_to_render,
                  rtc::Thread* worker_thread,
                  VideoRenderType render_type,
				   rtc::VideoSinkWants wants = rtc::VideoSinkWants());

  virtual ~VideoRenderImpl();

  int StartRender() override;
  int StopRender() override;

  int UpdateVideoTrack(webrtc::VideoTrackInterface* track_to_render,rtc::VideoSinkWants wants) override;

  // implement rtc::VideoSinkInterface<webrtc::VideoFrame>
  void OnFrame(const webrtc::VideoFrame& frame) override;

  void* WindowPtr() override  { return video_window_; }
  bool RegisterRemoteVideoResoluteCallback(
      ECMedia_FrameSizeChangeCallback* callback) override{return false;}

 private:
  rtc::Thread* worker_thread_;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
  IVideoRender* _ptrRenderer;
  void* video_window_;
};

#endif  // VIDEO_RENDERER_H