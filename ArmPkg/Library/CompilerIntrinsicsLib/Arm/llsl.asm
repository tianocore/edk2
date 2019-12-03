//------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------



    INCLUDE AsmMacroExport.inc

;
;VOID
;EFIAPI
;__aeabi_llsl (
; IN  VOID    *Destination,
; IN  VOID    *Source,
; IN  UINT32  Size
; );
;

 RVCT_ASM_EXPORT __aeabi_llsl
    SUBS     r3,r2,#0x20
    BPL      {pc} + 0x18  ; 0x1c
    RSB      r3,r2,#0x20
    LSL      r1,r1,r2
    ORR      r1,r1,r0,LSR r3
    LSL      r0,r0,r2
    BX       lr
    LSL      r1,r0,r3
    MOV      r0,#0
    BX       lr

    END

