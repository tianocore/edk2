//------------------------------------------------------------------------------
//
// Copyright (c) 2011-2014, ARM Limited. All rights reserved.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------



    INCLUDE AsmMacroExport.inc

;
;VOID
;EFIAPI
;__aeabi_memmove (
; IN  VOID          *Destination,
; IN  CONST VOID    *Source,
; IN  UINT32        Size
; );
;
 RVCT_ASM_EXPORT __aeabi_memmove
  CMP     r2, #0
  BXEQ    lr
  CMP     r0, r1
  BXEQ    lr
  BHI     memmove_backward

memmove_forward
  LDRB    r3, [r1], #1
  STRB    r3, [r0], #1
  SUBS    r2, r2, #1
  BNE     memmove_forward
  BX      lr

memmove_backward
  add     r0, r2
  add     r1, r2
memmove_backward_loop
  LDRB    r3, [r1, #-1]!
  STRB    r3, [r0, #-1]!
  SUBS    r2, r2, #1
  BNE     memmove_backward_loop
  BX      lr

  END
