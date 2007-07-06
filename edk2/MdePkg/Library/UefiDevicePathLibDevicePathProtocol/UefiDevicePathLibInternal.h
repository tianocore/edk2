/** @file
  Internal Header file for UEFI Device Path Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UEFI_DEVICEPATH_LIB_INTERNAL_H_
#define _UEFI_DEVICEPATH_LIB_INTERNAL_H_


/**
  The constructor function caches the pointer to DevicePathUtilites protocol.
  
  The constructor function locates DevicePathUtilities protocol from protocol database.
  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS. 

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DevicePathLibConstructor (
  IN      EFI_HANDLE                ImageHandle,
  IN      EFI_SYSTEM_TABLE          *SystemTable
  )
;

#endif
