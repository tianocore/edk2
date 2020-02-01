//------------------------------------------------------------------------------ 
//
// RISC-V reset vector (VTF file).
//
// Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php.
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------
.data
.align 12
.section .text

.global ASM_PFX(_ModuleEntryPoint)
                           
.org 0x0e00                 // User mode trap.
    li      a0, 0x11111000
    jalr    x0, a0, 0x123   // Jump to user mode handler declared in RISC-V SecMain.

.org 0x0e40                 // Supervisor-mode trap
    li      a0, 0x11111000
    jalr    x0, a0, 0x123   // Jump to supervisor mode handler declared in RISC-V SecMain.

.org 0x0e80                 // Hypervisor-mode trap
    li      a0, 0x11111000
    jalr    x0, a0, 0x123   // Jump to hypervisor mode handler declared in RISC-V SecMain.

.org 0x0ec0                 // Machine-mode trap
    li      a0, 0x11111000
    jalr    x0, a0, 0x123   // Jump to machine mode handler declared in RISC-V SecMain.

.org 0x0efc                 // NMI
    jal NMIShortHandler

.org 0x0f00                 // Reset vector started at 0xFF~FFF00
                            // Build tool(GenFV) will put this binary on top of firmware volume. 
.dword lLoadHigh20BitAddress - ASM_PFX(_ModuleEntryPoint)
.dword lLoadLow12BitAddress - ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
lLoadHigh20BitAddress:
    li      a0, 0x12345000  // Target address to SecCore will be fixed by GenFV.
lLoadLow12BitAddress:
    jalr    x0, a0, 0x678

//
// NMMI handler
//
NMIShortHandler:
    jal     NMIShortHandler


