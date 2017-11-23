//
//  ECImageBeautyFaceFilter.h
//  ECImageEecoding
//
//  Created by jianqiangzhang on 16/5/27.
//  Copyright © 2016年 jianqiangzhang. All rights reserved.
//

#import "ECImageFilterGroup.h"

@class ECImageThreeInputFilter;
@class ECImageBrightnessFilter;
@class ECImageContrastFilter;
@class ECImageSaturationFilter;
@class ECImageBilateralFilter;
@class ECImageCannyEdgeDetectionFilter;
@class ECImageCombinationFilter;
@class ECImageHSBFilter;

@interface ECImageBeautyFaceFilter : ECImageFilterGroup
{
    ECImageBrightnessFilter         *brightnessFilter;
    ECImageContrastFilter           *contrastFilter;
    ECImageSaturationFilter         *saturationFilter;
    ECImageBilateralFilter          *bilateralFilter;
    ECImageCannyEdgeDetectionFilter *cannyEdgeFilter;
    ECImageCombinationFilter        *combinationFilter;
    ECImageHSBFilter                *hsbFilter;
}
/**
 *  A normalization factor for the distance between central color and sample color
 *
 *  @param value default 2.0
 */
- (void)setDistanceNormalizationFactor:(CGFloat)value;

/**
 *  Set brightness and saturation
 *
 *  @param brightness [0.0, 2.0], default 1.05
 *  @param saturation [0.0, 2.0], default 1.05
 */
- (void)setBrightness:(CGFloat)brightness saturation:(CGFloat)saturation;
@end
