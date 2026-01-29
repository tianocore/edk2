;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
;*   SPDX-License-Identifier: BSD-2-Clause-Patent
;*
;*    CpuAsm.asm
;*
;*   Abstract:
;*
;------------------------------------------------------------------------------

#include <Base.h>

    SECTION .text

extern ASM_PFX(SecCoreStartupWithStack)

;
; SecCore Entry Point
;
; Processor is in flat protected mode
;
; @param[in]  EAX   Initial value of the EAX register (BIST: Built-in Self Test)
; @param[in]  DI    'BP': boot-strap processor, or 'AP': application processor
; @param[in]  EBP   Pointer to the start of the Boot Firmware Volume
; @param[in]  DS    Selector allowing flat access to all addresses
; @param[in]  ES    Selector allowing flat access to all addresses
; @param[in]  FS    Selector allowing flat access to all addresses
; @param[in]  GS    Selector allowing flat access to all addresses
; @param[in]  SS    Selector allowing flat access to all addresses
;
; @return     None  This routine does not return
;
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):

    ;
    ; Fill the temporary RAM with the initial stack value.
    ; The loop below will seed the heap as well, but that's harmless.
    ;
    mov     eax, FixedPcdGet32 (PcdInitValueInTempStack)      ; dword to store
    mov     edi, FixedPcdGet32 (PcdOvmfSecPeiTempRamBase)     ; base address,
                                                              ;   relative to
                                                              ;   ES
    mov     ecx, FixedPcdGet32 (PcdOvmfSecPeiTempRamSize) / 4 ; dword count
    cld                                                       ; store from base
                                                              ;   up
    rep stosd

    ;
    ; Load temporary RAM stack based on PCDs
    ;
    %define SEC_TOP_OF_STACK (FixedPcdGet32 (PcdOvmfSecPeiTempRamBase) + \
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
    call    ASM_PFX(SecCoreStartupWithStack)

