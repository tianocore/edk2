/** @file

  Copyright (c) 2015 - 2016, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2023, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BASE_RISCV_MMU_LIB_H_
#define BASE_RISCV_MMU_LIB_H_

#define PTE_ATTRIBUTES_MASK  0xE

/**
  The API to flush all local TLBs.

**/
VOID
EFIAPI
RiscVLocalTlbFlushAll (
  VOID
  );

/**
  The API to flush local TLB at a virtual address.

  @param  VirtAddr  The virtual address.

**/
VOID
EFIAPI
RiscVLocalTlbFlush (
  UINTN  VirtAddr
  );

/**
  Free resources of translation table recursively.

  @param  TranslationTable  The pointer of table.
  @param  Level             The current level.

**/
VOID
EFIAPI
FreePageTablesRecursive (
  IN  UINT64  *TranslationTable,
  IN  UINTN   Level
  );

/**
  Update region mapping at root table.

  @param  RegionStart           The start address of the region.
  @param  RegionLength          The length of the region.
  @param  AttributeSetMask      The attribute mask to be set.
  @param  AttributeClearMask    The attribute mask to be clear.
  @param  RootTable             The pointer of root table.
  @param  TableIsLive           TRUE if this is live update, FALSE otherwise.

  @retval EFI_INVALID_PARAMETER The RegionStart or RegionLength was not valid.
  @retval EFI_OUT_OF_RESOURCES  Not enough resource.
  @retval EFI_SUCCESS           The operation succesfully.

**/
EFI_STATUS
EFIAPI
UpdateRegionMapping (
  IN  UINT64   RegionStart,
  IN  UINT64   RegionLength,
  IN  UINT64   AttributeSetMask,
  IN  UINT64   AttributeClearMask,
  IN  UINT64   *RootTable,
  IN  BOOLEAN  TableIsLive
  );

/**
  The API to set a GCD attribute on an memory region.

  @param  BaseAddress             The base address of the region.
  @param  Length                  The length of the region.
  @param  Attributes              The GCD attributes.

  @retval EFI_INVALID_PARAMETER   The BaseAddress or Length was not valid.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource.
  @retval EFI_SUCCESS             The operation succesfully.

**/
EFI_STATUS
EFIAPI
RiscVSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes
  );

/**
  The API to configure and enable RISC-V MMU with the highest mode supported.

  @retval EFI_OUT_OF_RESOURCES    Not enough resource.
  @retval EFI_SUCCESS             The operation succesfully.

**/
EFI_STATUS
EFIAPI
RiscVConfigureMmu (
  VOID
  );

#endif /* BASE_RISCV_MMU_LIB_H_ */
