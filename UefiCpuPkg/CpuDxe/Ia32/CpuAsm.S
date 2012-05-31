#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials
#*   are licensed and made available under the terms and conditions of the BSD License
#*   which accompanies this distribution.  The full text of the license may be found at
#*   http://opensource.org/licenses/bsd-license.php
#*
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#*
#*    CpuAsm.S
#*
#*   Abstract:
#*
#------------------------------------------------------------------------------


#.MMX
#.XMM

#EXTRN ASM_PFX(mErrorCodeFlag):DWORD # Error code flags for exceptions


#
# point to the external interrupt vector table
#
ExternalVectorTablePtr:
    .byte      0, 0, 0, 0

ASM_GLOBAL ASM_PFX(InitializeExternalVectorTablePtr)
ASM_PFX(InitializeExternalVectorTablePtr):
    movl    4(%esp), %eax
    movl    %eax, ExternalVectorTablePtr
    ret

#------------------------------------------------------------------------------
# VOID
# SetCodeSelector (
#   UINT16 Selector
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(SetCodeSelector)
ASM_PFX(SetCodeSelector):
    movl    4(%esp), %ecx
    subl    $0x10, %esp 
    leal    setCodeSelectorLongJump, %eax 
    movl    %eax, (%esp)
    movw    %cx, 4(%esp)
    .byte   0xFF, 0x2C, 0x24   # jmp *(%esp)  note:(FWORD jmp) 
setCodeSelectorLongJump:
    addl    $0x10, %esp 
    ret

#------------------------------------------------------------------------------
# VOID
# SetDataSelectors (
#   UINT16 Selector
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(SetDataSelectors)
ASM_PFX(SetDataSelectors):
    movl    4(%esp), %ecx
    movw    %cx, %ss
    movw    %cx, %ds
    movw    %cx, %es
    movw    %cx, %fs
    movw    %cx, %gs
    ret

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
    movw    (%ecx), %cx
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

#; call into exception handler
    movl    ExternalVectorTablePtr, %eax  # get the interrupt vectors base
    orl     %eax, %eax                        # NULL?
    jz      nullExternalExceptionHandler

    mov     4(%ebp), %ecx
    movl    (%eax,%ecx,4), %eax
    orl     %eax, %eax                        # NULL?
    jz      nullExternalExceptionHandler

#; Prepare parameter and call
    movl    %esp, %edx
    pushl   %edx
    movl    4(%ebp), %edx
    pushl   %edx

    #
    # Call External Exception Handler
    #
    call    *%eax
    addl    $8, %esp

nullExternalExceptionHandler:

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


#END

