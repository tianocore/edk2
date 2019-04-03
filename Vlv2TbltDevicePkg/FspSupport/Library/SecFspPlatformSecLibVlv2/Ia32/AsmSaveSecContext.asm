;------------------------------------------------------------------------------
;
; Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;  SecEntry.asm
;
; Abstract:
;
;  This is the code that goes from real-mode to protected mode.
;  It consumes the reset vector, calls two basic APIs from FSP binary.
;
;------------------------------------------------------------------------------

.686p
.xmm
.model flat,c
.code

;----------------------------------------------------------------------------
;  MMX Usage:
;              MM0 = BIST State
;              MM5 = Save time-stamp counter value high32bit
;              MM6 = Save time-stamp counter value low32bit.
;
;  It should be same as SecEntry.asm and PeiCoreEntry.asm.
;----------------------------------------------------------------------------

AsmSaveBistValue   PROC PUBLIC
  mov     eax, [esp+4]
  movd    mm0, eax
  ret
AsmSaveBistValue   ENDP

AsmSaveTickerValue   PROC PUBLIC
  mov     eax, [esp+4]
  movd    mm6, eax
  mov     eax, [esp+8]
  movd    mm5, eax
  ret
AsmSaveTickerValue   ENDP

END
