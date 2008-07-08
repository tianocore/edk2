/** @file

  This file defines capsule update guid, capsule variable name and 
  capsule guid hob data strucutre, which are required by capsule update feature.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_CAPSULE_VENDOR_GUID_H__
#define __EFI_CAPSULE_VENDOR_GUID_H__

//
// Note -- This guid is used as a variable GUID (depending on implementation)
// for the capsule variable if the capsule pointer is passes through reset
// via a variable.
//
#define EFI_CAPSULE_VENDOR_GUID  \
  { 0x711C703F, 0xC285, 0x4B10, { 0xA3, 0xB0, 0x36, 0xEC, 0xBD, 0x3C, 0x8B, 0xE2 } }

//
// Name of capsule variable
// 
#define EFI_CAPSULE_VARIABLE_NAME L"CapsuleUpdateData"

extern EFI_GUID gEfiCapsuleVendorGuid;

//
// Data structure of capsule guid hob 
//
typedef struct {
  EFI_PHYSICAL_ADDRESS BaseAddress;
  UINT32 Length;
} CAPSULE_HOB_INFO;

#endif // #ifndef _EFI_CAPSULE_VENDOR_GUID_H_
