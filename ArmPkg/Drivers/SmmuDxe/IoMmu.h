/** @file IoMmu.h

    This file is the IoMmu header file for SMMU driver.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef IOMMU_H_
#define IOMMU_H_

/**
  Page Table bit definitions used by the Smmu/IoMmu for mapping
  <https://developer.arm.com/documentation/102105/ka-07>
  Section D8.3.1 VMSAv8-64 descriptor formats
**/
#define PAGE_TABLE_READ_BIT         (0x1 << 6)
#define PAGE_TABLE_WRITE_BIT        (0x1 << 7)
#define PAGE_TABLE_ENTRY_VALID_BIT  0x1
#define PAGE_TABLE_BLOCK_OFFSET     0xFFF
#define PAGE_TABLE_ACCESS_FLAG      (0x1 << 10)
#define PAGE_TABLE_DESCRIPTOR       (0x1 << 1)
#define PAGE_TABLE_READ_WRITE_FROM_IOMMU_ACCESS(IoMmuAccess)  (IoMmuAccess << 6)

// Forward declaration; full definition lives in SmmuV3.h.
typedef struct _SMMU_INFO SMMU_INFO;

typedef UINT64 PAGE_TABLE_ENTRY;

#define PAGE_TABLE_SIZE  (EFI_PAGE_SIZE / sizeof(PAGE_TABLE_ENTRY))  // Number of entries in a page table

// Page Table Structure used by SMMU
typedef struct _PAGE_TABLE {
  PAGE_TABLE_ENTRY    Entries[PAGE_TABLE_SIZE];
} PAGE_TABLE;

/**
  Update the page table mapping with the given physical address and flags.

  @param [in]  SmmuInfo                   SMMU instance.
  @param [in]  Root                       Pointer to the root page table.
  @param [in]  Vmid                       VMID for associated page table root.
  @param [in]  PhysicalAddress            Physical address to map.
  @param [in]  Bytes                      Number of bytes to map.
  @param [in]  Flags                      Flags to set for the mapping. 12 bits or less.
  @param [in]  Valid                      Boolean to indicate if the entry is valid.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
UpdatePageTable (
  IN SMMU_INFO   *SmmuInfo,
  IN PAGE_TABLE  *Root,
  IN UINT16      Vmid,
  IN UINT64      PhysicalAddress,
  IN UINT64      Bytes,
  IN UINT16      Flags,
  IN BOOLEAN     Valid
  );

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

#endif
