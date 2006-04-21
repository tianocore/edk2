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
;   CopyMem.Asm
;
; Abstract:
;
;   CopyMem function
;
; Notes:
;
;------------------------------------------------------------------------------

    .386
    .model  flat,C
    .code

InternalMemCopyMem  PROC    USES    esi edi
    mov     esi, [esp + 16]             ; esi <- Source
    mov     edi, [esp + 12]             ; edi <- Destination
    mov     edx, [esp + 20]             ; edx <- Count
    lea     eax, [edi + edx - 1]        ; eax <- End of Destination
    cmp     esi, edi
    jae     @F
    cmp     eax, esi
    jae     @CopyBackward               ; Copy backward if overlapped
@@:
    mov     ecx, edx
    and     edx, 3
    shr     ecx, 2
    rep     movsd                       ; Copy as many Dwords as possible
    jmp     @CopyBytes
@CopyBackward:
    mov     edi, eax                    ; edi <- End of Destination
    lea     esi, [esi + edx - 1]        ; esi <- End of Source
    std
@CopyBytes:
    mov     ecx, edx
    rep     movsb                       ; Copy bytes backward
    cld
    mov     eax, [esp + 12]             ; eax <- Destination as return value
    ret
InternalMemCopyMem  ENDP

    END
