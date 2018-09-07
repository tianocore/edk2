;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
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
;   InterlockedDecrement.Asm
;
; Abstract:
;
;   InterlockedDecrement function
;
; Notes:
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; InternalSyncDecrement (
;   IN      volatile UINT32           *Value
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalSyncDecrement)
ASM_PFX(InternalSyncDecrement):
    mov       eax, 0FFFFFFFFh
    lock xadd dword [rcx], eax
    dec       eax
    ret
