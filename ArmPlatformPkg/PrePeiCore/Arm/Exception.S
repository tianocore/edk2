//
//  Copyright (c) 2011, ARM Limited. All rights reserved.
//
# This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <AutoGen.h>

#start of the code section
.text
.align 5

# IMPORT
GCC_ASM_IMPORT(PeiCommonExceptionEntry)

# EXPORT
GCC_ASM_EXPORT(PeiVectorTable)

//============================================================
//Default Exception Handlers
//============================================================


ASM_PFX(PeiVectorTable):
  b _DefaultResetHandler
  b _DefaultUndefined
  b _DefaultSWI
  b _DefaultPrefetchAbort
  b _DefaultDataAbort
  b _DefaultReserved
  b _DefaultIrq
  b _DefaultFiq

//
// Default Exception handlers: There is no plan to return from any of these exceptions.
// No context saving at all.
//
_DefaultResetHandler:
  mov  r1, lr
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #0
  blx  ASM_PFX(PeiCommonExceptionEntry)

_DefaultUndefined:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #1
  blx  ASM_PFX(PeiCommonExceptionEntry)

_DefaultSWI:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #2
  blx  ASM_PFX(PeiCommonExceptionEntry)

_DefaultPrefetchAbort:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #3
  blx  ASM_PFX(PeiCommonExceptionEntry)

_DefaultDataAbort:
  sub  r1, LR, #8
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #4
  blx  ASM_PFX(PeiCommonExceptionEntry)

_DefaultReserved:
  mov  r1, lr
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #5
  blx  ASM_PFX(PeiCommonExceptionEntry)

_DefaultIrq:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #6
  blx  ASM_PFX(PeiCommonExceptionEntry)

_DefaultFiq:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #7
  blx  ASM_PFX(PeiCommonExceptionEntry)

