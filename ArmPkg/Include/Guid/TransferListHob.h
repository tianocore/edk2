/** @file
  GUID for the HOB that contains the copy of the Transfer List

  Copyright (C) 2024, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - https://github.com/FirmwareHandoff/firmware_handoff

**/

#ifndef ARM_TRANSFER_LIST_HOB_H__
#define ARM_TRANSFER_LIST_HOB_H__

#define ARM_TRANSFER_LIST_HOB_GUID  {\
          0xebe7bae8, 0xfe18, 0x43c5, \
          { 0xbf, 0x3f, 0xf2, 0xb1, 0xaf, 0xb2, 0xdf, 0xb8 } \
        }

extern EFI_GUID  gArmTransferListHobGuid;

#endif
