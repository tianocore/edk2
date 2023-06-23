/** @file
  Library to discover RISC-V extensions

  Copyright (c) 2023 Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RISCV_EXT_DISCOVERY_LIB_H_
#define RISCV_EXT_DISCOVERY_LIB_H_

EFIAPI
UINT32
IsRiscVExtSupported (
  CHAR8  *Ext
  );

#endif
