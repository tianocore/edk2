/** @file

  Copyright (c) 2024, Canonical Services Ltd<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BASE_RISCV_FPU_LIB_H_
#define BASE_RISCV_FPU_LIB_H_

/**
  Initialize floating point unit

**/
EFI_STATUS
EFIAPI
RiscVInitializeFpu (
  VOID
  );

#endif /* BASE_RISCV_FPU_LIB_H_ */
