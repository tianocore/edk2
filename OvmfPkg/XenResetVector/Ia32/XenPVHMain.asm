;------------------------------------------------------------------------------
; @file
; An entry point use by Xen when a guest is started in PVH mode.
;
; Copyright (c) 2019, Citrix Systems, Inc.
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    32

xenPVHMain:
    ;
    ; 'BP' to indicate boot-strap processor
    ;
    mov     di, 'BP'

    ;
    ; Store "Start of day" struct pointer for later use
    ;
    mov     dword[PVH_SPACE (0)], ebx
    mov     dword[PVH_SPACE (4)], 'XPVH'

    ;
    ; calculate delta between build-addr and run position
    ;
    mov     esp, PVH_SPACE(16)          ; create a temporary stack
    call    .delta
.delta:
    pop     edx                         ; get addr of .delta
    sub     edx, ADDR_OF(.delta)        ; calculate delta

    ;
    ; Find address of GDT and gdtr and fix the later
    ;
    mov     ebx, ADDR_OF(gdtr)
    add     ebx, edx                    ; add delta gdtr
    mov     eax, ADDR_OF(GDT_BASE)
    add     eax, edx                    ; add delta to GDT_BASE
    mov     dword[ebx + 2], eax         ; fix GDT_BASE addr in gdtr
    lgdt    [ebx]

    mov     eax, SEC_DEFAULT_CR0
    mov     cr0, eax

    ;
    ; push return addr to the stack, then return far
    ;
    push    dword LINEAR_CODE_SEL          ; segment to select
    mov     eax, ADDR_OF(.jmpToNewCodeSeg) ; return addr
    add     eax, edx                       ; add delta to return addr
    push    eax
    retf
.jmpToNewCodeSeg:

    mov     eax, SEC_DEFAULT_CR4
    mov     cr4, eax

    mov     ax, LINEAR_SEL
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ;
    ; ESP will be used as initial value of the EAX register
    ; in Main.asm
    ;
    xor     esp, esp

    ;
    ; parameter for Flat32SearchForBfvBase
    ;
    mov     eax, ADDR_OF(fourGigabytes)
    add     eax, edx ; add delta

    ;
    ; Jump to the main routine of the pre-SEC code
    ; skiping the 16-bit part of the routine and
    ; into the 32-bit flat mode part
    ;
    OneTimeCallRet TransitionFromReal16To32BitFlat
