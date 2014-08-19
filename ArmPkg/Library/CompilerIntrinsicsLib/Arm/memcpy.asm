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


    EXPORT  __aeabi_memcpy

    AREA    Memcpy, CODE, READONLY

;
;VOID
;EFIAPI
;__aeabi_memcpy (
; IN  VOID    *Destination,
; IN  VOID    *Source,
; IN  UINT32  Size
; );
;
__aeabi_memcpy
  cmp     r2, #0
  bxeq    lr
  push    {lr}
  mov     lr, r0
L5
  ldrb  r3, [r1], #1
  strb  r3, [lr], #1
  subs r2, r2, #1
  bne  L5
  pop  {pc}

  END
