;; @file
;  Run before FSP SecCore to rebase SecCore and PeiCore
;
; Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;
    SECTION .text

extern ASM_PFX(PreFspSecCommon)

;----------------------------------------------------------------------------
; NotifyPhase API
;
; This FSP API will notify the FSP about the different phases in the boot
; process
;
;----------------------------------------------------------------------------
global ASM_PFX(NotifyPhaseApi)
ASM_PFX(NotifyPhaseApi):
  mov    rax,  2 ; FSP_API_INDEX.NotifyPhaseApiIndex
  jmp    ASM_PFX(PreFspSecCommon)

;----------------------------------------------------------------------------
; FspSiliconInit API
;
; This FSP API initializes the CPU and the chipset including the IO
; controllers in the chipset to enable normal operation of these devices.
;
;----------------------------------------------------------------------------
global ASM_PFX(FspSiliconInitApi)
ASM_PFX(FspSiliconInitApi):
  mov    rax,  5 ; FSP_API_INDEX.FspSiliconInitApiIndex
  jmp    ASM_PFX(PreFspSecCommon)

;----------------------------------------------------------------------------
; Module Entrypoint API
; Note the API is not added in PreFspSecCommon.nasm to prevent the linker from fully optimizing out the NASM file
;----------------------------------------------------------------------------
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  jmp $
