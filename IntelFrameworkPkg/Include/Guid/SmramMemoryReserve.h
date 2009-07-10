/** @file
  Definition of GUIDed HOB for reserving SMRAM regions.

  This file defines:
  * the GUID used to identify the GUID HOB for reserving SMRAM regions.
  * the data structure of SMRAM descriptor to describe SMRAM candidate regions
  * values of state of SMRAM candidate regions
  * the GUID specific data structure of HOB for reserving SMRAM regions.
  This GUIDed HOB can be used to convey the existence of the T-SEG reservation and H-SEG usage

  Copyright (c) 2007 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  GUIDs defined in SmmCis spec version 0.9

**/

#ifndef _EFI_SMM_PEI_SMRAM_MEMORY_RESERVE_H_
#define _EFI_SMM_PEI_SMRAM_MEMORY_RESERVE_H_

#define EFI_SMM_PEI_SMRAM_MEMORY_RESERVE \
  { \
    0x6dadf1d1, 0xd4cc, 0x4910, {0xbb, 0x6e, 0x82, 0xb1, 0xfd, 0x80, 0xff, 0x3d } \
  }

///
/// Describes the candidate regions for SMRAM that are
/// supported by this platform.
///
typedef struct {
  EFI_PHYSICAL_ADDRESS  PhysicalStart;  ///< Designates the physical address of the SMRAM in memory.
  EFI_PHYSICAL_ADDRESS  CpuStart;       ///< Designates the address of the SMRAM, as seen by software executing on the processors. 
  UINT64                PhysicalSize;   ///< Describes the number of bytes in the SMRAM region.
  UINT64                RegionState;    ///< Describes the accessibility attributes of the SMRAM.
} EFI_SMRAM_DESCRIPTOR;

///
/// Definition of SMRAM states, used as value for EFI_SMRAM_DESCRIPTOR.RegionState.
///@{
#define EFI_SMRAM_OPEN                0x00000001
#define EFI_SMRAM_CLOSED              0x00000002
#define EFI_SMRAM_LOCKED              0x00000004
#define EFI_CACHEABLE                 0x00000008
#define EFI_ALLOCATED                 0x00000010
///@}

/**
* GUID specific data structure of HOB for reserving SMRAM regions.
*
* Inconsistent with specification here: 
* EFI_HOB_SMRAM_DESCRIPTOR_BLOCK has been changed to EFI_SMRAM_HOB_DESCRIPTOR_BLOCK.
* This inconsistency is kept in code in order for backward compatibility.
**/
typedef struct {
  ///
  /// Designates the number of possible regions in the system
  /// that can be usable for SMRAM. 
  ///
  UINTN                 NumberOfSmmReservedRegions;
  ///
  /// Used throughout this protocol to describe the candidate
  /// regions for SMRAM that are supported by this platform. 
  ///
  EFI_SMRAM_DESCRIPTOR  Descriptor[1];
} EFI_SMRAM_HOB_DESCRIPTOR_BLOCK;

extern EFI_GUID gEfiSmmPeiSmramMemoryReserveGuid;

#endif

