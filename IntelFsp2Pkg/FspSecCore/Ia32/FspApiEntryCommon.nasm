;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;;

    SECTION .text

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
  mov    dword  [esp + (4 * 7)], eax
  popad
exit:
  ret

FspApiCommon2:
  popad
  cmp    eax, 3   ; FspMemoryInit API
  jz     FspApiCommon3

  call   ASM_PFX(AsmGetFspInfoHeader)
  jmp    ASM_PFX(Loader2PeiSwitchStack)

FspApiCommon3:
  jmp    ASM_PFX(FspApiCommonContinue)

