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

EXTERNDEF   C   m16Start:BYTE
EXTERNDEF   C   m16Size:WORD
EXTERNDEF   C   mThunk16Attr:WORD
EXTERNDEF   C   m16Gdt:WORD
EXTERNDEF   C   m16GdtrBase:WORD
EXTERNDEF   C   mTransition:WORD

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
_EFLAGS     DD      ?
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
                DD      ?

_BackFromUserCode   PROC
    push    ss
    push    cs
    DB      66h
    call    @Base                       ; push eip
@Base:
    pushf                               ; pushfd actually
    cli                                 ; disable interrupts
    push    gs
    push    fs
    push    es
    push    ds
    pushaw                              ; pushad actually
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
    DB      67h
    lea     bp, [esp + sizeof (IA32_REGS)]
    mov     word ptr (IA32_REGS ptr [esi - sizeof (IA32_REGS)])._ESP, bp
    mov     ebx, (IA32_REGS ptr [esi - sizeof (IA32_REGS)])._EIP
    shl     ax, 4                       ; shl eax, 4
    add     bp, ax                      ; add ebp, eax
    DB      66h, 0b8h                   ; mov eax, imm32
SavedCr4    DD      ?
    mov     cr4, eax
    DB      66h
    lgdt    fword ptr cs:[edi + (offset SavedGdt - offset @Base)]
    DB      66h, 0b8h                   ; mov eax, imm32
SavedCr0    DD      ?
    mov     cr0, eax
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
_16Idtr         FWORD   (1 SHL 10) - 1
_16Gdtr         LABEL   FWORD
                DW      offset GdtEnd - offset _NullSegDesc - 1
_16GdtrBase     DD      offset _NullSegDesc

_ToUserCode PROC
    mov     edx, ss
    mov     ss, ecx                     ; set new segment selectors
    mov     ds, ecx
    mov     es, ecx
    mov     fs, ecx
    mov     gs, ecx
    mov     cr0, eax
    mov     cr4, ebp                    ; real mode starts at next instruction
    mov     ss, esi                     ; set up 16-bit stack segment
    xchg    sp, bx                      ; set up 16-bit stack pointer
    DB      66h
    call    @Base                       ; push eip
@Base:
    pop     bp                          ; ebp <- offset @Base
    mov     cs:[esi + (offset SavedSs - offset @Base)], edx
    mov     cs:[esi + (offset SavedEsp - offset @Base)], bx
    DB      66h
    lidt    fword ptr cs:[esi + (offset _16Idtr - offset @Base)]
    popaw                               ; popad actually
    pop     ds
    pop     es
    pop     fs
    pop     gs
    popf                                ; popfd
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
InternalAsmThunk16  PROC    USES    ebp ebx esi edi ds  es  fs  gs
    mov     esi, [esp + 36]             ; esi <- RegSet
    movzx   edx, (IA32_REGS ptr [esi])._SS
    mov     edi, (IA32_REGS ptr [esi])._ESP
    add     edi, - (sizeof (IA32_REGS) + 4) ; reserve stack space
    mov     ebx, edi                    ; ebx <- stack offset
    imul    eax, edx, 16                ; eax <- edx * 16
    push    sizeof (IA32_REGS) / 4
    add     edi, eax                    ; edi <- linear address of 16-bit stack
    pop     ecx
    rep     movsd                       ; copy RegSet
    mov     eax, [esp + 40]             ; eax <- address of transition code
    mov     esi, edx                    ; esi <- 16-bit stack segment
    lea     edx, [eax + (offset SavedCr0 - offset m16Start)]
    mov     ecx, eax
    and     ecx, 0fh
    shl     eax, 12
    lea     ecx, [ecx + (offset _BackFromUserCode - offset m16Start)]
    mov     ax, cx
    stosd                               ; [edi] <- return address of user code
    sgdt    fword ptr [edx + (offset SavedGdt - offset SavedCr0)]
    sidt    fword ptr [esp + 36]        ; save IDT stack in argument space
    mov     eax, cr0
    mov     [edx], eax                  ; save CR0 in SavedCr0
    and     eax, 7ffffffeh              ; clear PE, PG bits
    mov     ebp, cr4
    mov     [edx + (offset SavedCr4 - offset SavedCr0)], ebp
    and     ebp, 300h                   ; clear all but PCE and OSFXSR bits
    push    10h
    pop     ecx                         ; ecx <- selector for data segments
    lgdt    fword ptr [edx + (offset _16Gdtr - offset SavedCr0)]
    call    fword ptr [edx + (offset _EntryPoint - offset SavedCr0)]
    lidt    fword ptr [esp + 36]        ; restore protected mode IDTR
    lea     eax, [ebp - sizeof (IA32_REGS)]
    ret
InternalAsmThunk16  ENDP

    END
