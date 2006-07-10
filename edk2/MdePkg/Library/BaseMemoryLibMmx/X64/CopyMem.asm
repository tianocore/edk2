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
;   CopyMem.asm
;
; Abstract:
;
;   CopyMem function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

InternalMemCopyMem  PROC    USES    rsi rdi
    mov     rsi, rdx                    ; rsi <- Source
    mov     rdi, rcx                    ; rdi <- Destination
    lea     r9, [rdi + r8 - 1]          ; r9 <- End of Destination
    cmp     rsi, rdi
    mov     rax, rdi                    ; rax <- Destination as return value
    jae     @F
    cmp     r9, rsi
    jae     @CopyBackward               ; Copy backward if overlapped
@@:
    xor     rcx, rcx
    sub     rcx, rsi
    and     rcx, 7                      ; rcx + rsi aligns on 8-byte boundary
    jz      @F
    cmp     rcx, r8
    cmova   rcx, r8
    sub     r8, rcx                     ; r8 <- remaining bytes to copy
    rep     movsb
@@:
    mov     rcx, r8
    and     r8, 7
    shr     rcx, 3                      ; rcx <- # of Qwords to copy
    jz      @CopyBytes
    DB      49h, 0fh, 7eh, 0c2h         ; movq    r10,  mm0    ; save mm0
@@:
    DB      48h, 0fh, 6fh, 06h          ; movq    mm0, [rsi]
    DB      48h, 0fh, 0e7h, 07h         ; movntq  [rdi], mm0
    add     rsi, 8
    add     rdi, 8
    loop    @B
    mfence
    DB      49h, 0fh, 6eh, 0c2h          ; movq    mm0, r10    ; restore mm0
    jmp     @CopyBytes
@CopyBackward:
    mov     rdi, r9                     ; rdi <- End of Destination
    lea     rsi, [rsi + r8 - 1]         ; rsi <- End of Source
    std                                 ; set direction flag
@CopyBytes:
    mov     rcx, r8
    rep     movsb                       ; Copy bytes backward
    cld
    ret
InternalMemCopyMem  ENDP

    END
