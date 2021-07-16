;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
;*   SPDX-License-Identifier: BSD-2-Clause-Patent
;*
;*    CpuAsm.asm
;*
;*   Abstract:
;*
;------------------------------------------------------------------------------

#include <Base.h>
%include "TdxCommondefs.inc"

DEFAULT REL
SECTION .text

extern ASM_PFX(SecCoreStartupWithStack)

%macro  tdcall  0
  db  0x66, 0x0f, 0x01, 0xcc
%endmacro

;
; SecCore Entry Point
;
; Processor is in flat protected mode
;
; @param[in]  RAX   Initial value of the EAX register (BIST: Built-in Self Test)
; @param[in]  DI    'BP': boot-strap processor, or 'AP': application processor
; @param[in]  RBP   Pointer to the start of the Boot Firmware Volume
; @param[in]  DS    Selector allowing flat access to all addresses
; @param[in]  ES    Selector allowing flat access to all addresses
; @param[in]  FS    Selector allowing flat access to all addresses
; @param[in]  GS    Selector allowing flat access to all addresses
; @param[in]  SS    Selector allowing flat access to all addresses
;
; @return     None  This routine does not return
;
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):

    ;
    ; Guest type is stored in OVMF_WORK_AREA
    ;
    %define OVMF_WORK_AREA        FixedPcdGet32 (PcdOvmfWorkAreaBase)
    %define VM_GUEST_TYPE_TDX     2
    mov     eax, OVMF_WORK_AREA
    cmp     byte[eax], VM_GUEST_TYPE_TDX
    jne     InitStack

    mov     rax, TDCALL_TDINFO
    tdcall

    ;
    ; R8  [31:0]  NUM_VCPUS
    ;     [63:32] MAX_VCPUS
    ; R9  [31:0]  VCPU_INDEX
    ; Td Guest set the VCPU0 as the BSP, others are the APs
    ; APs jump to spinloop and get released by DXE's MpInitLib
    ;
    mov     rax, r9
    and     rax, 0xffff
    test    rax, rax
    jne     ParkAp

InitStack:

    ;
    ; Fill the temporary RAM with the initial stack value.
    ; The loop below will seed the heap as well, but that's harmless.
    ;
    mov     rax, (FixedPcdGet32 (PcdInitValueInTempStack) << 32) | FixedPcdGet32 (PcdInitValueInTempStack)
                                                              ; qword to store
    mov     rdi, FixedPcdGet32 (PcdOvmfSecPeiTempRamBase)     ; base address,
                                                              ;   relative to
                                                              ;   ES
    mov     rcx, FixedPcdGet32 (PcdOvmfSecPeiTempRamSize) / 8 ; qword count
    cld                                                       ; store from base
                                                              ;   up
    rep stosq

    ;
    ; Load temporary RAM stack based on PCDs
    ;
    %define SEC_TOP_OF_STACK (FixedPcdGet32 (PcdOvmfSecPeiTempRamBase) + \
                          FixedPcdGet32 (PcdOvmfSecPeiTempRamSize))
    mov     rsp, SEC_TOP_OF_STACK
    nop

    ;
    ; Setup parameters and call SecCoreStartupWithStack
    ;   rcx: BootFirmwareVolumePtr
    ;   rdx: TopOfCurrentStack
    ;
    mov     rcx, rbp
    mov     rdx, rsp
    sub     rsp, 0x20
    call    ASM_PFX(SecCoreStartupWithStack)

    ;
    ; Note: BSP never gets here. APs will be unblocked by DXE
    ;
    ; R8  [31:0]  NUM_VCPUS
    ;     [63:32] MAX_VCPUS
    ; R9  [31:0]  VCPU_INDEX
    ;
ParkAp:

    mov     rbp,  r9

.do_wait_loop:
    mov     rsp, FixedPcdGet32 (PcdOvmfSecGhcbBackupBase)

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
    jne     .check_command

    ;
    ; AP Accept Pages
    ;
    ; Accept Pages in TDX is time-consuming, especially for big memory.
    ; One of the mitigation is to accept pages by BSP and APs parallely.
    ;
    ; For example, there are 4 CPUs (1 BSP and 3 APs). Totally there are
    ; 1G memory to be accepted.
    ;
    ; BSP is responsible for the memory regions of:
    ;    Start : StartAddress + ChunkSize * (4) * Index
    ;    Length: ChunkSize
    ; APs is reponsible for the memory regions of:
    ;    Start : StartAddress + ChunkSize * (4) * Index + ChunkSize * CpuId
    ;    Length: ChunkSize
    ;
    ; TDCALL_TDACCEPTPAGE supports the PageSize of 4K and 2M. Sometimes when
    ; the PageSize is 2M, TDX_PAGE_SIZE_MISMATCH is returned as the error code.
    ; In this case, TDVF need fall back to 4k PageSize to accept again.
    ;
    ; If any errors happened in accept pages, an error code is recorded in
    ; Mailbox [ErrorsOffset + CpuIndex]
    ;
.ap_accept_page:

    ;
    ; Clear the errors and fallback flag
    ;
    mov     al, ERROR_NON
    mov     byte[rsp + ErrorsOffset + rbp], al
    xor     r12, r12

    ;
    ; Get PhysicalAddress/ChunkSize/PageSize
    ;
    mov     rcx, [rsp + AcceptPageArgsPhysicalStart]
    mov     rbx, [rsp + AcceptPageArgsChunkSize]

    ;
    ; Set AcceptPageLevel based on the AcceptPagesize
    ; Currently only 2M/4K page size is acceptable
    ;
    mov     r15, [rsp + AcceptPageArgsPageSize]
    cmp     r15, SIZE_4KB
    je      .set_4kb
    cmp     r15, SIZE_2MB
    je      .set_2mb

    mov     al, ERROR_INVALID_ACCEPT_PAGE_SIZE
    mov     byte[rsp + ErrorsOffset + rbp], al
    jmp     .do_finish_command

.set_4kb:
    mov     r15, PAGE_ACCEPT_LEVEL_4K
    jmp     .physical_address

.set_2mb:
    mov     r15, PAGE_ACCEPT_LEVEL_2M

.physical_address:
    ;
    ; PhysicalAddress += (CpuId * ChunkSize)
    ;
    xor     rdx, rdx
    mov     eax, ebp
    mul     ebx
    add     rcx, rax
    shl     rdx, 32
    add     rcx, rdx

.do_accept_next_range:
    ;
    ; Make sure we don't accept page beyond ending page
    ; This could happen is ChunkSize crosses the end of region
    ;
    cmp     rcx, [rsp + AcceptPageArgsPhysicalEnd ]
    jge     .do_finish_command

    ;
    ; Save starting address for this region
    ;
    mov     r11, rcx

    ;
    ; Size = MIN(ChunkSize, PhysicalEnd - PhysicalAddress);
    ;
    mov     rax, [rsp + AcceptPageArgsPhysicalEnd]
    sub     rax, rcx
    cmp     rax, rbx
    jge     .do_accept_loop
    mov     rbx, rax

.do_accept_loop:
    ;
    ; RCX: Accept address
    ; R15: Accept Page Level
    ; R12: Flag of fall back accept
    ;
    mov     rax, TDCALL_TDACCEPTPAGE
    xor     rdx, rdx
    or      rcx, r15

    tdcall

    ;
    ; Check status code in RAX
    ;
    test    rax, rax
    jz      .accept_success

    shr     rax, 32
    cmp     eax, TDX_PAGE_ALREADY_ACCEPTED
    jz      .already_accepted

    cmp     eax, TDX_PAGE_SIZE_MISMATCH
    jz      .accept_size_mismatch

    ;
    ; other error
    ;
    mov     al, ERROR_ACCEPT_PAGE_ERROR
    mov     byte[rsp + ErrorsOffset + rbp], al
    jmp     .do_finish_command

.accept_size_mismatch:
    ;
    ; Check the current PageLevel.
    ; ACCEPT_LEVEL_4K is the least level and cannot fall back any more.
    ; If in this case, just record the error and return
    ;
    cmp     r15, PAGE_ACCEPT_LEVEL_4K
    jne     .do_fallback_accept
    mov     al, ERROR_INVALID_FALLBACK_PAGE_LEVEL
    mov     byte[rsp + ErrorsOffset + rbp], al
    jmp     .do_finish_command

.do_fallback_accept:
    ;
    ; In fall back accept, just loop 512 times (2M = 512 * 4K)
    ; Save the rcx in r13.
    ; Decrease the PageLevel in R15.
    ; R12 indicates it is in a fall back accept loop.
    ;
    mov     r14, 512
    and     rcx, ~0x3ULL
    mov     r13, rcx
    xor     rdx, rdx
    dec     r15
    mov     r12, 1

    jmp     .do_accept_loop

.accept_success:
    ;
    ; Keep track of how many accepts per cpu
    ;
    inc dword[rsp + TalliesOffset + rbp * 4]

    ;
    ; R12 indicate whether it is a fall back accept
    ; If it is a success of fall back accept
    ; Just loop 512 times to .do_accept_loop
    ;
    test    r12, r12
    jz      .normal_accept_success

    ;
    ; This is fallback accept success
    ;
    add     rcx, SIZE_4KB
    dec     r14
    test    r14, r14
    jz      .fallback_accept_done
    jmp     .do_accept_loop

.fallback_accept_done:
    ;
    ; Fall back accept done.
    ; Restore the start address to RCX from R13
    ; Clear the fall back accept flag
    ;
    mov     rcx, r13
    inc     r15
    xor     r12, r12

.already_accepted:
    ;
    ; Handle the sitution of fall back accpet
    ;
    test    r12, r12
    jnz     .accept_success

.normal_accept_success:
    ;
    ; Reduce accept size by a PageSize, and increment address
    ;
    mov     r12, [rsp + AcceptPageArgsPageSize]
    sub     rbx, r12
    add     rcx, r12
    xor     r12, r12

    ;
    ; We may be given multiple pages to accept, make sure we
    ; aren't done
    ;
    test    rbx, rbx
    jne     .do_accept_loop

    ;
    ; Restore address before, and then increment by stride (num-cpus * ChunkSize)
    ;
    xor     rdx, rdx
    mov     rcx, r11
    mov     eax, r8d
    mov     ebx, [rsp + AcceptPageArgsChunkSize]
    mul     ebx
    add     rcx, rax
    shl     rdx, 32
    add     rcx, rdx
    jmp     .do_accept_next_range

.do_finish_command:
    mov       eax, 0FFFFFFFFh
    lock xadd dword [rsp + CpusExitingOffset], eax
    dec       eax

.check_exiting_cnt:
    cmp       eax, 0
    je        .do_wait_loop
    mov       eax, dword[rsp + CpusExitingOffset]
    jmp       .check_exiting_cnt

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
