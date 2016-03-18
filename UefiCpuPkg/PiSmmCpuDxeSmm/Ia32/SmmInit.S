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
#   SmmInit.S
#
# Abstract:
#
#   Functions for relocating SMBASE's for all processors
#
#------------------------------------------------------------------------------

ASM_GLOBAL   ASM_PFX(gSmmCr0)
ASM_GLOBAL   ASM_PFX(gSmmCr3)
ASM_GLOBAL   ASM_PFX(gSmmCr4)
ASM_GLOBAL   ASM_PFX(gcSmmInitTemplate)
ASM_GLOBAL   ASM_PFX(gcSmmInitSize)
ASM_GLOBAL   ASM_PFX(gSmmJmpAddr)
ASM_GLOBAL   ASM_PFX(SmmRelocationSemaphoreComplete)
ASM_GLOBAL   ASM_PFX(gSmmInitStack)
ASM_GLOBAL   ASM_PFX(gcSmiInitGdtr)

.equ            PROTECT_MODE_CS, 0x08
.equ            PROTECT_MODE_DS, 0x20

    .text

ASM_PFX(gcSmiInitGdtr):
            .word      0
            .quad      0

SmmStartup:
    .byte   0x66,0xb8
ASM_PFX(gSmmCr3):    .space     4
    movl    %eax, %cr3
    .byte   0x67,0x66
    lgdt    %cs:(ASM_PFX(gcSmiInitGdtr) - SmmStartup)(%ebp)
    .byte   0x66,0xb8
ASM_PFX(gSmmCr4):    .space     4
    movl    %eax, %cr4
    .byte   0x66,0xb8
ASM_PFX(gSmmCr0):    .space     4
    .byte   0xbf, PROTECT_MODE_DS, 0      # mov di, PROTECT_MODE_DS
    movl    %eax, %cr0
    .byte   0x66,0xea                     # jmp far [ptr48]
ASM_PFX(gSmmJmpAddr): .long Start32bit
    .word   PROTECT_MODE_CS
Start32bit:
    movl    %edi,%ds
    movl    %edi,%es
    movl    %edi,%fs
    movl    %edi,%gs
    movl    %edi,%ss
    .byte   0xbc                        # mov esp, imm32
ASM_PFX(gSmmInitStack):  .space  4
    call    ASM_PFX(SmmInitHandler)
    rsm

ASM_PFX(gcSmmInitTemplate):

_SmmInitTemplate:
    .byte 0x66
    movl    $SmmStartup, %ebp
    .byte 0x66, 0x81, 0xed, 0, 0, 3, 0  # sub ebp, 0x30000
    jmp     *%bp                        # jmp ebp actually

ASM_PFX(gcSmmInitSize):   .word  . - ASM_PFX(gcSmmInitTemplate)


ASM_PFX(SmmRelocationSemaphoreComplete):
    pushl   %eax
    movl    ASM_PFX(mRebasedFlag), %eax
    movb    $1, (%eax)
    popl    %eax
    jmp     *ASM_PFX(mSmmRelocationOriginalAddress)
