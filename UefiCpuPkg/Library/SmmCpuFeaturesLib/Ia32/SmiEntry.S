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
ASM_GLOBAL  ASM_PFX(FeaturePcdGet (PcdCpuSmmStackGuard))
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
.equ            DSC_OTHERSEG, 0x1A

.equ            PROTECT_MODE_CS, 0x08
.equ            PROTECT_MODE_DS, 0x20
.equ            TSS_SEGMENT,     0x40

    .text
ASM_PFX(gcStmSmiHandlerTemplate):

_StmSmiEntryPoint:
    .byte 0xbb                          # mov bx, imm16
    .word _StmGdtDesc - _StmSmiEntryPoint + 0x8000
    .byte 0x2e,0xa1                     # mov ax, cs:[offset16]
    .word DSC_OFFSET + DSC_GDTSIZ
    decl    %eax
    movl    %eax, %cs:(%edi)            # mov cs:[bx], ax
    .byte 0x66,0x2e,0xa1                # mov eax, cs:[offset16]
    .word   DSC_OFFSET + DSC_GDTPTR
    movw    %ax, %cs:2(%edi)
    movw    %ax, %bp                    # ebp = GDT base
    .byte 0x66
    lgdt    %cs:(%edi)
# Patch ProtectedMode Segment
    .byte   0xb8                        # mov ax, imm16
    .word   PROTECT_MODE_CS             # set AX for segment directly
    movl    %eax, %cs:-2(%edi)          # mov cs:[bx - 2], ax
# Patch ProtectedMode entry
    .byte 0x66, 0xbf                    # mov edi, SMBASE
ASM_PFX(gStmSmbase): .space 4
    .byte 0x67
    lea     ((Start32bit - _StmSmiEntryPoint) + 0x8000)(%edi), %ax
    movw     %ax, %cs:-6(%edi)
    movl    %cr0, %ebx
    .byte 0x66
    andl    $0x9ffafff3, %ebx
    .byte 0x66
    orl     $0x23, %ebx
    movl    %ebx, %cr0
    .byte 0x66,0xea
    .space  4
    .space  2
_StmGdtDesc:   .space 4
            .space 2

Start32bit:
    movw    $PROTECT_MODE_DS, %ax
    movl    %eax,%ds
    movl    %eax,%es
    movl    %eax,%fs
    movl    %eax,%gs
    movl    %eax,%ss
    .byte   0xbc                          # mov esp, imm32
ASM_PFX(gStmSmiStack): .space 4
    movl    $ASM_PFX(gStmSmiHandlerIdtr), %eax
    lidt    (%eax)
    jmp     ProtFlatMode

ProtFlatMode:
    .byte   0xb8                           # mov eax, imm32
ASM_PFX(gStmSmiCr3): .space 4
    movl    %eax, %cr3
#
# Need to test for CR4 specific bit support
#
    movl    $1, %eax
    cpuid                                  # use CPUID to determine if specific CR4 bits are supported
    xorl    %eax, %eax                     # Clear EAX
    testl   $BIT2, %edx                    # Check for DE capabilities
    jz      L8
    orl     $BIT3, %eax
L8:
    testl   $BIT6, %edx                    # Check for PAE capabilities
    jz      L9
    orl     $BIT5, %eax
L9:
    testl   $BIT7, %edx                    # Check for MCE capabilities
    jz      L10
    orl     $BIT6, %eax
L10:
    testl   $BIT24, %edx                   # Check for FXSR capabilities
    jz      L11
    orl     $BIT9, %eax
L11:
    testl   $BIT25, %edx                   # Check for SSE capabilities
    jz      L12
    orl     $BIT10, %eax
L12:                                       # as cr4.PGE is not set here, refresh cr3
    movl    %eax, %cr4                     # in PreModifyMtrrs() to flush TLB.

    cmpb    $0, ASM_PFX(FeaturePcdGet (PcdCpuSmmStackGuard))
    jz      L5
# Load TSS
    movb    $0x89, (TSS_SEGMENT + 5)(%ebp) # clear busy flag
    movl    $TSS_SEGMENT, %eax
    ltrw    %ax
L5:

# enable NXE if supported
    .byte   0xb0                           # mov al, imm8
ASM_PFX(gStmXdSupported): .byte 1
    cmpb    $0, %al
    jz      SkipXd
#
# Check XD disable bit
#
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    pushl   %edx                           # save MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx                    # MSR_IA32_MISC_ENABLE[34]
    jz      L13
    andw    $0x0FFFB, %dx                  # clear XD Disable bit if it is set
    wrmsr
L13:
    movl    $MSR_EFER, %ecx
    rdmsr
    orw     $MSR_EFER_XD,%ax               # enable NXE
    wrmsr
    jmp     XdDone
SkipXd:
    subl    $4, %esp
XdDone:

    movl    %cr0, %ebx
    orl     $0x080010023, %ebx             # enable paging + WP + NE + MP + PE
    movl    %ebx, %cr0
    leal    DSC_OFFSET(%edi),%ebx
    movw    DSC_DS(%ebx),%ax
    movl    %eax, %ds
    movw    DSC_OTHERSEG(%ebx),%ax
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movw    DSC_SS(%ebx),%ax
    movl    %eax, %ss

CommonHandler:
    movl    4(%esp), %ebx

    pushl   %ebx
    movl    $ASM_PFX(CpuSmmDebugEntry), %eax
    call    *%eax
    addl    $4, %esp

    pushl   %ebx
    movl    $ASM_PFX(SmiRendezvous), %eax
    call    *%eax
    addl    $4, %esp

    pushl   %ebx
    movl    $ASM_PFX(CpuSmmDebugExit), %eax
    call    *%eax
    addl    $4, %esp

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

_StmSmiHandler:
#
# Check XD disable bit
#
    xorl    %esi, %esi
    movl    $ASM_PFX(gStmXdSupported), %eax
    movb    (%eax), %al
    cmpb    $0, %al
    jz      StmXdDone
    movl    $MSR_IA32_MISC_ENABLE, %ecx
    rdmsr
    movl    %edx, %esi                     # save MSR_IA32_MISC_ENABLE[63-32]
    testl   $BIT2, %edx                    # MSR_IA32_MISC_ENABLE[34]
    jz      L14
    andw    $0x0FFFB, %dx                  # clear XD Disable bit if it is set
    wrmsr
L14:
    movl    $MSR_EFER, %ecx
    rdmsr
    orw     $MSR_EFER_XD,%ax               # enable NXE
    wrmsr
StmXdDone:
    push    %esi

    # below step is needed, because STM does not run above code.
    # we have to run below code to set IDT/CR0/CR4
    movl    $ASM_PFX(gStmSmiHandlerIdtr), %eax
    lidt    (%eax)

    movl    %cr0, %eax
    orl     $0x80010023, %eax           # enable paging + WP + NE + MP + PE
    movl    %eax, %cr0
#
# Need to test for CR4 specific bit support
#
    movl    $1, %eax
    cpuid                               # use CPUID to determine if specific CR4 bits are supported
    movl    %cr4, %eax                  # init EAX
    testl   $BIT2, %edx                 # Check for DE capabilities
    jz      L28
    orl     $BIT3, %eax
L28:
    testl   $BIT6, %edx                 # Check for PAE capabilities
    jz      L29
    orl     $BIT5, %eax
L29:
    testl   $BIT7, %edx                 # Check for MCE capabilities
    jz      L30
    orl     $BIT6, %eax
L30:
    testl   $BIT24, %edx                # Check for FXSR capabilities
    jz      L31
    orl     $BIT9, %eax
L31:
    testl   $BIT25, %edx                # Check for SSE capabilities
    jz      L32
    orl     $BIT10, %eax
L32:                                    # as cr4.PGE is not set here, refresh cr3
    movl    %eax, %cr4                  # in PreModifyMtrrs() to flush TLB.
    # STM init finish
    jmp     CommonHandler


ASM_PFX(gcStmSmiHandlerSize)  :  .word      . - _StmSmiEntryPoint
ASM_PFX(gcStmSmiHandlerOffset):  .word      _StmSmiHandler - _StmSmiEntryPoint

