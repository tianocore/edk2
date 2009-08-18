#------------------------------------------------------------------------------ 
#
# CpuSleep() for ARM
#
# Copyright (c) 2006 - 2009, Intel Corporation<BR>
# Portions copyright (c) 2008-2009 Apple Inc.<BR>
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

.text
.p2align 2
.globl ASM_PFX(CpuSleep)

#/**
#  Places the CPU in a sleep state until an interrupt is received.
#
#  Places the CPU in a sleep state until an interrupt is received. If interrupts
#  are disabled prior to calling this function, then the CPU will be placed in a
#  sleep state indefinitely.
#
#**/
#VOID
#EFIAPI
#CpuSleep (
#  VOID
#  );
#
ASM_PFX(CpuSleep):
    mov r0,#0
    mcr p15,0,r0,c7,c0,4   ;Wait for Interrupt instruction
    bx  lr
