/** @file
  This GUID will be used to save MTRR_SETTINGS at EndOfDxe by LockBox
  and restore at S3 boot PEI phase for s3 usage.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef S3_MTRR_SETTING_H__
#define S3_MTRR_SETTING_H__

#define EDKII_S3_MTRR_SETTING_GUID \
  { \
    0xd77baa84, 0xb332, 0x4463, {0x9f, 0x1d, 0xce, 0x81, 0x00, 0xfe, 0x7f, 0x35 } \
  }

extern EFI_GUID  gEdkiiS3MtrrSettingGuid;

#endif
