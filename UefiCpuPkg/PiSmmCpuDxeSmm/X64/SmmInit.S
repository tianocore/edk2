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
ASM_GLOBAL   ASM_PFX(gSmmJmpAddr)
ASM_GLOBAL   ASM_PFX(gcSmmInitTemplate)
ASM_GLOBAL   ASM_PFX(gcSmmInitSize)
ASM_GLOBAL   ASM_PFX(mRebasedFlagAddr32)
ASM_GLOBAL   ASM_PFX(SmmRelocationSemaphoreComplete)
ASM_GLOBAL   ASM_PFX(SmmRelocationSemaphoreComplete32)
ASM_GLOBAL   ASM_PFX(mSmmRelocationOriginalAddressPtr32)
ASM_GLOBAL   ASM_PFX(gSmmInitStack)
ASM_GLOBAL   ASM_PFX(gcSmiInitGdtr)


    .text

ASM_PFX(gcSmiInitGdtr):
            .word      0
            .quad      0

SmmStartup:
    .byte 0x66,0xb8                     # mov eax, imm32
ASM_PFX(gSmmCr3):    .space     4
    movq    %rax, %cr3
    .byte 0x66,0x2e
    lgdt    (ASM_PFX(gcSmiInitGdtr) - SmmStartup)(%ebp)
    .byte 0x66,0xb8                     # mov eax, imm32
ASM_PFX(gSmmCr4):    .space     4
    orb     $2, %ah                     # enable XMM registers access
    movq    %rax, %cr4
    .byte 0x66
    movl    $0xc0000080,%ecx            # IA32_EFER MSR
    rdmsr
    orb     $1,%ah                      # set LME bit
    wrmsr
    .byte 0x66,0xb8                     # mov eax, imm32
ASM_PFX(gSmmCr0):    .space     4
    movq    %rax, %cr0
    .byte 0x66,0xea                     # far jmp to long mode
ASM_PFX(gSmmJmpAddr): .quad      LongMode
LongMode:                               # long-mode starts here
    .byte 0x48,0xbc                     # mov rsp, imm64
ASM_PFX(gSmmInitStack):  .space  8
    andw  $0xfff0, %sp                  # make sure RSP is 16-byte aligned
    #
    # Accoring to X64 calling convention, XMM0~5 are volatile, we need to save
    # them before calling C-function.
    #
    subq     $0x60, %rsp
    movdqa   %xmm0, 0x0(%rsp)
    movdqa   %xmm1, 0x10(%rsp)
    movdqa   %xmm2, 0x20(%rsp)
    movdqa   %xmm3, 0x30(%rsp)
    movdqa   %xmm4, 0x40(%rsp)
    movdqa   %xmm5, 0x50(%rsp)


    addq  $-0x20, %rsp
    call  ASM_PFX(SmmInitHandler)
    addq  $0x20, %rsp
    #
    # Restore XMM0~5 after calling C-function.
    #
    movdqa  0x0(%rsp), %xmm0
    movdqa  0x10(%rsp), %xmm1
    movdqa  0x20(%rsp), %xmm2
    movdqa  0x30(%rsp), %xmm3
    movdqa  0x40(%rsp), %xmm4
    movdqa  0x50(%rsp), %xmm5

    rsm

ASM_PFX(gcSmmInitTemplate):

_SmmInitTemplate:
    .byte 0x66,0x2e,0x8b,0x2e           # mov ebp, cs:[@F]
    .word L1 - _SmmInitTemplate + 0x8000
    .byte 0x66, 0x81, 0xed, 0, 0, 3, 0  # sub ebp, 0x30000
    jmp     *%bp                        # jmp ebp actually
L1:
    .quad      SmmStartup

ASM_PFX(gcSmmInitSize):   .word  . - ASM_PFX(gcSmmInitTemplate)

ASM_PFX(SmmRelocationSemaphoreComplete):
    # Create a simple stack frame to store RAX and the original RSM location
    pushq   %rax  # Used to store return address
    pushq   %rax

    # Load the original RSM location onto stack
    movabsq $ASM_PFX(mSmmRelocationOriginalAddress), %rax
    movq    (%rax), %rax
    movq    %rax, 0x08(%rsp)

    # Update rebase flag
    movabsq $ASM_PFX(mRebasedFlag), %rax
    movq    (%rax), %rax
    movb    $1, (%rax)

    #restore RAX and return to original RSM location
    popq    %rax
    retq

#
# Semaphore code running in 32-bit mode
#
ASM_PFX(SmmRelocationSemaphoreComplete32):
    #
    # movb $1, ()
    #
    .byte   0xc6, 0x05
ASM_PFX(mRebasedFlagAddr32):
    .long   0
    .byte   1
    #
    # jmpd ()
    #
    .byte   0xff, 0x25
ASM_PFX(mSmmRelocationOriginalAddressPtr32):
    .long   0
