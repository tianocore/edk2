;------------------------------------------------------------------------------
; @file
; Transition from 32 bit flat protected mode into 64 bit flat protected mode
;
; Copyright (c) 2008 - 2009, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

BITS    32

;
; Modified:  EAX
;
Transition32FlatTo64Flat:

    mov     eax, ((ADDR_OF_START_OF_RESET_CODE & ~0xfff) - 0x1000)
    mov     cr3, eax

    mov     eax, cr4
    bts     eax, 5                      ; enable PAE
    mov     cr4, eax                    

    mov     ecx, 0xc0000080
    rdmsr
    bts     eax, 8                      ; set LME
    wrmsr

    mov     eax, cr0
    bts     eax, 31                     ; set PG
    mov     cr0, eax                    ; enable paging

    jmp     LINEAR_CODE64_SEL:ADDR_OF(jumpTo64BitAndLandHere)
BITS    64
jumpTo64BitAndLandHere:

    debugShowPostCode POSTCODE_64BIT_MODE

    OneTimeCallRet Transition32FlatTo64Flat

