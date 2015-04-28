#------------------------------------------------------------------------------
#
# Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
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
   pushl %eax
   mov   $0, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception1Handle):
   cli
   pushl %eax
   mov   $1, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception2Handle):
   cli
   pushl %eax
   mov   $2, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception3Handle):
   cli
   pushl %eax
   mov   $3, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception4Handle):
   cli
   pushl %eax
   mov   $4, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception5Handle):
   cli
   pushl %eax
   mov   $5, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception6Handle):
   cli
   pushl %eax
   mov   $6, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception7Handle):
   cli
   pushl %eax
   mov   $7, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception8Handle):
   cli
   pushl %eax
   mov   $8, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception9Handle):
   cli
   pushl %eax
   mov   $9, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception10Handle):
   cli
   pushl %eax
   mov   $10, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception11Handle):
   cli
   pushl %eax
   mov   $11, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception12Handle):
   cli
   pushl %eax
   mov   $12, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception13Handle):
   cli
   pushl %eax
   mov   $13, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception14Handle):
   cli
   pushl %eax
   mov   $14, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception15Handle):
   cli
   pushl %eax
   mov   $15, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception16Handle):
   cli
   pushl %eax
   mov   $16, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception17Handle):
   cli
   pushl %eax
   mov   $17, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception18Handle):
   cli
   pushl %eax
   mov   $18, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(Exception19Handle):
   cli
   pushl %eax
   mov   $19, %eax
   jmp   ASM_PFX(CommonEntry)
AGENT_HANDLER_SIGNATURE
ASM_PFX(TimerInterruptHandle):
   cli
   pushl %eax
   mov   $32, %eax
   jmp   ASM_PFX(CommonEntry)


ASM_PFX(CommonEntry):

#---------------------------------------;
# _CommonEntry                  ;
#----------------------------------------------------------------------------;
# The follow algorithm is used for the common interrupt routine.
# Entry from each interrupt with a push eax and eax=interrupt number
#
# +---------------------+
# +    EFlags           +
# +---------------------+
# +    CS               +
# +---------------------+
# +    EIP              +
# +---------------------+
# +    Error Code       +
# +---------------------+
# + EAX / Vector Number +
# +---------------------+
# +    EBP              +
# +---------------------+ <-- EBP
#

# We need to determine if any extra data was pushed by the exception
  cmpl    $DEBUG_EXCEPT_DOUBLE_FAULT, %eax
  je      NoExtrPush
  cmpl    $DEBUG_EXCEPT_INVALID_TSS, %eax
  je      NoExtrPush
  cmpl    $DEBUG_EXCEPT_SEG_NOT_PRESENT, %eax
  je      NoExtrPush
  cmpl    $DEBUG_EXCEPT_STACK_FAULT, %eax
  je      NoExtrPush
  cmpl    $DEBUG_EXCEPT_GP_FAULT, %eax
  je      NoExtrPush
  cmpl    $DEBUG_EXCEPT_PAGE_FAULT, %eax
  je      NoExtrPush
  cmpl    $DEBUG_EXCEPT_ALIGNMENT_CHECK, %eax
  je      NoExtrPush

  pushl    (%esp)
  movl     $0, 4(%esp)

NoExtrPush:

  pushl   %ebp
  movl    %esp,%ebp

  #
  # Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
  # is 16-byte aligned
  #
  andl    $0xfffffff0,%esp
  subl    $12,%esp

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
  pushl   0x4(%ebp)
  pushl   %ebx
  pushl   %ecx
  pushl   %edx
  mov     %eax, %ebx                   # save vector in ebx
  leal    24(%ebp),%ecx
  pushl   %ecx                         # save original ESP
  pushl   (%ebp)
  pushl   %esi
  pushl   %edi

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
## insure FXSAVE/FXRSTOR is enabled in CR4...
## ... while we're at it, make sure DE is also enabled...
  mov     $1, %eax
  pushl   %ebx                         # temporarily save value of ebx on stack 
  cpuid                                # use CPUID to determine if FXSAVE/FXRESTOR 
                                       # and DE are supported
  popl    %ebx                         # retore value of ebx that was overwritten
                                       # by CPUID 
  movl    %cr4, %eax
  pushl   %eax                         # push cr4 firstly
  testl   $BIT24, %edx                 # Test for FXSAVE/FXRESTOR support
  jz      L1
  orl     $BIT9, %eax                  # Set CR4.OSFXSR
L1:    
  testl   $BIT2, %edx                  # Test for Debugging Extensions support
  jz      L2
  orl     $BIT3, %eax                  # Set CR4.DE
L2:    
  movl    %eax, %cr4
  movl    %cr3, %eax
  pushl   %eax
  movl    %cr2, %eax
  pushl   %eax
  xorl    %eax,%eax
  pushl   %eax
  movl    %cr0, %eax
  pushl   %eax

## UINT32  Gs, Fs, Es, Ds, Cs, Ss;
  movl    %ss,%eax
  pushl   %eax
  movzwl  16(%ebp), %eax
  pushl   %eax
  movl    %ds,%eax
  pushl   %eax
  movl    %es,%eax
  pushl   %eax
  movl    %fs,%eax
  pushl   %eax
  movl    %gs,%eax
  pushl   %eax

## UINT32  Eip;
  pushl   12(%ebp)

## UINT32  Gdtr[2], Idtr[2];
  subl    $8,%esp
  sidt    (%esp)
  subl    $8,%esp
  sgdt    (%esp)

## UINT32  Ldtr, Tr;
  xorl    %eax,%eax
  strl    %eax
  pushl   %eax
  sldtl   %eax
  pushl   %eax

## UINT32  EFlags;
  pushl   20(%ebp)

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
  movl    %dr7, %eax
  pushl   %eax
## clear Dr7 while executing debugger itself
  xorl    %eax,%eax
  movl    %eax, %dr7

  movl    %dr6, %eax
  pushl   %eax
## insure all status bits in dr6 are clear...
  xorl    %eax,%eax
  movl    %eax, %dr6

  movl    %dr3, %eax
  pushl   %eax
  movl    %dr2, %eax
  pushl   %eax
  movl    %dr1, %eax
  pushl   %eax
  movl    %dr0, %eax
  pushl   %eax

## FX_SAVE_STATE_IA32 FxSaveState;
  subl    $512,%esp
  movl    %esp,%edi
  testl   $BIT24, %edx     # Test for FXSAVE/FXRESTOR support.
                           # edx still contains result from CPUID above
  jz      L3
  .byte 0x0f, 0xae, 0x07   # fxsave [edi]
L3:  

## save the exception data
  pushl   8(%esp)

## Clear Direction Flag
  cld
	
## Prepare parameter and call C function
  pushl   %esp
  pushl   %ebx
  call    ASM_PFX(InterruptProcess)
  addl    $8,%esp

## skip the exception data
  addl    $4,%esp

## FX_SAVE_STATE_IA32 FxSaveState;
  movl    %esp,%esi
  movl    $1, %eax
  cpuid                    # use CPUID to determine if FXSAVE/FXRESTOR
                           # are supported
  testl   $BIT24, %edx     # Test for FXSAVE/FXRESTOR support
  jz      L4
  .byte 0x0f, 0xae, 0x0e   # fxrstor [esi]
L4:  
  addl    $512,%esp

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
  popl    %eax
  movl    %eax, %dr0
  popl    %eax
  movl    %eax, %dr1
  popl    %eax
  movl    %eax, %dr2
  popl    %eax
  movl    %eax, %dr3
## skip restore of dr6.  We cleared dr6 during the context save.
  addl    $4,%esp
  popl    %eax
  movl    %eax, %dr7

## UINT32  EFlags;
  popl    20(%ebp)

## UINT32  Ldtr, Tr;
## UINT32  Gdtr[2], Idtr[2];
## Best not let anyone mess with these particular registers...
  addl     $24,%esp

## UINT32  Eip;
   pop     12(%ebp)

## UINT32  Gs, Fs, Es, Ds, Cs, Ss;
## NOTE - modified segment registers could hang the debugger...  We
##        could attempt to insulate ourselves against this possibility,
##        but that poses risks as well.
##
  popl    %gs
  popl    %fs
  popl    %es
  popl    %ds
  popl    16(%ebp)
  popl    %ss

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
  popl    %eax
  movl    %eax, %cr0
  addl    $4,%esp   # not for Cr1
  popl    %eax
  movl    %eax, %cr2
  popl    %eax
  movl    %eax, %cr3
  popl    %eax
  movl    %eax, %cr4

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
  popl    %edi
  popl    %esi
  addl    $4,%esp  # not for ebp
  addl    $4,%esp  # not for esp
  popl    %edx
  popl    %ecx
  popl    %ebx
  popl    %eax

  movl    %ebp,%esp
  popl    %ebp
  addl    $8,%esp   # skip eax
  iretl
  
