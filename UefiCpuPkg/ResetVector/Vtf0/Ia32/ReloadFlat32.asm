;------------------------------------------------------------------------------
; @file
;   Load the GDT and set the CR0, then jump to Flat 32 protected mode.
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

%define SEC_DEFAULT_CR0  0x00000023
%define SEC_DEFAULT_CR4  0x640

BITS    32

;
; Modified:  EAX, EBX, CR0, CR4, DS, ES, FS, GS, SS
;
ReloadFlat32:

    cli
    mov     ebx, ADDR_OF(gdtr)
    lgdt    [ebx]

    mov     eax, SEC_DEFAULT_CR0
    mov     cr0, eax

    jmp     LINEAR_CODE_SEL:dword ADDR_OF(jumpToFlat32BitAndLandHere)
BITS    32
jumpToFlat32BitAndLandHere:

    mov     eax, SEC_DEFAULT_CR4
    mov     cr4, eax

    debugShowPostCode POSTCODE_32BIT_MODE

    mov     ax, LINEAR_SEL
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    OneTimeCallRet ReloadFlat32

