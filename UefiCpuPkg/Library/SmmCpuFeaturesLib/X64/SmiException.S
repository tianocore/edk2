#------------------------------------------------------------------------------
#
# Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>
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
#   SmiException.S
#
# Abstract:
#
#   Exception handlers used in SM mode
#
#------------------------------------------------------------------------------

ASM_GLOBAL  ASM_PFX(gcStmPsd)

ASM_GLOBAL  ASM_PFX(SmmStmExceptionHandler)
ASM_GLOBAL  ASM_PFX(SmmStmSetup)
ASM_GLOBAL  ASM_PFX(SmmStmTeardown)

.equ  CODE_SEL,             0x38
.equ  DATA_SEL,             0x20
.equ  TR_SEL,               0x40

.equ  MSR_IA32_MISC_ENABLE, 0x1A0
.equ  MSR_EFER,             0x0c0000080
.equ  MSR_EFER_XD,          0x0800

    .data

#
# This structure serves as a template for all processors.
#
ASM_PFX(gcStmPsd):
            .ascii     "TXTPSSIG"
            .word      PSD_SIZE
            .word      1              # Version
            .long      0              # LocalApicId
            .byte      0xF            # Cr4Pse;Cr4Pae;Intel64Mode;ExecutionDisableOutsideSmrr
            .byte      0              # BIOS to STM
            .byte      0              # STM to BIOS
            .byte      0
            .word      CODE_SEL
            .word      DATA_SEL
            .word      DATA_SEL
            .word      DATA_SEL
            .word      TR_SEL
            .word      0
            .quad      0              # SmmCr3
            .quad      ASM_PFX(_OnStmSetup)
            .quad      ASM_PFX(_OnStmTeardown)
            .quad      0              # SmmSmiHandlerRip - SMM guest entrypoint
            .quad      0              # SmmSmiHandlerRsp
            .quad      0
            .long      0
            .long      0x80010100     # RequiredStmSmmRevId
            .quad      ASM_PFX(_OnException)
            .quad      0              # ExceptionStack
            .word      DATA_SEL
            .word      0x1F           # ExceptionFilter
            .long      0
            .quad      0
            .quad      0              # BiosHwResourceRequirementsPtr
            .quad      0              # AcpiRsdp
            .byte      0              # PhysicalAddressBits
.equ  PSD_SIZE,  . - ASM_PFX(gcStmPsd)

    .text
#------------------------------------------------------------------------------
# SMM Exception handlers
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(_OnException)
ASM_PFX(_OnException):
    movq %rsp, %rcx
    subq $0x28, %rsp
    call ASM_PFX(SmmStmExceptionHandler)
    addq $0x28, %rsp
    movl %eax, %ebx
    movl $4, %eax
    .byte  0xf, 0x1, 0xc1 # VMCALL
    jmp .

ASM_GLOBAL ASM_PFX(_OnStmSetup)
ASM_PFX(_OnStmSetup):
#
# Check XD disable bit
#
    xorq    %r8, %r8
    movabsq $ASM_PFX(gStmXdSupported), %rax
    movb    (%rax), %al
    cmpb    $0, %al
    jz      StmXdDone1
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    movq    %rdx, %r8                  # save MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx                # MSR_IA32_MISC_ENABLE[34]
    jz      L13
    andw    $0x0FFFB, %dx              # clear XD Disable bit if it is set
    wrmsr
L13:
    movl    $MSR_EFER, %ecx
    rdmsr
    orw     $MSR_EFER_XD,%ax            # enable NXE
    wrmsr
StmXdDone1:
    pushq   %r8

  subq $0x20, %rsp
  call ASM_PFX(SmmStmSetup)
  addq 0x20, %rsp

    movabsq $ASM_PFX(gStmXdSupported), %rax
    movb    (%rax), %al
    cmpb    $0, %al
    jz      L14
    popq    %rdx                        # get saved MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx
    jz      L14
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    orw     $BIT2, %dx                  # set XD Disable bit if it was set before entering into SMM
    wrmsr
L14:

  rsm

ASM_GLOBAL ASM_PFX(_OnStmTeardown)
ASM_PFX(_OnStmTeardown):
#
# Check XD disable bit
#
    xorq    %r8, %r8
    movabsq $ASM_PFX(gStmXdSupported), %rax
    movb    (%rax), %al
    cmpb    $0, %al
    jz      StmXdDone2
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    movq    %rdx, %r8                  # save MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx                # MSR_IA32_MISC_ENABLE[34]
    jz      L15
    andw    $0x0FFFB, %dx              # clear XD Disable bit if it is set
    wrmsr
L15:
    movl    $MSR_EFER, %ecx
    rdmsr
    orw     $MSR_EFER_XD,%ax            # enable NXE
    wrmsr
StmXdDone2:
    pushq   %r8

  subq $0x20, %rsp
  call ASM_PFX(SmmStmTeardown)
  addq $0x20, %rsp

    movabsq $ASM_PFX(gStmXdSupported), %rax
    movb    (%rax), %al
    cmpb    $0, %al
    jz      L16
    popq    %rdx                        # get saved MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx
    jz      L16
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    orw     $BIT2, %dx                  # set XD Disable bit if it was set before entering into SMM
    wrmsr
L16:

  rsm

