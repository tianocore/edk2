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
    ; ESP will be used as initial value of the EAX register
    ; in Main.asm
    ;
    xor     esp, esp

    ;
    ; Store "Start of day" struct pointer for later use
    ;
    mov     dword[PVH_SPACE (0)], ebx
    mov     dword[PVH_SPACE (4)], 'XPVH'

    mov     ebx, ADDR_OF(gdtr)
    lgdt    [ebx]

    mov     eax, SEC_DEFAULT_CR0
    mov     cr0, eax

    jmp     LINEAR_CODE_SEL:ADDR_OF(.jmpToNewCodeSeg)
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
    ; Jump to the main routine of the pre-SEC code
    ; skiping the 16-bit part of the routine and
    ; into the 32-bit flat mode part
    ;
    OneTimeCallRet TransitionFromReal16To32BitFlat
