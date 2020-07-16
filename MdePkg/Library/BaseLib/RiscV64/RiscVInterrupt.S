//------------------------------------------------------------------------------
//
// RISC-V Supervisor Mode interrupt enable/disable
//
// Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(RiscVDisableSupervisorModeInterrupts)
ASM_GLOBAL ASM_PFX(RiscVEnableSupervisorModeInterrupt)
ASM_GLOBAL ASM_PFX(RiscVGetSupervisorModeInterrupts)

#define  SSTATUS_SIE                 0x00000002
#define  CSR_SSTATUS                 0x100
  #define  SSTATUS_SPP_BIT_POSITION  8

//
// This routine disables supervisor mode interrupt
//
ASM_PFX(RiscVDisableSupervisorModeInterrupts):
  add   sp, sp, -(__SIZEOF_POINTER__)
  sd    a1, (sp)
  li    a1, SSTATUS_SIE
  csrc  CSR_SSTATUS, a1
  ld    a1, (sp)
  add   sp, sp, (__SIZEOF_POINTER__)
  ret

//
// This routine enables supervisor mode interrupt
//
ASM_PFX(RiscVEnableSupervisorModeInterrupt):
  add   sp, sp, -2*(__SIZEOF_POINTER__)
  sd    a0, (0*__SIZEOF_POINTER__)(sp)
  sd    a1, (1*__SIZEOF_POINTER__)(sp)

  csrr  a0, CSR_SSTATUS
  and   a0, a0, (1 << SSTATUS_SPP_BIT_POSITION)
  bnez  a0, InTrap      // We are in supervisor mode (SMode)
                        // trap handler.
                        // Skip enabling SIE becasue SIE
                        // is set to disabled by RISC-V hart
                        // when the trap takes hart to SMode.

  li    a1, SSTATUS_SIE
  csrs  CSR_SSTATUS, a1
InTrap:
  ld    a0, (0*__SIZEOF_POINTER__)(sp)
  ld    a1, (1*__SIZEOF_POINTER__)(sp)
  add   sp, sp, 2*(__SIZEOF_POINTER__)
  ret

//
// This routine returns supervisor mode interrupt
// status.
//
ASM_PFX(RiscVGetSupervisorModeInterrupts):
  csrr a0, CSR_SSTATUS
  andi a0, a0, SSTATUS_SIE
  ret

