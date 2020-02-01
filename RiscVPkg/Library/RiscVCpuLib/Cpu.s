//------------------------------------------------------------------------------ 
//
// RISC-V CPU functions.
//
// Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php.
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------
#include <Base.h>
#include <RiscV.h>

.data

.text
.align 3

.global ASM_PFX(RiscVSetScratch)
.global ASM_PFX(RiscVGetScratch)
.global ASM_PFX(RiscVReadMachineTimer)
.global ASM_PFX(RiscVSetMachineTimerCmp)
.global ASM_PFX(RiscVGetMachineTrapCause)

//
// Set machine mode scratch.
// @param a0 : Pointer to RISCV_MACHINE_MODE_CONTEXT.
//
ASM_PFX (RiscVSetScratch):
    csrrw a1, RISCV_CSR_MACHINE_MSCRATCH, a0
    ret

//
// Get machine mode scratch.
// @retval a0 : Pointer to RISCV_MACHINE_MODE_CONTEXT.
//
ASM_PFX (RiscVGetScratch):
    csrrs a0, RISCV_CSR_MACHINE_MSCRATCH, 0
    ret

//
// Read machine timer CSR.
// @retval a0 : 32-bit machine timer.
//
ASM_PFX (RiscVReadMachineTimer):
    csrrs a0, RISCV_CSR_MACHINE_MTIME, 0
    ret

//
// Set machine timer compare CSR.
// @param a0 : UINT32
//
ASM_PFX (RiscVSetMachineTimerCmp):
    csrrw a1, RISCV_CSR_MACHINE_MTIMECMP, a0
    ret

//
// Get machine trap cause CSR.
//
ASM_PFX (RiscVGetMachineTrapCause):
    csrrs a0, RISCV_CSR_MACHINE_MCAUSE, 0
    ret
