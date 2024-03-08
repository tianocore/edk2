/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_MMU_LIB_H_
#define CPU_MMU_LIB_H_

#include <Uefi/UefiBaseType.h>

/**
  Sets the Attributes  of the specified memory region.

  @param[in]  BaseAddress    The base address of the memory region to set the Attributes.
  @param[in]  Length         The length of the memory region to set the Attributes.
  @param[in]  Attributes     The bitmask of attributes to set, which refer to UEFI SPEC
                             7.2.3(EFI_BOOT_SERVICES.GetMemoryMap()).
  @param[in]  AttributeMask  Mask of memory attributes to take into account.

  @retval  EFI_SUCCESS    The Attributes was set successfully
**/
EFI_STATUS
EFIAPI
SetMemoryRegionAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes,
  IN UINT64                AttributeMask
  );

#endif // CPU_MMU_LIB_H_
