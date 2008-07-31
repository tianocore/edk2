/** @file

  FVB Extension protocol that extends the FVB Class in a component fashion.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FVB_EXTENSION_H__
#define __FVB_EXTENSION_H__

#define EFI_FVB_EXTENSION_PROTOCOL_GUID  \
  {0x53a4c71b, 0xb581, 0x4170, {0x91, 0xb3, 0x8d, 0xb8, 0x7a, 0x4b, 0x5c, 0x46 } }

typedef struct _EFI_FVB_EXTENSION_PROTOCOL EFI_FVB_EXTENSION_PROTOCOL;

//
//  FVB Extension Function Prototypes
//
/**
  Erases and initializes a specified range of a firmware volume block

  @param[in]     This           Pointer to the FVB Extension protocol instance
  @param[in]     StartLba       The starting logical block index to be erased
  @param[in]     OffsetStartLba Offset into the starting block at which to 
                                begin erasing    
  @param[in]     LastLba        The last logical block index to be erased
  @param[in]     OffsetLastLba  Offset into the last block at which to end erasing     

  @retval EFI_SUCCESS           The specified range was erased successfully
  @retval EFI_ACCESS_DENIED     The firmware volume block is in the WriteDisabled state
  @retval EFI_DEVICE_ERROR      The block device is not functioning correctly and 
                                could not be written. Firmware device may have been
                                partially erased
**/
typedef
EFI_STATUS
(EFIAPI * EFI_FV_ERASE_CUSTOM_BLOCK)(
  IN EFI_FVB_EXTENSION_PROTOCOL   *This,
  IN EFI_LBA                      StartLba,
  IN UINTN                        OffsetStartLba,
  IN EFI_LBA                      LastLba,
  IN UINTN                        OffsetLastLba
);

//
// FVB Extension PROTOCOL
//
struct _EFI_FVB_EXTENSION_PROTOCOL {
  EFI_FV_ERASE_CUSTOM_BLOCK               EraseFvbCustomBlock;
};

extern EFI_GUID                           gEfiFvbExtensionProtocolGuid;

#endif

