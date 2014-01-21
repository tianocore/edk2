      TITLE   SecEntry.asm
;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials
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

#include <Base.h>

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
    ; Load temporary RAM stack based on PCDs
    ;
    SEC_TOP_OF_STACK EQU (FixedPcdGet32 (PcdOvmfSecPeiTempRamBase) + \
                          FixedPcdGet32 (PcdOvmfSecPeiTempRamSize))
    mov     eax, SEC_TOP_OF_STACK
    mov     esp, eax
    nop

    ;
    ; Setup parameters and call SecCoreStartupWithStack
    ;   [esp]   return address for call
    ;   [esp+4] BootFirmwareVolumePtr
    ;   [esp+8] TopOfCurrentStack
    ;
    push    eax
    push    ebp
    call    SecCoreStartupWithStack

_ModuleEntryPoint ENDP

END
