//------------------------------------------------------------------------------ 
//
// RISC-V Sec module.
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

.global ASM_PFX(_ModuleEntryPoint)

.dword ASM_PFX(TrapFromUserModeHandler)
.align 3
.dword ASM_PFX(TrapFromSupervisorModeHandler)
.align 3
.dword ASM_PFX(TrapFromHypervisorModeHandler)
.align 3
.dword ASM_PFX(TrapFromMachineModeHandler)
.align 3
.dword ASM_PFX(NmiHandler)
.align 3
ASM_PFX(_ModuleEntryPoint):
    //
    // Set vector base to.
    //
//    li      a0, 0xfffffec0
//    csrrw   a1, 0x301, a0   // Machine vector base.
//    li      a0, 0xfffffe80
//    csrrw   a1, 0x201, a0   // Hypervisor vector base.
//    li      a0, 0xfffffe40
//    csrrw   a1, 0x101, a0   // Supervisor vector base.
    li      a0, 0xfffffe00
    csrrw   a1, RISCV_CSR_MACHINE_MTVEC, a0   // Machine vector base.

    //
    // Platform SEC PEI temporary memory init.
    //
    call    RiscVPlatformTemporaryMemInit
    //
    // Set up temporary memory for SEC and PEI phase.
    // PcdOvmfSecPeiTempRamBase and PcdOvmfSecPeiTempRamSize
    // map to the specific region within system memory.
    //
    li      a0, FixedPcdGet32 (PcdRiscVSecPeiTempRamBase)
    li      a1, FixedPcdGet32 (PcdRiscVSecPeiTempRamSize)
    add     a2, a0, a1      // a2 is top of temporary memory.
    add     sp, a0, a1      // Set stack pointer to top of temporary memory.
    
    //
    // Call startup with stack
    //
    li      a1, 0x20
    sub     sp, sp, a1
    li      a0, FixedPcdGet32 (PcdRiscVPeiFvBase) // Load boot FV in a0.
    add     a1, a2, 0                             // Load top of stack in a1.
    call    SecCoreStartupWithStack
    //
    // Never return here.
    //

//
// User mode trap handler.
//
ASM_PFX(TrapFromUserModeHandler):
    call    RiscVUserModeTrapHandler
    eret

//
//Supervisor mode trap handler.
//
ASM_PFX(TrapFromSupervisorModeHandler):
    call    RiscVSupervisorModeTrapHandler
    eret

//
// Hypervisor mode trap handler.
//
ASM_PFX(TrapFromHypervisorModeHandler):
    call    RiscVHypervisorModeTrapHandler
    eret

//
// Machine mode trap handler.
//
ASM_PFX(TrapFromMachineModeHandler):
    call    RiscVMachineModeTrapHandler
    eret

//
// NMI trap handler.
//
ASM_PFX(NmiHandler):
    call    RiscVNmiHandler
    eret

