/** @file
  Support for Microsoft Secure MOR implementation, defined at 
  Microsoft Secure MOR implementation.
  https://msdn.microsoft.com/en-us/library/windows/hardware/mt270973(v=vs.85).aspx

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_H__
#define __MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_H__

#define MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_GUID \
  { \
    0xBB983CCF, 0x151D, 0x40E1, {0xA0, 0x7B, 0x4A, 0x17, 0xBE, 0x16, 0x82, 0x92} \
  }

#define MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME L"MemoryOverwriteRequestControlLock"

//
// VendorGuid: {BB983CCF-151D-40E1-A07B-4A17BE168292}
// Name:       MemoryOverwriteRequestControlLock
// Attributes: NV+BS+RT
// Size:       0x1 byte
//
// The BIOS initializes MemoryOverwriteRequestControlLock to a value of 0x00
// before BDS (BOOT#### processing). When the OS loader calls SetVariable by
// specifying 0x01, the access mode for both MemoryOverwriteRequestControlLock
// and MemoryOverwriteRequestControl is changed to read-only. If any other
// value is specified in the SetVariable call, it fails with the 
// EFI_INVALID_PARAMETER error code.
//

extern EFI_GUID gEfiMemoryOverwriteRequestControlLockGuid;

#endif
