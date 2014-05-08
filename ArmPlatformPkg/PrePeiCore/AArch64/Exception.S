#
#  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

#include <AsmMacroIoLibV8.h>
#include <Base.h>
#include <AutoGen.h>

.text
.align 11

GCC_ASM_EXPORT(PeiVectorTable)

//============================================================
//Default Exception Handlers
//============================================================

ASM_PFX(PeiVectorTable):


#define TO_HANDLER                                              \
   EL1_OR_EL2(x1)                                               \
1: mrs  x1, elr_el1    /* EL1 Exception Link Register */       ;\
   b    3f                                                     ;\
2: mrs  x1, elr_el2    /* EL2 Exception Link Register */       ;\
3: bl   ASM_PFX(PeiCommonExceptionEntry)                       ;


//
// Default Exception handlers: There is no plan to return from any of these exceptions.
// No context saving at all.
//

.align 7
_DefaultSyncExceptHandler_t:
  mov  x0, #EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS
  TO_HANDLER

.align 7
_DefaultIrq_t:
  mov  x0, #EXCEPT_AARCH64_IRQ
  TO_HANDLER

.align 7
_DefaultFiq_t:
  mov  x0, #EXCEPT_AARCH64_FIQ
  TO_HANDLER

.align 7
_DefaultSError_t:
  mov  x0, #EXCEPT_AARCH64_SERROR
  TO_HANDLER

.align 7
_DefaultSyncExceptHandler_h:
  mov  x0, #EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS
  TO_HANDLER

.align 7
_DefaultIrq_h:
  mov  x0, #EXCEPT_AARCH64_IRQ
  TO_HANDLER

.align 7
_DefaultFiq_h:
  mov  x0, #EXCEPT_AARCH64_FIQ
  TO_HANDLER

.align 7
_DefaultSError_h:
  mov  x0, #EXCEPT_AARCH64_SERROR
  TO_HANDLER
