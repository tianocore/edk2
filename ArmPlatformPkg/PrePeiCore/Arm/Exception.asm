//
//  Copyright (c) 2011, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <AutoGen.h>

  IMPORT  PeiCommonExceptionEntry
  EXPORT  PeiVectorTable

  PRESERVE8
  AREA    PrePeiCoreException, CODE, READONLY, CODEALIGN, ALIGN=5

//============================================================
//Default Exception Handlers
//============================================================


PeiVectorTable
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
_DefaultResetHandler
   mov  r1, lr
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #0
   blx   PeiCommonExceptionEntry

_DefaultUndefined
   sub  r1, LR, #4
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #1
   blx   PeiCommonExceptionEntry

_DefaultSWI
   sub  r1, LR, #4
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #2
   blx   PeiCommonExceptionEntry

_DefaultPrefetchAbort
   sub  r1, LR, #4
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #3
   blx   PeiCommonExceptionEntry

_DefaultDataAbort
   sub  r1, LR, #8
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #4
   blx   PeiCommonExceptionEntry

_DefaultReserved
   mov  r1, lr
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #5
   blx   PeiCommonExceptionEntry

_DefaultIrq
   sub  r1, LR, #4
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #6
   blx   PeiCommonExceptionEntry

_DefaultFiq
   sub  r1, LR, #4
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #7
   blx   PeiCommonExceptionEntry

  END
