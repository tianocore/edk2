/** @file
  This file defines a SMM_PROFILE_DATA_HOB.
  This is a Memory Allocation HOB including the SmmProfileData address/size info.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_PROFILE_DATA_H_
#define SMM_PROFILE_DATA_H_

#include <PiPei.h>

#define EDKII_SMM_PROFILE_DATA_GUID \
  { \
    0x26ef081d, 0x19b0, 0x4c42, {0xa2, 0x57, 0xa7, 0xf5, 0x9f, 0x8b, 0xd0, 0x38}  \
  }

//
// MM Logging attribute.
// The memory with this attribute access will be recorded in SMM Profile.
//
#define EDKII_MM_RESOURCE_ATTRIBUTE_LOGGING  0x10000000

extern EFI_GUID  gEdkiiSmmProfileDataGuid;

#endif
