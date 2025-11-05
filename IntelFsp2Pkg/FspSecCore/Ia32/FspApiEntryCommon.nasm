;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2016 - 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

    SECTION .text

STACK_SAVED_EAX_OFFSET       EQU   4 * 7 ; size of a general purpose register * eax index

;
; Following functions will be provided in C
;
extern ASM_PFX(Loader2PeiSwitchStack)
extern ASM_PFX(FspApiCallingCheck)

;
; Following functions will be provided in ASM
;
extern ASM_PFX(FspApiCommonContinue)
extern ASM_PFX(AsmGetFspInfoHeader)

;----------------------------------------------------------------------------
; FspApiCommon API
;
; This is the FSP API common entry point to resume the FSP execution
;
;----------------------------------------------------------------------------
global ASM_PFX(FspApiCommon)
ASM_PFX(FspApiCommon):
  ;
  ; EAX holds the API index
  ;

  ;
  ; Stack must be ready
  ;
  push   eax
  add    esp, 4
  cmp    eax, dword  [esp - 4]
  jz     FspApiCommon1
  mov    eax, 080000003h
  jmp    exit

FspApiCommon1:
  ;
  ; Verify the calling condition
  ;
  pushad
  push   DWORD [esp + (4 * 8 + 4)]  ; push ApiParam
  push   eax                ; push ApiIdx
  call   ASM_PFX(FspApiCallingCheck)
  add    esp, 8
  cmp    eax, 0
  jz     FspApiCommon2
  mov    dword  [esp + STACK_SAVED_EAX_OFFSET], eax
  popad
exit:
  ret

FspApiCommon2:
  popad
  cmp    eax, 3   ; FspMemoryInit API
  jz     FspApiCommon3

  cmp    eax, 6   ; FspMultiPhaseSiInitApiIndex API
  jz     FspApiCommon3

  cmp    eax, 8   ; FspMultiPhaseMemInitApiIndex API
  jz     FspApiCommon3

  call   ASM_PFX(AsmGetFspInfoHeader)
  jmp    ASM_PFX(Loader2PeiSwitchStack)

FspApiCommon3:
  jmp    ASM_PFX(FspApiCommonContinue)

