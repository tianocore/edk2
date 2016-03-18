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
#   SmiException.S
#
# Abstract:
#
#   Exception handlers used in SM mode
#
#------------------------------------------------------------------------------

ASM_GLOBAL  ASM_PFX(SmiPFHandler)
ASM_GLOBAL  ASM_PFX(PageFaultStubFunction)
ASM_GLOBAL  ASM_PFX(gSmiMtrrs)
ASM_GLOBAL  ASM_PFX(gcSmiIdtr)
ASM_GLOBAL  ASM_PFX(gcSmiGdtr)
ASM_GLOBAL  ASM_PFX(gcPsd)
ASM_GLOBAL  ASM_PFX(FeaturePcdGet (PcdCpuSmmProfileEnable))

    .data

NullSeg:    .quad 0                     # reserved by architecture
CodeSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x9b
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
ProtModeCodeSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x9b
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
ProtModeSsSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x93
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
DataSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x93
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
CodeSeg16:
            .word -1
            .word 0
            .byte 0
            .byte 0x9b
            .byte 0x8f
            .byte 0
DataSeg16:
            .word -1
            .word 0
            .byte 0
            .byte 0x93
            .byte 0x8f
            .byte 0
CodeSeg64:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x9b
            .byte 0xaf                  # LimitHigh
            .byte 0                     # BaseHigh
.equ  GDT_SIZE, .- NullSeg

TssSeg:
            .word      TSS_DESC_SIZE -1    # LimitLow
            .word      0                   # BaseLow
            .byte      0                   # BaseMid
            .byte      0x89
            .byte      0x00                # LimitHigh
            .byte      0                   # BaseHigh
ExceptionTssSeg:
            .word      TSS_DESC_SIZE - 1   # LimitLow
            .word      0                   # BaseLow
            .byte      0                   # BaseMid
            .byte      0x89
            .byte      0x00                # LimitHigh
            .byte      0                   # BaseHigh

.equ  CODE_SEL,          CodeSeg32 - NullSeg
.equ  DATA_SEL,          DataSeg32 - NullSeg
.equ  TSS_SEL,           TssSeg - NullSeg
.equ  EXCEPTION_TSS_SEL, ExceptionTssSeg - NullSeg

# IA32 TSS fields
.equ  TSS_ESP0,          4
.equ  TSS_SS0,           8
.equ  TSS_ESP1,          12
.equ  TSS_SS1,           16
.equ  TSS_ESP2,          20
.equ  TSS_SS2,           24
.equ  TSS_CR3,           28
.equ  TSS_EIP,           32
.equ  TSS_EFLAGS,        36
.equ  TSS_EAX,           40
.equ  TSS_ECX,           44
.equ  TSS_EDX,           48
.equ  TSS_EBX,           52
.equ  TSS_ESP,           56
.equ  TSS_EBP,           60
.equ  TSS_ESI,           64
.equ  TSS_EDI,           68
.equ  TSS_ES,            72
.equ  TSS_CS,            76
.equ  TSS_SS,            80
.equ  TSS_DS,            84
.equ  TSS_FS,            88
.equ  TSS_GS,            92
.equ  TSS_LDT,           96

# Create 2 TSS segments just after GDT
TssDescriptor:
            .word      0                   # PreviousTaskLink
            .word      0                   # Reserved
            .long      0                   # ESP0
            .word      0                   # SS0
            .word      0                   # Reserved
            .long      0                   # ESP1
            .word      0                   # SS1
            .word      0                   # Reserved
            .long      0                   # ESP2
            .word      0                   # SS2
            .word      0                   # Reserved
            .long      0                   # CR3
            .long      0                   # EIP
            .long      0                   # EFLAGS
            .long      0                   # EAX
            .long      0                   # ECX
            .long      0                   # EDX
            .long      0                   # EBX
            .long      0                   # ESP
            .long      0                   # EBP
            .long      0                   # ESI
            .long      0                   # EDI
            .word      0                   # ES
            .word      0                   # Reserved
            .word      0                   # CS
            .word      0                   # Reserved
            .word      0                   # SS
            .word      0                   # Reserved
            .word      0                   # DS
            .word      0                   # Reserved
            .word      0                   # FS
            .word      0                   # Reserved
            .word      0                   # GS
            .word      0                   # Reserved
            .word      0                   # LDT Selector
            .word      0                   # Reserved
            .word      0                   # T
            .word      0                   # I/O Map Base
.equ TSS_DESC_SIZE, . - TssDescriptor

ExceptionTssDescriptor:
            .word      0                   # PreviousTaskLink
            .word      0                   # Reserved
            .long      0                   # ESP0
            .word      0                   # SS0
            .word      0                   # Reserved
            .long      0                   # ESP1
            .word      0                   # SS1
            .word      0                   # Reserved
            .long      0                   # ESP2
            .word      0                   # SS2
            .word      0                   # Reserved
            .long      0                   # CR3
            .long      PFHandlerEntry      # EIP
            .long      00000002            # EFLAGS
            .long      0                   # EAX
            .long      0                   # ECX
            .long      0                   # EDX
            .long      0                   # EBX
            .long      0                   # ESP
            .long      0                   # EBP
            .long      0                   # ESI
            .long      0                   # EDI
            .word      DATA_SEL            # ES
            .word      0                   # Reserved
            .word      CODE_SEL            # CS
            .word      0                   # Reserved
            .word      DATA_SEL            # SS
            .word      0                   # Reserved
            .word      DATA_SEL            # DS
            .word      0                   # Reserved
            .word      DATA_SEL            # FS
            .word      0                   # Reserved
            .word      DATA_SEL            # GS
            .word      0                   # Reserved
            .word      0                   # LDT Selector
            .word      0                   # Reserved
            .word      0                   # T
            .word      0                   # I/O Map Base

ASM_PFX(gcPsd):
            .ascii  "PSDSIG  "
            .word      PSD_SIZE
            .word 2
            .word      1 << 2
            .word      CODE_SEL
            .word      DATA_SEL
            .word      DATA_SEL
            .word      DATA_SEL
            .word 0
            .long 0
            .long 0
            .long 0
            .long 0
            .quad 0
            .long      NullSeg
            .long 0
            .long      GDT_SIZE
            .long 0
            .space 24, 0
            .long      ASM_PFX(gSmiMtrrs)
            .long 0
.equ  PSD_SIZE,  . - ASM_PFX(gcPsd)

ASM_PFX(gcSmiGdtr):  .word      GDT_SIZE - 1
                     .long      NullSeg

ASM_PFX(gcSmiIdtr):  .word      IDT_SIZE - 1
                     .long      _SmiIDT

_SmiIDT:
# The following segment repeats 32 times:
# No. 1
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 2
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 3
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 4
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 5
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 6
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 7
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 8
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 9
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 10
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 11
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 12
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 13
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 14
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 15
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 16
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 17
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 18
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 19
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 20
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 21
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 22
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 23
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 24
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 25
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 26
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 27
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 28
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 29
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 30
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 31
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31
# No. 32
    .word 0                             # Offset 0:15
    .word      CODE_SEL
    .byte 0                             # Unused
    .byte 0x8e                          # Interrupt Gate, Present
    .word 0                             # Offset 16:31

.equ  IDT_SIZE, . - _SmiIDT

TaskGateDescriptor:
    .word      0                        # Reserved
    .word      EXCEPTION_TSS_SEL        # TSS Segment selector
    .byte      0                        # Reserved
    .byte      0x85                     # Task Gate, present, DPL = 0
    .word      0                        # Reserved

    .text

#------------------------------------------------------------------------------
# PageFaultIdtHandlerSmmProfile is the entry point for all exceptions
#
# Stack:
#+---------------------+
#+    EFlags           +
#+---------------------+
#+    CS               +
#+---------------------+
#+    EIP              +
#+---------------------+
#+    Error Code       +
#+---------------------+
#+    Vector Number    +
#+---------------------+
#+    EBP              +
#+---------------------+ <-- EBP
#
# RSP set to odd multiple of 8 means ErrCode PRESENT
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(PageFaultIdtHandlerSmmProfile)
ASM_PFX(PageFaultIdtHandlerSmmProfile):
    pushl   $0x0e               # Page Fault
    pushl   %ebp
    movl    %esp, %ebp


    #
    # Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
    # is 16-byte aligned
    #
    andl    $0xfffffff0, %esp
    subl    $12, %esp

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    pushl   %eax
    pushl   %ecx
    pushl   %edx
    pushl   %ebx
    leal    (6*4)(%ebp), %ecx
    pushl   %ecx                          # ESP
    pushl   (%ebp)                        # EBP
    pushl   %esi
    pushl   %edi

## UINT32  Gs, Fs, Es, Ds, Cs, Ss;
    movl    %ss, %eax
    pushl   %eax
    movzwl  (4*4)(%ebp), %eax
    pushl   %eax
    movl    %ds, %eax
    pushl   %eax
    movl    %es, %eax
    pushl   %eax
    movl    %fs, %eax
    pushl   %eax
    movl    %gs, %eax
    pushl   %eax

## UINT32  Eip;
    movl    (3*4)(%ebp), %eax
    pushl   %eax

## UINT32  Gdtr[2], Idtr[2];
    subl    $8, %esp
    sidt    (%esp)
    movl    2(%esp), %eax
    xchgl   (%esp), %eax
    andl    $0xffff, %eax
    movl    %eax, 4(%esp)

    subl    $8, %esp
    sgdt    (%esp)
    movl    2(%esp), %eax
    xchgl   (%esp), %eax
    andl    $0xffff, %eax
    movl    %eax, 4(%esp)

## UINT32  Ldtr, Tr;
    xorl    %eax, %eax
    strw    %ax
    pushl   %eax
    sldtw   %ax
    pushl   %eax

## UINT32  EFlags;
    movl    (5*4)(%ebp), %eax
    pushl   %eax

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
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

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
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

## FX_SAVE_STATE_IA32 FxSaveState;
    subl    $512, %esp
    movl    %esp, %edi
    .byte   0x0f, 0xae, 0x07                  #fxsave [edi]

# UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

## UINT32  ExceptionData;
    pushl   (2*4)(%ebp)

## call into exception handler

## Prepare parameter and call
    movl    %esp, %edx
    pushl   %edx
    movl    (1*4)(%ebp), %edx
    pushl   %edx

    #
    # Call External Exception Handler
    #
    movl    $ASM_PFX(SmiPFHandler), %eax
    call    *%eax
    addl    $8, %esp
    jmp     L4

L4:
## UINT32  ExceptionData;
    addl    $4, %esp

## FX_SAVE_STATE_IA32 FxSaveState;
    movl    %esp, %esi
    .byte   0xf, 0xae, 0xe                 # fxrstor [esi]
    addl    $512, %esp

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
## Skip restoration of DRx registers to support debuggers
## that set breakpoints in interrupt/exception context
    addl    $4*6, %esp

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    popl    %eax
    movl    %eax, %cr0
    addl    $4, %esp                       # not for Cr1
    popl    %eax
    movl    %eax, %cr2
    popl    %eax
    movl    %eax, %cr3
    popl    %eax
    movl    %eax, %cr4

## UINT32  EFlags;
    popl    (5*4)(%ebp)

## UINT32  Ldtr, Tr;
## UINT32  Gdtr[2], Idtr[2];
## Best not let anyone mess with these particular registers...
    addl    $24, %esp

## UINT32  Eip;
    popl    (3*4)(%ebp)

## UINT32  Gs, Fs, Es, Ds, Cs, Ss;
## NOTE - modified segment registers could hang the debugger...  We
##        could attempt to insulate ourselves against this possibility,
##        but that poses risks as well.
##
    popl    %gs
    popl    %fs
    popl    %es
    popl    %ds
    popl    (4*4)(%ebp)
    popl    %ss

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    popl    %edi
    popl    %esi
    addl    $4, %esp                      # not for ebp
    addl    $4, %esp                      # not for esp
    popl    %ebx
    popl    %edx
    popl    %ecx
    popl    %eax

    movl    %ebp, %esp
    popl    %ebp

# Enable TF bit after page fault handler runs
    btsl    $8, 16(%esp)                  # EFLAGS

    addl    $8, %esp                      # skip INT# & ErrCode
Return:
    iret
#
# Page Fault Exception Handler entry when SMM Stack Guard is enabled
# Executiot starts here after a task switch
#
PFHandlerEntry:
#
# Get this processor's TSS
#
    subl    $8, %esp
    sgdt    2(%esp)
    movl    4(%esp), %eax                 # GDT base
    addl    $8, %esp
    movl    (TSS_SEL+2)(%eax), %ecx
    shll    $8, %ecx
    movb    (TSS_SEL+7)(%eax), %cl
    rorl    $8, %ecx                      # ecx = TSS base

    movl    %esp, %ebp

    #
    # Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
    # is 16-byte aligned
    #
    andl    $0xfffffff0, %esp
    subl    $12, %esp

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    pushl   TSS_EAX(%ecx)
    pushl   TSS_ECX(%ecx)
    pushl   TSS_EDX(%ecx)
    pushl   TSS_EBX(%ecx)
    pushl   TSS_ESP(%ecx)
    pushl   TSS_EBP(%ecx)
    pushl   TSS_ESI(%ecx)
    pushl   TSS_EDI(%ecx)

## UINT32  Gs, Fs, Es, Ds, Cs, Ss;
    movzwl  TSS_SS(%ecx), %eax
    pushl   %eax
    movzwl  TSS_CS(%ecx), %eax
    pushl   %eax
    movzwl  TSS_DS(%ecx), %eax
    pushl   %eax
    movzwl  TSS_ES(%ecx), %eax
    pushl   %eax
    movzwl  TSS_FS(%ecx), %eax
    pushl   %eax
    movzwl  TSS_GS(%ecx), %eax
    pushl   %eax

## UINT32  Eip;
    pushl   TSS_EIP(%ecx)

## UINT32  Gdtr[2], Idtr[2];
    subl    $8, %esp
    sidt    (%esp)
    movl    2(%esp), %eax
    xchgl   (%esp), %eax
    andl    $0xFFFF, %eax
    movl    %eax, 4(%esp)

    subl    $8, %esp
    sgdt    (%esp)
    movl    2(%esp), %eax
    xchgl   (%esp), %eax
    andl    $0xFFFF, %eax
    movl    %eax, 4(%esp)

## UINT32  Ldtr, Tr;
    movl    $TSS_SEL, %eax
    pushl   %eax
    movzwl  TSS_LDT(%ecx), %eax
    pushl   %eax

## UINT32  EFlags;
    pushl   TSS_EFLAGS(%ecx)

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
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

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
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

## FX_SAVE_STATE_IA32 FxSaveState;
## Clear TS bit in CR0 to avoid Device Not Available Exception (#NM)
## when executing fxsave/fxrstor instruction
    clts
    subl    $512, %esp
    movl    %esp, %edi
    .byte   0x0f, 0xae, 0x07                   #fxsave [edi]

# UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

## UINT32  ExceptionData;
    pushl   (%ebp)

## call into exception handler
    movl    %ecx, %ebx
    movl    $ASM_PFX(SmiPFHandler), %eax

## Prepare parameter and call
    movl    %esp, %edx
    pushl   %edx
    movl    $14, %edx
    pushl   %edx

    #
    # Call External Exception Handler
    #
    call    *%eax
    addl    $8, %esp

    movl    %ebx, %ecx
## UINT32  ExceptionData;
    addl    $4, %esp

## FX_SAVE_STATE_IA32 FxSaveState;
    movl    %esp, %esi
    .byte   0xf, 0xae, 0xe                     # fxrstor [esi]
    addl    $512, %esp

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
## Skip restoration of DRx registers to support debuggers
## that set breakpoints in interrupt/exception context
    addl    $4*6, %esp

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    popl    %eax
    movl    %eax, %cr0
    addl    $4, %esp                           # not for Cr1
    popl    %eax
    movl    %eax, %cr2
    popl    %eax
    movl    %eax, TSS_CR3(%ecx)
    popl    %eax
    movl    %eax, %cr4

## UINT32  EFlags;
    popl    TSS_EFLAGS(%ecx)

## UINT32  Ldtr, Tr;
## UINT32  Gdtr[2], Idtr[2];
## Best not let anyone mess with these particular registers...
    addl    $24, %esp

## UINT32  Eip;
    popl    TSS_EIP(%ecx)

## UINT32  Gs, Fs, Es, Ds, Cs, Ss;
## NOTE - modified segment registers could hang the debugger...  We
##        could attempt to insulate ourselves against this possibility,
##        but that poses risks as well.
##
    popl    %eax
    movw    %ax, TSS_GS(%ecx)
    popl    %eax
    movw    %ax, TSS_FS(%ecx)
    popl    %eax
    movw    %ax, TSS_ES(%ecx)
    popl    %eax
    movw    %ax, TSS_DS(%ecx)
    popl    %eax
    movw    %ax, TSS_CS(%ecx)
    popl    %eax
    movw    %ax, TSS_SS(%ecx)

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    popl    TSS_EDI(%ecx)
    popl    TSS_ESI(%ecx)
    addl    $4, %esp                           # not for ebp
    addl    $4, %esp                           # not for esp
    popl    TSS_EBX(%ecx)
    popl    TSS_EDX(%ecx)
    popl    TSS_ECX(%ecx)
    popl    TSS_EAX(%ecx)

    movl    %ebp, %esp

# Set single step DB# if SMM profile is enabled and page fault exception happens
    cmpb    $0, ASM_PFX(FeaturePcdGet (PcdCpuSmmProfileEnable))
    jz      Done2
# Create return context for iret in stub function
     movl   TSS_ESP(%ecx), %eax                   # Get old stack pointer
     movl   TSS_EIP(%ecx), %ebx
     movl   %ebx, -0xc(%eax)                      # create EIP in old stack
     movzwl TSS_CS(%ecx), %ebx
     movl   %ebx, -0x8(%eax)                      # create CS in old stack
     movl   TSS_EFLAGS(%ecx), %ebx
     btsl   $8,%ebx
     movl   %ebx, -0x4(%eax)                      # create eflags in old stack
     movl   TSS_ESP(%ecx), %eax                   # Get old stack pointer
     subl   $12, %eax                             # minus 12 byte
     movl   %eax, TSS_ESP(%ecx)                   # Set new stack pointer

# Replace the EIP of interrupted task with stub function
    movl    $ASM_PFX(PageFaultStubFunction), %eax
    movl   %eax, TSS_EIP(%ecx)
# Jump to the iret so next page fault handler as a task will start again after iret.

Done2:

    addl    $4, %esp                            # skip ErrCode

    jmp     Return

ASM_PFX(PageFaultStubFunction):
#
# we need clean TS bit in CR0 to execute
# x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions.
#
    clts
    iret

ASM_GLOBAL ASM_PFX(InitializeIDTSmmStackGuard)
ASM_PFX(InitializeIDTSmmStackGuard):
    pushl   %ebx
#
# If SMM Stack Guard feature is enabled, the Page Fault Exception entry in IDT
# is a Task Gate Descriptor so that when a Page Fault Exception occurs,
# the processors can use a known good stack in case stack ran out.
#
    leal    _SmiIDT + 14 * 8, %ebx
    leal    TaskGateDescriptor, %edx
    movl    (%edx), %eax
    movl    %eax, (%ebx)
    movl    4(%edx), %eax
    movl    %eax, 4(%ebx)

    popl    %ebx
    ret
