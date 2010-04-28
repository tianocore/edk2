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
;   EfiCopyMemRep1.asm
;
; Abstract:
;
;   CopyMem function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; VOID
; EfiCommonLibCopyMem (
;   OUT     VOID                      *Destination,
;   IN      VOID                      *Source,
;   IN      UINTN                     Count
;   );
;------------------------------------------------------------------------------
EfiCommonLibCopyMem  PROC    USES    rsi rdi
    cmp     rdx, rcx                    ; if Source == Destination, do nothing
    je      @CopyMemDone
    cmp     r8, 0                       ; if Count == 0, do nothing
    je      @CopyMemDone
    mov     rsi, rdx                    ; rsi <- Source
    mov     rdi, rcx                    ; rdi <- Destination
    lea     r9, [rsi + r8 - 1]          ; r9 <- End of Source
    cmp     rsi, rdi
    jae     @CopyBytes
    cmp     r9, rdi
    jb      @CopyBytes                  ; Copy backward if overlapped
    mov     rsi, r9                     ; rsi <- End of Source
    lea     rdi, [rdi + r8 - 1]         ; esi <- End of Destination
    std                                 ; set direction flag
@CopyBytes:
    mov     rcx, r8
    rep     movsb                       ; Copy bytes backward
    cld
@CopyMemDone:
    ret
EfiCommonLibCopyMem  ENDP

    END

