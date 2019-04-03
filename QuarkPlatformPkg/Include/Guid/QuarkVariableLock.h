/** @file

Guid and variable name used to trigger quark lock of specific UEFI variables.

Copyright (c) 2013 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _QUARK_VARIABLE_LOCK_GUID_H_
#define _QUARK_VARIABLE_LOCK_GUID_H_

#define QUARK_VARIABLE_LOCK_GUID \
  { \
    0xeef749c2, 0xc047, 0x4d6e, { 0xb1, 0xbc, 0xd3, 0x6e, 0xb3, 0xa5, 0x55, 0x9c } \
  }

#define QUARK_VARIABLE_LOCK_NAME  L"QuarkVariableLock"

extern EFI_GUID gQuarkVariableLockGuid;

#endif
