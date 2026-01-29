/** @file
  Define the structure for Debug Print Error Level Guid Hob.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL_H_
#define UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack (1)
//
// ErrorLevel: The error level of the debug message.
// Bits for ErrorLevel is declared in
// edk2\MdePkg\Include\Library\DebugLib.h
//
typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  UINT32                              ErrorLevel;
} UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL;

#pragma pack()

#define UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL_REVISION  1

extern GUID  gEdkiiDebugPrintErrorLevelGuid;
#endif
