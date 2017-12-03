#import "ECImageContext.h"
#import "ECImageFramebuffer.h"

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
// For now, just redefine this on the Mac
typedef NS_ENUM(NSInteger, UIImageOrientation) {
    UIImageOrientationUp,            // default orientation
    UIImageOrientationDown,          // 180 deg rotation
    UIImageOrientationLeft,          // 90 deg CCW
    UIImageOrientationRight,         // 90 deg CW
    UIImageOrientationUpMirrored,    // as above but image mirrored along other axis. horizontal flip
    UIImageOrientationDownMirrored,  // horizontal flip
    UIImageOrientationLeftMirrored,  // vertical flip
    UIImageOrientationRightMirrored, // vertical flip
};
#endif

void runOnMainQueueWithoutDeadlocking(void (^block)(void));
void ec_runSynchronouslyOnVideoProcessingQueue(void (^block)(void));
void ec_runAsynchronouslyOnVideoProcessingQueue(void (^block)(void));
void runSynchronouslyOnContextQueue(ECImageContext *context, void (^block)(void));
void runAsynchronouslyOnContextQueue(ECImageContext *context, void (^block)(void));
void reportAvailableMemoryForECImage(NSString *tag);

@class ECImageMovieWriter;

/** ECImage's base source object
 
 Images or frames of video are uploaded from source objects, which are subclasses of ECImageOutput. These include:
 
 - ECImageVideoCamera (for live video from an iOS camera) 
 - ECImageStillCamera (for taking photos with the camera)
 - ECImagePicture (for still images)
 - ECImageMovie (for movies)
 
 Source objects upload still image frames to OpenGL ES as textures, then hand those textures off to the next objects in the processing chain.
 */
@interface ECImageOutput : NSObject
{
    ECImageFramebuffer *outputFramebuffer;
    
    NSMutableArray *targets, *targetTextureIndices;
    
    CGSize inputTextureSize, cachedMaximumOutputSize, forcedMaximumSize;
    
    BOOL overrideInputSize;
    
    BOOL allTargetsWantMonochromeData;
    BOOL usingNextFrameForImageCapture;
}

@property(readwrite, nonatomic) BOOL shouldSmoothlyScaleOutput;
@property(readwrite, nonatomic) BOOL shouldIgnoreUpdatesToThisTarget;
@property(readwrite, nonatomic, retain) ECImageMovieWriter *audioEncodingTarget;
@property(readwrite, nonatomic, unsafe_unretained) id<ECImageInput> targetToIgnoreForUpdates;
@property(nonatomic, copy) void(^frameProcessingCompletionBlock)(ECImageOutput*, CMTime);
@property(nonatomic) BOOL enabled;
@property(readwrite, nonatomic) GPUTextureOptions outputTextureOptions;

/// @name Managing targets
- (void)setInputFramebufferForTarget:(id<ECImageInput>)target atIndex:(NSInteger)inputTextureIndex;
- (ECImageFramebuffer *)framebufferForOutput;
- (void)removeOutputFramebuffer;
- (void)notifyTargetsAboutNewOutputTexture;

/** Returns an array of the current targets.
 */
- (NSArray*)targets;

/** Adds a target to receive notifications when new frames are available.
 
 The target will be asked for its next available texture.
 
 See [ECImageInput newFrameReadyAtTime:]
 
 @param newTarget Target to be added
 */
- (void)addTarget:(id<ECImageInput>)newTarget;

/** Adds a target to receive notifications when new frames are available.
 
 See [ECImageInput newFrameReadyAtTime:]
 
 @param newTarget Target to be added
 */
- (void)addTarget:(id<ECImageInput>)newTarget atTextureLocation:(NSInteger)textureLocation;

/** Removes a target. The target will no longer receive notifications when new frames are available.
 
 @param targetToRemove Target to be removed
 */
- (void)removeTarget:(id<ECImageInput>)targetToRemove;

/** Removes all targets.
 */
- (void)removeAllTargets;

/// @name Manage the output texture

- (void)forceProcessingAtSize:(CGSize)frameSize;
- (void)forceProcessingAtSizeRespectingAspectRatio:(CGSize)frameSize;

/// @name Still image processing

- (void)useNextFrameForImageCapture;
- (CGImageRef)newCGImageFromCurrentlyProcessedOutput;
- (CGImageRef)newCGImageByFilteringCGImage:(CGImageRef)imageToFilter;

// Platform-specific image output methods
// If you're trying to use these methods, remember that you need to set -useNextFrameForImageCapture before running -processImage or running video and calling any of these methods, or you will get a nil image
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
- (UIImage *)imageFromCurrentFramebuffer;
- (UIImage *)imageFromCurrentFramebufferWithOrientation:(UIImageOrientation)imageOrientation;
- (UIImage *)imageByFilteringImage:(UIImage *)imageToFilter;
- (CGImageRef)newCGImageByFilteringImage:(UIImage *)imageToFilter;
#else
- (NSImage *)imageFromCurrentFramebuffer;
- (NSImage *)imageFromCurrentFramebufferWithOrientation:(UIImageOrientation)imageOrientation;
- (NSImage *)imageByFilteringImage:(NSImage *)imageToFilter;
- (CGImageRef)newCGImageByFilteringImage:(NSImage *)imageToFilter;
#endif

- (BOOL)providesMonochromeOutput;

@end
