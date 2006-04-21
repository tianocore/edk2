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

    .data

NullSegSel      DQ      0
_16CsSegSel     LABEL   QWORD
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
                DQ      offset NullSegSel

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
_RFLAGS     DQ      ?
_EIP        DD      ?
_CS         DW      ?
_SS         DW      ?
IA32_REGS   ENDS

InternalAsmThunk16  PROC    USES    rbp rbx rsi rdi r12 r13 r14 r15
    mov     eax, ds
    push    rax
    mov     eax, es
    push    rax
    push    fs
    push    gs
    mov     rsi, rcx                    ; rsi <- RegSet
    push    sizeof (IA32_REGS)
    pop     rcx
    movzx   r8, (IA32_REGS ptr [rsi])._SS
    xor     rdi, rdi
    mov     edi, (IA32_REGS ptr [rsi])._ESP
    sub     rdi, rcx                    ; reserve space on realmode stack
    push    rdi                         ; save stack offset
    imul    rax, r8, 16
    add     rdi, rax                    ; rdi <- linear address of 16-bit stack
    rep     movsb                       ; copy RegSet
    mov     rsi, r8                     ; si <- 16-bit stack segment
    pop     rbx                         ; rbx <- 16-bit stack offset
    mov     rdi, rdx                    ; rdi <- realmode patch
    lea     eax, @BackToThunk           ; rax <- address to back from real mode
    push    rax                         ; use in a far return
    mov     eax, cs
    mov     [rsp + 4], eax              ; save CS
    lea     eax, @16Return              ; thus @Return must < 4GB
    stosd                               ; set ret address offset
    xor     eax, eax
    stosw                               ; set ret CS base to 0
    mov     eax, esp
    stosd                               ; rsp must < 4GB
    mov     eax, ss
    stosd
    mov     rax, cr0
    mov     ecx, eax                    ; ecx <- CR0
    and     ecx, 7ffffffeh              ; clear PE, PG bits
    stosd
    mov     rax, cr4
    mov     ebp, eax
    and     ebp, 300h                   ; clear all but PCE and OSFXSR bits
    stosd
    sidt    fword ptr [rsp + 70h]       ; use parameter space to save IDTR
    sgdt    fword ptr [rdi]
    lea     edi, _16Idtr
    lea     eax, @16Start               ; rax <- seg:offset of @16Start
    push    rax
    mov     dword ptr [rsp + 4], 8
    push    10h
    pop     rax                         ; rax <- 10h as dataseg selector
    lgdt    _16Gdtr
    retf
@16Start:                               ; 16-bit starts here
    mov     ss, eax                     ; set SS to be a 16-bit segment
    mov     cr0, rcx                    ; disable protected mode
    mov     cr4, rbp
    DB      66h
    mov     ecx, 0c0000080h
    rdmsr
    and     ah, NOT 1                   ; clear LME
    wrmsr
    mov     ss, esi                     ; set up 16-bit stack
    mov     sp, bx                      ; mov esp, ebx actually
    lidt    fword ptr [edi]
    DB      66h, 61h                    ; popad
    DB      1fh                         ; pop ds
    DB      7                           ; pop es
    pop     fs
    pop     gs
    add     sp, 8                       ; skip _RFLAGS
    DB      66h
    retf                                ; transfer control to 16-bit code
@16Return:
    DB      66h
    push    0                           ; high order 32 bits of rflags
    pushf                               ; pushfd actually
    push    gs
    push    fs
    DB      6                           ; push es
    DB      1eh                         ; push ds
    DB      66h, 60h                    ; pushad
    DB      67h, 66h, 0c5h, 74h, 24h, 30h   ; lds esi, [esp + 12*4]
    DB      66h
    mov     eax, [esi + 12]
    mov     cr4, rax                    ; restore CR4
    DB      66h
    lgdt    fword ptr [esi + 16]
    DB      66h
    mov     ecx, 0c0000080h
    rdmsr
    or      ah, 1                       ; set LME
    wrmsr
    DB      66h
    mov     eax, [esi + 8]
    mov     cr0, rax                    ; restore CR0
    xor     ax, ax                      ; xor eax, eax actually
    mov     eax, ss
    mov     dword ptr (IA32_REGS ptr [esp])._SS, eax
    shl     ax, 4                       ; shl eax, 4 actually
    add     ax, sp                      ; add eax, esp actually
    add     sp, sizeof (IA32_REGS)      ; add esp, sizeof (IA32_REGS)
    DB      66h
    mov     dword ptr (IA32_REGS ptr [esp - sizeof (IA32_REGS)])._ESP, esp
    DB      66h
    lss     esp, fword ptr [esi]        ; restore protected mode stack
    DB      66h
    retf                                ; go back to protected mode
@BackToThunk:
    lidt    fword ptr [rsp + 68h]       ; restore protected mode IDTR
    shl     rax, 32
    shr     rax, 32                     ; clear high order 32 bits of RAX
    pop     gs
    pop     fs
    pop     rcx
    mov     es, ecx
    pop     rcx
    mov     ds, ecx
    ret
InternalAsmThunk16  ENDP

    END
