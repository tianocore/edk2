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
#   SmiEntry.S
#
# Abstract:
#
#   Code template of the SMI handler for a particular processor
#
#------------------------------------------------------------------------------

ASM_GLOBAL  ASM_PFX(gcStmSmiHandlerTemplate)
ASM_GLOBAL  ASM_PFX(gcStmSmiHandlerSize)
ASM_GLOBAL  ASM_PFX(gcStmSmiHandlerOffset)
ASM_GLOBAL  ASM_PFX(gStmSmiCr3)
ASM_GLOBAL  ASM_PFX(gStmSmiStack)
ASM_GLOBAL  ASM_PFX(gStmSmbase)
ASM_GLOBAL  ASM_PFX(gStmXdSupported)
ASM_GLOBAL  ASM_PFX(gStmSmiHandlerIdtr)

.equ            MSR_IA32_MISC_ENABLE, 0x1A0
.equ            MSR_EFER, 0xc0000080
.equ            MSR_EFER_XD, 0x800

#
# Constants relating to TXT_PROCESSOR_SMM_DESCRIPTOR
#
.equ            DSC_OFFSET, 0xfb00
.equ            DSC_GDTPTR, 0x48
.equ            DSC_GDTSIZ, 0x50
.equ            DSC_CS, 0x14
.equ            DSC_DS, 0x16
.equ            DSC_SS, 0x18
.equ            DSC_OTHERSEG, 0x1a
#
# Constants relating to CPU State Save Area
#
.equ            SSM_DR6,   0xffd0
.equ            SSM_DR7,   0xffc8

.equ            PROTECT_MODE_CS, 0x08
.equ            PROTECT_MODE_DS, 0x20
.equ            LONG_MODE_CS, 0x38
.equ            TSS_SEGMENT, 0x40
.equ            GDT_SIZE, 0x50

    .text

ASM_PFX(gcStmSmiHandlerTemplate):

_StmSmiEntryPoint:
    #
    # The encoding of BX in 16-bit addressing mode is the same as of RDI in 64-
    # bit addressing mode. And that coincidence has been used in the following
    # "64-bit like" 16-bit code. Be aware that once RDI is referenced as a
    # base address register, it is actually BX that is referenced.
    #
    .byte 0xbb                          # mov bx, imm16
    .word _StmGdtDesc - _StmSmiEntryPoint + 0x8000
    #
    # fix GDT descriptor
    #
    .byte 0x2e,0xa1                     # mov ax, cs:[offset16]
    .word      DSC_OFFSET + DSC_GDTSIZ
    .byte 0x48                          # dec ax
    .byte 0x2e
    movl    %eax, (%rdi)                # mov cs:[bx], ax
    .byte 0x66,0x2e,0xa1                # mov eax, cs:[offset16]
    .word      DSC_OFFSET + DSC_GDTPTR
    .byte 0x2e
    movw    %ax, 2(%rdi)
    .byte 0x66,0x2e
    lgdt    (%rdi)
    #
    # Patch ProtectedMode Segment
    #
    .byte 0xb8
    .word PROTECT_MODE_CS
    .byte 0x2e
    movl    %eax, -2(%rdi)
    #
    # Patch ProtectedMode entry
    #
    .byte 0x66, 0xbf                    # mov edi, SMBASE
ASM_PFX(gStmSmbase): .space 4
    lea     ((ProtectedMode - _StmSmiEntryPoint) + 0x8000)(%edi), %ax
    .byte 0x2e
    movw    %ax, -6(%rdi)
    #
    # Switch into ProtectedMode
    #
    movq    %cr0, %rbx
    .byte 0x66
    andl    $0x9ffafff3, %ebx
    .byte 0x66
    orl     $0x00000023, %ebx

    movq    %rbx, %cr0
    .byte 0x66, 0xea
    .space 6

_StmGdtDesc:    .space  6

ProtectedMode:
    movw    $PROTECT_MODE_DS, %ax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    %eax, %ss
    .byte   0xbc                       # mov esp, imm32
ASM_PFX(gStmSmiStack):   .space  4
    jmp     ProtFlatMode

ProtFlatMode:
    .byte   0xb8
ASM_PFX(gStmSmiCr3):    .space  4
    movq    %rax, %cr3
    movl    $0x668,%eax                 # as cr4.PGE is not set here, refresh cr3
    movq    %rax, %cr4                  # in PreModifyMtrrs() to flush TLB.
# Load TSS
    subl    $8, %esp                    # reserve room in stack
    sgdt    (%rsp)
    movl    2(%rsp), %eax               # eax = GDT base
    addl    $8, %esp
    movb    $0x89, %dl
    movb    %dl, (TSS_SEGMENT + 5)(%rax) # clear busy flag
    movl    $TSS_SEGMENT, %eax
    ltr     %ax

# enable NXE if supported
    .byte   0xb0                        # mov al, imm8
ASM_PFX(gStmXdSupported): .byte 1
    cmpb    $0, %al
    jz      SkipXd
#
# Check XD disable bit
#
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    subl    $4, %esp
    pushq   %rdx                       # save MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx                # MSR_IA32_MISC_ENABLE[34]
    jz      L13
    andw    $0x0FFFB, %dx              # clear XD Disable bit if it is set
    wrmsr
L13:
    movl    $MSR_EFER, %ecx
    rdmsr
    orw     $MSR_EFER_XD,%ax            # enable NXE
    wrmsr
    jmp     XdDone
SkipXd:
    subl    $8, %esp
XdDone:

    #
    # Switch to LongMode
    #
    pushq    $LONG_MODE_CS                # push cs hardcore here
    call     Base                         # push return address for retf later
Base:
    addl    $(LongMode - Base), (%rsp)  # offset for far retf, seg is the 1st arg

    movl    $MSR_EFER, %ecx
    rdmsr
    orb     $1,%ah                      # enable LME
    wrmsr
    movq    %cr0, %rbx
    orl     $0x080010023, %ebx          # enable paging + WP + NE + MP + PE
    movq    %rbx, %cr0
    retf
LongMode:                               # long mode (64-bit code) starts here
    movabsq $ASM_PFX(gStmSmiHandlerIdtr), %rax
    lidt    (%rax)
    lea     (DSC_OFFSET)(%rdi), %ebx
    movw    DSC_DS(%rbx), %ax
    movl    %eax,%ds
    movw    DSC_OTHERSEG(%rbx), %ax
    movl    %eax,%es
    movl    %eax,%fs
    movl    %eax,%gs
    movw    DSC_SS(%rbx), %ax
    movl    %eax,%ss

CommonHandler:
    movq    8(%rsp), %rbx
    # Save FP registers

    subq    $0x200, %rsp
    .byte   0x48                        # FXSAVE64
    fxsave  (%rsp)

    addq    $-0x20, %rsp

    movq    %rbx, %rcx
    movabsq $ASM_PFX(CpuSmmDebugEntry), %rax
    call    *%rax

    movq    %rbx, %rcx
    movabsq $ASM_PFX(SmiRendezvous), %rax
    call    *%rax

    movq    %rbx, %rcx
    movabsq $ASM_PFX(CpuSmmDebugExit), %rax
    call    *%rax

    addq    $0x20, %rsp

    #
    # Restore FP registers
    #
    .byte   0x48                        # FXRSTOR64
    fxrstor (%rsp)

    addq    $0x200, %rsp

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

_StmSmiHandler:
#
# Check XD disable bit
#
    xorq    %r8, %r8
    movabsq $ASM_PFX(gStmXdSupported), %rax
    movb    (%rax), %al
    cmpb    $0, %al
    jz      StmXdDone
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    movq    %rdx, %r8                  # save MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx                # MSR_IA32_MISC_ENABLE[34]
    jz      L14
    andw    $0x0FFFB, %dx              # clear XD Disable bit if it is set
    wrmsr
L14:
    movl    $MSR_EFER, %ecx
    rdmsr
    orw     $MSR_EFER_XD,%ax            # enable NXE
    wrmsr
StmXdDone:
    pushq   %r8

    # below step is needed, because STM does not run above code.
    # we have to run below code to set IDT/CR0/CR4
    movabsq $ASM_PFX(gStmSmiHandlerIdtr), %rax
    lidt    (%rax)

    movq    %cr0, %rax
    orl     $0x80010023, %eax
    movq    %rax, %cr0
    movq    %cr4, %rax
    movl    $0x668, %eax                # as cr4.PGE is not set here, refresh cr3
    movq    %rax, %cr4                  # in PreModifyMtrrs() to flush TLB.
    # STM init finish
    jmp     CommonHandler

ASM_PFX(gcStmSmiHandlerSize)  :  .word      . - _StmSmiEntryPoint
ASM_PFX(gcStmSmiHandlerOffset):  .word      _StmSmiHandler - _StmSmiEntryPoint
