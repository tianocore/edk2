;------------------------------------------------------------------------------
;
; Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   AsmFuncs.asm
;
; Abstract:
;
;   Debug interrupt handle functions.
;
;------------------------------------------------------------------------------

#include "DebugException.h"


externdef InterruptProcess:near

data SEGMENT

public          Exception0Handle, TimerInterruptHandle, ExceptionStubHeaderSize

AGENT_HANDLER_SIGNATURE  MACRO
  db   41h, 47h, 54h, 48h       ; SIGNATURE_32('A','G','T','H')
ENDM

ExceptionStubHeaderSize        dd      Exception1Handle - Exception0Handle ;
CommonEntryAddr                dq      CommonEntry ;

.code

AGENT_HANDLER_SIGNATURE
Exception0Handle:
    cli
    push    rcx
    mov     rcx, 0
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE	
Exception1Handle:
    cli
    push    rcx
    mov     rcx, 1
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception2Handle:
    cli
    push    rcx
    mov     rcx, 2
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception3Handle:
    cli
    push    rcx
    mov     rcx, 3
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception4Handle:
    cli
    push    rcx
    mov     rcx, 4
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception5Handle:
    cli
    push    rcx
    mov     rcx, 5
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception6Handle:
    cli
    push    rcx
    mov     rcx, 6
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception7Handle:
    cli
    push    rcx
    mov     rcx, 7
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception8Handle:
    cli
    push    rcx
    mov     rcx, 8
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception9Handle:
    cli
    push    rcx
    mov     rcx, 9
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception10Handle:
    cli
    push    rcx
    mov     rcx, 10
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception11Handle:
    cli
    push    rcx
    mov     rcx, 11
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception12Handle:
    cli
    push    rcx
    mov     rcx, 12
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception13Handle:
    cli
    push    rcx
    mov     rcx, 13
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception14Handle:
    cli
    push    rcx
    mov     rcx, 14
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception15Handle:
    cli
    push    rcx
    mov     rcx, 15
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception16Handle:
    cli
    push    rcx
    mov     rcx, 16
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception17Handle:
    cli
    push    rcx
    mov     rcx, 17
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception18Handle:
    cli
    push    rcx
    mov     rcx, 18
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
Exception19Handle:
    cli
    push    rcx
    mov     rcx, 19
    jmp     qword ptr [CommonEntryAddr]
AGENT_HANDLER_SIGNATURE
TimerInterruptHandle:
    cli
    push    rcx
    mov     rcx, 32
    jmp     qword ptr [CommonEntryAddr]

CommonEntry:
    ; We need to determine if any extra data was pushed by the exception
    cmp     rcx, DEBUG_EXCEPT_DOUBLE_FAULT
    je      NoExtrPush
    cmp     rcx, DEBUG_EXCEPT_INVALID_TSS
    je      NoExtrPush
    cmp     rcx, DEBUG_EXCEPT_SEG_NOT_PRESENT
    je      NoExtrPush
    cmp     rcx, DEBUG_EXCEPT_STACK_FAULT
    je      NoExtrPush
    cmp     rcx, DEBUG_EXCEPT_GP_FAULT
    je      NoExtrPush
    cmp     rcx, DEBUG_EXCEPT_PAGE_FAULT
    je      NoExtrPush
    cmp     rcx, DEBUG_EXCEPT_ALIGNMENT_CHECK
    je      NoExtrPush

    push    [rsp]
    mov     qword ptr [rsp + 8], 0

NoExtrPush:
    push    rbp
    mov     rbp, rsp

    ; store UINT64  r8, r9, r10, r11, r12, r13, r14, r15;
    push    r15
    push    r14
    push    r13
    push    r12
    push    r11
    push    r10
    push    r9
    push    r8

    mov     r8, cr8
    push    r8

    ; store UINT64  Rdi, Rsi, Rbp, Rsp, Rdx, Rcx, Rbx, Rax;
    push    rax
    push    rbx
    push    qword ptr [rbp + 8]       ; original rcx
    push    rdx
    push    qword ptr [rbp + 6 * 8]   ; original rsp
    push    qword ptr [rbp]           ; original rbp
    push    rsi
    push    rdi

    ;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    ;; insure FXSAVE/FXRSTOR is enabled in CR4...
    ;; ... while we're at it, make sure DE is also enabled...
    mov     rax, cr4
    or      rax, 208h
    mov     cr4, rax
    push    rax
    mov     rax, cr3
    push    rax
    mov     rax, cr2
    push    rax
    push    0
    mov     rax, cr0
    push    rax

    xor     rax, rax
    mov     rax, Ss
    push    rax
    mov     rax, Cs
    push    rax
    mov     rax, Ds
    push    rax
    mov     rax, Es
    push    rax
    mov     rax, Fs
    push    rax
    mov     rax, Gs
    push    rax

    ;; EIP
    mov     rax, [rbp + 8 * 3] ; EIP
    push    rax

    ;; UINT64  Gdtr[2], Idtr[2];
    sub  rsp, 16
    sidt fword ptr [rsp]
    sub  rsp, 16
    sgdt fword ptr [rsp]

    ;; UINT64  Ldtr, Tr;
    xor  rax, rax
    str  ax
    push rax
    sldt ax
    push rax

    ;; EFlags
    mov     rax, [rbp + 8 * 5]
    push    rax

    ;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
    mov     rax, dr7
    push    rax

    ;; clear Dr7 while executing debugger itself
    xor     rax, rax
    mov     dr7, rax

    ;; Dr6
    mov     rax, dr6
    push    rax

    ;; insure all status bits in dr6 are clear...
    xor     rax, rax
    mov     dr6, rax

    mov     rax, dr3
    push    rax
    mov     rax, dr2
    push    rax
    mov     rax, dr1
    push    rax
    mov     rax, dr0
    push    rax

    ;; Clear Direction Flag
    cld

    sub     rsp, 512
    mov     rdi, rsp
    ;; Clear the buffer
    xor     rax, rax
    push    rcx
    mov     rcx, 64 ;= 512 / 8
    rep     stosq
    pop     rcx
    mov     rdi, rsp
    db 0fh, 0aeh, 00000111y ;fxsave [rdi]

    ;; save the exception data
    push    qword ptr [rbp + 16]

    ; call the C interrupt process function
    mov     rdx, rsp      ; Structure
    mov     r15, rcx      ; save vector in r15
    
    ;
    ; Per X64 calling convention, allocate maximum parameter stack space
    ; and make sure RSP is 16-byte aligned
    ;
    sub     rsp, 32 + 8
    call    InterruptProcess
    add     rsp, 32 + 8

    ;; skip the exception data
    add     rsp, 8
    
    mov     rsi, rsp
    db 0fh, 0aeh, 00001110y ; fxrstor [rsi]
    add     rsp, 512

    ;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
    pop     rax
    mov     dr0, rax
    pop     rax
    mov     dr1, rax
    pop     rax
    mov     dr2, rax
    pop     rax
    mov     dr3, rax
    ;; skip restore of dr6.  We cleared dr6 during the context save.
    add     rsp, 8
    pop     rax
    mov     dr7, rax

    ;; set EFlags
    pop     qword ptr [rbp + 8 * 5]

    ;; UINT64  Ldtr, Tr;
    ;; UINT64  Gdtr[2], Idtr[2];
    ;; Best not let anyone mess with these particular registers...
    add     rsp, 24 * 2

    ;; UINT64  Eip;
    pop     qword ptr [rbp + 8 * 3]   ; set EIP in stack

    ;; UINT64  Gs, Fs, Es, Ds, Cs, Ss;
    ;; NOTE - modified segment registers could hang the debugger...  We
    ;;        could attempt to insulate ourselves against this possibility,
    ;;        but that poses risks as well.
    ;;
    pop     rax
    pop     rax
    pop     rax
    mov     es, rax
    pop     rax
    mov     ds, rax
    pop     qword ptr [rbp + 8 * 4]    ; Set CS in stack
    pop     rax
    mov     ss, rax

    ;; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4;
    pop     rax
    mov     cr0, rax
    add     rsp, 8    ; skip for Cr1
    pop     rax
    mov     cr2, rax
    pop     rax
    mov     cr3, rax
    pop     rax
    mov     cr4, rax

    ;; restore general register
    pop    rdi
    pop    rsi
    add    rsp, 8  ; skip rbp
    add    rsp, 8  ; skip rsp
    pop    rdx
    pop    rcx
    pop    rbx
    pop    rax

    pop    r8
    mov    cr8, r8

    ; store UINT64  r8, r9, r10, r11, r12, r13, r14, r15;
    pop     r8
    pop     r9
    pop     r10
    pop     r11
    pop     r12
    pop     r13
    pop     r14
    pop     r15

    mov     rsp, rbp
    pop     rbp
    add     rsp, 16      ; skip rcx and error code

    iretq

END
