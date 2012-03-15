#------------------------------------------------------------------------------ ;
# Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   ExceptionHandlerAsm.S
#
# Abstract:
#
#   x64 CPU Exception Handler
#
# Notes:
#
#------------------------------------------------------------------------------


#EXTRN ASM_PFX(mErrorCodeFlag):DWORD # Error code flags for exceptions

ASM_GLOBAL ASM_PFX(CommonExceptionHandler)
ASM_GLOBAL ASM_PFX(CommonInterruptEntry)


.text

#
# point to the external interrupt vector table
#
Exception0Handle:
    pushq   $0
    jmp     ASM_PFX(CommonInterruptEntry)
Exception1Handle:
    pushq   $1
    jmp     ASM_PFX(CommonInterruptEntry)
Exception2Handle:
    pushq   $2
    jmp     ASM_PFX(CommonInterruptEntry)
Exception3Handle:
    pushq   $3
    jmp     ASM_PFX(CommonInterruptEntry)
Exception4Handle:
    pushq   $4
    jmp     ASM_PFX(CommonInterruptEntry)
Exception5Handle:
    pushq   $5
    jmp     ASM_PFX(CommonInterruptEntry)
Exception6Handle:
    pushq   $6
    jmp     ASM_PFX(CommonInterruptEntry)
Exception7Handle:
    pushq   $7
    jmp     ASM_PFX(CommonInterruptEntry)
Exception8Handle:
    pushq   $8
    jmp     ASM_PFX(CommonInterruptEntry)
Exception9Handle:
    pushq   $9
    jmp     ASM_PFX(CommonInterruptEntry)
Exception10Handle:
    pushq   $10
    jmp     ASM_PFX(CommonInterruptEntry)
Exception11Handle:
    pushq   $11
    jmp     ASM_PFX(CommonInterruptEntry)
Exception12Handle:
    pushq   $12
    jmp     ASM_PFX(CommonInterruptEntry)
Exception13Handle:
    pushq   $13
    jmp     ASM_PFX(CommonInterruptEntry)
Exception14Handle:
    pushq   $14
    jmp     ASM_PFX(CommonInterruptEntry)
Exception15Handle:
    pushq   $15
    jmp     ASM_PFX(CommonInterruptEntry)
Exception16Handle:
    pushq   $16
    jmp     ASM_PFX(CommonInterruptEntry)
Exception17Handle:
    pushq   $17
    jmp     ASM_PFX(CommonInterruptEntry)
Exception18Handle:
    pushq   $18
    jmp     ASM_PFX(CommonInterruptEntry)
Exception19Handle:
    pushq   $19
    jmp     ASM_PFX(CommonInterruptEntry)
Exception20Handle:
    pushq   $20
    jmp     ASM_PFX(CommonInterruptEntry)
Exception21Handle:
    pushq   $21
    jmp     ASM_PFX(CommonInterruptEntry)
Exception22Handle:
    pushq   $22
    jmp     ASM_PFX(CommonInterruptEntry)
Exception23Handle:
    pushq   $23
    jmp     ASM_PFX(CommonInterruptEntry)
Exception24Handle:
    pushq   $24
    jmp     ASM_PFX(CommonInterruptEntry)
Exception25Handle:
    pushq   $25
    jmp     ASM_PFX(CommonInterruptEntry)
Exception26Handle:
    pushq   $26
    jmp     ASM_PFX(CommonInterruptEntry)
Exception27Handle:
    pushq   $27
    jmp     ASM_PFX(CommonInterruptEntry)
Exception28Handle:
    pushq   $28
    jmp     ASM_PFX(CommonInterruptEntry)
Exception29Handle:
    pushq   $29
    jmp     ASM_PFX(CommonInterruptEntry)
Exception30Handle:
    pushq   $30
    jmp     ASM_PFX(CommonInterruptEntry)
Exception31Handle:
    pushq   $31
    jmp     ASM_PFX(CommonInterruptEntry)


#---------------------------------------;
# CommonInterruptEntry                  ;
#---------------------------------------;
# The follow algorithm is used for the common interrupt routine.

ASM_GLOBAL ASM_PFX(CommonInterruptEntry)
ASM_PFX(CommonInterruptEntry):
    cli
    #
    # All interrupt handlers are invoked through interrupt gates, so
    # IF flag automatically cleared at the entry point
    #
    #
    # Calculate vector number
    #
    xchgq   (%rsp), %rcx # get the return address of call, actually, it is the address of vector number.
    cmp     $32, %ecx          # Intel reserved vector for exceptions?
    jae     NoErrorCode
    pushq   %rax
    leaq    ASM_PFX(mErrorCodeFlag)(%rip), %rax
    bt      %ecx, (%rax) 
    popq    %rax
    jc      CommonInterruptEntry_al_0000

NoErrorCode:

    #
    # Push a dummy error code on the stack
    # to maintain coherent stack map
    #
    pushq   (%rsp)
    movq    $0, 8(%rsp)
CommonInterruptEntry_al_0000:
    pushq   %rbp
    movq    %rsp, %rbp

    #
    # Stack:
    # +---------------------+ <-- 16-byte aligned ensured by processor
    # +    Old SS           +
    # +---------------------+
    # +    Old RSP          +
    # +---------------------+
    # +    RFlags           +
    # +---------------------+
    # +    CS               +
    # +---------------------+
    # +    RIP              +
    # +---------------------+
    # +    Error Code       +
    # +---------------------+
    # + RCX / Vector Number +
    # +---------------------+
    # +    RBP              +
    # +---------------------+ <-- RBP, 16-byte aligned
    #


    #
    # Since here the stack pointer is 16-byte aligned, so
    # EFI_FX_SAVE_STATE_X64 of EFI_SYSTEM_CONTEXT_x64
    # is 16-byte aligned
    #

#; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
#; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
    pushq    %r15
    pushq    %r14
    pushq    %r13
    pushq    %r12
    pushq    %r11
    pushq    %r10
    pushq    %r9
    pushq    %r8
    pushq    %rax
    pushq    8(%rbp)   # RCX
    pushq    %rdx
    pushq    %rbx
    pushq    48(%rbp)  # RSP
    pushq    (%rbp)    # RBP
    pushq    %rsi
    pushq    %rdi

#; UINT64  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
    movzwq  56(%rbp), %rax
    pushq   %rax                      # for ss
    movzwq  32(%rbp), %rax
    pushq   %rax                      # for cs
    movl    %ds, %eax
    pushq   %rax
    movl    %es, %eax
    pushq   %rax
    movl    %fs, %eax
    pushq   %rax
    movl    %gs, %eax
    pushq   %rax

    movq    %rcx, 8(%rbp)                # save vector number

#; UINT64  Rip;
    pushq   24(%rbp)

#; UINT64  Gdtr[2], Idtr[2];
    xorq    %rax, %rax
    pushq   %rax
    pushq   %rax
    sidt    (%rsp)
    xchgq   2(%rsp), %rax
    xchgq   (%rsp), %rax
    xchgq   8(%rsp), %rax

    xorq    %rax, %rax
    pushq   %rax
    pushq   %rax
    sgdt    (%rsp)
    xchgq   2(%rsp), %rax
    xchgq   (%rsp), %rax
    xchgq   8(%rsp), %rax

#; UINT64  Ldtr, Tr;
    xorq    %rax, %rax
    str     %ax
    pushq   %rax
    sldt    %ax
    pushq   %rax

#; UINT64  RFlags;
    pushq   40(%rbp)

#; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
    movq    %cr8, %rax
    pushq   %rax
    movq    %cr4, %rax
    orq     $0x208, %rax 
    movq    %rax, %cr4 
    pushq   %rax
    mov     %cr3, %rax 
    pushq   %rax
    mov     %cr2, %rax 
    pushq   %rax
    xorq    %rax, %rax
    pushq   %rax
    mov     %cr0, %rax 
    pushq   %rax

#; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
    movq    %dr7, %rax
    pushq   %rax
    movq    %dr6, %rax
    pushq   %rax
    movq    %dr3, %rax
    pushq   %rax
    movq    %dr2, %rax
    pushq   %rax
    movq    %dr1, %rax
    pushq   %rax
    movq    %dr0, %rax
    pushq   %rax

#; FX_SAVE_STATE_X64 FxSaveState;
    subq    $512, %rsp
    movq    %rsp, %rdi
    .byte 0x0f, 0x0ae, 0x07 #fxsave [rdi]

#; UEFI calling convention for x64 requires that Direction flag in EFLAGs is clear
    cld

#; UINT32  ExceptionData;
    pushq   16(%rbp)

#; Prepare parameter and call
    mov     8(%rbp), %rcx
    mov     %rsp, %rdx
    #
    # Per X64 calling convention, allocate maximum parameter stack space
    # and make sure RSP is 16-byte aligned
    #
    subq    $40, %rsp 
    call    ASM_PFX(CommonExceptionHandler)
    addq    $40, %rsp

    cli
#; UINT64  ExceptionData;
    addq    $8, %rsp

#; FX_SAVE_STATE_X64 FxSaveState;

    movq    %rsp, %rsi
    .byte   0x0f, 0x0ae, 0x0E # fxrstor [rsi]
    addq    $512, %rsp

#; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
#; Skip restoration of DRx registers to support in-circuit emualators
#; or debuggers set breakpoint in interrupt/exception context
    addq    $48, %rsp

#; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
    popq    %rax
    movq    %rax, %cr0
    addq    $8, %rsp   # not for Cr1
    popq    %rax
    movq    %rax, %cr2
    popq    %rax
    movq    %rax, %cr3
    popq    %rax
    movq    %rax, %cr4
    popq    %rax
    movq    %rax, %cr8

#; UINT64  RFlags;
    popq    40(%rbp)

#; UINT64  Ldtr, Tr;
#; UINT64  Gdtr[2], Idtr[2];
#; Best not let anyone mess with these particular registers...
    addq    $48, %rsp

#; UINT64  Rip;
    popq    24(%rbp)

#; UINT64  Gs, Fs, Es, Ds, Cs, Ss;
    popq    %rax
    # mov   %rax, %gs ; not for gs
    popq    %rax
    # mov   %rax, %fs ; not for fs
    # (X64 will not use fs and gs, so we do not restore it)
    popq    %rax
    movl    %eax, %es
    popq    %rax
    movl    %eax, %ds
    popq    32(%rbp)  # for cs
    popq    56(%rbp)  # for ss

#; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
#; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
    popq    %rdi
    popq    %rsi
    addq    $8, %rsp              # not for rbp
    popq    48(%rbp)              # for rsp
    popq    %rbx
    popq    %rdx
    popq    %rcx
    popq    %rax
    popq    %r8
    popq    %r9
    popq    %r10
    popq    %r11
    popq    %r12
    popq    %r13
    popq    %r14
    popq    %r15

    movq    %rbp, %rsp
    popq    %rbp
    addq    $16, %rsp
    iretq


#-------------------------------------------------------------------------------------
#  AsmGetAddressMap (&AddressMap);
#-------------------------------------------------------------------------------------
# comments here for definition of address map
ASM_GLOBAL ASM_PFX(GetTemplateAddressMap)
ASM_PFX(GetTemplateAddressMap):

        movabsq      $Exception0Handle, %rax
        movq         %rax, (%rcx)
        movq         $(Exception1Handle - Exception0Handle), 0x08(%rcx)

        ret

#END


