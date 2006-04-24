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
;   WriteMsr64.Asm
;
; Abstract:
;
;   AsmWriteMsr64 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .586P
    .model  flat
    .code

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmWriteMsr64 (
;   IN UINT32  Index,
;   IN UINT64  Value
;   );
;------------------------------------------------------------------------------
_AsmWriteMsr64  PROC
    mov     edx, [esp + 12]
    mov     eax, [esp + 8]
    mov     ecx, [esp + 4]
    wrmsr
    ret
_AsmWriteMsr64  ENDP

    END
