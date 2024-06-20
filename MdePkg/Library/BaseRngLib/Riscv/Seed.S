#-- ----------------------------------------------------------------------------
#
#ReadSeed () for RISC-V
#
#Copyright (c) 2024 Rivos, Inc.
#
#SPDX-License-Identifier: BSD-2-Clause-Patent
#
#-- ----------------------------------------------------------------------------

#include <Register/RiscV64/RiscVImpl.h>

.text
  .p2align 4

ASM_FUNC (ReadSeed)
#The SEED CSR must only be accessed with read-write instructions
csrrw a0, 0x15, x0
ret
