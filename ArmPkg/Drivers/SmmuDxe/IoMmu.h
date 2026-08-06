/** @file IoMmu.h

    This file is the IoMmu header file for SMMU driver.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  Page Table bit definitions used by the Smmu/IoMmu for mapping.
  See ARM ARM section D8.3.1 (VMSAv8-64 descriptor formats):
  <https://developer.arm.com/documentation/ddi0487/mc/>

  Note: shared architectural bit definitions (access flag, inner shareable,
  etc.) are pulled from <AArch64/AArch64Mmu.h> (via <Library/ArmLib.h>) as
  TT_AF, TT_SH_INNER_SHAREABLE. Only Stage-2 specific and
  IOMMU-specific helpers are defined here.
**/
#define PAGE_TABLE_ENTRY_VALID_BIT  0x1
#define PAGE_TABLE_BLOCK_MASK       0xFFF
#define PAGE_TABLE_DESCRIPTOR       (0x1 << 1)
#define PAGE_TABLE_READ_WRITE_FROM_IOMMU_ACCESS(IoMmuAccess)  (IoMmuAccess << 6)
#define PAGE_TABLE_S2_MEMATTR_NORMAL_WB  (0xF << 2)

typedef UINT64 PAGE_TABLE_ENTRY;

#define PAGE_TABLE_SIZE  (EFI_PAGE_SIZE / sizeof(PAGE_TABLE_ENTRY))  // Number of entries in a page table

// Page Table Structure used by SMMU
typedef struct _PAGE_TABLE {
  PAGE_TABLE_ENTRY    Entries[PAGE_TABLE_SIZE];
} PAGE_TABLE;

/**
  Installs the IOMMU Protocol on this SMMU instance.

  @retval EFI_SUCCESS           All the protocol interface was installed.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory in pool to install all the protocols.
  @retval EFI_ALREADY_STARTED   A Device Path Protocol instance was passed in that is already present in
                                the handle database.
  @retval EFI_INVALID_PARAMETER Handle is NULL.
  @retval EFI_INVALID_PARAMETER Protocol is already installed on the handle specified by Handle.
**/
EFI_STATUS
IoMmuInit (
  VOID
  );
