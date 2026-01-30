//
// Enable Vector for RISC-V
//
// Copyright (c) 2026, ZTE Inc. All rights reserved.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
#include <Register/RiscV64/RiscVImpl.h>

.global ASM_PFX(InitializeVectorUnits)

ASM_PFX(InitializeVectorUnits):
    csrr  a0, CSR_SSTATUS
    li    a1, SSTATUS_VS
    or    a0, a0, a1
    csrw  CSR_SSTATUS, a0
    li    a0, 0
    ret
