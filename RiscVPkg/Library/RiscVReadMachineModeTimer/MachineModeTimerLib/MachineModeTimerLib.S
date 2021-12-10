//------------------------------------------------------------------------------
//
// Read mtimer through platform library.
//
// Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------
#include <RiscVImpl.h>

.data

.text
.align 3

.global ASM_PFX(RiscVReadMachineTimerInterface)
//
// Read machine mode timer CSR.
// @retval a0 : 64-bit machine timer.
//
ASM_PFX (RiscVReadMachineTimerInterface):
    call RiscVReadMachineTimer
    ret

