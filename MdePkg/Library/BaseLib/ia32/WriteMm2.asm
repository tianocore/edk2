;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   WriteMm2.Asm
;
; Abstract:
;
;   AsmWriteMm2 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .586P
    .model  flat
    .xmm
    .code

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmWriteMm2 (
;   IN UINT64   Value
;   );
;------------------------------------------------------------------------------
_AsmWriteMm2    PROC
    movq    mm2, [esp + 4]
    ret
_AsmWriteMm2    ENDP

    END
