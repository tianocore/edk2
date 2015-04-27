#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2012 - 2015, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials
#*   are licensed and made available under the terms and conditions of the BSD License
#*   which accompanies this distribution.  The full text of the license may be found at
#*   http://opensource.org/licenses/bsd-license.php
#*
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#*
#*    ExceptionHandlerAsm.S
#*
#*   Abstract:
#*
#*     IA32 CPU Exception Handler
#
#------------------------------------------------------------------------------


#.MMX
#.XMM

ASM_GLOBAL ASM_PFX(CommonExceptionHandler)
ASM_GLOBAL ASM_PFX(CommonInterruptEntry)
ASM_GLOBAL ASM_PFX(HookAfterStubHeaderEnd)

#EXTRN ASM_PFX(mErrorCodeFlag):DWORD           # Error code flags for exceptions
#EXTRN ASM_PFX(mDoFarReturnFlag):DWORD         # Do far return flag

.text

#
# exception handler stub table
#
Exception0Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   0
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception1Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   1
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception2Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   2
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception3Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   3
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception4Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   4
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception5Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   5
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception6Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   6
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception7Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   7
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception8Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   8
    pushl   %eax
     .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception9Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   9
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception10Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   10
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception11Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   11
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception12Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   12
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception13Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   13
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception14Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   14
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception15Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   15
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception16Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   16
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception17Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   17
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception18Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   18
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception19Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   19
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception20Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   20
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception21Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   21
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception22Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   22
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception23Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   23
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception24Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   24
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception25Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   25
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception26Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   26
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception27Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   27
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception28Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   28
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception29Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   29
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception30Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   30
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax
Exception31Handle:
    .byte   0x6a    #  push #VectorNum
    .byte   31
    pushl   %eax
    .byte   0xB8
    .long   ASM_PFX(CommonInterruptEntry)
    jmp     *%eax

HookAfterStubBegin:
    .byte   0x6a       # push
VectorNum:
    .byte   0          # 0 will be fixed
    pushl   %eax
    .byte   0xB8       # movl    ASM_PFX(HookAfterStubHeaderEnd), %eax
    .long   ASM_PFX(HookAfterStubHeaderEnd)
    jmp     *%eax
ASM_GLOBAL ASM_PFX(HookAfterStubHeaderEnd)
ASM_PFX(HookAfterStubHeaderEnd):
    popl    %eax
    subl    $8, %esp        # reserve room for filling exception data later
    pushl   8(%esp)
    xchgl   (%esp), %ecx    # get vector number
    bt      %ecx, ASM_PFX(mErrorCodeFlag)
    jnc     NoErrorData
    pushl    (%esp)         # addition push if exception data needed
NoErrorData:
    xchg    (%esp), %ecx    # restore ecx
    pushl   %eax

#---------------------------------------;
# CommonInterruptEntry                  ;
#---------------------------------------;
# The follow algorithm is used for the common interrupt routine.

ASM_GLOBAL ASM_PFX(CommonInterruptEntry)
ASM_PFX(CommonInterruptEntry):
    cli
    popl    %eax
    #
    # All interrupt handlers are invoked through interrupt gates, so
    # IF flag automatically cleared at the entry point
    #

    #
    # Get vector number from top of stack
    #
    xchgl   (%esp), %ecx
    andl    $0x0FF, %ecx      # Vector number should be less than 256
    cmpl    $32, %ecx         # Intel reserved vector for exceptions?
    jae     NoErrorCode
    bt      %ecx, ASM_PFX(mErrorCodeFlag)
    jc      HasErrorCode

NoErrorCode:

    #
    # Stack:
    # +---------------------+
    # +    EFlags           +
    # +---------------------+
    # +    CS               +
    # +---------------------+
    # +    EIP              +
    # +---------------------+
    # +    ECX              +
    # +---------------------+ <-- ESP
    #
    # Registers:
    #   ECX - Vector Number
    #

    #
    # Put Vector Number on stack
    #
    pushl   %ecx

    #
    # Put 0 (dummy) error code on stack, and restore ECX
    #
    xorl    %ecx, %ecx  # ECX = 0
    xchgl   4(%esp), %ecx

    jmp     ErrorCodeAndVectorOnStack

HasErrorCode:

    #
    # Stack:
    # +---------------------+
    # +    EFlags           +
    # +---------------------+
    # +    CS               +
    # +---------------------+
    # +    EIP              +
    # +---------------------+
    # +    Error Code       +
    # +---------------------+
    # +    ECX              +
    # +---------------------+ <-- ESP
    #
    # Registers:
    #   ECX - Vector Number
    #

    #
    # Put Vector Number on stack and restore ECX
    #
    xchgl   (%esp), %ecx 

ErrorCodeAndVectorOnStack:
    pushl   %ebp
    movl    %esp, %ebp

    #
    # Stack:
    # +---------------------+
    # +    EFlags           +
    # +---------------------+
    # +    CS               +
    # +---------------------+
    # +    EIP              +
    # +---------------------+
    # +    Error Code       +
    # +---------------------+
    # +    Vector Number    +
    # +---------------------+
    # +    EBP              +
    # +---------------------+ <-- EBP
    #

    #
    # Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
    # is 16-byte aligned
    #
    andl    $0x0fffffff0, %esp 
    subl    $12, %esp

    subl    $8, %esp
    pushl   $0         # check EXCEPTION_HANDLER_CONTEXT.OldIdtHandler
    pushl   $0         # check EXCEPTION_HANDLER_CONTEXT.ExceptionDataFlag
       
#; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    pushl   %eax
    pushl   %ecx
    pushl   %edx
    pushl   %ebx
    leal    24(%ebp), %ecx
    pushl   %ecx                          # ESP
    pushl   (%ebp)              # EBP
    pushl   %esi
    pushl   %edi

#; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
    movl    %ss, %eax
    pushl   %eax
    movzwl  16(%ebp), %eax 
    pushl   %eax
    movl    %ds, %eax
    pushl   %eax
    movl    %es, %eax
    pushl   %eax
    movl    %fs, %eax
    pushl   %eax
    movl    %gs, %eax
    pushl   %eax

#; UINT32  Eip;
    movl    12(%ebp), %eax
    pushl   %eax

#; UINT32  Gdtr[2], Idtr[2];
    subl    $8, %esp
    sidt    (%esp)
    movl    2(%esp), %eax
    xchgl   (%esp), %eax
    andl    $0x0FFFF, %eax 
    movl    %eax, 4(%esp)

    subl    $8, %esp
    sgdt    (%esp)
    movl    2(%esp), %eax
    xchgl   (%esp), %eax
    andl    $0x0FFFF, %eax 
    movl    %eax, 4(%esp)

#; UINT32  Ldtr, Tr;
    xorl    %eax, %eax
    str     %ax
    pushl   %eax
    sldt    %ax
    pushl   %eax

#; UINT32  EFlags;
    movl    20(%ebp), %eax
    pushl   %eax

#; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
## insure FXSAVE/FXRSTOR is enabled in CR4...
## ... while we're at it, make sure DE is also enabled...
    mov     $1, %eax
    pushl   %ebx                         # temporarily save value of ebx on stack 
    cpuid                                # use CPUID to determine if FXSAVE/FXRESTOR
                                         # and DE are supported
    popl    %ebx                         # retore value of ebx that was overwritten
                                         # by CPUID 
    movl    %cr4, %eax
    pushl   %eax                         # push cr4 firstly
    testl   $BIT24, %edx                 # Test for FXSAVE/FXRESTOR support
    jz      L1
    orl     $BIT9, %eax                  # Set CR4.OSFXSR
L1:    
    testl   $BIT2, %edx                  # Test for Debugging Extensions support
    jz      L2
    orl     $BIT3, %eax                  # Set CR4.DE
L2:    
    movl    %eax, %cr4
    movl    %cr3, %eax
    pushl   %eax
    movl    %cr2, %eax
    pushl   %eax
    xorl    %eax, %eax
    pushl   %eax
    movl    %cr0, %eax
    pushl   %eax

#; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
    movl    %dr7, %eax
    pushl   %eax
    movl    %dr6, %eax
    pushl   %eax
    movl    %dr3, %eax
    pushl   %eax
    movl    %dr2, %eax
    pushl   %eax
    movl    %dr1, %eax
    pushl   %eax
    movl    %dr0, %eax
    pushl   %eax

#; FX_SAVE_STATE_IA32 FxSaveState;
    subl    $512, %esp
    movl    %esp, %edi
    testl   $BIT24, %edx     # Test for FXSAVE/FXRESTOR support.  
                             # edx still contains result from CPUID above
    jz      L3
    .byte      0x0f, 0x0ae, 0x07 #fxsave [edi]
L3:  

#; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

#; UINT32  ExceptionData;
    pushl   8(%ebp)

#; Prepare parameter and call
    movl    %esp, %edx
    pushl   %edx
    movl    4(%ebp), %edx
    pushl   %edx

    #
    # Call External Exception Handler
    #
    call    ASM_PFX(CommonExceptionHandler)
    addl    $8, %esp

    cli
#; UINT32  ExceptionData;
    addl    $4, %esp

#; FX_SAVE_STATE_IA32 FxSaveState;
    movl    %esp, %esi
    movl    $1, %eax
    cpuid                    # use CPUID to determine if FXSAVE/FXRESTOR 
                             # are supported
    testl   $BIT24, %edx     # Test for FXSAVE/FXRESTOR support
    jz      L4
    .byte      0x0f, 0x0ae, 0x0e # fxrstor [esi]
L4:  
    addl    $512, %esp

#; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
#; Skip restoration of DRx registers to support in-circuit emualators
#; or debuggers set breakpoint in interrupt/exception context
    addl    $24, %esp

#; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    popl    %eax
    movl    %eax, %cr0
    addl    $4, %esp    # not for Cr1
    popl    %eax
    movl    %eax, %cr2
    popl    %eax
    movl    %eax, %cr3
    popl    %eax
    movl    %eax, %cr4

#; UINT32  EFlags;
    popl    20(%ebp)

#; UINT32  Ldtr, Tr;
#; UINT32  Gdtr[2], Idtr[2];
#; Best not let anyone mess with these particular registers...
    addl    $24, %esp

#; UINT32  Eip;
    popl    12(%ebp)

#; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
#; NOTE - modified segment registers could hang the debugger...  We
#;        could attempt to insulate ourselves against this possibility,
#;        but that poses risks as well.
#;
    popl    %gs
    popl    %fs
    popl    %es
    popl    %ds
    popl    16(%ebp)
    popl    %ss

#; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    popl    %edi
    popl    %esi
    addl    $4, %esp   # not for ebp
    addl    $4, %esp   # not for esp
    popl    %ebx
    popl    %edx
    popl    %ecx
    popl    %eax

    popl    -8(%ebp)
    popl    -4(%ebp)
    movl    %ebp, %esp
    popl    %ebp
    addl    $8, %esp
    cmpl    $0, -16(%esp)  # check EXCEPTION_HANDLER_CONTEXT.OldIdtHandler
    jz      DoReturn
    cmpl    $1, -20(%esp)  # check EXCEPTION_HANDLER_CONTEXT.ExceptionDataFlag
    jz      ErrorCode
    jmp     *-16(%esp)
ErrorCode:
    subl    $4, %esp
    jmp     *-12(%esp)

DoReturn:
    cmpl    $0, ASM_PFX(mDoFarReturnFlag)
    jz      DoIret
    pushl   8(%esp)       # save EFLAGS
    addl    $16, %esp
    pushl   -8(%esp)      # save CS in new location
    pushl   -8(%esp)      # save EIP in new location
    pushl   -8(%esp)      # save EFLAGS in new location
    popfl                 # restore EFLAGS
    lret                  # far return

DoIret:
    iretl


#---------------------------------------;
# _AsmGetTemplateAddressMap             ;
#---------------------------------------;
# 
# Protocol prototype
#   AsmGetTemplateAddressMap (
#     EXCEPTION_HANDLER_TEMPLATE_MAP *AddressMap
#   );
#           
# Routine Description:
# 
#  Return address map of interrupt handler template so that C code can generate
#  interrupt table.
# 
# Arguments:
# 
# 
# Returns: 
# 
#   Nothing
#
# 
# Input:  [ebp][0]  = Original ebp
#         [ebp][4]  = Return address
#          
# Output: Nothing
#          
# Destroys: Nothing
#-----------------------------------------------------------------------------;
#-------------------------------------------------------------------------------------
#  AsmGetAddressMap (&AddressMap);
#-------------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(AsmGetTemplateAddressMap)
ASM_PFX(AsmGetTemplateAddressMap):

        pushl       %ebp
        movl        %esp,%ebp
        pushal

        movl        0x8(%ebp), %ebx
        movl        $Exception0Handle, (%ebx)
        movl        $(Exception1Handle - Exception0Handle), 0x4(%ebx)
        movl        $(HookAfterStubBegin), 0x8(%ebx)

        popal
        popl        %ebp
        ret
#-------------------------------------------------------------------------------------
#  AsmVectorNumFixup (*NewVectorAddr, VectorNum, *OldVectorAddr);
#-------------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(AsmVectorNumFixup)
ASM_PFX(AsmVectorNumFixup):
        movl  8(%esp), %eax
        movl  4(%esp), %ecx
        movb  %al, (VectorNum - HookAfterStubBegin)(%ecx)
        ret
