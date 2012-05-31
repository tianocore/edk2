#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
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

#EXTRN ASM_PFX(mErrorCodeFlag):DWORD # Error code flags for exceptions

.text

#
# exception handler stub table
#
Exception0Handle:
    pushl   $0
    jmp     ASM_PFX(CommonInterruptEntry)
Exception1Handle:
    pushl   $1
    jmp     ASM_PFX(CommonInterruptEntry)
Exception2Handle:
    pushl   $2
    jmp     ASM_PFX(CommonInterruptEntry)
Exception3Handle:
    pushl    $3
    jmp     ASM_PFX(CommonInterruptEntry)
Exception4Handle:
    pushl    $4
    jmp     ASM_PFX(CommonInterruptEntry)
Exception5Handle:
    pushl    $5
    jmp     ASM_PFX(CommonInterruptEntry)
Exception6Handle:
    pushl    $6
    jmp     ASM_PFX(CommonInterruptEntry)
Exception7Handle:
    pushl    $7
    jmp     ASM_PFX(CommonInterruptEntry)
Exception8Handle:
    pushl    $8
    jmp     ASM_PFX(CommonInterruptEntry)
Exception9Handle:
    pushl    $9
    jmp     ASM_PFX(CommonInterruptEntry)
Exception10Handle:
    pushl    $10
    jmp     ASM_PFX(CommonInterruptEntry)
Exception11Handle:
    pushl    $11
    jmp     ASM_PFX(CommonInterruptEntry)
Exception12Handle:
    pushl    $12
    jmp     ASM_PFX(CommonInterruptEntry)
Exception13Handle:
    pushl    $13
    jmp     ASM_PFX(CommonInterruptEntry)
Exception14Handle:
    pushl    $14
    jmp     ASM_PFX(CommonInterruptEntry)
Exception15Handle:
    pushl    $15
    jmp     ASM_PFX(CommonInterruptEntry)
Exception16Handle:
    pushl    $16
    jmp     ASM_PFX(CommonInterruptEntry)
Exception17Handle:
    pushl    $17
    jmp     ASM_PFX(CommonInterruptEntry)
Exception18Handle:
    pushl    $18
    jmp     ASM_PFX(CommonInterruptEntry)
Exception19Handle:
    pushl    $19
    jmp     ASM_PFX(CommonInterruptEntry)
Exception20Handle:
    pushl    $20
    jmp     ASM_PFX(CommonInterruptEntry)
Exception21Handle:
    pushl    $21
    jmp     ASM_PFX(CommonInterruptEntry)
Exception22Handle:
    pushl    $22
    jmp     ASM_PFX(CommonInterruptEntry)
Exception23Handle:
    pushl    $23
    jmp     ASM_PFX(CommonInterruptEntry)
Exception24Handle:
    pushl    $24
    jmp     ASM_PFX(CommonInterruptEntry)
Exception25Handle:
    pushl    $25
    jmp     ASM_PFX(CommonInterruptEntry)
Exception26Handle:
    pushl    $26
    jmp     ASM_PFX(CommonInterruptEntry)
Exception27Handle:
    pushl    $27
    jmp     ASM_PFX(CommonInterruptEntry)
Exception28Handle:
    pushl    $28
    jmp     ASM_PFX(CommonInterruptEntry)
Exception29Handle:
    pushl    $29
    jmp     ASM_PFX(CommonInterruptEntry)
Exception30Handle:
    pushl    $30
    jmp     ASM_PFX(CommonInterruptEntry)
Exception31Handle:
    pushl    $31
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
    # Get the return address of call, actually, it is the
    # address of vector number.
    #
    xchgl   (%esp), %ecx
    andl    $0x0FFFF, %ecx
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
    movl    %cr4, %eax
    orl     $0x208, %eax
    movl    %eax, %cr4
    pushl   %eax
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
    .byte      0x0f, 0x0ae, 0x07 #fxsave [edi]

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
    .byte      0x0f, 0x0ae, 0x0e # fxrstor [esi]
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

    movl    %ebp, %esp
    popl    %ebp
    addl    $8, %esp
    iretl


#---------------------------------------;
# _GetTemplateAddressMap                  ;
#----------------------------------------------------------------------------;
# 
# Protocol prototype
#   GetTemplateAddressMap (
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
ASM_GLOBAL ASM_PFX(GetTemplateAddressMap)
ASM_PFX(GetTemplateAddressMap):

        pushl       %ebp
        movl        %esp,%ebp
        pushal

        movl        0x8(%ebp), %ebx
        movl        $Exception0Handle, (%ebx)
        movl        $(Exception1Handle - Exception0Handle), 0x4(%ebx)

        popal
        popl        %ebp
        ret

