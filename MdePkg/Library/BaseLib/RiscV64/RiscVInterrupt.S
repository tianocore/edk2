//------------------------------------------------------------------------------ 
//
// Cpu interrupt enable/disable for RISC-V
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

ASM_GLOBAL ASM_PFX(RiscVDisableInterrupts)
ASM_GLOBAL ASM_PFX(RiscVEnableInterrupt)
ASM_GLOBAL ASM_PFX(RiscVGetInterrupts)

ASM_PFX(RiscVDisableInterrupts):
  li    a1, 0xee
  csrrc a0, 0x304, a1
  ret

ASM_PFX(RiscVEnableInterrupt):
  li    a1, 0xee
  csrrw a0, 0x304, a1
  ret

ASM_PFX(RiscVGetInterrupts):
  csrrw a0, 0x304, 0
  ret
