#------------------------------------------------------------------------------ ;
# Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
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



ASM_GLOBAL ASM_PFX(CommonExceptionHandler)
ASM_GLOBAL ASM_PFX(CommonInterruptEntry)
ASM_GLOBAL ASM_PFX(HookAfterStubHeaderEnd)

#EXTRN ASM_PFX(mErrorCodeFlag):DWORD # Error code flags for exceptions
#EXTRN ASM_PFX(mDoFarReturnFlag):QWORD  # Do far return flag
.text

#
# exception handler stub table
#
Exception0Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   0
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception1Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   1
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception2Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   2
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception3Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   3
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception4Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   4
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception5Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   5
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception6Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   6
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception7Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   7
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception8Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   8
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception9Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   9
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception10Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   10
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception11Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   11
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception12Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   12
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception13Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   13
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception14Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   14
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception15Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   15
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception16Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   16
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception17Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   17
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception18Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   18
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception19Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   19
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception20Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   20
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception21Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   21
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception22Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   22
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception23Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   23
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception24Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   24
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception25Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   25
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception26Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   26
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception27Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   27
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception28Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   28
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception29Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   29
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception30Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   30
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
Exception31Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   31
    pushq   %rax
    .byte   0x48, 0xB8
    .quad   ASM_PFX(CommonInterruptEntry)
    jmp     *%rax
    
HookAfterStubHeaderBegin:
    .byte   0x6a      # push
VectorNum:
    .byte   0         # 0 will be fixed 
    pushq   %rax
    .byte   0x48, 0xB8      # movq    ASM_PFX(HookAfterStubHeaderEnd), %rax
    .quad   ASM_PFX(HookAfterStubHeaderEnd)
    jmp     *%rax
ASM_GLOBAL ASM_PFX(HookAfterStubHeaderEnd)
ASM_PFX(HookAfterStubHeaderEnd):
    movq    %rsp, %rax
    andl    $0x0fffffff0, %esp  # make sure 16-byte aligned for exception context
    subq    $0x18, %rsp         # reserve room for filling exception data later
    pushq   %rcx
    movq    8(%rax), %rcx
    pushq   %rax
    movabsl ASM_PFX(mErrorCodeFlag), %eax
    bt      %ecx, %eax
    popq    %rax
    jnc     NoErrorData
    pushq   (%rsp)            # push additional rcx to make stack alignment
NoErrorData:
    xchgq   (%rsp), %rcx      # restore rcx, save Exception Number in stack
    pushq   (%rax)            # push rax into stack to keep code consistence

#---------------------------------------;
# CommonInterruptEntry                  ;
#---------------------------------------;
# The follow algorithm is used for the common interrupt routine.

ASM_GLOBAL ASM_PFX(CommonInterruptEntry)
ASM_PFX(CommonInterruptEntry):
    cli
    popq    %rax
    #
    # All interrupt handlers are invoked through interrupt gates, so
    # IF flag automatically cleared at the entry point
    #
    #
    # Calculate vector number
    #
    xchgq   (%rsp), %rcx       # get the return address of call, actually, it is the address of vector number.
    andq     $0x0FF, %rcx
    cmp     $32, %ecx          # Intel reserved vector for exceptions?
    jae     NoErrorCode
    pushq   %rax
    movabsl ASM_PFX(mErrorCodeFlag), %eax
    bt      %ecx, %eax
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
    pushq   $0          # check EXCEPTION_HANDLER_CONTEXT.OldIdtHandler
    pushq   $0          # check EXCEPTION_HANDLER_CONTEXT.ExceptionDataFlag

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
    cmpq    $0, -32(%rsp)      # check EXCEPTION_HANDLER_CONTEXT.OldIdtHandler
    jz      DoReturn           # check EXCEPTION_HANDLER_CONTEXT.ExceptionDataFlag
    cmpb    $1, -40(%rsp)
    jz      ErrorCode
    jmp     *-32(%rsp)
ErrorCode:
    subq    $8, %rsp
    jmp     *-24(%rsp)

DoReturn:
    pushq   %rax
    movabsq ASM_PFX(mDoFarReturnFlag), %rax
    cmpq    $0, %rax          # Check if need to do far return instead of IRET
    popq    %rax
    jz      DoIret
    pushq   %rax
    movq    %rsp, %rax        # save old RSP to rax
    movq    0x20(%rsp), %rsp
    pushq   0x10(%rax)        # save CS in new location
    pushq   0x8(%rax)         # save EIP in new location
    pushq   0x18(%rax)        # save EFLAGS in new location
    movq    (%rax), %rax      # restore rax
    popfq                     # restore EFLAGS
    .byte   0x48              # prefix to composite "retq" with next "retf"
    retf                      # far return
DoIret:
    iretq


#-------------------------------------------------------------------------------------
#  AsmGetTemplateAddressMap (&AddressMap);
#-------------------------------------------------------------------------------------
# comments here for definition of address map
ASM_GLOBAL ASM_PFX(AsmGetTemplateAddressMap)
ASM_PFX(AsmGetTemplateAddressMap):

        movabsq      $Exception0Handle, %rax
        movq         %rax, (%rcx)
        movq         $(Exception1Handle - Exception0Handle), 0x08(%rcx)
        movabsq      $HookAfterStubHeaderBegin, %rax
        movq         %rax, 0x10(%rcx)
        ret

#-------------------------------------------------------------------------------------
#  AsmVectorNumFixup (*VectorBase, VectorNum);
#-------------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(AsmVectorNumFixup)
ASM_PFX(AsmVectorNumFixup):
        movq  %rdx, %rax
        movb  %al, (VectorNum - HookAfterStubHeaderBegin)(%rcx)
        ret

#END


