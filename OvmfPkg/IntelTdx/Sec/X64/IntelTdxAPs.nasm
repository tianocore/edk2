;------------------------------------------------------------------------------
; @file
; Intel TDX APs
;
; Copyright (c) 2021 - 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

%include "TdxCommondefs.inc"

    ;
    ; Note: BSP never gets here. APs will be unblocked by DXE
    ;
    ; R8  [31:0]  NUM_VCPUS
    ;     [63:32] MAX_VCPUS
    ; R9  [31:0]  VCPU_INDEX
    ;
ParkAp:

do_wait_loop:
    ;
    ; register itself in [rsp + CpuArrivalOffset]
    ;
    mov       rax, 1
    lock xadd dword [rsp + CpuArrivalOffset], eax
    inc       eax

.check_arrival_cnt:
    cmp       eax, r8d
    je        .check_command
    mov       eax, dword[rsp + CpuArrivalOffset]
    jmp       .check_arrival_cnt

.check_command:
    mov     eax, dword[rsp + CommandOffset]
    cmp     eax, MpProtectedModeWakeupCommandNoop
    je      .check_command

    cmp     eax, MpProtectedModeWakeupCommandWakeup
    je      .do_wakeup

    cmp     eax, MpProtectedModeWakeupCommandAcceptPages
    je      .do_accept_pages

    ; Don't support this command, so ignore
    jmp     .check_command

.do_accept_pages:
    ;
    ; Read the top stack address from arguments
    mov     rsi, [rsp + AcceptPageArgsTopStackAddress]

    ;
    ; Calculate the top stack address of the AP.
    ; ApStackAddr = BaseStackAddr + (vCpuIndex) * ApStackSize
    xor     rdx, rdx
    xor     rbx, rbx
    xor     rax, rax
    mov     eax, [rsp + AcceptPageArgsApStackSize]
    mov     ebx, r9d    ; vCpuIndex
    mul     ebx
    add     rsi, rax    ; now rsi is ApStackAddr

.start_accept_pages:
    ;
    ; Read the function address which will be called
    mov     rax, [rsp + WakeupVectorOffset]

    ;
    ; vCPU index as the first argument
    mov     ecx, r9d
    mov     rdx, [rsp + AcceptPageArgsPhysicalStart]
    mov     r8, [rsp + AcceptPageArgsPhysicalEnd]

    ; save the Mailbox address to rbx
    mov     rbx, rsp

    ;
    ; set AP Stack
    mov     rsp, rsi
    nop

    ; save rax (the Mailbox address)
    push    rbx

    call    rax

    ; recove rsp
    pop     rbx
    mov     rsp, rbx
    ;
    ; recover r8, r9
    mov     rax, 1
    tdcall

    mov     eax, 0FFFFFFFFh
    lock xadd dword [rsp + CpusExitingOffset], eax
    dec     eax

.check_exiting_cnt:
    cmp     eax, 0
    je      do_wait_loop
    mov     eax, dword[rsp + CpusExitingOffset]
    jmp     .check_exiting_cnt

.do_wakeup:
    ;
    ; BSP sets these variables before unblocking APs
    ;   RAX:  WakeupVectorOffset
    ;   RBX:  Relocated mailbox address
    ;   RBP:  vCpuId
    ;
    mov     rax, 0
    mov     eax, dword[rsp + WakeupVectorOffset]
    mov     rbx, [rsp + WakeupArgsRelocatedMailBox]
    nop
    jmp     rax
    jmp     $
