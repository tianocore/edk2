/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    FvbExtension.h

Abstract:

  FVB Extension protocol that extends the FVB Class in a component fashion.

--*/

#ifndef _FVB_EXTENSION_H_
#define _FVB_EXTENSION_H_

#define EFI_FVB_EXTENSION_PROTOCOL_GUID  \
  {0x53a4c71b, 0xb581, 0x4170, {0x91, 0xb3, 0x8d, 0xb8, 0x7a, 0x4b, 0x5c, 0x46} }

EFI_FORWARD_DECLARATION (EFI_FVB_EXTENSION_PROTOCOL);

//
//  FVB Extension Function Prototypes
//
typedef
EFI_STATUS
(EFIAPI * EFI_FV_ERASE_CUSTOM_BLOCK) (
  IN EFI_FVB_EXTENSION_PROTOCOL   *This,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba
);

//
// IPMI TRANSPORT PROTOCOL
//
struct _EFI_FVB_EXTENSION_PROTOCOL {
  EFI_FV_ERASE_CUSTOM_BLOCK               EraseFvbCustomBlock;
 };

extern EFI_GUID                           gEfiFvbExtensionProtocolGuid;

#endif

