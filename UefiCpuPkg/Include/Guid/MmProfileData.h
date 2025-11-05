/** @file
  This file contains related definitions to support MM Profile feature in standalone MM.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_PROFILE_DATA_H_
#define MM_PROFILE_DATA_H_

///
/// This GUID is assigned to the Name field of EFI_HOB_MEMORY_ALLOCATION.AllocDescriptor.
/// It signifies that the corresponding EFI_HOB_MEMORY_ALLOCATION HOB points to the location of MM Profile data.
/// MM Profile is a feature designed to log accesses to non-MM regions by standalone MM.
/// It stores these access logs within the MM Profile data.
///
#define MM_PROFILE_DATA_HOB_GUID \
  { \
    0x26ef081d, 0x19b0, 0x4c42, {0xa2, 0x57, 0xa7, 0xf5, 0x9f, 0x8b, 0xd0, 0x38}  \
  }

///
/// In standalone MM, the policy for accessing non-MM regions is simplified:
///    Non-MM regions and their access policies are specified by EFI_HOB_RESOURCE_DESCRIPTOR HOBs.
/// Accesses to regions marked with the MM_RESOURCE_ATTRIBUTE_LOGGING attribute
/// are permitted in standalone MM, and these accesses are logged in the MM Profile data.
/// This attribute is not utilized by the SMM Profile feature in traditional SMM.
///
#define MM_RESOURCE_ATTRIBUTE_LOGGING  0x10000000

extern EFI_GUID  gMmProfileDataHobGuid;

#endif
