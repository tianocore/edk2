;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
;  VOID
;  EFIAPI
;  IoReadFifo8 (
;    IN UINTN                  Port,              // rcx
;    IN UINTN                  Size,              // rdx
;    IN VOID                   *Buffer            // r8
;    );
;------------------------------------------------------------------------------
IoReadFifo8 PROC

    xchg    rcx, rdx
    xchg    rdi, r8             ; rdi: buffer address; r8: save rdi
rep insb
    mov     rdi, r8             ; restore rdi
    ret

IoReadFifo8 ENDP

    END

