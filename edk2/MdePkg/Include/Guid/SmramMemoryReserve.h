/** @file
  GUID for use in reserving SMRAM regions.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    

  Module Name:  SmramMemoryReserve.h

  @par Revision Reference:
  GUIDs defined in SmmCis spec version 0.9

**/

#ifndef _EFI_SMM_PEI_SMRAM_MEMORY_RESERVE_H_
#define _EFI_SMM_PEI_SMRAM_MEMORY_RESERVE_H_

#define EFI_SMM_PEI_SMRAM_MEMORY_RESERVE \
  { \
    0x6dadf1d1, 0xd4cc, 0x4910, {0xbb, 0x6e, 0x82, 0xb1, 0xfd, 0x80, 0xff, 0x3d } \
  }

//
// *******************************************************
//  EFI_SMRAM_DESCRIPTOR
// *******************************************************
//
typedef struct {
  EFI_PHYSICAL_ADDRESS  PhysicalStart;  // Phsyical location in DRAM
  EFI_PHYSICAL_ADDRESS  CpuStart;       // Address CPU uses to access the SMI handler
  // May or may not match PhysicalStart
  //
  UINT64                PhysicalSize;
  UINT64                RegionState;
} EFI_SMRAM_DESCRIPTOR;

//
// *******************************************************
//  EFI_SMRAM_STATE
// *******************************************************
//
#define EFI_SMRAM_OPEN                0x00000001
#define EFI_SMRAM_CLOSED              0x00000002
#define EFI_SMRAM_LOCKED              0x00000004
#define EFI_CACHEABLE                 0x00000008
#define EFI_ALLOCATED                 0x00000010
#define EFI_NEEDS_TESTING             0x00000020
#define EFI_NEEDS_ECC_INITIALIZATION  0x00000040

//
// *******************************************************
//  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK
// *******************************************************
//
typedef struct {
  UINTN                 NumberOfSmmReservedRegions;
  EFI_SMRAM_DESCRIPTOR  Descriptor[1];
} EFI_SMRAM_HOB_DESCRIPTOR_BLOCK;

extern EFI_GUID gEfiSmmPeiSmramMemoryReserve;

#endif
