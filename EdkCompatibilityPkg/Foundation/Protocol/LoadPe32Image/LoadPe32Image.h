/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    LoadPe32Image.h

Abstract:

    Load PE32 Image Protocol

--*/

#ifndef _LOAD_PE32_IMAGE_H_
#define _LOAD_PE32_IMAGE_H_

#define PE32_IMAGE_PROTOCOL_GUID  \
  {0x5cb5c776,0x60d5,0x45ee,{0x88,0x3c,0x45,0x27,0x8,0xcd,0x74,0x3f} }

#define EFI_LOAD_PE_IMAGE_ATTRIBUTE_NONE                                 0x00
#define EFI_LOAD_PE_IMAGE_ATTRIBUTE_RUNTIME_REGISTRATION                 0x01
#define EFI_LOAD_PE_IMAGE_ATTRIBUTE_DEBUG_IMAGE_INFO_TABLE_REGISTRATION  0x02

EFI_FORWARD_DECLARATION (EFI_PE32_IMAGE_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *LOAD_PE_IMAGE) (
  IN EFI_PE32_IMAGE_PROTOCOL  *This,
  IN  EFI_HANDLE                       ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL         *FilePath,
  IN  VOID                             *SourceBuffer       OPTIONAL,
  IN  UINTN                            SourceSize,
  IN  EFI_PHYSICAL_ADDRESS             DstBuffer           OPTIONAL,
  OUT UINTN                            *NumberOfPages      OPTIONAL,
  OUT EFI_HANDLE                       *ImageHandle,
  OUT EFI_PHYSICAL_ADDRESS             *EntryPoint         OPTIONAL,
  IN  UINT32                           Attribute
  );

typedef
EFI_STATUS
(EFIAPI *UNLOAD_PE_IMAGE) (
  IN EFI_PE32_IMAGE_PROTOCOL  *This,
  IN EFI_HANDLE                       ImageHandle
  );

struct _EFI_PE32_IMAGE_PROTOCOL {
  LOAD_PE_IMAGE     LoadPeImage;
  UNLOAD_PE_IMAGE  UnLoadPeImage;
};

extern EFI_GUID gEfiLoadPeImageGuid;

#endif

