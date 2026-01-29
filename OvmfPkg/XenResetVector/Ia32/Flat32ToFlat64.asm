;------------------------------------------------------------------------------
; @file
; Transition from 32 bit flat protected mode into 64 bit flat protected mode
;
; Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
; Copyright (c) 2019, Citrix Systems, Inc.
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    32

;
; Modified:  EAX, EBX, ECX, EDX, ESP
;
Transition32FlatTo64Flat:

    OneTimeCall SetCr3ForPageTables64

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

    ;
    ; backup ESP
    ;
    mov     ebx, esp

    ;
    ; recalculate delta
    ;
    mov     esp, PVH_SPACE(16)
    call    .delta
.delta:
    pop     edx
    sub     edx, ADDR_OF(.delta)

    ;
    ; push return addr and seg to the stack, then return far
    ;
    push    dword LINEAR_CODE64_SEL
    mov     eax, ADDR_OF(jumpTo64BitAndLandHere)
    add     eax, edx                             ; add delta
    push    eax
    retf

BITS    64
jumpTo64BitAndLandHere:

    ;
    ; restore ESP
    ;
    mov     esp, ebx

    debugShowPostCode POSTCODE_64BIT_MODE

    OneTimeCallRet Transition32FlatTo64Flat

