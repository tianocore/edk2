;-----------------------------------------------------------------------------
;
; Copyright (c) 2021, AMD. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;-----------------------------------------------------------------------------

%include "Nasm.inc"

    SECTION .text

;-----------------------------------------------------------------------------
;  UINTN
;  EFIAPI
;  AsmPvalidate (
;    IN   UINT32              RmpPageSize
;    IN   UINT32              Validate,
;    IN   PHYSICAL_ADDRESS    Address
;    )
;-----------------------------------------------------------------------------
global ASM_PFX(AsmPvalidate)
ASM_PFX(AsmPvalidate):
  mov     rax, r8

  PVALIDATE

  ; Save the value of carry flag after the PVALIDATE returned.
  setb    dl

  ; The PVALIDATE instruction returns the status in rax register.
  cmp     rax, 0
  jne     PvalidateExit

  ; Check the carry flag to determine whether there was RMP entry update.
  cmp     dl, 0
  jz      PvalidateExit

  ; Return the PVALIDATE_RET_NO_RMPUPDATE.
  mov     rax, 255

PvalidateExit:
  ret
