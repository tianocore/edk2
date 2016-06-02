/** @file
  Define the variable data structures used for physical presence storage data.

Copyright (c) 2016, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TCG_PHYSICAL_PRESENCE_STORAGE_DATA_GUID_H__
#define __TCG_PHYSICAL_PRESENCE_STORAGE_DATA_GUID_H__

#define EFI_TCG_PHYSICAL_PRESENCE_STORAGE_DATA_GUID \
  { \
    0x2EBE3E34, 0xB3CD, 0x471A, { 0xBF, 0x87, 0xB3, 0xC6, 0x6E, 0xE0, 0x74, 0x9A} \
  }

//
// This variable is used to save TCG2 Management Flags and corresponding operations.
// It should be protected from malicious software (e.g. Set it as read-only variable). 
//
#define TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS_VARIABLE  L"TcgPhysicalPresenceStorageFlags"
typedef struct {
  UINT32  PPFlags;
} EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS;

extern EFI_GUID  gEfiTcgPhysicalPresenceStorageGuid;

#endif

