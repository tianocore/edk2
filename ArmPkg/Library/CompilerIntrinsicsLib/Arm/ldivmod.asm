//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008-2009 Apple Inc. All rights reserved.
//
// All rights reserved. This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------


    EXPORT  __aeabi_ldivmod
    EXTERN  __aeabi_uldivmod
           
    AREA    Math, CODE, READONLY

;
;UINT32
;EFIAPI
;__aeabi_uidivmode (
;  IN UINT32  Dividen
;  IN UINT32  Divisor
;  );
;

__aeabi_ldivmod
    PUSH     {r4,lr}
    ASRS     r4,r1,#1
    EOR      r4,r4,r3,LSR #1
    BPL      {pc} + 0xc  ; 0x18
    RSBS     r0,r0,#0
    RSC      r1,r1,#0
    TST      r3,r3
    BPL      {pc} + 0xc  ; 0x28
    RSBS     r2,r2,#0
    RSC      r3,r3,#0
    BL       __aeabi_uldivmod  ;
    TST      r4,#0x40000000
    BEQ      {pc} + 0xc  ; 0x3c
    RSBS     r0,r0,#0
    RSC      r1,r1,#0
    TST      r4,#0x80000000
    BEQ      {pc} + 0xc  ; 0x4c
    RSBS     r2,r2,#0
    RSC      r3,r3,#0
    POP      {r4,pc}
 
    END


