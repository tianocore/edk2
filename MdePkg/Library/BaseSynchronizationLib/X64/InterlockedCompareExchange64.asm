;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
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
;   InterlockedCompareExchange64.Asm
;
; Abstract:
;
;   InterlockedCompareExchange64 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; InterlockedCompareExchange64 (
;   IN      UINT64                    *Value,
;   IN      UINT64                    CompareValue,
;   IN      UINT64                    ExchangeValue
;   );
;------------------------------------------------------------------------------
InternalSyncCompareExchange64   PROC
    mov     rax, rdx
    lock    cmpxchg [rcx], r8
    ret
InternalSyncCompareExchange64   ENDP

    END
