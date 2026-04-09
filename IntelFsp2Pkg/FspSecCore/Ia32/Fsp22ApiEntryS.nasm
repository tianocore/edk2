;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

    SECTION .text

;
; Following functions will be provided in C
;
extern ASM_PFX(FspApiCommon)
extern ASM_PFX(FspMultiPhaseSiInitApiHandler)

;----------------------------------------------------------------------------
; NotifyPhase API
;
; This FSP API will notify the FSP about the different phases in the boot
; process
;
;----------------------------------------------------------------------------
global ASM_PFX(NotifyPhaseApi)
ASM_PFX(NotifyPhaseApi):
  mov    eax,  2 ; FSP_API_INDEX.NotifyPhaseApiIndex
  jmp    ASM_PFX(FspApiCommon)

;----------------------------------------------------------------------------
; FspSiliconInit API
;
; This FSP API initializes the CPU and the chipset including the IO
; controllers in the chipset to enable normal operation of these devices.
;
;----------------------------------------------------------------------------
global ASM_PFX(FspSiliconInitApi)
ASM_PFX(FspSiliconInitApi):
  mov    eax,  5 ; FSP_API_INDEX.FspSiliconInitApiIndex
  jmp    ASM_PFX(FspApiCommon)

;----------------------------------------------------------------------------
; FspMultiPhaseSiInitApi API
;
; This FSP API provides multi-phase silicon initialization, which brings greater
; modularity beyond the existing FspSiliconInit() API.
; Increased modularity is achieved by adding an extra API to FSP-S.
; This allows the bootloader to add board specific initialization steps throughout
; the SiliconInit flow as needed.
;
;----------------------------------------------------------------------------
global ASM_PFX(FspMultiPhaseSiInitApi)
ASM_PFX(FspMultiPhaseSiInitApi):
  mov    eax,  6 ; FSP_API_INDEX.FspMultiPhaseSiInitApiIndex
  jmp    ASM_PFX(FspApiCommon)

;----------------------------------------------------------------------------
; FspApiCommonContinue API
;
; This is the FSP API common entry point to resume the FSP execution
;
;----------------------------------------------------------------------------
global ASM_PFX(FspApiCommonContinue)
ASM_PFX(FspApiCommonContinue):
  ;
  ; Handle FspMultiPhaseSiInitApiIndex API
  ;
  cmp    eax, 6
  jnz    NotMultiPhaseSiInitApi

  pushad
  push   DWORD [esp + (4 * 8 + 4)]  ; push ApiParam
  push   eax                ; push ApiIdx
  call   ASM_PFX(FspMultiPhaseSiInitApiHandler)
  add    esp, 8
  mov    dword  [esp + (4 * 7)], eax
  popad
  ret

NotMultiPhaseSiInitApi:
  jmp $
  ret

;----------------------------------------------------------------------------
; TempRamInit API
;
; Empty function for WHOLEARCHIVE build option
;
;----------------------------------------------------------------------------
global ASM_PFX(TempRamInitApi)
ASM_PFX(TempRamInitApi):
  jmp $
  ret

;----------------------------------------------------------------------------
; Module Entrypoint API
;----------------------------------------------------------------------------
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  jmp $

