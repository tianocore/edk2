/** @file
  This file defines the hob structure for memory map information.

  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEMORY_MAP_INFO_GUID_H_
#define MEMORY_MAP_INFO_GUID_H_

#include <Library/PcdLib.h>

///
/// Memory Map Information GUID
///
extern EFI_GUID  gLoaderMemoryMapInfoGuid;

#pragma pack(1)
typedef struct {
  UINT64    Base;
  UINT64    Size;
  UINT8     Type;
  UINT8     Flag;
  UINT8     Reserved[6];
} MEMORY_MAP_ENTRY;

typedef struct {
  UINT8               Revision;
  UINT8               Reserved0[3];
  UINT32              Count;
  MEMORY_MAP_ENTRY    Entry[0];
} MEMORY_MAP_INFO;
#pragma pack()

#endif
