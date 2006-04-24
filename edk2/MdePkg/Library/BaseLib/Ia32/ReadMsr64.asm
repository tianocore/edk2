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
;   ReadMsr64.Asm
;
; Abstract:
;
;   AsmReadMsr64 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .586P
    .model  flat
    .code

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; AsmReadMsr32 (
;   IN UINT32  Index
;   );
;------------------------------------------------------------------------------
_AsmReadMsr32   PROC
    ;
    ; AsmReadMsr32 shares the same implementation with AsmReadMsr64 and thus no
    ; code inside this function
    ;
_AsmReadMsr32   ENDP

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMsr64 (
;   IN UINT64  Index
;   );
;------------------------------------------------------------------------------
_AsmReadMsr64   PROC
    mov     ecx, [esp + 4]
    rdmsr
    ret
_AsmReadMsr64   ENDP

    END
