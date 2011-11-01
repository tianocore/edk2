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
#include <AutoGen.h>
#include "SecInternal.h"

.text
.align 5

GCC_ASM_IMPORT(SecCommonExceptionEntry)
GCC_ASM_EXPORT(SecVectorTable)

//============================================================
// Default Exception Handlers
//============================================================
  
  
ASM_PFX(SecVectorTable):
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
  blx  ASM_PFX(SecCommonExceptionEntry)

_DefaultUndefined:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #1
  blx  ASM_PFX(SecCommonExceptionEntry)

_DefaultSWI:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #2
  blx  ASM_PFX(SecCommonExceptionEntry)

_DefaultPrefetchAbort:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #3
  blx  ASM_PFX(SecCommonExceptionEntry)

_DefaultDataAbort:
  sub  r1, LR, #8
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #4
  blx  ASM_PFX(SecCommonExceptionEntry)

_DefaultReserved:
  mov  r1, lr
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #5
  blx  ASM_PFX(SecCommonExceptionEntry)

_DefaultIrq:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #6
  blx  ASM_PFX(SecCommonExceptionEntry)

_DefaultFiq:
  sub  r1, LR, #4
  # Switch to SVC for common stack
  cps  #0x13
  mov  r0, #7
  blx  ASM_PFX(SecCommonExceptionEntry)

