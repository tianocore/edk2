/** @file
  This file defines STATUS_CODE_USE_SERIAL structure which indicates StatusCode is
  reported via serial port or not.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_STATUS_CODE_USE_SERIAL_H_
#define MM_STATUS_CODE_USE_SERIAL_H_

///
/// The GUID of the StatusCodeUseSerial GUIDed HOB.
///
#define MM_STATUS_CODE_USE_SERIAL_HOB_GUID \
  { \
    0xbb55aa97, 0xc7f2, 0x4f60, {0xac, 0xc3, 0x16, 0xf6, 0xe8, 0xb5, 0x07, 0x79}  \
  }

///
/// The structure defines the data layout of the StatusCodeUseSerial GUIDed HOB.
///
typedef struct {
  ///
  /// Whether StatusCode is reported via serial port.
  /// The value shall match with the PcdStatusCodeUseSerial.
  ///
  BOOLEAN    StatusCodeUseSerial;
} MM_STATUS_CODE_USE_SERIAL;

extern EFI_GUID  gMmStatusCodeUseSerialHobGuid;

#endif
