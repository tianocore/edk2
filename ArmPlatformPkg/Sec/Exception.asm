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

  IMPORT  SecCommonExceptionEntry
  EXPORT  SecVectorTable
  
  PRESERVE8
  AREA    SecException, CODE, READONLY, CODEALIGN, ALIGN=5

//============================================================
//Default Exception Handlers
//============================================================
  
//FIXME: One of the EDK2 tool is broken. It does not look to respect the alignment. Even, if we specify 32-byte alignment for this file.
Dummy1        DCD      0
Dummy2        DCD      0
  
SecVectorTable
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
   blx   SecCommonExceptionEntry

_DefaultUndefined
   sub  r1, LR
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #1
   blx   SecCommonExceptionEntry

_DefaultSWI
   sub  r1, LR, #4
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #2
   blx   SecCommonExceptionEntry

_DefaultPrefetchAbort
   sub  r1, LR, #4
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #3
   blx   SecCommonExceptionEntry

_DefaultDataAbort
   sub  r1, LR, #8
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #4
   blx   SecCommonExceptionEntry

_DefaultReserved
   mov  r1, lr
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #5
   blx   SecCommonExceptionEntry
   
_DefaultIrq
   sub  r1, LR, #4
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #6
   blx   SecCommonExceptionEntry

_DefaultFiq
   sub  r1, LR, #4
   cps       #0x13                     ; Switch to SVC for common stack
   mov  r0, #7
   blx   SecCommonExceptionEntry

  END
