;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   Thunk.asm
;
; Abstract:
;
;   Real mode thunk
;
;------------------------------------------------------------------------------

    .686p
    .model  flat,C

    .data

NullSegSel      DQ      0
_16BitCsSel     LABEL   QWORD
                DW      -1
                DW      0
                DB      0
                DB      9bh
                DB      8fh             ; 16-bit segment
                DB      0
_16BitDsSel     LABEL   QWORD
                DW      -1
                DW      0
                DB      0
                DB      93h
                DB      8fh             ; 16-bit segment
                DB      0
GdtEnd          LABEL   QWORD

    .const

_16Gdtr         LABEL   FWORD
                DW      offset GdtEnd - offset NullSegSel - 1
                DD      offset NullSegSel

_16Idtr         FWORD   (1 SHL 10) - 1

    .code

IA32_REGS   STRUC   4t
_EDI        DD      ?
_ESI        DD      ?
_EBP        DD      ?
_ESP        DD      ?
_EBX        DD      ?
_EDX        DD      ?
_ECX        DD      ?
_EAX        DD      ?
_DS         DW      ?
_ES         DW      ?
_FS         DW      ?
_GS         DW      ?
_EFLAGS     DD      ?
_EIP        DD      ?
_CS         DW      ?
_SS         DW      ?
IA32_REGS   ENDS

InternalAsmThunk16  PROC    USES    ebp ebx esi edi ds  es  fs  gs
    mov     esi, [esp + 36]             ; esi <- RegSet
    push    sizeof (IA32_REGS)
    pop     ecx
    movzx   edx, (IA32_REGS ptr [esi])._SS
    mov     edi, (IA32_REGS ptr [esi])._ESP
    sub     edi, ecx                    ; reserve space on realmode stack
    push    edi                         ; save stack offset
    imul    eax, edx, 16                ; eax <- edx * 16
    add     edi, eax                    ; edi <- linear address of 16-bit stack
    rep     movsb                       ; copy RegSet
    mov     esi, edx                    ; esi <- 16-bit stack segment
    pop     ebx                         ; ebx <- 16-bit stack offset
    mov     edi, [esp + 40]             ; edi <- realmode patch
    push    cs                          ; save CS segment selector
    push    offset @BackToThunk         ; offset to back from real mode
    mov     eax, offset @16Return
    stosd
    xor     eax, eax
    stosw                               ; set CS base to 0
    mov     eax, esp
    stosd
    mov     eax, ss
    stosd
    mov     eax, cr0
    mov     ecx, eax                    ; ecx <- CR0
    and     ecx, 7ffffffeh              ; clear PE, PG bits
    stosd
    mov     eax, cr4
    mov     ebp, eax
    and     ebp, 300h                   ; clear all but PCE and OSFXSR bits
    stosd
    sidt    fword ptr [esp + 44]        ; use parameter space to save IDTR
    sgdt    fword ptr [edi]
    lidt    _16Idtr
    push    10h
    pop     eax
    push    8
    push    offset @16Start
    lgdt    _16Gdtr
    retf
@16Start:                               ; 16-bit starts here
    mov     ss, eax                     ; set SS to be a 16-bit segment
    mov     cr0, ecx
    mov     cr4, ebp
    mov     ss, esi                     ; set up 16-bit stack
    mov     sp, bx                      ; mov esp, ebx actually
    popaw                               ; popad actually
    pop     ds
    pop     es
    pop     fs
    pop     gs
    add     sp, 4                       ; skip _EFLAGS
    DB      66h
    retf                                ; transfer control to 16-bit code
@16Return:
    pushf                               ; pushfd actually
    push    gs
    push    fs
    push    es
    push    ds
    pushaw                              ; pushad actually
    DB      67h, 66h
    lds     esi, fword ptr (IA32_REGS ptr [esp])._EIP
    DB      67h, 66h
    mov     eax, [esi + 12]
    mov     cr4, eax                    ; restore CR4
    DB      67h, 66h
    lgdt    fword ptr [esi + 16]
    DB      67h, 66h
    mov     eax, [esi + 8]
    mov     cr0, eax                    ; restore CR0
    xor     ax, ax                      ; xor eax, eax actually
    mov     eax, ss
    DB      67h
    mov     dword ptr (IA32_REGS ptr [esp])._SS, eax
    shl     ax, 4                       ; shl eax, 4 actually
    add     ax, sp                      ; add eax, esp actually
    add     sp, sizeof (IA32_REGS)      ; add esp, sizeof (IA32_REGS)
    DB      67h, 66h
    mov     dword ptr (IA32_REGS ptr [esp - sizeof (IA32_REGS)])._ESP, esp
    DB      67h, 66h
    lss     esp, fword ptr [esi]        ; restore protected mode stack
    DB      66h
    retf                                ; go back to protected mode
@BackToThunk:
    lidt    fword ptr [esp + 36]        ; restore protected mode IDTR
    ret
InternalAsmThunk16  ENDP

    END
