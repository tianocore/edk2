/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  CapsuleVendor.h

Abstract:

  Capsule update Guid definitions

--*/

#ifndef __EFI_CAPSULE_INFO_GUID_H__
#define __EFI_CAPSULE_INFO_GUID_H__

typedef struct {
  UINT32   CapsuleArrayNumber;
  VOID*    CapsulePtr[1];
} EFI_CAPSULE_TABLE;

typedef struct {
  UINT32      CapsuleGuidNumber;
  EFI_GUID    CapsuleGuidPtr[1];
} EFI_CAPSULE_INFO_TABLE;

//
// This GUID is used for collecting all capsules' Guids who install in ConfigTable.
//
#define EFI_CAPSULE_INFO_GUID \
  { \
    0x8B34EAC7, 0x2690, 0x460B, { 0x8B, 0xA5, 0xD5, 0xCF, 0x32, 0x83, 0x17, 0x35 } \
  }

extern EFI_GUID gEfiCapsuleInfoGuid;

#endif // #ifndef _EFI_CAPSULE_INFO_GUID_H_
