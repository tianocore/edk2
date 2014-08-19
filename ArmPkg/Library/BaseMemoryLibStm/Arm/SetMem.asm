;------------------------------------------------------------------------------
;
; SetMem() worker for ARM
;
; This file started out as C code that did 64 bit moves if the buffer was
; 32-bit aligned, else it does a byte copy. It also does a byte copy for
; any trailing bytes. It was updated to do 32-byte at a time.
;
; Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;

/**
  Set Buffer to Value for Size bytes.

  @param  Buffer   Memory to set.
  @param  Length   Number of bytes to set
  @param  Value    Value of the set operation.

  @return Buffer

VOID *
EFIAPI
InternalMemSetMem (
  OUT     VOID                      *Buffer,
  IN      UINTN                     Length,
  IN      UINT8                     Value
  )
**/

  EXPORT InternalMemSetMem

  AREA AsmMemStuff, CODE, READONLY

InternalMemSetMem
  stmfd  sp!, {r4-r11, lr}
  tst    r0, #3
  movne  r3, #0
  moveq  r3, #1
  cmp    r1, #31
  movls lr, #0
  andhi  lr, r3, #1
  cmp    lr, #0
  mov    r12, r0
  bne    L31
L32
  mov    r3, #0
  b      L43
L31
  and   r4, r2, #0xff
  orr   r4, r4, r4, LSL #8
  orr   r4, r4, r4, LSL #16
  mov   r5, r4
  mov   r6, r4
  mov   r7, r4
  mov   r8, r4
  mov   r9, r4
  mov   r10, r4
  mov   r11, r4
  b      L32
L34
  cmp      lr, #0
  streqb  r2, [r12], #1
  subeq    r1, r1, #1
  beq      L43
  sub      r1, r1, #32
  cmp      r1, #31
  movls    lr, r3
  stmia    r12!, {r4-r11}
L43
  cmp      r1, #0
  bne      L34
  ldmfd    sp!, {r4-r11, pc}

  END
