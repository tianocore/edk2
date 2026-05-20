// ------------------------------------------------------------------------------
//
// Enable FPU for RISC-V
//
// Copyright (c) 2024, Canonical Services Ltd.<BR>
// Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// ------------------------------------------------------------------------------
#include <Register/RiscV64/RiscVImpl.h>

.global ASM_PFX(InitializeFloatingPointUnits)

ASM_PFX(InitializeFloatingPointUnits):
    csrr  a0, CSR_SSTATUS
    li    a1, MSTATUS_FS
    or    a0, a0, a1
    csrw  CSR_SSTATUS, a0
    csrw  CSR_FCSR, x0
    li    a0, 0
    ret
