;*****************************************************************************
;*
;*   Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*   Module Name:
;*
;*    Thunk.asm
;*  
;*   Abstract:
;*  
;*    Real mode thunk
;*  
;*****************************************************************************

EXTERNDEF   mCode16Size:QWORD

    .const

mCode16Size     DQ      _Code16End - _Code16Addr

    .data

NullSegSel      DQ      0
_16CsSegSel     LABEL   QWORD
                DW      -1
                DW      0
                DB      0
                DB      9bh
                DB      8fh             ; 16-bit segment
                DB      0
_16DsSegSel     LABEL   QWORD
                DW      -1
                DW      0
                DB      0
                DB      93h
                DB      8fh             ; 16-bit segment
                DB      0

_16Gdtr         LABEL   FWORD
                DW      $ - offset NullSegSel - 1
                DQ      offset NullSegSel

    .code

STACK_PARAM_SIZE  EQU  16

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

_STK16      STRUC   1t
RetEip      DD      ?
RetCs       DW      ?
ThunkFlags  DW      ?
SavedGdtr   FWORD   ?
Resvd1      DW      ?
SavedCr0    DD      ?
SavedCr4    DD      ?
_STK16      ENDS

_Thunk16    PROC    USES    rbp rbx rsi rdi r12 r13 r14 r15

    push    fs
    push    gs

    mov     r12d, ds
    mov     r13d, es
    mov     r14d, ss
    mov     r15, rsp
    mov     rsi, rcx
    movzx   r10, (IA32_REGS ptr [rsi])._SS
    xor     rdi, rdi
    mov     edi, (IA32_REGS ptr [rsi])._ESP
    add     rdi, - sizeof (IA32_REGS) - sizeof (_STK16)
    push    rdi
    imul    rax, r10, 16
    add     rdi, rax
    push    sizeof (IA32_REGS) / 4
    pop     rcx
    rep     movsd

    ; copy eflags to stack frame
    mov     rax, (IA32_REGS ptr [rsi - sizeof(IA32_REGS)])._RFLAGS
    mov     [rdi - sizeof(IA32_REGS) - STACK_PARAM_SIZE - 8], rax

    pop     rbx                         ; rbx <- 16-bit stack offset
    lea     eax, @F                     ; return offset
    stosd
    mov     eax, cs                     ; return segment
    stosw
    mov     eax, edx                    ; THUNK Flags
    stosw
    sgdt    fword ptr [rsp + 58h]       ; save GDTR
    mov     rax, [rsp + 58h]
    stosq
    mov     rax, cr0                    ; save CR0
    mov     esi, eax                    ; esi <- CR0 to set
    stosd
    mov     rax, cr4                    ; save CR4
    stosd
    sidt    fword ptr [rsp + 58h]       ; save IDTR
    and     esi, 07ffffffeh             ; clear PE & PG bits
    mov     rdi, r10                    ; rdi <- 16-bit stack segment

    shl     r8, 16
    push    r8                          ; far jmp address
    lea     eax, @16Bit
    push    rax
    mov     word ptr [rsp + 4], 8
    lgdt    _16Gdtr
    retf
@16Bit:
    DB      66h
    mov     ecx, 0c0000080h
    mov     cr0, rsi                    ; disable PE & PG
    rdmsr
    and     ah, NOT 1
    wrmsr                               ; clear LME bit
    mov     rax, cr4
    and     al, NOT 30h                 ; clear PAE & PSE
    mov     cr4, rax
    retf
@@:
    xor     rax, rax
    mov     ax, ss
    shl     eax, 4
    add     eax, esp                    ; rax <- address of 16-bit stack
    mov     rsp, r15
    lidt    fword ptr [rsp + 58h]       ; restore IDTR
    mov     ds, r12d
    mov     es, r13d
    mov     ss, r14d
    pop     gs
    pop     fs
    ret
_Thunk16    ENDP

    ALIGN   10h

_Code16Addr PROC
_Code16Addr ENDP

RealMode    PROC
    mov     ss, edi
    mov     sp, bx                      ; set up 16-bit stack
    DB      2eh, 0fh, 1, 1eh
    DW      _16Idtr - _Code16Addr       ; lidt _16Idtr
    DB      66h, 61h                    ; popad
    DB      1fh                         ; pop ds
    DB      7                           ; pop es
    pop     fs
    pop     gs
    sub     esp, (sizeof(IA32_REGS) - 16) + STACK_PARAM_SIZE + 8

    DB      66h, 9Dh                    ; popfd
    add     esp, 4                      ; skip high part of RFLAGS
    DB      67h, 0f7h, 44h, 24h         ; test    (_STK16 ptr [esp + STACK_PARAM_SIZE + sizeof(IA32_REGS)]).ThunkFlags, 1
    DB      (STACK_PARAM_SIZE + sizeof(IA32_REGS) + 6)
    DB      1, 0
    jz      @F
    pushfq                              ; pushf, actually, when it's INT#
@@:
    DB      0eh                         ; push cs
    DB      68h                         ; push /iw
    DW      @FarCallRet - _Code16Addr
    jz      @F
    DB      66h
    jmp     fword ptr [esp + 6 + STACK_PARAM_SIZE + sizeof(IA32_REGS) - 8]
@@:
    DB      66h
    jmp     fword ptr [esp + 4 + STACK_PARAM_SIZE + sizeof(IA32_REGS) - 8]
@FarCallRet:
    add     esp, (sizeof(IA32_REGS) - 16) + STACK_PARAM_SIZE + 8
    DB      66h
    push    0                           ; push a dword of zero
    pushf                               ; pushfd, actually
    push    gs
    push    fs
    DB      6                           ; push es
    DB      1eh                         ; push ds
    DB      66h, 60h                    ; pushad
    cli

    DB      66h
    lgdt    (_STK16 ptr [esp + sizeof(IA32_REGS)]).SavedGdtr
    DB      66h
    mov     eax, (_STK16 ptr [esp + sizeof(IA32_REGS)]).SavedCr4
    mov     cr4, rax
    DB      66h
    mov     ecx, 0c0000080h
    rdmsr
    or      ah, 1
    wrmsr                               ; set LME
    DB      66h
    mov     eax, (_STK16 ptr [esp + sizeof(IA32_REGS)]).SavedCr0
    mov     cr0, rax
    DB      66h
    jmp     fword ptr (_STK16 ptr [esp + sizeof(IA32_REGS)]).RetEip

RealMode    ENDP

_16Idtr     FWORD   (1 SHL 10) - 1

_Code16End:

    END
