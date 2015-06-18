/** @file
  SMM End Of Dxe protocol as defined in the PI 1.2.1 specification.

  This protocol is a mandatory protocol published by the PI platform code prior to invoking any
  3rd party content, including options ROM's and UEFI executables that are not from the platform manufacturer.
  There is an associated event GUID that is signaled for the DXE drivers called EFI_END_OF_DXE_EVENT_GUID.

  Copyright (c) 2012 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_END_OF_DXE_H_
#define _SMM_END_OF_DXE_H_

#define EFI_SMM_END_OF_DXE_PROTOCOL_GUID \
  { \
    0x24e70042, 0xd5c5, 0x4260, { 0x8c, 0x39, 0xa, 0xd3, 0xaa, 0x32, 0xe9, 0x3d } \
  }

extern EFI_GUID gEfiSmmEndOfDxeProtocolGuid;

#endif
