#import "ECImageFilterGroup.h"

@class ECImagePicture;

/** A photo filter based on Soft Elegance Photoshop action
    http://h-d-stock.deviantart.com/art/H-D-A-soft-elegance-70107603
 */

// Note: If you want to use this effect you have to add 
//       lookup_soft_elegance_1.png and lookup_soft_elegance_2.png
//       from Resources folder to your application bundle.

@interface ECImageSoftEleganceFilter : ECImageFilterGroup
{
    ECImagePicture *lookupImageSource1;
    ECImagePicture *lookupImageSource2;
}

@end
