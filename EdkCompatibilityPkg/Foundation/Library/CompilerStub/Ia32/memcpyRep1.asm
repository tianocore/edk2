;------------------------------------------------------------------------------
;
; Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   CopyMem.asm
;
; Abstract:
;
;   memcpy function
;
; Notes:
;
;------------------------------------------------------------------------------

    .686
    .model  flat,C
    .code

;------------------------------------------------------------------------------
;  VOID *
;  memcpy (
;    IN VOID   *Destination,
;    IN VOID   *Source,
;    IN UINTN  Count
;    );
;------------------------------------------------------------------------------
memcpy  PROC    USES    esi edi
    mov     esi, [esp + 16]             ; esi <- Source
    mov     edi, [esp + 12]             ; edi <- Destination
    mov     edx, [esp + 20]             ; edx <- Count
    cmp     esi, edi
    je      @CopyDone
    cmp     edx, 0
    je      @CopyDone
    lea     eax, [esi + edx - 1]         ; eax <- End of Source
    cmp     esi, edi
    jae     @F
    cmp     eax, edi
    jb      @F                           ; Copy backward if overlapped
    mov     esi, eax                     ; esi <- End of Source
    lea     edi, [edi + edx - 1]         ; edi <- End of Destination
    std
@@:
    mov     ecx, edx
    rep     movsb                        ; Copy bytes backward
    cld
@CopyDone:
    mov     eax, [esp + 12]
    ret
memcpy  ENDP

    END
