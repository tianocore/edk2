/** @file

  Tcg addtional services to measure PeImage and ActionString

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _TCG_PLATFORM_PROTOCOL_H_
#define _TCG_PLATFORM_PROTOCOL_H_

#define EFI_TCG_PLATFORM_PROTOCOL_GUID  \
  { 0x8c4c9a41, 0xbf56, 0x4627, { 0x9e, 0xa, 0xc8, 0x38, 0x6d, 0x66, 0x11, 0x5c } }

typedef struct tdEFI_TCG_PLATFORM_PROTOCOL EFI_TCG_PLATFORM_PROTOCOL;

//
// EFI TCG Platform Protocol
//
/**
  
  Measure PE/COFF Image File prior to the application of any fix-ups or relocations.
  
  @param  BootPolicy      If TRUE, indicates that the request originates from the boot manager,
                          and that the boot manager is attempting to load FilePath as a boot selection.
  @param  ImageAddress    The memory address to PE/COFF image.
  @param  ImageSize       The size of PE/COFF image.
  @param  LinkTimeBase    The image base address in the original PeImage.
  @param  ImageType       The subsystem type of the PeImage.
  @param  DeviceHandle    The handle to device matched the file path. 
  @param  FilePath        The specific file path from which the image is loaded.
  
  @retval EFI_SUCCESS           Measure successfully.
  @retval EFI_UNSUPPORTED       The loaded PeImage is not supported.
  @retval EFI_OUT_OF_RESOURCES  The resource of memory is not enough.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TCG_MEASURE_PE_IMAGE) (
  IN      BOOLEAN                   BootPolicy,
  IN      EFI_PHYSICAL_ADDRESS      ImageAddress,
  IN      UINTN                     ImageSize,
  IN      UINTN                     LinkTimeBase,
  IN      UINT16                    ImageType,
  IN      EFI_HANDLE                DeviceHandle,
  IN      EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  
  Measure efi action string.
  
  @param  ActionString  Pointer to action string.
  
  @retval EFI_SUCCESS   Measure action string successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TCG_MEASURE_ACTION) (
  IN      CHAR8                     *ActionString
  );

struct tdEFI_TCG_PLATFORM_PROTOCOL {
  EFI_TCG_MEASURE_PE_IMAGE          MeasurePeImage;
  EFI_TCG_MEASURE_ACTION            MeasureAction;
};

extern EFI_GUID                     gEfiTcgPlatformProtocolGuid;

#endif
