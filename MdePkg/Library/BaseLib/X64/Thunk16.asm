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

m16Size         DW      InternalAsmThunk16 - m16Start
mThunk16Attr    DW      _ThunkAttr - m16Start
m16Gdt          DW      _NullSeg - m16Start
m16GdtrBase     DW      _16GdtrBase - m16Start
mTransition     DW      _EntryPoint - m16Start

    .code

m16Start    LABEL   BYTE

SavedGdt    LABEL   FWORD
            DW      ?
            DQ      ?

;------------------------------------------------------------------------------
; _BackFromUserCode() takes control in real mode after 'retf' has been executed
; by user code. It will be shadowed to somewhere in memory below 1MB.
;------------------------------------------------------------------------------
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
    mov     ax, cs
    shl     ax, 4
    lea     ax, [eax + ebx + (@64BitCode - @Base)]
    DB      2eh                         ; cs:
    mov     [rdi + (@64Eip - @Base)], ax
    DB      66h, 0b8h                   ; mov eax, imm32
SavedCr4    DD      ?
    mov     cr4, rax
    DB      66h, 2eh
    lgdt    fword ptr [rdi + (SavedGdt - @Base)]
    DB      66h
    mov     ecx, 0c0000080h
    rdmsr
    or      ah, 1
    wrmsr
    DB      66h, 0b8h                   ; mov eax, imm32
SavedCr0    DD      ?
    mov     cr0, rax
    DB      66h, 0eah                   ; jmp far cs:@64Bit
@64Eip      DD      ?
SavedCs     DW      ?
@64BitCode:
    DB      48h, 0b8h                   ; mov rax, imm64
SavedRip    DQ      ?
    jmp     rax                         ; return to caller
_BackFromUserCode   ENDP

_EntryPoint DD      _ToUserCode - m16Start
            DW      CODE16
_16Gdtr     LABEL   FWORD
            DW      GDT_SIZE - 1
_16GdtrBase DQ      _NullSeg
_16Idtr     FWORD   (1 SHL 10) - 1

;------------------------------------------------------------------------------
; _ToUserCode() takes control in real mode before passing control to user code.
; It will be shadowed to somewhere in memory below 1MB.
;------------------------------------------------------------------------------
_ToUserCode PROC
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
    mov     sp, bx                      ; set up 16-bit stack pointer
    DB      66h
    call    @Base                       ; push eip
@Base:
    pop     bp                          ; ebp <- address of @Base
    push    [esp + sizeof (IA32_REGS) + 2]
    lea     eax, [rsi + (@RealMode - @Base)]
    push    rax
    retf
@RealMode:
    DB      66h, 2eh                    ; CS and operand size override
    lidt    fword ptr [rsi + (_16Idtr - @Base)]
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

CODE16  = _16Code - $
DATA16  = _16Data - $
DATA32  = _32Data - $

_NullSeg    DQ      0
_16Code     LABEL   QWORD
            DW      -1
            DW      0
            DB      0
            DB      9bh
            DB      8fh                 ; 16-bit segment, 4GB limit
            DB      0
_16Data     LABEL   QWORD
            DW      -1
            DW      0
            DB      0
            DB      93h
            DB      8fh                 ; 16-bit segment, 4GB limit
            DB      0
_32Data     LABEL   QWORD
            DW      -1
            DW      0
            DB      0
            DB      93h
            DB      0cfh                ; 16-bit segment, 4GB limit
            DB      0

GDT_SIZE = $ - _NullSeg

;------------------------------------------------------------------------------
; IA32_REGISTER_SET *
; EFIAPI
; InternalAsmThunk16 (
;   IN      IA32_REGISTER_SET         *RegisterSet,
;   IN OUT  VOID                      *Transition
;   );
;------------------------------------------------------------------------------
InternalAsmThunk16  PROC    USES    rbp rbx rsi rdi
    mov     r10d, ds
    mov     r11d, es
    mov     r9d, ss
    push    fs
    push    gs
    mov     rsi, rcx
    movzx   r8d, (IA32_REGS ptr [rsi])._SS
    mov     edi, (IA32_REGS ptr [rsi])._ESP
    lea     rdi, [edi - (sizeof (IA32_REGS) + 4)]
    imul    eax, r8d, 16                ; eax <- r8d(stack segment) * 16
    mov     ebx, edi                    ; ebx <- stack for 16-bit code
    push    sizeof (IA32_REGS) / 4
    add     edi, eax                    ; edi <- linear address of 16-bit stack
    pop     rcx
    rep     movsd                       ; copy RegSet
    lea     ecx, [rdx + (SavedCr4 - m16Start)]
    mov     eax, edx                    ; eax <- transition code address
    and     edx, 0fh
    shl     eax, 12
    lea     ax, [rdx + (_BackFromUserCode - m16Start)]
    stosd                               ; [edi] <- return address of user code
    sgdt    fword ptr [rcx + (SavedGdt - SavedCr4)]
    sidt    fword ptr [rsp + 38h]       ; save IDT stack in argument space
    mov     rax, cr0
    mov     [rcx + (SavedCr0 - SavedCr4)], eax
    and     eax, 7ffffffeh              ; clear PE, PG bits
    mov     rbp, cr4
    mov     [rcx], ebp                  ; save CR4 in SavedCr4
    and     ebp, 300h                   ; clear all but PCE and OSFXSR bits
    mov     esi, r8d                    ; esi <- 16-bit stack segment
    DB      6ah, DATA32                 ; push DATA32
    pop     rdx                         ; rdx <- 32-bit data segment selector
    lgdt    fword ptr [rcx + (_16Gdtr - SavedCr4)]
    mov     ss, edx
    pushfq
    lea     edx, [rdx + DATA16 - DATA32]
    lea     r8, @RetFromRealMode
    mov     [rcx + (SavedRip - SavedCr4)], r8
    mov     r8d, cs
    mov     [rcx + (SavedCs - SavedCr4)], r8w
    mov     r8, rsp
    jmp     fword ptr [rcx + (_EntryPoint - SavedCr4)]
@RetFromRealMode:
    mov     rsp, r8
    popfq
    lidt    fword ptr [rsp + 38h]       ; restore protected mode IDTR
    lea     eax, [rbp - sizeof (IA32_REGS)]
    pop     gs
    pop     fs
    mov     ss, r9d
    mov     es, r11d
    mov     ds, r10d
    ret
InternalAsmThunk16  ENDP

    END
