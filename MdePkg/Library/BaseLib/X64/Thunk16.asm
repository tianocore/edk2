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

EXTERNDEF   m16Start:BYTE
EXTERNDEF   m16Size:WORD
EXTERNDEF   mThunk16Attr:WORD
EXTERNDEF   m16Gdt:WORD
EXTERNDEF   m16GdtrBase:WORD
EXTERNDEF   mTransition:WORD

THUNK_ATTRIBUTE_BIG_REAL_MODE               EQU 1
THUNK_ATTRIBUTE_DISABLE_A20_MASK_INT_15     EQU 2
THUNK_ATTRIBUTE_DISABLE_A20_MASK_KBD_CTRL   EQU 4

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
_EFLAGS     DQ      ?
_EIP        DD      ?
_CS         DW      ?
_SS         DW      ?
IA32_REGS   ENDS

    .const

m16Size         DW      offset InternalAsmThunk16 - offset m16Start
mThunk16Attr    DW      offset _ThunkAttr - offset m16Start
m16Gdt          DW      offset _NullSegDesc - offset m16Start
m16GdtrBase     DW      offset _16GdtrBase - offset m16Start
mTransition     DW      offset _EntryPoint - offset m16Start

    .code

m16Start    LABEL   BYTE

SavedGdt        LABEL   FWORD
                DW      ?
                DQ      ?

_BackFromUserCode   PROC
    DB      16h                         ; push ss
    DB      0eh                         ; push cs
    DB      66h
    call    @Base                       ; push eip
@Base:
    DB      66h
    push    0                           ; reserved high order 32 bits of EFlags
    pushf                               ; pushfd actually
    cli                                 ; disable interrupts
    push    gs
    push    fs
    DB      6                           ; push es
    DB      1eh                         ; push ds
    DB      66h, 60h                    ; pushad
    DB      66h, 0bah                   ; mov edx, imm32
_ThunkAttr  DD      ?
    test    dl, THUNK_ATTRIBUTE_DISABLE_A20_MASK_INT_15
    jz      @1
    mov     eax, 15cd2401h              ; mov ax, 2401h & int 15h
    cli                                 ; disable interrupts
    jnc     @2
@1:
    test    dl, THUNK_ATTRIBUTE_DISABLE_A20_MASK_KBD_CTRL
    jz      @2
    in      al, 92h
    or      al, 2
    out     92h, al                     ; deactivate A20M#
@2:
    mov     eax, ss
    lea     bp, [esp + sizeof (IA32_REGS)]
    mov     word ptr (IA32_REGS ptr [rsi - sizeof (IA32_REGS)])._ESP, bp
    mov     ebx, (IA32_REGS ptr [rsi - sizeof (IA32_REGS)])._EIP
    shl     ax, 4                       ; shl eax, 4
    add     bp, ax                      ; add ebp, eax
    DB      66h, 0b8h                   ; mov eax, imm32
SavedCr4    DD      ?
    mov     cr4, rax
    DB      66h, 2eh
    lgdt    fword ptr [rdi + (offset SavedGdt - offset @Base)]
    DB      66h
    mov     ecx, 0c0000080h
    rdmsr
    or      ah, 1
    wrmsr
    DB      66h, 0b8h                   ; mov eax, imm32
SavedCr0    DD      ?
    mov     cr0, rax
    DB      0b8h                        ; mov ax, imm16
SavedSs     DW      ?
    mov     ss, eax
    DB      66h, 0bch                   ; mov esp, imm32
SavedEsp    DD      ?
    DB      66h
    retf                                ; return to protected mode
_BackFromUserCode   ENDP

_EntryPoint     DD      offset _ToUserCode - offset m16Start
                DW      8h
_16Gdtr         LABEL   FWORD
                DW      offset GdtEnd - offset _NullSegDesc - 1
_16GdtrBase     DQ      offset _NullSegDesc
_16Idtr         FWORD   (1 SHL 10) - 1

_ToUserCode PROC
    mov     edi, ss
    mov     ss, edx                     ; set new segment selectors
    mov     ds, edx
    mov     es, edx
    mov     fs, edx
    mov     gs, edx
    DB      66h
    mov     ecx, 0c0000080h
    mov     cr0, rax                    ; real mode starts at next instruction
    rdmsr
    and     ah, NOT 1
    wrmsr
    mov     cr4, rbp
    mov     ss, esi                     ; set up 16-bit stack segment
    xchg    sp, bx                      ; set up 16-bit stack pointer
    DB      66h
    call    @Base                       ; push eip
@Base:
    pop     bp                          ; ebp <- offset @Base
    DB      2eh                         ; cs:
    mov     [rsi + (offset SavedSs - offset @Base)], edi
    DB      2eh                         ; cs:
    mov     [rsi + (offset SavedEsp - offset @Base)], bx
    DB      66h, 2eh                    ; CS and operand size override
    lidt    fword ptr [rsi + (offset _16Idtr - offset @Base)]
    DB      66h, 61h                    ; popad
    DB      1fh                         ; pop ds
    DB      07h                         ; pop es
    pop     fs
    pop     gs
    popf                                ; popfd
    lea     sp, [esp + 4]               ; skip high order 32 bits of EFlags
    DB      66h
    retf                                ; transfer control to user code
_ToUserCode ENDP

_NullSegDesc    DQ      0
_16CsDesc       LABEL   QWORD
                DW      -1
                DW      0
                DB      0
                DB      9bh
                DB      8fh             ; 16-bit segment, 4GB limit
                DB      0
_16DsDesc       LABEL   QWORD
                DW      -1
                DW      0
                DB      0
                DB      93h
                DB      8fh             ; 16-bit segment, 4GB limit
                DB      0
GdtEnd          LABEL   QWORD

;
;   @param  RegSet  Pointer to a IA32_DWORD_REGS structure
;   @param  Transition  Pointer to the transition code
;   @return The address of the 16-bit stack after returning from user code
;
InternalAsmThunk16  PROC    USES    rbp rbx rsi rdi
    mov     r10d, ds
    mov     r11d, es
    push    fs
    push    gs
    mov     rsi, rcx
    movzx   r8d, (IA32_REGS ptr [rsi])._SS
    mov     edi, (IA32_REGS ptr [rsi])._ESP
    lea     rdi, [edi - (sizeof (IA32_REGS) + 4)]
    imul    eax, r8d, 16                ; eax <- r8d(stack segment) * 16
    mov     ebx, edi                    ; ebx <- stack offset for 16-bit code
    push    sizeof (IA32_REGS) / 4
    add     edi, eax                    ; edi <- linear address of 16-bit stack
    pop     rcx
    rep     movsd                       ; copy RegSet
    lea     ecx, [rdx + (offset SavedCr4 - offset m16Start)]
    mov     eax, edx                    ; eax <- transition code address
    and     edx, 0fh
    shl     eax, 12
    lea     edx, [rdx + (offset _BackFromUserCode - offset m16Start)]
    mov     ax, dx
    stosd                               ; [edi] <- return address of user code
    sgdt    fword ptr [rcx + (offset SavedGdt - offset SavedCr4)]
    sidt    fword ptr [rsp + 38h]       ; save IDT stack in argument space
    mov     rax, cr0
    mov     [rcx + (offset SavedCr0 - offset SavedCr4)], eax
    and     eax, 7ffffffeh              ; clear PE, PG bits
    mov     rbp, cr4
    mov     [rcx], ebp                  ; save CR4 in SavedCr4
    and     ebp, 300h                   ; clear all but PCE and OSFXSR bits
    mov     esi, r8d                    ; esi <- 16-bit stack segment
    push    10h
    pop     rdx                         ; rdx <- selector for data segments
    lgdt    fword ptr [rcx + (offset _16Gdtr - offset SavedCr4)]
    call    fword ptr [rcx + (offset _EntryPoint - offset SavedCr4)]
    lidt    fword ptr [rsp + 38h]       ; restore protected mode IDTR
    lea     eax, [rbp - sizeof (IA32_REGS)]
    pop     gs
    pop     fs
    mov     es, r11d
    mov     ds, r10d
    ret
InternalAsmThunk16  ENDP

    END
