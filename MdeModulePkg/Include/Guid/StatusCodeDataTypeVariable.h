/** @file
  This file defines the GUID and data structure used to pass variable setting
  failure information to the Status Code Protocol.

Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _STATUS_CODE_DATA_TYPE_VARIABLE_H_
#define _STATUS_CODE_DATA_TYPE_VARIABLE_H_

///
/// The Global ID used to identify a structure of type EDKII_SET_VARIABLE_STATUS.
/// The status code value is PcdGet32 (PcdErrorCodeSetVariable).
///
#define EDKII_STATUS_CODE_DATA_TYPE_VARIABLE_GUID \
  { \
    0xf6ee6dbb, 0xd67f, 0x4ea0, { 0x8b, 0x96, 0x6a, 0x71, 0xb1, 0x9d, 0x84, 0xad } \
  }

typedef struct {
  EFI_GUID      Guid;
  UINTN         NameSize;
  UINTN         DataSize;
  EFI_STATUS    SetStatus;
  UINT32        Attributes;
  // CHAR16  Name[];
  // UINT8   Data[];
} EDKII_SET_VARIABLE_STATUS;

extern EFI_GUID  gEdkiiStatusCodeDataTypeVariableGuid;

#endif // _STATUS_CODE_DATA_TYPE_VARIABLE_H_
