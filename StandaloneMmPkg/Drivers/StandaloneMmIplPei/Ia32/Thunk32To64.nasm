;------------------------------------------------------------------------------
;
; Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;    Thunk32To64.nasm
;
; Abstract:
;
;   This is the assembly code to transition from long mode to compatibility
;   mode to execute 32-bit code and then transit back to long mode.
;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
; Procedure:    AsmExecute64BitCode
;
; Input:        None
;
; Output:       None
;
; Prototype:    UINT32
;               AsmExecute64BitCode (
;                 IN UINT64           Function,
;                 IN UINT64           Param1,
;                 IN UINT64           Param2,
;                 IN IA32_DESCRIPTOR  *InternalGdtr
;                 );
;
;
; Description:  A thunk function to execute 32-bit code in long mode.
;
;------------------------------------------------------------------------------
global ASM_PFX(AsmExecute64BitCode)
ASM_PFX(AsmExecute64BitCode):
;
; +---------+
; | EIP(64) |
; +---------+
; | CS (64) |
; +---------+
; | EIP(32) |
; +---------+
; | CS (32) |<-ESP (16 bytes aligned)
; +---------+
; | ...     |
; +---------+
; | ebx     |<-EBP
; +---------+
; | ebp     |<-EBP + 4
; +---------+
; | esi     |<-EBP + 8
; +---------+
; | edi     |<-EBP + 12
; +---------+
; | RFlags  |<-EBP + 16
; +---------+
; | RetAddr |<-EBP (org)
; +---------+
; | Func    |<-EBP + 24
; | Func    |
; +---------+
; | Param1  |<-EBP + 32
; | Param1  |
; +---------+
; | Param2  |<-EBP + 40
; | Param2  |
; +---------+
; | Gdtr    |
; +---------+
;
    ;
    ; Save general purpose register and RFlags register
    ;
    pushfd
    push    edi
    push    esi
    push    ebp
    push    ebx
    mov     ebp, esp

    and     esp, 0FFFFFFF0h

    push    010h                        ; protected mode selector on stack
    mov     eax, Compatible             ; offset for LongMode
    push    eax                         ; offset on stack

    push    038h                        ; long mode selector on stack
    mov     eax, LongMode               ; offset for LongMode
    push    eax                         ; offset on stack

    mov     eax, cr4
    or      al, 020h
    mov     cr4, eax                    ; enable PAE
    mov     ecx, 0c0000080h
    rdmsr
    or      ah, 1                       ; set LME
    wrmsr
    mov     eax, cr0
    bts     eax, 31                     ; set PG
    mov     cr0, eax                    ; enable paging
    retf                                ; topmost 2 dwords hold the address
LongMode:                               ; long mode starts here

    ; Call long mode function
    DB      67h, 48h                    ; 32-bit address size, 64-bit operand size
    mov     eax, [ebp + 24]             ; mov rbx, [ebp + 24]
    DB      67h, 48h
    mov     ecx, [ebp + 24 + 8]         ; mov rcx, [ebp + 24 + 8]
    DB      67h, 48h
    mov     edx, [ebp + 24 + 16]        ; mov rdx, [ebp + 24 + 16]

    DB      48h
    add     esp, -20h                   ; add rsp, -20h
    call    eax                         ; call rax
    DB      48h
    add     esp, 20h                    ; add rsp, 20h

    ; after long mode function call
    mov     ebx, eax

    retf
Compatible:
    mov     ecx, cr0
    btc     ecx, 31                     ; clear PG
    mov     cr0, ecx                    ; disable paging
    mov     ecx, 0C0000080h
    rdmsr
    btc     eax, 8                      ; clear LME
    wrmsr

    ;
    ; Restore C register and eax hold the return status from 32-bit function.
    ; Note: Do not touch rax from now which hold the return value from IA32 function
    ;
    mov     eax, ebx                    ; put return status to EAX
    mov     esp, ebp                    ; restore stack pointer
    pop     ebx
    pop     ebp
    pop     esi
    pop     edi
    popfd

    ret
