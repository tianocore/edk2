//------------------------------------------------------------------------------
//
// Read Machine mode timer using shadow CSR.
//
// Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------
#include <IndustryStandard/RiscV.h>

.data

.text
.align 3

.global ASM_PFX(RiscVReadMachineTimerInterface)
//
// Read machine mode timer CSR through shadow CSR.
// @retval a0 : 64-bit machine timer.
//
ASM_PFX (RiscVReadMachineTimerInterface):
    csrr a0, RISCV_CSR_TIME
    ret
