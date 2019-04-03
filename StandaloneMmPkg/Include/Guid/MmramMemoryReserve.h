/** @file
  Definition of GUIDed HOB for reserving MMRAM regions.

  This file defines:
  * the GUID used to identify the GUID HOB for reserving MMRAM regions.
  * the data structure of MMRAM descriptor to describe MMRAM candidate regions
  * values of state of MMRAM candidate regions
  * the GUID specific data structure of HOB for reserving MMRAM regions.
  This GUIDed HOB can be used to convey the existence of the T-SEG reservation and H-SEG usage

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  GUIDs defined in MmCis spec version 0.9.

**/

#ifndef _EFI_MM_PEI_MMRAM_MEMORY_RESERVE_H_
#define _EFI_MM_PEI_MMRAM_MEMORY_RESERVE_H_

#define EFI_MM_PEI_MMRAM_MEMORY_RESERVE \
  { \
    0x0703f912, 0xbf8d, 0x4e2a, {0xbe, 0x07, 0xab, 0x27, 0x25, 0x25, 0xc5, 0x92 } \
  }

/**
* GUID specific data structure of HOB for reserving MMRAM regions.
*
* Inconsistent with specification here:
* EFI_HOB_MMRAM_DESCRIPTOR_BLOCK has been changed to EFI_MMRAM_HOB_DESCRIPTOR_BLOCK.
* This inconsistency is kept in code in order for backward compatibility.
**/
typedef struct {
  ///
  /// Designates the number of possible regions in the system
  /// that can be usable for MMRAM.
  ///
  /// Inconsistent with specification here:
  /// In Framework MM CIS 0.91 specification, it defines the field type as UINTN.
  /// However, HOBs are supposed to be CPU neutral, so UINT32 should be used instead.
  ///
  UINT32                NumberOfMmReservedRegions;
  ///
  /// Used throughout this protocol to describe the candidate
  /// regions for MMRAM that are supported by this platform.
  ///
  EFI_MMRAM_DESCRIPTOR  Descriptor[1];
} EFI_MMRAM_HOB_DESCRIPTOR_BLOCK;

extern EFI_GUID gEfiMmPeiSmramMemoryReserveGuid;

#endif

