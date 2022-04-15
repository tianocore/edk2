;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

    SECTION .text

%include    "PushPopRegsNasm.inc"

STACK_SAVED_RAX_OFFSET       EQU   8 * 7 ; size of a general purpose register * rax index

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
  ; RAX holds the API index
  ;

  ;
  ; Stack must be ready
  ;
  push   rax
  add    rsp, 8
  cmp    rax, [rsp - 8]
  jz     FspApiCommon1
  mov    rax, 08000000000000003h
  jmp    exit

FspApiCommon1:
  ;
  ; Verify the calling condition
  ;
  PUSHA_64
  mov    rdx, rcx           ; move ApiParam to rdx
  mov    rcx, rax           ; move ApiIdx to rcx
  call   ASM_PFX(FspApiCallingCheck)
  cmp    rax, 0
  jz     FspApiCommon2
  mov    [rsp + STACK_SAVED_RAX_OFFSET], rax
  POPA_64
exit:
  ret

FspApiCommon2:
  POPA_64
  cmp    rax, 3   ; FspMemoryInit API
  jz     FspApiCommon3

  cmp    rax, 6   ; FspMultiPhaseSiInitApiIndex API
  jz     FspApiCommon3

  call   ASM_PFX(AsmGetFspInfoHeader)
  jmp    ASM_PFX(Loader2PeiSwitchStack)

FspApiCommon3:
  jmp    ASM_PFX(FspApiCommonContinue)

