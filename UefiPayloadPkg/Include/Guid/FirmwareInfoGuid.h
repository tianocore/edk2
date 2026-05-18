/** @file
  This file defines the hob structure for firmware related information from a
  bootloader

  Copyright (c) 2025, 3mdeb Sp. z o.o.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

///
/// Firmware information GUID
///
extern EFI_GUID  gEfiFirmwareInfoHobGuid;

typedef struct {
  EFI_GUID    Type;
  CHAR8       VersionStr[64];
  UINT32      Version;
  UINT32      LowestSupportedVersion;
  UINT32      ImageSize;
} FIRMWARE_INFO;
