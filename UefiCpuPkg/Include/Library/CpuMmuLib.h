/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_MMU_LIB_H_
#define CPU_MMU_LIB_H_

#include <Uefi/UefiBaseType.h>

/**
  Maps the memory region in the page table to the specified attributes.

  @param[in, out] PageTable      The pointer to the page table to update, or pointer to NULL
                                 if a new page table is to be created.
  @param[in]      PageWalkCfg    The page walk controller configure.
  @param[in]      BaseAddress    The base address of the memory region to set the Attributes.
  @param[in]      Length         The length of the memory region to set the Attributes.
  @param[in]      Attributes     The bitmask of attributes to set, which refer to UEFI SPEC
                                 7.2.3(EFI_BOOT_SERVICES.GetMemoryMap()).
  @param[in]      AttributeMask  Mask of memory attributes to take into account.

  @retval EFI_SUCCESS            The Attributes was set successfully or Length is 0.
  @retval EFI_INVALID_PARAMETER  PageTable is NULL.
  @retval EFI_UNSUPPORTED        *PageTable is NULL.
**/
EFI_STATUS
EFIAPI
MemoryRegionMap (
  IN OUT UINTN                 *PageTable  OPTIONAL,
  IN     UINT64                PageWalkCfg,
  IN     EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN     UINT64                Length,
  IN     UINT64                Attributes,
  IN     UINT64                AttributeMask
  );

#endif // CPU_MMU_LIB_H_
