#------------------------------------------------------------------------------
#
# Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
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

ASM_GLOBAL  ASM_PFX(gcSmiHandlerTemplate)
ASM_GLOBAL  ASM_PFX(gcSmiHandlerSize)
ASM_GLOBAL  ASM_PFX(gSmiCr3)
ASM_GLOBAL  ASM_PFX(gSmiStack)
ASM_GLOBAL  ASM_PFX(gSmbase)
ASM_GLOBAL  ASM_PFX(gSmiHandlerIdtr)

#
# Constants relating to PROCESSOR_SMM_DESCRIPTOR
#
.equ            DSC_OFFSET, 0xfb00
.equ            DSC_GDTPTR, 0x30
.equ            DSC_GDTSIZ, 0x38
.equ            DSC_CS, 14
.equ            DSC_DS, 16
.equ            DSC_SS, 18
.equ            DSC_OTHERSEG, 20
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

ASM_PFX(gcSmiHandlerTemplate):

_SmiEntryPoint:
    #
    # The encoding of BX in 16-bit addressing mode is the same as of RDI in 64-
    # bit addressing mode. And that coincidence has been used in the following
    # "64-bit like" 16-bit code. Be aware that once RDI is referenced as a
    # base address register, it is actually BX that is referenced.
    #
    .byte 0xbb                          # mov bx, imm16
    .word _GdtDesc - _SmiEntryPoint + 0x8000
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
ASM_PFX(gSmbase): .space 4
    lea     ((ProtectedMode - _SmiEntryPoint) + 0x8000)(%edi), %ax
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

_GdtDesc:    .space  6

ProtectedMode:
    movw    $PROTECT_MODE_DS, %ax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    %eax, %ss
    .byte   0xbc                       # mov esp, imm32
ASM_PFX(gSmiStack):   .space  4
    jmp     ProtFlatMode

ProtFlatMode:
    .byte   0xb8
ASM_PFX(gSmiCr3):    .space  4
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

    #
    # Switch to LongMode
    #
    pushq    $LONG_MODE_CS                # push cs hardcore here
    call     Base                         # push return address for retf later
Base:
    addl    $(LongMode - Base), (%rsp)  # offset for far retf, seg is the 1st arg
    movl    $0xc0000080, %ecx
    rdmsr
    orb     $1,%ah
    wrmsr
    movq    %cr0, %rbx
    orl     $0x080010000, %ebx          # enable paging + WP
    movq    %rbx, %cr0
    retf
LongMode:                               # long mode (64-bit code) starts here
    movabsq $ASM_PFX(gSmiHandlerIdtr), %rax
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
#   jmp     _SmiHandler                 ; instruction is not needed

_SmiHandler:
    movq    (%rsp), %rbx
    # Save FP registers

    subq    $0x208, %rsp
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

    rsm

ASM_PFX(gcSmiHandlerSize):    .word      . - _SmiEntryPoint
