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

.equ  MSR_IA32_MISC_ENABLE, 0x1A0
.equ  MSR_EFER,             0xc0000080
.equ  MSR_EFER_XD,          0x800

.equ  CODE_SEL,          0x08
.equ  DATA_SEL,          0x20
.equ  TSS_SEL,           0x40

    .data

ASM_PFX(gcStmPsd):
            .ascii  "TXTPSSIG"
            .word      PSD_SIZE
            .word      1              # Version
            .long      0              # LocalApicId
            .byte      0x5            # Cr4Pse;Cr4Pae;Intel64Mode;ExecutionDisableOutsideSmrr
            .byte      0              # BIOS to STM
            .byte      0              # STM to BIOS
            .byte      0
            .word      CODE_SEL
            .word      DATA_SEL
            .word      DATA_SEL
            .word      DATA_SEL
            .word      TSS_SEL
            .word      0
            .quad      0              # SmmCr3
            .long      ASM_PFX(_OnStmSetup)
            .long      0
            .long      ASM_PFX(_OnStmTeardown)
            .long      0
            .quad      0              # SmmSmiHandlerRip - SMM guest entrypoint
            .quad      0              # SmmSmiHandlerRsp
            .quad      0
            .long      0
            .long      0x80010100     # RequiredStmSmmRevId
            .long      ASM_PFX(_OnException)
            .long      0
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
    movl  %esp, %ecx
    pushl %ecx
    call  ASM_PFX(SmmStmExceptionHandler)
    addl  $4, %esp

    movl  %eax, %ebx
    movl  $4, %eax
    .byte 0xf, 0x1, 0xc1 # VMCALL
    jmp .

ASM_GLOBAL ASM_PFX(_OnStmSetup)
ASM_PFX(_OnStmSetup):
#
# Check XD disable bit
#
    xorl    %esi, %esi
    movl    $ASM_PFX(gStmXdSupported), %eax
    movb    (%eax), %al
    cmpb    $0, %al
    jz      StmXdDone1
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    movl    %edx, %esi                     # save MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx                    # MSR_IA32_MISC_ENABLE[34]
    jz      L13
    andw    $0x0FFFB, %dx                  # clear XD Disable bit if it is set
    wrmsr
L13:
    movl    $MSR_EFER, %ecx
    rdmsr
    orw     $MSR_EFER_XD,%ax               # enable NXE
    wrmsr
StmXdDone1:
    push    %esi

  call ASM_PFX(SmmStmSetup)

    movl    $ASM_PFX(gStmXdSupported), %eax
    movb    (%eax), %al
    cmpb    $0, %al
    jz      L14
    popl    %edx                        # get saved MSR_IA32_MISC_ENABLE[63-32]
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
    xorl    %esi, %esi
    movl    $ASM_PFX(gStmXdSupported), %eax
    movb    (%eax), %al
    cmpb    $0, %al
    jz      StmXdDone2
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    movl    %edx, %esi                     # save MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx                    # MSR_IA32_MISC_ENABLE[34]
    jz      L15
    andw    $0x0FFFB, %dx                  # clear XD Disable bit if it is set
    wrmsr
L15:
    movl    $MSR_EFER, %ecx
    rdmsr
    orw     $MSR_EFER_XD,%ax               # enable NXE
    wrmsr
StmXdDone2:
    push    %esi

  call ASM_PFX(SmmStmTeardown)

    movl    $ASM_PFX(gStmXdSupported), %eax
    movb    (%eax), %al
    cmpb    $0, %al
    jz      L16
    popl    %edx                        # get saved MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx
    jz      L16
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    orw     $BIT2, %dx                  # set XD Disable bit if it was set before entering into SMM
    wrmsr
L16:

  rsm
