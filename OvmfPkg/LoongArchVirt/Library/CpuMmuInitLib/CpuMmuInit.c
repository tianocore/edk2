/** @file
  CPU Memory Map Unit Initialization library instance.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/CpuMmuLib.h>
#include <Library/DebugLib.h>
#include <Register/LoongArch64/Csr.h>
#include <Register/LoongArch64/Cpucfg.h>

//
// Because the page size in edk2 is 4KB, the lowest level
// page table is align to 12 bits, and the page table width
// of other levels is set to 9 bits by default, which will
// be 3 or 4 or 5 level page tables, and continuous.
//
// Correspondence between max virtual memory address width
// and page table level:
// 39 bit >= VA > 31 bit, 3 level page tables
// 48 bit >= VA > 40 bit, 4 level page tables
// 57 bit >= VA > 49 bit, 5 level page tables
//
#define DEFAULT_BIT_WIDTH_PER_LEVEL  (EFI_PAGE_SHIFT - 3)

/**
  Decided page walker width, level.

  @param[in, out]   PageWalkCfg   Page walker value instance.
  @param[in]        BitWidt       The bit width what you want, 0 is means use the default bit width.

  @retval  PageTableLevelNum      The max page table level.
**/
STATIC
UINT8
DecidePageWalkConfiguration (
  IN OUT UINT64  *PageWalkCfg OPTIONAL,
  IN     UINT8   BitWidth
  )
{
  CPUCFG_REG1_INFO_DATA  CpucfgReg1Data;
  UINT8                  CpuVirtMemAddressWidth;
  UINT8                  PageTableLevelNum;
  UINT8                  CurrentPageTableLevel;
  UINT32                 Pwcl0Value;
  UINT32                 Pwcl1Value;

  //
  // If BitWidth is 0, use the default bit width.
  //
  if (BitWidth == 0) {
    BitWidth = DEFAULT_BIT_WIDTH_PER_LEVEL;
  }

  //
  // Get the the CPU virtual memory address width.
  //
  AsmCpucfg (CPUCFG_REG1_INFO, &CpucfgReg1Data.Uint32);

  CpuVirtMemAddressWidth = (UINT8)(CpucfgReg1Data.Bits.VALEN + 1);

  //
  // Statisitics the maximum page table level
  //
  PageTableLevelNum = 0x0;
  if (((CpuVirtMemAddressWidth - EFI_PAGE_SHIFT) % BitWidth) > 0) {
    PageTableLevelNum++;
  }

  PageTableLevelNum += (CpuVirtMemAddressWidth - EFI_PAGE_SHIFT) / BitWidth;

  //
  // Set page table level
  //
  Pwcl0Value = 0x0;
  Pwcl1Value = 0x0;
  for (CurrentPageTableLevel = 0x0; CurrentPageTableLevel < PageTableLevelNum; CurrentPageTableLevel++) {
    if (CurrentPageTableLevel < 0x3) {
      // Less then or equal to level 3
      Pwcl0Value |= ((BitWidth * CurrentPageTableLevel + EFI_PAGE_SHIFT) << 10 * CurrentPageTableLevel) |
                    BitWidth << (10 * CurrentPageTableLevel + 5);
    } else {
      // Lager then level 3
      Pwcl1Value |= ((BitWidth * CurrentPageTableLevel + EFI_PAGE_SHIFT) << 12 * (CurrentPageTableLevel - 3)) |
                    BitWidth << (12 * (CurrentPageTableLevel - 3) + 6);
    }

    DEBUG ((
      DEBUG_INFO,
      "%a  %d Level %d DIR shift %d.\n",
      __func__,
      __LINE__,
      (CurrentPageTableLevel + 1),
      (BitWidth * CurrentPageTableLevel + EFI_PAGE_SHIFT)
      ));
  }

  *PageWalkCfg = ((UINT64)Pwcl1Value << 32) | Pwcl0Value;

  return PageTableLevelNum;
}

/**
  Create a page table and initialize the memory management unit(MMU).

  @param[in]   MemoryTable           A pointer to a memory ragion table.

  @retval  EFI_SUCCESS                Configure MMU successfully.
           EFI_INVALID_PARAMETER      MemoryTable is NULL.
           EFI_UNSUPPORTED            MemoryRegionMap failed or out of memory space or size not aligned
                                      or MaxLivel out of bound.
**/
EFI_STATUS
EFIAPI
ConfigureMemoryManagementUnit (
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryTable
  )
{
  EFI_STATUS  Status;
  UINTN       PageTable;
  UINT64      PageWalkCfg;
  UINT8       MaxLevel;

  if (MemoryTable == NULL) {
    ASSERT (MemoryTable != NULL);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Automatically obtain the current appropriate page walker configuration.
  //
  MaxLevel = DecidePageWalkConfiguration (&PageWalkCfg, 0);

  if ((MaxLevel < 0) || (MaxLevel > 5)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Clear PGD series registers.
  //
  CsrWrite (LOONGARCH_CSR_PGDL, 0x0);
  CsrWrite (LOONGARCH_CSR_PGDH, 0x0);

  PageTable = 0;
  while (MemoryTable->NumberOfPages != 0) {
    DEBUG ((
      DEBUG_INFO,
      "%a %d VirtualBase %p VirtualEnd %p Attributes %p .\n",
      __func__,
      __LINE__,
      MemoryTable->VirtualStart,
      (EFI_PAGES_TO_SIZE (MemoryTable->NumberOfPages) + MemoryTable->VirtualStart),
      MemoryTable->Attribute
      ));

    Status = MemoryRegionMap (
               &PageTable,
               PageWalkCfg,
               MemoryTable->VirtualStart,
               EFI_PAGES_TO_SIZE (MemoryTable->NumberOfPages),
               MemoryTable->Attribute,
               0x0
               );

    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    MemoryTable++;
  }

  //
  // Configure page walker.
  //
  CsrWrite (LOONGARCH_CSR_PWCTL0, (UINT32)PageWalkCfg);
  if ((PageWalkCfg >> 32) != 0x0) {
    CsrWrite (LOONGARCH_CSR_PWCTL1, (UINT32)(PageWalkCfg >> 32));
  }

  //
  // Set page size
  //
  CsrXChg (LOONGARCH_CSR_TLBIDX, (DEFAULT_PAGE_SIZE << CSR_TLBIDX_SIZE), CSR_TLBIDX_SIZE_MASK);
  CsrWrite (LOONGARCH_CSR_STLBPGSIZE, DEFAULT_PAGE_SIZE);
  CsrXChg (LOONGARCH_CSR_TLBREHI, (DEFAULT_PAGE_SIZE << CSR_TLBREHI_PS_SHIFT), CSR_TLBREHI_PS);

  //
  // Enable MMU
  //
  CsrWrite (LOONGARCH_CSR_PGDL, PageTable);

  //
  // Enable Paging
  //
  CsrXChg (LOONGARCH_CSR_CRMD, BIT4, BIT4|BIT3);

  DEBUG ((DEBUG_INFO, "%a %d Enable MMU Start PageBassAddress %p.\n", __func__, __LINE__, PageTable));

  return EFI_SUCCESS;
}
