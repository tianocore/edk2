/** @file

  Copyright (c) 2015 - 2016, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2023, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BASE_RISCV_MMU_LIB_H_
#define BASE_RISCV_MMU_LIB_H_

VOID
EFIAPI
RiscVLocalTlbFlushAll (
  VOID
  );

VOID
EFIAPI
RiscVLocalTlbFlush (
  UINTN  VirtAddr
  );

EFI_STATUS
EFIAPI
RiscVSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes
  );

EFI_STATUS
EFIAPI
RiscVConfigureMmu (
  VOID
  );

#endif /* BASE_RISCV_MMU_LIB_H_ */
