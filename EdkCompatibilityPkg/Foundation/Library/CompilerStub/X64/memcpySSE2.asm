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

    .code

;------------------------------------------------------------------------------
; VOID *
; memcpy (
;   OUT     VOID                      *DestinationBuffer,
;   IN      CONST VOID                *SourceBuffer,
;   IN      UINTN                     Length
;   );
;------------------------------------------------------------------------------
memcpy  PROC    USES    rsi rdi
    mov     rax, rcx                    ; rax <- Destination as return value
    cmp     rdx, rcx                    ; if Source == Destination, do nothing
    je      @CopyMemDone
    cmp     r8, 0                       ; if Count == 0, do nothing
    je      @CopyMemDone
    mov     rsi, rdx                    ; rsi <- Source
    mov     rdi, rcx                    ; rdi <- Destination
    lea     r9, [rsi + r8 - 1]          ; r9 <- Last byte of Source
    cmp     rsi, rdi
    jae     @F                          ; Copy forward if Source > Destination
    cmp     r9, rdi                     ; Overlapped?
    jae     @CopyBackward               ; Copy backward if overlapped
@@:
    xor     rcx, rcx
    sub     rcx, rdi                    ; rcx <- -rdi
    and     rcx, 15                     ; rcx + rsi should be 16 bytes aligned
    jz      @F                          ; skip if rcx == 0
    cmp     rcx, r8
    cmova   rcx, r8
    sub     r8, rcx
    rep     movsb
@@:
    mov     rcx, r8
    and     r8, 15
    shr     rcx, 4                      ; rcx <- # of DQwords to copy
    jz      @CopyBytes
@@:
    movdqu  xmm0, [rsi]                 ; rsi may not be 16-byte aligned
    movdqa  [rdi], xmm0                 ; rdi should be 16-byte aligned
    add     rsi, 16
    add     rdi, 16
    loop    @B
    jmp     @CopyBytes                  ; copy remaining bytes
@CopyBackward:
    mov     rsi, r9                     ; rsi <- Last byte of Source
    lea     rdi, [rdi + r8 - 1]         ; rdi <- Last byte of Destination
    std
@CopyBytes:
    mov     rcx, r8
    rep     movsb
    cld
@CopyMemDone:   
    ret
memcpy  ENDP

    END
