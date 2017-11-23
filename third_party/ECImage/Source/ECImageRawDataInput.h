#import "ECImageOutput.h"

// The bytes passed into this input are not copied or retained, but you are free to deallocate them after they are used by this filter.
// The bytes are uploaded and stored within a texture, so nothing is kept locally.
// The default format for input bytes is GPUPixelFormatBGRA, unless specified with pixelFormat:
// The default type for input bytes is GPUPixelTypeUByte, unless specified with pixelType:

typedef enum {
	GPUPixelFormatBGRA = GL_BGRA,
	GPUPixelFormatRGBA = GL_RGBA,
	GPUPixelFormatRGB = GL_RGB,
    GPUPixelFormatLuminance = GL_LUMINANCE
} GPUPixelFormat;

typedef enum {
	GPUPixelTypeUByte = GL_UNSIGNED_BYTE,
	GPUPixelTypeFloat = GL_FLOAT
} GPUPixelType;

@interface ECImageRawDataInput : ECImageOutput
{
    CGSize uploadedImageSize;
	
	dispatch_semaphore_t dataUpdateSemaphore;
    
    uint8_t *dst_argb;
}

// Initialization and teardown
- (id)initWithBytes:(GLubyte *)bytesToUpload size:(CGSize)imageSize;
- (id)initWithBytes:(GLubyte *)bytesToUpload size:(CGSize)imageSize pixelFormat:(GPUPixelFormat)pixelFormat;
- (id)initWithBytes:(GLubyte *)bytesToUpload size:(CGSize)imageSize pixelFormat:(GPUPixelFormat)pixelFormat type:(GPUPixelType)pixelType;

/** Input data pixel format
 */
@property (readwrite, nonatomic) GPUPixelFormat pixelFormat;
@property (readwrite, nonatomic) GPUPixelType   pixelType;

// Image rendering
- (void)updateDataFromBytes:(GLubyte *)bytesToUpload size:(CGSize)imageSize;
- (void)processData;
- (void)processDataForTimestamp:(CMTime)frameTime;
- (CGSize)outputImageSize;

@end
