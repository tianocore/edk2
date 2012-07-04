//------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------


    EXPORT  __aeabi_memset
    EXPORT  __aeabi_memclr
    EXPORT  __aeabi_memclr4

    AREA    Memset, CODE, READONLY

;
;VOID
;EFIAPI
;__aeabi_memset (
; IN  VOID    *Destination,
; IN  UINT32  Character,
; IN  UINT32  Size
; );
;
__aeabi_memset

  ; args = 0, pretend = 0, frame = 0
  ; frame_needed = 1, uses_anonymous_args = 0
  stmfd  sp!, {r7, lr}
  mov  ip, #0
  add  r7, sp, #0
  mov  lr, r0
  b  L9
L10
  and  r3, r1, #255
  add  ip, ip, #1
  strb  r3, [lr], #1
L9
  cmp  ip, r2
  bne  L10
  ldmfd  sp!, {r7, pc}

__aeabi_memclr
  mov   r2, r1
  mov   r1, #0
  b     __aeabi_memset

__aeabi_memclr4
  mov   r2, r1
  mov   r1, #0
  b     __aeabi_memset

  END
