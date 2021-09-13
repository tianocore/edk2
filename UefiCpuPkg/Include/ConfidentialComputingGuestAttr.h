/** @file
Definitions for Confidential Computing Attribute

Copyright (c) 2021 AMD Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CONFIDENTIAL_COMPUTING_GUEST_ATTR_H_
#define _CONFIDENTIAL_COMPUTING_GUEST_ATTR_H_

typedef enum {
  /* The guest is running with memory encryption disabled. */
  CC_ATTR_NOT_ENCRYPTED = 0,

  /* The guest is running with AMD SEV memory encryption enabled. */
  CC_ATTR_AMD_SEV      = 0x100,
  CC_ATTR_AMD_SEV_ES   = 0x101,
  CC_ATTR_AMD_SEV_SNP  = 0x102,

  /* The guest is running with Intel TDX memory encryption enabled. */
  CC_ATTR_INTEL_TDX = 0x200,
} CONFIDENTIAL_COMPUTING_GUEST_ATTR;

#endif
