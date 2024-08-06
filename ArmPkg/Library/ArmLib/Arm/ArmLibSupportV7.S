#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011-2013, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroLib.h>

ASM_FUNC(ArmIsMpCore)
  mrc     p15,0,R0,c0,c0,5
  // Get Multiprocessing extension (bit31) & U bit (bit30)
  and     R0, R0, #0xC0000000
  // if (bit31 == 1) && (bit30 == 0) then the processor is part of a multiprocessor system
  cmp     R0, #0x80000000
  moveq   R0, #1
  movne   R0, #0
  bx      LR

ASM_FUNC(ArmEnableAsynchronousAbort)
  cpsie   a
  isb
  bx      LR

ASM_FUNC(ArmDisableAsynchronousAbort)
  cpsid   a
  isb
  bx      LR

ASM_FUNC(ArmEnableIrq)
  cpsie   i
  isb
  bx      LR

ASM_FUNC(ArmDisableIrq)
  cpsid   i
  isb
  bx      LR

ASM_FUNC(ArmEnableFiq)
  cpsie   f
  isb
  bx      LR

ASM_FUNC(ArmDisableFiq)
  cpsid   f
  isb
  bx      LR

ASM_FUNC(ArmEnableInterrupts)
  cpsie   if
  isb
  bx      LR

ASM_FUNC(ArmDisableInterrupts)
  cpsid   if
  isb
  bx      LR

ASM_FUNC(ArmReadIdMmfr4)
  mrc    p15,0,r0,c0,c2,6     @ Read ID_MMFR4 Register
  bx     lr

// UINTN
// ReadCCSIDR (
//   IN UINT32 CSSELR
//   )
ASM_FUNC(ReadCCSIDR)
  mcr p15,2,r0,c0,c0,0   @ Write Cache Size Selection Register (CSSELR)
  isb
  mrc p15,1,r0,c0,c0,0   @ Read current CP15 Cache Size ID Register (CCSIDR)
  bx  lr

// UINT32
// ReadCCSIDR2 (
//   IN UINT32 CSSELR
//   )
ASM_FUNC(ReadCCSIDR2)
  mcr p15,2,r0,c0,c0,0   @ Write Cache Size Selection Register (CSSELR)
  isb
  mrc p15,1,r0,c0,c0,2   @ Read current CP15 Cache Size ID Register (CCSIDR2)
  bx  lr

// UINT32
// ReadCLIDR (
//   IN UINT32 CSSELR
//   )
ASM_FUNC(ReadCLIDR)
  mrc p15,1,r0,c0,c0,1 @ Read CP15 Cache Level ID Register
  bx  lr

ASM_FUNC(ArmReadNsacr)
  mrc     p15, 0, r0, c1, c1, 2
  bx      lr

ASM_FUNC(ArmWriteNsacr)
  mcr     p15, 0, r0, c1, c1, 2
  bx      lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
