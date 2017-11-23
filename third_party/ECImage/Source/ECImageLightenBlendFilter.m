#import "ECImageLightenBlendFilter.h"

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
NSString *const kECImageLightenBlendFragmentShaderString = SHADER_STRING
(
 varying highp vec2 textureCoordinate;
 varying highp vec2 textureCoordinate2;

 uniform sampler2D inputImageTexture;
 uniform sampler2D inputImageTexture2;
 
 void main()
 {
    lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
    lowp vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinate2);
    
    gl_FragColor = max(textureColor, textureColor2);
 }
);
#else
NSString *const kECImageLightenBlendFragmentShaderString = SHADER_STRING
(
 varying vec2 textureCoordinate;
 varying vec2 textureCoordinate2;
 
 uniform sampler2D inputImageTexture;
 uniform sampler2D inputImageTexture2;
 
 void main()
 {
     vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
     vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinate2);
     
     gl_FragColor = max(textureColor, textureColor2);
 }
 );
#endif

@implementation ECImageLightenBlendFilter

- (id)init;
{
    if (!(self = [super initWithFragmentShaderFromString:kECImageLightenBlendFragmentShaderString]))
    {
		return nil;
    }
    
    return self;
}

@end

