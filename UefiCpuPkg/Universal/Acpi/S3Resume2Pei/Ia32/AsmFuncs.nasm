;------------------------------------------------------------------------------ ;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   AsmFuncs.Asm
;
; Abstract:
;
;   Assembly function to set segment selectors.
;
; Notes:
;
;------------------------------------------------------------------------------

SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmSetDataSelectors (
;   IN UINT16   SelectorValue
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmSetDataSelectors)
ASM_PFX(AsmSetDataSelectors):
  mov     eax, [esp + 4]
o16 mov     ds, ax
o16 mov     es, ax
o16 mov     fs, ax
o16 mov     gs, ax
o16 mov     ss, ax
  ret

