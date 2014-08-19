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


    EXPORT  __aeabi_llsl

    AREA    Math, CODE, READONLY

;
;VOID
;EFIAPI
;__aeabi_llsl (
; IN  VOID    *Destination,
; IN  VOID    *Source,
; IN  UINT32  Size
; );
;

__aeabi_llsl
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

