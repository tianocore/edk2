/** @file
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent
  LZ4 Custom decompress algorithm Guid definition.
**/

#pragma once

///
/// The Global ID used to identify a section of an FFS file of type
/// EFI_SECTION_GUID_DEFINED, whose contents have been compressed using LZ4.
///
#define LZ4_CUSTOM_DECOMPRESS_GUID  \
  { 0xD47B5624, 0x5E79, 0x4616, { 0xB3, 0xDC, 0x5B, 0x60, 0xE7, 0xD0, 0xA9, 0x29 } }

extern GUID  gLz4CustomDecompressGuid;
