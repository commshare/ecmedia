
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#ifdef TARGET_OS_IPHONE
#import <QuartzCore/CALayer.h>
#import "video_capture_defines.h"
#import "video_capture_iOS_ObjC.h"
#import <stdint.h>

using namespace yuntongxunwebrtc;
using namespace videocapturemodule;

#include "msvideo.h"
#include "bilteral_filter.h"
#include "keyframe_detector.h"
#include "image_filter_factory.h"

//#define DEBUG_CAPTURE_YUV 1

namespace yuntongxunwebrtc {
    class IVideoRender;
    class VideoRenderCallback;
}

@interface ECIOSCaptureCCP : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {
@private
    AVCaptureDeviceInput *input;
    AVCaptureVideoDataOutput * output;
    pthread_mutex_t mutex;
    int frame_ind;
    float fps;
    float start_time;
    int frame_count;
    MSVideoSize mOutputVideoSize;
    MSVideoSize mCameraVideoSize;  //required size in portrait mode
    Boolean mDownScalingRequired;
    int mDeviceOrientation;
    #ifdef __APPLE_CC__
    VideoCaptureRotation mRotate;
    #endif
    MSAverageFPS averageFps;
    char fps_context[64];
    MSPicture *_pict;
    
    
    yuntongxunwebrtc::videocapturemodule::VideoCaptureiOS* _owner;
    
    // about opengl doRenderFrame
    IVideoRender* _ptrRenderer;
    VideoRenderCallback* _renderCallback;
    
    // UIView* parentView;
    BilteralFilterCore *bilteralFilter;
    KeyFrameDetectCore *keyframeDector;
    AVCaptureSession *_capture_session;
    Boolean isAppActive;
#if DEBUG_CAPTURE_YUV
    FILE *fout;
#endif
};

- (void)initIOSCapture;
- (int)start;
- (int)stop;
- (void)setSize:(MSVideoSize) size;
- (MSVideoSize*)getSize;
- (void)openDevice:(const char*) deviceId;
- (void)setFps:(float) value;
- (void)setBeautyFace:(BOOL)isEnable;
- (void)setVideoFilter:(ECImageFilterType) filter;

- (NSNumber*)registerOwner:(yuntongxunwebrtc::videocapturemodule::VideoCaptureiOS*)owner;
- (NSNumber*)setCaptureDeviceById:(char*)uniqueId;
- (NSNumber*)setCaptureHeight:(int)height AndWidth:(int)width AndFrameRate:(int)frameRate;
- (NSNumber*)startCapture;
- (NSNumber*)stopCapture;
- (NSNumber*)updateLossRate:(int)lossRate;
- (void)focusWithMode:(AVCaptureFocusMode)focusMode exposeWithMode:(AVCaptureExposureMode)exposureMode atDevicePoint:(CGPoint)point monitorSubjectAreaChange:(BOOL)monitorSubjectAreaChange;
- (void)subjectAreaDidChange:(NSNotification *)notification;
- (void)saveYUVtoFile:(unsigned char *)buf andwrap:(int)wrap andxsize:(int)xsize andysize:(int)ysize;
#ifdef __APPLE_CC__
- (void)setCaptureRotate:(VideoCaptureRotation)rotate;
#endif
- (BOOL) isAutoOrientation;
@property (nonatomic, retain, readonly) UIView* parentView;
@property (nonatomic, assign) bool triggered;
@property (nonatomic) dispatch_queue_t sessionQueue;

// iamge filter
@property (nonatomic, retain) ECImageRawDataInput *rawDataInput;
@property (nonatomic, retain) ECImageRawDataOutput *rawDataOutput;
@property (nonatomic, retain) ECImageView *ecImageView;
@property (nonatomic, retain) ECImageOutput<ECImageInput> *ecImageFilter;

@end


static void capture_queue_cleanup(void* p) {
    ECIOSCaptureCCP *capture = (__bridge_transfer ECIOSCaptureCCP*)p;
    capture = nil;
}
#endif

