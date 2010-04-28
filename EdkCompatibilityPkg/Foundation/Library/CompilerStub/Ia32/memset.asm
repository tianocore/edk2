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
;   SetMem.asm
;
; Abstract:
;
;   memset function
;
; Notes:
;
;------------------------------------------------------------------------------

    .686
    .model  flat,C
    .mmx
    .code

;------------------------------------------------------------------------------
;  VOID *
;  memset (
;    OUT VOID   *Buffer,
;    IN  UINT8  Value,
;    IN  UINTN  Count
;    )
;------------------------------------------------------------------------------
memset   PROC    USES    edi
    mov     al, [esp + 12]
    mov     ah, al
    shrd    edx, eax, 16
    shld    eax, edx, 16
    mov     ecx, [esp + 16]             ; ecx <- Count
    cmp     ecx, 0                      ; if Count == 0, do nothing
    je      @SetDone
    mov     edi, [esp + 8]              ; edi <- Buffer
    mov     edx, ecx
    and     edx, 7
    shr     ecx, 3                      ; # of Qwords to set
    jz      @SetBytes
    add     esp, -10h
    movq    [esp], mm0                  ; save mm0
    movq    [esp + 8], mm1              ; save mm1
    movd    mm0, eax
    movd    mm1, eax
    psllq   mm0, 32
    por     mm0, mm1                    ; fill mm0 with 8 Value's
@@:
    movq    [edi], mm0
    add     edi, 8
    loop    @B
    movq    mm0, [esp]                  ; restore mm0
    movq    mm1, [esp + 8]              ; restore mm1
    add     esp, 10h                    ; stack cleanup
@SetBytes:
    mov     ecx, edx
    rep     stosb
@SetDone:    
    mov     eax, [esp + 8]              ; eax <- Buffer as return value
    ret
memset   ENDP

    END
