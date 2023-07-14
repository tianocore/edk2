/** @file

  Copyright (c) 2015 - 2016, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2023, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BASE_RISCV_MMU_LIB_H_
#define BASE_RISCV_MMU_LIB_H_

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
