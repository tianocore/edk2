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

    .code

;------------------------------------------------------------------------------
; VOID *
; memset (
;   OUT     VOID                      *Buffer,    --> rcx
;   IN      UINT8                     Value,      --> rdx
;   IN      UINTN                     Length      --> r8
;   );
;------------------------------------------------------------------------------
memset   PROC    USES    rdi
    mov     rax, rcx    
    cmp     r8, 0                      ; if Size == 0, do nothing
    je      @SetDone
    mov     rax, rdx                   ; rdx <-> r8
    mov     rdx, r8                    ; rdx <- Length
    mov     r8, rax                    ; r8  <- Value
    mov     ah, al
    DB      48h, 0fh, 6eh, 0c0h         ; movd mm0, rax
    mov     r8, rcx
    mov     rdi, r8                     ; rdi <- Buffer
    mov     rcx, rdx
    and     edx, 7
    shr     rcx, 3
    jz      @SetBytes
    DB      0fh, 70h, 0C0h, 00h         ; pshufw mm0, mm0, 0h
@@:
    DB      48h, 0fh, 7eh, 07h          ; movd [rdi], mm0
    add     rdi, 8
    loop    @B
@SetBytes:
    mov     ecx, edx
    rep     stosb
    mov     rax, r8
@SetDone:
    ret
memset   ENDP

    END
