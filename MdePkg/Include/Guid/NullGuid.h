/** @file
  Definition of a NULL GUID.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NULL_GUID_H_
#define NULL_GUID_H_

// {00000000-0000-0000-0000-000000000000}
#define NULL_GUID \
  { \
    0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } \
  }

extern EFI_GUID gNullGuid;

#endif // NULL_GUID_H_
