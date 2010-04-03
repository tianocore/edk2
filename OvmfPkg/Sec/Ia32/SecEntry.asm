      TITLE   SecEntry.asm
;------------------------------------------------------------------------------
;*
;*   Copyright 2006 - 2009, Intel Corporation
;*   All rights reserved. This program and the accompanying materials
;*   are licensed and made available under the terms and conditions of the BSD License
;*   which accompanies this distribution.  The full text of the license may be found at
;*   http://opensource.org/licenses/bsd-license.php
;*
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;*
;*    CpuAsm.asm
;*
;*   Abstract:
;*
;------------------------------------------------------------------------------

#include "SecMain.h"

    .686
    .model  flat,C
    .code

EXTERN SecCoreStartupWithStack:PROC

;
; SecCore Entry Point
;
; Processor is in flat protected mode
;
; @param[in]  EAX   Initial value of the EAX register (BIST: Built-in Self Test)
; @param[in]  DI    'BP': boot-strap processor, or 'AP': application processor
; @param[in]  EBP   Pointer to the start of the Boot Firmware Volume
;
; @return     None  This routine does not return
;
_ModuleEntryPoint PROC PUBLIC

    ;
    ; Load temporary stack top at very low memory.  The C code
    ; can reload to a better address.
    ;
    mov     eax, INITIAL_TOP_OF_STACK
    mov     esp, eax
    nop

    ;
    ; Call into C code
    ;
    push    eax
    push    ebp
    call    SecCoreStartupWithStack

_ModuleEntryPoint ENDP

END
