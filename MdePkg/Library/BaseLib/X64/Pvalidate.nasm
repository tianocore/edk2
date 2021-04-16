;-----------------------------------------------------------------------------
;
; Copyright (c) 2020-2021, AMD. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   Pvalidate.Asm
;
; Abstract:
;
;   AsmPvalidate function
;
; Notes:
;
;-----------------------------------------------------------------------------

    SECTION .text

;-----------------------------------------------------------------------------
;  PvalidateRetValue
;  EFIAPI
;  AsmPvalidate (
;    IN   UINT32  RmpPageSize
;    IN   UINT32  Validate,
;    IN   UINTN   Address,
;    OUT  UINTN  *Eflags,
;    )
;-----------------------------------------------------------------------------
global ASM_PFX(AsmPvalidate)
ASM_PFX(AsmPvalidate):
  mov     rax, r8

  ; PVALIDATE instruction opcode
  DB      0xF2, 0x0F, 0x01, 0xFF

  ; Read the Eflags
  pushfq
  pop     r8
  mov     [r9], r8

  ; The PVALIDATE instruction returns the status in rax register.
  ret
