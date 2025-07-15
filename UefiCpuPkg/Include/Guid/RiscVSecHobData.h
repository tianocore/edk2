/** @file
  RISC-V SEC Data Hob to pass booting information between SEC, PEI and DXE.

  Copyright (c) 2025, Ventana Micro Systems Inc.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RISCV_SEC_HOB_DATA_
#define RISCV_SEC_HOB_DATA_

#include <PiPei.h>

#define RISCV_SEC_HANDOFF_HOB_GUID  { 0xe5ad277d, 0xc2a2, 0x4462, { 0xb1, 0x60, 0x1e, 0x37, 0x6e, 0xdd, 0xf1, 0x95 } }

typedef struct {
  UINTN    BootHartId;
  VOID     *FdtPointer;
} RISCV_SEC_HANDOFF_DATA;

#endif /* RISCV_SEC_HOB_DATA_ */
