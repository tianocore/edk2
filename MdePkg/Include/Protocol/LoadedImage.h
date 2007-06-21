/** @file
  EFI 1.0 Loaded image protocol definition.

  Every EFI driver and application is passed an image handle when it is loaded.
  This image handle will contain a Loaded Image Protocol.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  LoadedImage.h

**/

#ifndef __LOADED_IMAGE_PROTOCOL_H__
#define __LOADED_IMAGE_PROTOCOL_H__

#include <Protocol/DevicePath.h>

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
  { \
    0x5B1B31A1, 0x9562, 0x11d2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } \
  }

//
// Protocol GUID defined in EFI1.1.
// 
#define LOADED_IMAGE_PROTOCOL   EFI_LOADED_IMAGE_PROTOCOL_GUID

//
// EFI_SYSTEM_TABLE & EFI_IMAGE_UNLOAD are defined in EfiApi.h
//
#define EFI_LOADED_IMAGE_PROTOCOL_REVISION  0x1000

//
// Revision defined in EFI1.1.
// 
#define EFI_LOADED_IMAGE_INFORMATION_REVISION    EFI_LOADED_IMAGE_PROTOCOL_REVISION


typedef struct {
  UINT32                    Revision;
  EFI_HANDLE                ParentHandle;
  EFI_SYSTEM_TABLE          *SystemTable;

  //
  // Source location of image
  //
  EFI_HANDLE                DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  VOID                      *Reserved;

  //
  // Images load options
  //
  UINT32                    LoadOptionsSize;
  VOID                      *LoadOptions;

  //
  // Location of where image was loaded
  //
  VOID                      *ImageBase;
  UINT64                    ImageSize;
  EFI_MEMORY_TYPE           ImageCodeType;
  EFI_MEMORY_TYPE           ImageDataType;

  //
  // If the driver image supports a dynamic unload request
  //
  EFI_IMAGE_UNLOAD          Unload;

} EFI_LOADED_IMAGE_PROTOCOL;

//
// For backward-compatible with EFI1.1.
// 
typedef EFI_LOADED_IMAGE_PROTOCOL EFI_LOADED_IMAGE;

extern EFI_GUID gEfiLoadedImageProtocolGuid;

#endif
