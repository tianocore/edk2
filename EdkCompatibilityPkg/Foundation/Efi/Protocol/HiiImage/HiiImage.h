/*++

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    HiiImage.h
    
Abstract:

    EFI_HII_IMAGE_PROTOCOL from UEFI 2.1 specification.
    
    This protocol provides access to images in the images database.

Revision History

--*/

#ifndef __EFI_HII_IMAGE_PROTOCOL_H__
#define __EFI_HII_IMAGE_PROTOCOL_H__

#include "EfiHii.h"
#include EFI_PROTOCOL_DEFINITION (GraphicsOutput) 

//
// Global ID for the Hii Image Protocol.
//
#define EFI_HII_IMAGE_PROTOCOL_GUID \
  { \
    0x31a6406a, 0x6bdf, 0x4e46, {0xb2, 0xa2, 0xeb, 0xaa, 0x89, 0xc4, 0x9, 0x20} \
  }

EFI_FORWARD_DECLARATION (EFI_HII_IMAGE_PROTOCOL);

typedef UINT32 EFI_HII_DRAW_FLAGS;

typedef struct _EFI_IMAGE_INPUT {
  UINT32                             Flags;  
  UINT16                             Width;
  UINT16                             Height;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *Bitmap;
} EFI_IMAGE_INPUT;

#define EFI_IMAGE_TRANSPARENT          0x00000001

typedef struct _EFI_IMAGE_OUTPUT {
  UINT16 Width;
  UINT16 Height;
  union {
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Bitmap;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Screen;
  } Image;
} EFI_IMAGE_OUTPUT;

#define EFI_HII_DRAW_FLAG_CLIP         0x00000001
#define EFI_HII_DRAW_FLAG_TRANSPARENT  0x00000030
#define EFI_HII_DRAW_FLAG_DEFAULT      0x00000000
#define EFI_HII_DRAW_FLAG_FORCE_TRANS  0x00000010
#define EFI_HII_DRAW_FLAG_FORCE_OPAQUE 0x00000020
#define EFI_HII_DIRECT_TO_SCREEN       0x00000080

typedef
EFI_STATUS
(EFIAPI *EFI_HII_NEW_IMAGE) (
  IN  CONST EFI_HII_IMAGE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                 PackageList,
  OUT EFI_IMAGE_ID                   *ImageId,
  IN  CONST EFI_IMAGE_INPUT          *Image
  )
/*++

  Routine Description:
    This function adds the image Image to the group of images owned by PackageList, and returns
    a new image identifier (ImageId).                                                          
    
  Arguments:          
    This              - A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
    PackageList       - Handle of the package list where this image will be added.    
    ImageId           - On return, contains the new image id, which is unique within PackageList.
    Image             - Points to the image.
    
  Returns:
    EFI_SUCCESS            - The new image was added successfully.
    EFI_NOT_FOUND          - The specified PackageList could not be found in database.
    EFI_OUT_OF_RESOURCES   - Could not add the image due to lack of resources.
    EFI_INVALID_PARAMETER  - Image is NULL or ImageId is NULL.  
    
--*/    
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_IMAGE) (
  IN  CONST EFI_HII_IMAGE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_IMAGE_ID                   ImageId,
  OUT EFI_IMAGE_INPUT                *Image
  )
/*++

  Routine Description:
    This function retrieves the image specified by ImageId which is associated with
    the specified PackageList and copies it into the buffer specified by Image.
    
  Arguments:          
    This              - A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
    PackageList       - Handle of the package list where this image will be searched.    
    ImageId           - The image's id,, which is unique within PackageList.
    Image             - Points to the image.
                        
  Returns:
    EFI_SUCCESS            - The new image was returned successfully.
    EFI_NOT_FOUND          - The image specified by ImageId is not available.
                             The specified PackageList is not in the database.
    EFI_INVALID_PARAMETER  - The Image or ImageSize was NULL.
    EFI_OUT_OF_RESOURCES   - The bitmap could not be retrieved because there was not
                             enough memory.
    
--*/  
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_SET_IMAGE) (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                  PackageList,
  IN EFI_IMAGE_ID                    ImageId,
  IN CONST EFI_IMAGE_INPUT           *Image
  )
/*++

  Routine Description:
    This function updates the image specified by ImageId in the specified PackageListHandle to
    the image specified by Image.                                                             
    
  Arguments:          
    This              - A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
    PackageList       - The package list containing the images.
    ImageId           - The image's id,, which is unique within PackageList.
    Image             - Points to the image.
                        
  Returns:
    EFI_SUCCESS            - The new image was updated successfully.
    EFI_NOT_FOUND          - The image specified by ImageId is not in the database.
                             The specified PackageList is not in the database.    
    EFI_INVALID_PARAMETER  - The Image was NULL.
    
--*/  
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DRAW_IMAGE) (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_DRAW_FLAGS              Flags,
  IN CONST EFI_IMAGE_INPUT           *Image,
  IN OUT EFI_IMAGE_OUTPUT            **Blt,
  IN UINTN                           BltX,
  IN UINTN                           BltY
  )
/*++

  Routine Description:
    This function renders an image to a bitmap or the screen using the specified
    color and options. It draws the image on an existing bitmap, allocates a new
    bitmap or uses the screen. The images can be clipped.       
    
  Arguments:          
    This              - A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
    Flags             - Describes how the image is to be drawn.    
    Image             - Points to the image to be displayed.
    Blt               - If this points to a non-NULL on entry, this points to the
                        image, which is Width pixels wide and Height pixels high. 
                        The image will be drawn onto this image and 
                        EFI_HII_DRAW_FLAG_CLIP is implied. If this points to a 
                        NULL on entry, then a buffer will be allocated to hold 
                        the generated image and the pointer updated on exit. It
                        is the caller's responsibility to free this buffer.
    BltX, BltY        - Specifies the offset from the left and top edge of the 
                        output image of the first pixel in the image.
                        
  Returns:
    EFI_SUCCESS            - The image was successfully drawn.
    EFI_OUT_OF_RESOURCES   - Unable to allocate an output buffer for Blt.
    EFI_INVALID_PARAMETER  - The Image or Blt was NULL.
    EFI_INVALID_PARAMETER  - Any combination of Flags is invalid.
    
--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_HII_DRAW_IMAGE_ID) (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_DRAW_FLAGS              Flags,
  IN EFI_HII_HANDLE                  PackageList,
  IN EFI_IMAGE_ID                    ImageId,
  IN OUT EFI_IMAGE_OUTPUT            **Blt,
  IN UINTN                           BltX,
  IN UINTN                           BltY
  )

/*++

  Routine Description:
    This function renders an image to a bitmap or the screen using the specified
    color and options. It draws the image on an existing bitmap, allocates a new
    bitmap or uses the screen. The images can be clipped.       
    
  Arguments:          
    This              - A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
    Flags             - Describes how the image is to be drawn.
    PackageList       - The package list in the HII database to search for the 
                        specified image.
    ImageId           - The image's id, which is unique within PackageList.
    Blt               - If this points to a non-NULL on entry, this points to the
                        image, which is Width pixels wide and Height pixels high.
                        The image will be drawn onto this image and                
                        EFI_HII_DRAW_FLAG_CLIP is implied. If this points to a 
                        NULL on entry, then a buffer will be allocated to hold 
                        the generated image and the pointer updated on exit. It
                        is the caller's responsibility to free this buffer.
    BltX, BltY        - Specifies the offset from the left and top edge of the 
                        output image of the first pixel in the image.
                        
  Returns:
    EFI_SUCCESS            - The image was successfully drawn.
    EFI_OUT_OF_RESOURCES   - Unable to allocate an output buffer for Blt.
    EFI_NOT_FOUND          - The image specified by ImageId is not in the database. 
                             The specified PackageList is not in the database.                            
    EFI_INVALID_PARAMETER  - The Blt was NULL.    

--*/
;

//
// Interface structure for the EFI_HII_IMAGE_PROTOCOL
//
struct _EFI_HII_IMAGE_PROTOCOL {
  EFI_HII_NEW_IMAGE                  NewImage;
  EFI_HII_GET_IMAGE                  GetImage;
  EFI_HII_SET_IMAGE                  SetImage;
  EFI_HII_DRAW_IMAGE                 DrawImage;
  EFI_HII_DRAW_IMAGE_ID              DrawImageId;
};

extern EFI_GUID gEfiHiiImageProtocolGuid;

#endif
