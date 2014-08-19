#------------------------------------------------------------------------------
#
# CopyMem() worker for ARM
#
# This file started out as C code that did 64 bit moves if the buffer was
# 32-bit aligned, else it does a byte copy. It also does a byte copy for
# any trailing bytes. Update using VSTM/SLDM to do 128 byte copies.
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

/**
  Copy Length bytes from Source to Destination. Overlap is OK.

  This implementation

  @param  Destination Target of copy
  @param  Source      Place to copy from
  @param  Length      Number of bytes to copy

  @return Destination


VOID *
EFIAPI
InternalMemCopyMem (
  OUT     VOID                      *DestinationBuffer,
  IN      CONST VOID                *SourceBuffer,
  IN      UINTN                     Length
  )
**/
.text
.align 2
GCC_ASM_EXPORT(InternalMemCopyMem)

ASM_PFX(InternalMemCopyMem):
  stmfd  sp!, {r4, r9, lr}
  tst  r0, #3
  mov  r4, r0
  mov  r9, r0
  mov  ip, r2
  mov  lr, r1
  movne  r0, #0
  bne  L4
  tst  r1, #3
  movne  r3, #0
  moveq  r3, #1
  cmp  r2, #127
  movls  r0, #0
  andhi  r0, r3, #1
L4:
  cmp  r4, r1
  bcc  L26
  bls  L7
  rsb  r3, r1, r4
  cmp  ip, r3
  bcc  L26
  cmp  ip, #0
  beq  L7
  add  r9, r4, ip
  add  lr, ip, r1
  b  L16
L29:
  sub  ip, ip, #8
  cmp  ip, #7
  ldrd  r2, [lr, #-8]!
  movls  r0, #0
  cmp  ip, #0
  strd  r2, [r9, #-8]!
  beq  L7
L16:
  cmp  r0, #0
  bne  L29
  sub  r3, lr, #1
  sub  ip, ip, #1
  ldrb  r3, [r3, #0]
  sub  r2, r9, #1
  cmp  ip, #0
  sub  r9, r9, #1
  sub  lr, lr, #1
  strb  r3, [r2, #0]
  bne  L16
  b   L7
L11:
  ldrb  r3, [lr], #1
  sub  ip, ip, #1
  strb  r3, [r9], #1
L26:
  cmp  ip, #0
  beq  L7
L30:
  cmp  r0, #0
  beq  L11
  sub  ip, ip, #128          // 32
  cmp  ip, #127              // 31
  vldm     lr!, {d0-d15}
  movls  r0, #0
  cmp  ip, #0
  vstm  r9!, {d0-d15}
  bne  L30
L7:
  dsb
  mov  r0, r4
  ldmfd  sp!, {r4, r9, pc}


