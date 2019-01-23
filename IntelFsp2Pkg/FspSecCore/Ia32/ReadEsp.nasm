;; @file
;  Provide read ESP function
;
; Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; AsmReadEsp (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmReadEsp)
ASM_PFX(AsmReadEsp):
    mov     eax, esp
    ret

