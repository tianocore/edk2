#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
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
#   AsmFuncs.S
#
# Abstract:
#
#   Debug interrupt handle functions.
#
#------------------------------------------------------------------------------

#include "DebugException.h"

ASM_GLOBAL ASM_PFX(InterruptProcess)

ASM_GLOBAL ASM_PFX(Exception0Handle)
ASM_GLOBAL ASM_PFX(ExceptionStubHeaderSize)
ASM_GLOBAL ASM_PFX(TimerInterruptHandle)
ASM_GLOBAL ASM_PFX(CommonEntry)

.macro  AGENT_HANDLER_SIGNATURE
  .byte 0x41, 0x47, 0x54, 0x48   # AGENT_HANDLER_SIGNATURE     SIGNATURE_32('A','G','T','H')
.endm

.data

ASM_PFX(ExceptionStubHeaderSize):  .long     ASM_PFX(Exception1Handle) - ASM_PFX(Exception0Handle)


.text

AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception0Handle):
   cli
   pushq %rcx
   mov   $0, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception1Handle):
   cli
   pushq %rcx
   mov   $1, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception2Handle):
   cli
   pushq %rcx
   mov   $2, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception3Handle):
   cli
   pushq %rcx
   mov   $3, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception4Handle):
   cli
   pushq %rcx
   mov   $4, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception5Handle):
   cli
   pushq %rcx
   mov   $5, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception6Handle):
   cli
   pushq %rcx
   mov   $6, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception7Handle):
   cli
   pushq %rcx
   mov   $7, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception8Handle):
   cli
   pushq %rcx
   mov   $8, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception9Handle):
   cli
   pushq %rcx
   mov   $9, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception10Handle):
   cli
   pushq %rcx
   mov   $10, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception11Handle):
   cli
   pushq %rcx
   mov   $11, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception12Handle):
   cli
   pushq %rcx
   mov   $12, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception13Handle):
   cli
   pushq %rcx
   mov   $13, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception14Handle):
   cli
   pushq %rcx
   mov   $14, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception15Handle):
   cli
   pushq %rcx
   mov   $15, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception16Handle):
   cli
   pushq %rcx
   mov   $16, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception17Handle):
   cli
   pushq %rcx
   mov   $17, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception18Handle):
   cli
   pushq %rcx
   mov   $18, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception19Handle):
   cli
   pushq %rcx
   mov   $19, %rcx
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(TimerInterruptHandle):
   cli
   pushq %rcx
   mov   $32, %rcx
   jmp   ASM_PFX(CommonEntry)


ASM_PFX(CommonEntry):

#---------------------------------------;
# CommonInterruptEntry                  ;
#---------------------------------------;
# The follow algorithm is used for the common interrupt routine.

#
# +---------------------+ <-- 16-byte aligned ensured by processor
# +    Old SS           +
# +---------------------+
# +    Old RSP          +
# +---------------------+
# +    RFlags           +
# +---------------------+
# +    CS               +
# +---------------------+
# +    RIP              +
# +---------------------+
# +    Error Code       +
# +---------------------+
# + RCX / Vector Number +
# +---------------------+
# +    RBP              +
# +---------------------+ <-- RBP, 16-byte aligned
#

# We need to determine if any extra data was pushed by the exception
  cmpq    $DEBUG_EXCEPT_DOUBLE_FAULT, %rcx
  je      NoExtrPush
  cmpq    $DEBUG_EXCEPT_INVALID_TSS, %rcx
  je      NoExtrPush
  cmpq    $DEBUG_EXCEPT_SEG_NOT_PRESENT, %rcx
  je      NoExtrPush
  cmpq    $DEBUG_EXCEPT_STACK_FAULT, %rcx
  je      NoExtrPush
  cmpq    $DEBUG_EXCEPT_GP_FAULT, %rcx
  je      NoExtrPush
  cmpq    $DEBUG_EXCEPT_PAGE_FAULT, %rcx
  je      NoExtrPush
  cmpq    $DEBUG_EXCEPT_ALIGNMENT_CHECK, %rcx
  je      NoExtrPush

  pushq   (%rsp)
  movq    $0, 8(%rsp)

NoExtrPush:
  #
  # All interrupt handlers are invoked through interrupt gates, so
  # IF flag automatically cleared at the entry point
  pushq   %rbp
  movq    %rsp, %rbp

  #
  # Since here the stack pointer is 16-byte aligned, so
  # EFI_FX_SAVE_STATE_X64 of EFI_SYSTEM_CONTEXT_x64
  # is 16-byte aligned
  #

## UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
  pushq %r15
  pushq %r14
  pushq %r13
  pushq %r12
  pushq %r11
  pushq %r10
  pushq %r9
  pushq %r8

  movq  %cr8, %r8
  pushq %r8

## UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
  pushq %rax
  pushq %rbx
  pushq 8(%rbp)      # original rcx
  pushq %rdx
  pushq 48(%rbp)     # original rsp
  pushq (%rbp)       # original rbp
  pushq %rsi
  pushq %rdi

## UINT64  Cr0, Cr1, Cr2, Cr3, Cr4;
  movq    %cr4, %rax
  orq     $0x208, %rax
  movq    %rax, %cr4
  pushq   %rax
  movq    %cr3, %rax
  pushq   %rax
  movq    %cr2, %rax
  pushq   %rax
  xorq    %rax, %rax
  pushq   %rax
  movq    %cr0, %rax
  pushq   %rax

## UINT64  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
  xorq     %rax, %rax      # set rax to 0
  movzwq   56(%rbp), %rax
#  movq     %ss, %rax
  pushq    %rax
  movzwq   32(%rbp), %rax
#  movq     %cs, %rax
  pushq    %rax
  mov      %ds, %rax
  pushq    %rax
  mov      %es, %rax
  pushq    %rax
  mov      %fs, %rax
  pushq    %rax
  mov      %gs, %rax
  pushq    %rax

## UINT64  Rip;
  pushq    24(%rbp)

## UINT64  Gdtr[2], Idtr[2];
  subq     $16, %rsp
  sidt    (%rsp)
  subq     $16, %rsp
  sgdt    (%rsp)

## UINT64  Ldtr, Tr;
  xorq    %rax, %rax
  strw    %ax
  pushq   %rax
  sldtw   %ax
  pushq   %rax

## UINT64  RFlags;
  pushq   40(%rbp)

## UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
   movq    %dr7, %rax
  pushq   %rax
## clear Dr7 while executing debugger itself
   xorq    %rax, %rax
   movq    %rax, %dr7

   movq    %dr6, %rax
  pushq    %rax
## insure all status bits in dr6 are clear...
   xorq    %rax, %rax
   movq    %rax, %dr6

   movq    %dr3, %rax
  pushq    %rax
   movq    %dr2, %rax
  pushq    %rax
   movq    %dr1, %rax
  pushq    %rax
   movq    %dr0, %rax
  pushq    %rax

## FX_SAVE_STATE_X64 FxSaveState;
   subq    $512, %rsp
   movq    %rsp, %rdi
   .byte   0x0f, 0xae, 0b00000111

## save the exception data;
   pushq   16(%rbp)

## Clear Direction Flag
  cld

## Prepare parameter and call
#  movq    8(%rbp), %rcx
   movq    %rsp, %rdx
   movq    %rcx, %r15   # save vector in r15
  #
  # Per X64 calling convention, allocate maximum parameter stack space
  # and make sure RSP is 16-byte aligned
  #
   subq    $(32 + 8), %rsp
   call    ASM_PFX(InterruptProcess)
   addq    $(32 + 8), %rsp

## skip the exception data;
   addq    $8, %rsp

## FX_SAVE_STATE_X64 FxSaveState;

   movq    %rsp, %rsi
   .byte   0x0f, 0xae, 0b00001110
   addq    $512, %rsp

## UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
   popq     %rax
   movq     %rax, %dr0
   popq     %rax
   movq     %rax, %dr1
   popq     %rax
   movq     %rax, %dr2
   popq     %rax
   movq     %rax, %dr3
## skip restore of dr6.  We cleared dr6 during the context save.
   addq     $8, %rsp
   popq     %rax
   movq     %rax, %dr7

## UINT64  RFlags;
   popq    40(%rbp)

## UINT64  Ldtr, Tr;
## UINT64  Gdtr[2], Idtr[2];
## Best not let anyone mess with these particular registers...
   addq    $48, %rsp

## UINT64  Rip;
   popq    24(%rbp)

## UINT64  Gs, Fs, Es, Ds, Cs, Ss;
   popq     %rax
  # mov     gs, rax ; not for gs
   popq     %rax
  # mov     fs, rax ; not for fs
  # (X64 will not use fs and gs, so we do not restore it)
   popq     %rax
   mov      %rax, %es
   popq     %rax
   mov      %rax, %ds
   popq     32(%rbp)
   popq     56(%rbp)

## UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
   popq     %rax
   movq     %rax, %cr0
   addq     $8, %rsp
   popq     %rax
   movq     %rax, %cr2
   popq     %rax
   movq     %rax, %cr3
   popq     %rax
   movq     %rax, %cr4

## UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
## UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
   popq     %rdi
   popq     %rsi
   addq     $8, %rsp
   addq     $8, %rsp
   popq     %rdx
   popq     %rcx
   popq     %rbx
   popq     %rax

   popq     %r8
   movq     %r8, %cr8

   popq     %r8
   popq     %r9
   popq     %r10
   popq     %r11
   popq     %r12
   popq     %r13
   popq     %r14
   popq     %r15

   movq     %rbp, %rsp
   popq     %rbp
   addq     $16,  %rsp
   iretq
