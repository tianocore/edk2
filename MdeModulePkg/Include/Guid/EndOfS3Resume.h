/** @file
  This GUID will be installed at the end of S3 resume phase as protocol in SMM environment.
  It allows for SMM drivers to hook this point and do the required tasks.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __END_OF_S3_RESUME_H__
#define __END_OF_S3_RESUME_H__

#define EDKII_END_OF_S3_RESUME_GUID \
  { \
    0x96f5296d, 0x05f7, 0x4f3c, {0x84, 0x67, 0xe4, 0x56, 0x89, 0x0e, 0x0c, 0xb5 } \
  }

extern EFI_GUID  gEdkiiEndOfS3ResumeGuid;

#endif
