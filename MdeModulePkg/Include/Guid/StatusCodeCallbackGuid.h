/** @file
  GUID used to identify HOB for pointers to callback functios registered on
  PEI report status code router.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __STATUS_CODE_CALLBACK_H__
#define __STATUS_CODE_CALLBACK_H__

#define STATUS_CODE_CALLBACK_GUID \
  { \
    0xe701458c, 0x4900, 0x4ca5, {0xb7, 0x72, 0x3d, 0x37, 0x94, 0x9f, 0x79, 0x27} \
  }

extern EFI_GUID  gStatusCodeCallbackGuid;

#endif
