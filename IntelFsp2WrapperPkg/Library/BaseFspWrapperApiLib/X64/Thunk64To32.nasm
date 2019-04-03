;
; Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;
; Module Name:
;
;    Thunk64To32.nasm
;
; Abstract:
;
;   This is the assembly code to transition from long mode to compatibility mode to execute 32-bit code and then
;   transit back to long mode.
;
;-------------------------------------------------------------------------------
    DEFAULT REL
    SECTION .text
;----------------------------------------------------------------------------
; Procedure:    AsmExecute32BitCode
;
; Input:        None
;
; Output:       None
;
; Prototype:    UINT32
;               AsmExecute32BitCode (
;                 IN UINT64           Function,
;                 IN UINT64           Param1,
;                 IN UINT64           Param2,
;                 IN IA32_DESCRIPTOR  *InternalGdtr
;                 );
;
;
; Description:  A thunk function to execute 32-bit code in long mode.
;
;----------------------------------------------------------------------------
global ASM_PFX(AsmExecute32BitCode)
ASM_PFX(AsmExecute32BitCode):
    ;
    ; save IFLAG and disable it
    ;
    pushfq
    cli

    ;
    ; save orignal GDTR and CS
    ;
    mov     rax, ds
    push    rax
    mov     rax, cs
    push    rax
    sub     rsp, 0x10
    sgdt    [rsp]
    ;
    ; load internal GDT
    ;
    lgdt    [r9]
    ;
    ; Save general purpose register and rflag register
    ;
    pushfq
    push    rdi
    push    rsi
    push    rbp
    push    rbx

    ;
    ; save CR3
    ;
    mov     rax, cr3
    mov     rbp, rax

    ;
    ; Prepare the CS and return address for the transition from 32-bit to 64-bit mode
    ;
    mov     rax, dword 0x10              ; load long mode selector
    shl     rax, 32
    lea     r9,  [ReloadCS]          ;Assume the ReloadCS is under 4G
    or      rax, r9
    push    rax
    ;
    ; Save parameters for 32-bit function call
    ;
    mov     rax, r8
    shl     rax, 32
    or      rax, rdx
    push    rax
    ;
    ; save the 32-bit function entry and the return address into stack which will be
    ; retrieve in compatibility mode.
    ;
    lea     rax, [ReturnBack]   ;Assume the ReloadCS is under 4G
    shl     rax, 32
    or      rax, rcx
    push    rax

    ;
    ; let rax save DS
    ;
    mov     rax, dword 0x18

    ;
    ; Change to Compatible Segment
    ;
    mov     rcx, dword 0x8               ; load compatible mode selector
    shl     rcx, 32
    lea     rdx, [Compatible] ; assume address < 4G
    or      rcx, rdx
    push    rcx
    retf

Compatible:
    ; reload DS/ES/SS to make sure they are correct referred to current GDT
    mov     ds, ax
    mov     es, ax
    mov     ss, ax

    ;
    ; Disable paging
    ;
    mov     rcx, cr0
    btc     ecx, 31
    mov     cr0, rcx
    ;
    ; Clear EFER.LME
    ;
    mov     ecx, 0xC0000080
    rdmsr
    btc     eax, 8
    wrmsr

; Now we are in protected mode
    ;
    ; Call 32-bit function. Assume the function entry address and parameter value is less than 4G
    ;
    pop    rax                 ; Here is the function entry
    ;
    ; Now the parameter is at the bottom of the stack,  then call in to IA32 function.
    ;
    jmp   rax
ReturnBack:
    mov   ebx, eax             ; save return status
    pop   rcx                  ; drop param1
    pop   rcx                  ; drop param2

    ;
    ; restore CR4
    ;
    mov     rax, cr4
    bts     eax, 5
    mov     cr4, rax

    ;
    ; restore CR3
    ;
    mov     eax, ebp
    mov     cr3, rax

    ;
    ; Set EFER.LME to re-enable ia32-e
    ;
    mov     ecx, 0xC0000080
    rdmsr
    bts     eax, 8
    wrmsr
    ;
    ; Enable paging
    ;
    mov     rax, cr0
    bts     eax, 31
    mov     cr0, rax
; Now we are in compatible mode

    ;
    ; Reload cs register
    ;
    retf
ReloadCS:
    ;
    ; Now we're in Long Mode
    ;
    ;
    ; Restore C register and eax hold the return status from 32-bit function.
    ; Note: Do not touch rax from now which hold the return value from IA32 function
    ;
    mov     eax, ebx ; put return status to EAX
    pop     rbx
    pop     rbp
    pop     rsi
    pop     rdi
    popfq
    ;
    ; Switch to orignal GDT and CS. here rsp is pointer to the orignal GDT descriptor.
    ;
    lgdt    [rsp]
    ;
    ; drop GDT descriptor in stack
    ;
    add     rsp, 0x10
    ;
    ; switch to orignal CS and GDTR
    ;
    pop     r9                 ; get  CS
    shl     r9,  32            ; rcx[32..47] <- Cs
    lea     rcx, [.0]
    or      rcx, r9
    push    rcx
    retf
.0:
    ;
    ; Reload original DS/ES/SS
    ;
    pop     rcx
    mov     ds, rcx
    mov     es, rcx
    mov     ss, rcx

    ;
    ; Restore IFLAG
    ;
    popfq

    ret

