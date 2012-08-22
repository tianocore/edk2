;------------------------------------------------------------------------------ ;
; Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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

.686
.model  flat,C

.code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmSetDataSelectors (
;   IN UINT16   SelectorValue
;   );
;------------------------------------------------------------------------------
AsmSetDataSelectors   PROC near public
  mov     eax, [esp + 4]
  mov     ds, ax
  mov     es, ax
  mov     fs, ax
  mov     gs, ax
  mov     ss, ax   
  ret
AsmSetDataSelectors   ENDP

END
