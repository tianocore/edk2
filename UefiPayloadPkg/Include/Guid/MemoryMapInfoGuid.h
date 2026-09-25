/** @file
  This file defines the hob structure for memory map information.

  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Library/PcdLib.h>

///
/// Memory Map Information GUID
///
extern EFI_GUID  gLoaderMemoryMapInfoGuid;

///
/// MEMORY_MAP_ENTRY.Flag bits.
///
/// A bootloader that already knows a range is device MMIO can say so
/// explicitly rather than leaving UefiPayloadEntry to classify the range by
/// the below/above mTopOfLowerUsableDram heuristic.  The bit is additive: a
/// payload that predates it simply falls back to the heuristic, and a
/// bootloader that does not set it behaves exactly as before.
///
#define MEM_MAP_FLAG_MMIO  BIT0

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
