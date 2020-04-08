//------------------------------------------------------------------------------
//
// Null instance of Timer CSR functions.
//
// Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------
#include <Base.h>

.data

.text
.align 3

.global ASM_PFX(RiscVReadMachineTimer)
.global ASM_PFX(RiscVSetMachineTimerCmp)
.global ASM_PFX(RiscVReadMachineTimerCmp)

//
// Read machine timer CSR.
// @retval a0 : 64-bit machine timer.
//
ASM_PFX (RiscVReadMachineTimer):
    ret

//
// Set machine timer compare CSR.
// @param a0 : UINT64
//
ASM_PFX (RiscVSetMachineTimerCmp):
    ret

//
// Read machine timer compare CSR.
// @param a0 : UINT64
//
ASM_PFX (RiscVReadMachineTimerCmp):
    ret
