#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011 - 2016, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroLib.h>

ASM_FUNC(ArmArchLibGetPhysicalAddressBits)
  mrs   x0, id_aa64mmfr0_el1
  adr   x1, .LPARanges
  and   x0, x0, #0xf
  ldrb  w0, [x1, x0]
  ret

//
// Bits 0..3 of the AA64MFR0_EL1 system register encode the size of the
// physical address space support on this CPU:
// 0 == 32 bits, 1 == 36 bits, etc etc
// 7 and up are reserved
//
.LPARanges:
  .byte 32, 36, 40, 42, 44, 48, 52,  0
  .byte  0,  0,  0,  0,  0,  0,  0,  0

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
