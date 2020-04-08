//------------------------------------------------------------------------------
//
// RISC-V CPU functions.
//
// Copyright (c) 2016 - 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------
#include <Base.h>
#include <RiscVImpl.h>

.data

.text
.align 3

//
// Set machine mode scratch.
// @param a0 : Pointer to RISCV_MACHINE_MODE_CONTEXT.
//
ASM_FUNC (RiscVSetMachineScratch)
    csrrw a1, RISCV_CSR_MACHINE_MSCRATCH, a0
    ret

//
// Get machine mode scratch.
// @retval a0 : Pointer to RISCV_MACHINE_MODE_CONTEXT.
//
ASM_FUNC (RiscVGetMachineScratch)
    csrrs a0, RISCV_CSR_MACHINE_MSCRATCH, 0
    ret

//
// Get machine trap cause CSR.
//
ASM_FUNC (RiscVGetMachineTrapCause)
    csrrs a0, RISCV_CSR_MACHINE_MCAUSE, 0
    ret

//
// Get machine interrupt enable
//
ASM_FUNC (RiscVReadMachineInterruptEnable)
    csrr a0, RISCV_CSR_MACHINE_MIE
    ret

//
// Get machine interrupt pending
//
ASM_FUNC (RiscVReadMachineInterruptPending)
    csrr a0, RISCV_CSR_MACHINE_MIP
    ret

//
// Get machine status
//
ASM_FUNC (RiscVReadMachineStatus)
    csrr a0, RISCV_CSR_MACHINE_MSTATUS
    ret

//
// Set machine status
//
ASM_FUNC (RiscVWriteMachineStatus)
    csrw RISCV_CSR_MACHINE_MSTATUS, a0
    ret

//
// Get machine trap vector
//
ASM_FUNC (RiscVReadMachineTrapVector)
    csrr a0, RISCV_CSR_MACHINE_MTVEC
    ret

//
// Read machine ISA
//
ASM_FUNC (RiscVReadMachineIsa)
    csrr a0, RISCV_CSR_MACHINE_MISA
    ret

//
// Read machine vendor ID
//
ASM_FUNC (RiscVReadMachineVendorId)
    csrr a0, RISCV_CSR_MACHINE_MVENDORID
    ret

//
// Read machine architecture ID
//
ASM_FUNC (RiscVReadMachineArchitectureId)
    csrr a0, RISCV_CSR_MACHINE_MARCHID
    ret

//
// Read machine implementation ID
//
ASM_FUNC (RiscVReadMachineImplementId)
    csrr a0, RISCV_CSR_MACHINE_MIMPID
    ret

//
// Set Supervisor Address Translation and
// Protection Register.
//
ASM_FUNC (RiscVSetSupervisorAddressTranslationRegister)
    csrw  RISCV_CSR_SUPERVISOR_SATP, a0
    ret

