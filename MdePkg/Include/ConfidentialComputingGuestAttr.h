/** @file
Definitions for Confidential Computing Attribute

Copyright (c) 2021 AMD Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CONFIDENTIAL_COMPUTING_GUEST_ATTR_H_
#define CONFIDENTIAL_COMPUTING_GUEST_ATTR_H_

typedef enum {
  /* The guest is running with memory encryption disabled. */
  CCAttrNotEncrypted = 0,

  /* The guest is running with AMD SEV memory encryption enabled. */
  CCAttrAmdSev    = 0x100,
  CCAttrAmdSevEs  = 0x101,
  CCAttrAmdSevSnp = 0x102,

  /* The guest is running with Intel TDX memory encryption enabled. */
  CCAttrIntelTdx = 0x200,
} CONFIDENTIAL_COMPUTING_GUEST_ATTR;

#endif
