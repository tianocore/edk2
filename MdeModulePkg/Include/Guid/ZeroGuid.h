/** @file
  GUID has all zero values.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ZERO_GUID_H__
#define __ZERO_GUID_H__

#define ZERO_GUID \
  { \
    0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} \
  }

extern EFI_GUID gZeroGuid;

#endif
