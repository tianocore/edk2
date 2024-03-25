/** @file
  CPU Memory Map Unit Initialization library instance.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/CpuMmuLib.h>
#include <Library/CpuMmuInitLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Protocol/DebugSupport.h>
#include <Register/LoongArch64/Csr.h>
#include <Register/LoongArch64/Cpucfg.h>
#include "TlbExceptionHandle.h"

//
// For coding convenience, define the maximum valid
// LoongArch exception.
// Since UEFI V2.11, it will be present in DebugSupport.h.
//
#define MAX_LOONGARCH_EXCEPTION  64

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
#define BIT_WIDTH_PER_LEVEL  9
#define MAX_SIZE_OF_PGD      ((1 << BIT_WIDTH_PER_LEVEL) * 8) // 512 items, 8 Byte each.

/**
  Create a page table and initialize the memory management unit(MMU).

  @param[in]   MemoryTable           A pointer to a memory ragion table.
  @param[out]  TranslationTableBase  A pointer to a translation table base address.
  @param[out]  TranslationTableSize  A pointer to a translation table base size.

  @retval  EFI_SUCCESS                Configure MMU successfully.
           EFI_INVALID_PARAMETER      MemoryTable is NULL.
           EFI_UNSUPPORTED            Out of memory space or size not aligned.
**/
EFI_STATUS
EFIAPI
ConfigureMemoryManagementUnit (
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryTable,
  OUT VOID                   **TranslationTableBase OPTIONAL,
  OUT UINTN                  *TranslationTableSize  OPTIONAL
  )
{
  VOID                   *TranslationTable;
  UINTN                  Length;
  UINTN                  TlbReEntry;
  UINTN                  TlbReEntryOffset;
  UINTN                  Remaining;
  EFI_STATUS             Status;
  CPUCFG_REG1_INFO_DATA  CpucfgReg1Data;
  UINT8                  CpuVirtMemAddressWidth;
  UINT8                  PageTableLevelNum;
  UINT8                  CurrentPageTableLevel;
  UINT32                 Pwcl0Value;
  UINT32                 Pwcl1Value;

  if (MemoryTable == NULL) {
    ASSERT (MemoryTable != NULL);
    return EFI_INVALID_PARAMETER;
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
  if (((CpuVirtMemAddressWidth - EFI_PAGE_SHIFT) % BIT_WIDTH_PER_LEVEL) > 0) {
    PageTableLevelNum++;
  }

  PageTableLevelNum += (CpuVirtMemAddressWidth - EFI_PAGE_SHIFT) / BIT_WIDTH_PER_LEVEL;

  //
  // Set page table level
  //
  Pwcl0Value = 0x0;
  Pwcl1Value = 0x0;
  for (CurrentPageTableLevel = 0x0; CurrentPageTableLevel < PageTableLevelNum; CurrentPageTableLevel++) {
    if (CurrentPageTableLevel < 0x3) {
      // Less then or equal to level 3
      Pwcl0Value |= ((BIT_WIDTH_PER_LEVEL * CurrentPageTableLevel + EFI_PAGE_SHIFT) << 10 * CurrentPageTableLevel) |
                    BIT_WIDTH_PER_LEVEL << (10 * CurrentPageTableLevel + 5);
    } else {
      // Lager then level 3
      Pwcl1Value |= ((BIT_WIDTH_PER_LEVEL * CurrentPageTableLevel + EFI_PAGE_SHIFT) << 12 * (CurrentPageTableLevel - 3)) |
                    BIT_WIDTH_PER_LEVEL << (12 * (CurrentPageTableLevel - 3) + 6);
    }

    DEBUG ((
      DEBUG_INFO,
      "%a  %d Level %d DIR shift %d.\n",
      __func__,
      __LINE__,
      (CurrentPageTableLevel + 1),
      (BIT_WIDTH_PER_LEVEL * CurrentPageTableLevel + EFI_PAGE_SHIFT)
      ));
  }

  CsrWrite (LOONGARCH_CSR_PWCTL0, Pwcl0Value);
  if (Pwcl1Value != 0x0) {
    CsrWrite (LOONGARCH_CSR_PWCTL1, Pwcl1Value);
  }

  //
  // Set page size
  //
  CsrXChg (LOONGARCH_CSR_TLBIDX, (DEFAULT_PAGE_SIZE << CSR_TLBIDX_SIZE), CSR_TLBIDX_SIZE_MASK);
  CsrWrite (LOONGARCH_CSR_STLBPGSIZE, DEFAULT_PAGE_SIZE);
  CsrXChg (LOONGARCH_CSR_TLBREHI, (DEFAULT_PAGE_SIZE << CSR_TLBREHI_PS_SHIFT), CSR_TLBREHI_PS);

  //
  // Create PGD and set the PGD address to PGDL
  //
  TranslationTable = AllocatePages (EFI_SIZE_TO_PAGES (MAX_SIZE_OF_PGD));
  ZeroMem (TranslationTable, MAX_SIZE_OF_PGD);

  if (TranslationTable == NULL) {
    goto FreeTranslationTable;
  }

  CsrWrite (LOONGARCH_CSR_PGDL, (UINTN)TranslationTable);

  //
  // Fill page tables
  //
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

    Status = SetMemoryRegionAttributes (
               MemoryTable->VirtualStart,
               EFI_PAGES_TO_SIZE (MemoryTable->NumberOfPages),
               MemoryTable->Attribute,
               0x0
               );

    if (EFI_ERROR (Status)) {
      goto FreeTranslationTable;
    }

    MemoryTable++;
  }

  //
  // Set TLB exception handler
  //
  ///
  /// TLB Re-entry address at the end of exception vector, a vector is up to 512 bytes,
  /// so the starting address is: total exception vector size + total interrupt vector size + base.
  /// The total size of TLB handler and exception vector size and interrupt vector size should not
  /// be lager than 64KB.
  ///
  Length           = (UINTN)HandleTlbRefillEnd - (UINTN)HandleTlbRefillStart;
  TlbReEntryOffset = (MAX_LOONGARCH_EXCEPTION + MAX_LOONGARCH_INTERRUPT) * 512;
  Remaining        = TlbReEntryOffset % SIZE_4KB;
  if (Remaining != 0x0) {
    TlbReEntryOffset += (SIZE_4KB - Remaining);
  }

  TlbReEntry = PcdGet64 (PcdLoongArchExceptionVectorBaseAddress) + TlbReEntryOffset;
  if ((TlbReEntryOffset + Length) > SIZE_64KB) {
    goto FreeTranslationTable;
  }

  //
  // Ensure that TLB refill exception base address alignment is equals to 4KB and is valid.
  //
  if (TlbReEntry & (SIZE_4KB - 1)) {
    goto FreeTranslationTable;
  }

  CopyMem ((VOID *)TlbReEntry, HandleTlbRefillStart, Length);
  InvalidateInstructionCacheRange ((VOID *)(UINTN)HandleTlbRefillStart, Length);

  //
  // Set the address of TLB refill exception handler
  //
  SetTlbRebaseAddress ((UINTN)TlbReEntry);

  //
  // Enable MMU
  //
  CsrXChg (LOONGARCH_CSR_CRMD, BIT4, BIT4|BIT3);

  DEBUG ((DEBUG_INFO, "%a %d Enable MMU Start PageBassAddress %p.\n", __func__, __LINE__, TranslationTable));

  //
  // Set MMU enable flag.
  //
  return EFI_SUCCESS;

FreeTranslationTable:
  if (TranslationTable != NULL) {
    FreePages (TranslationTable, EFI_SIZE_TO_PAGES (MAX_SIZE_OF_PGD));
  }

  return EFI_UNSUPPORTED;
}
