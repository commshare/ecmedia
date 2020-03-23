//
//  ECImageToasterFilter.h
//  FWMeituApp
//
//  Created by hzkmn on 16/1/11.
//  Copyright © 2016年 ForrestWoo co,.ltd. All rights reserved.
//

#import "ECImageFilterGroup.h"
#import "ECImageSixInputFilter.h"
#import "ECImagePicture.h"

@interface FWFilter15 : ECImageSixInputFilter

@end

@interface ECImageToasterFilter : ECImageFilterGroup
{
    ECImagePicture *imageSource1;
    ECImagePicture *imageSource2;
    ECImagePicture *imageSource3;
    ECImagePicture *imageSource4;
    ECImagePicture *imageSource5;
}

@end