/** @file
  Tiano Custom decompress Guid definition.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TIANO_CUSTOM_DECOMPRESS_GUID_H__
#define __TIANO_CUSTOM_DECOMPRESS_GUID_H__

///
/// The Global ID used to identify a section of an FFS file of type
/// EFI_SECTION_GUID_DEFINED, whose contents have been compressed using
/// Tiano Custom compression.
///
#define TIANO_CUSTOM_DECOMPRESS_GUID  \
  { 0xA31280AD, 0x481E, 0x41B6, { 0x95, 0xE8, 0x12, 0x7F, 0x4C, 0x98, 0x47, 0x79 } }

extern GUID gTianoCustomDecompressGuid;

#endif
