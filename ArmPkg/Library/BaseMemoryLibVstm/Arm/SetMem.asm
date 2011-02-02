;------------------------------------------------------------------------------ 
;
; SetMem() worker for ARM
;
; This file started out as C code that did 64 bit moves if the buffer was
; 32-bit aligned, else it does a byte copy. It also does a byte copy for
; any trailing bytes. Update to use VSTM/VLDM to do 128 byte writes.
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
\s\s
\s\sEXPORT InternalMemSetMem
\s\s
\s\sAREA AsmMemStuff, CODE, READONLY

InternalMemSetMem
\s\sstmfd\s\ssp!, {lr}
\s\stst\s\s  r0, #3
\s\smovne\s\sr3, #0
\s\smoveq\s\sr3, #1
\s\scmp\s\s  r1, #127
\s\smovls lr, #0
\s\sandhi\s\slr, r3, #1
\s\scmp\s\s  lr, #0
\s\smov\s\s  r12, r0
\s\sbne\s\s  L31
L32
\s\smov\s\s  r3, #0
\s\sb\s\s    L43
L31
  vdup.8  q0,r2
  vmov    q1,q0
  vmov    q2,q0
  vmov    q3,q0
  vmov    q4,q0
  vmov    q5,q0
  vmov    q6,q0
  vmov    q7,q0
\s\sb\s\s      L32
L34
\s\scmp\s\s    lr, #0
\s\sstreqb\s\sr2, [r12], #1
\s\ssubeq\s\s  r1, r1, #1
\s\sbeq\s\s    L43
\s\ssub\s\s    r1, r1, #128
\s\scmp\s\s    r1, #127
\s\smovls\s\s  lr, r3
\s\svstm    r12!, {d0-d15}
L43
\s\scmp\s\s    r1, #0
\s\sbne\s\s    L34
\s\sldmfd\s\s  sp!, {pc}
\s\s
  END
  