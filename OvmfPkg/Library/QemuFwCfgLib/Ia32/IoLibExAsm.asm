;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

    .586P
    .model  flat,C
    .code

;------------------------------------------------------------------------------
;  VOID
;  EFIAPI
;  IoReadFifo8 (
;    IN UINTN                  Port,
;    IN UINTN                  Size,
;    IN VOID                   *Buffer
;    );
;------------------------------------------------------------------------------
IoReadFifo8 PROC

    mov     dx, [esp + 4]
    mov     ecx, [esp + 8]
    push    edi
    mov     edi, [esp + 16]
rep insb
    pop     edi
    ret

IoReadFifo8 ENDP


;------------------------------------------------------------------------------
;  VOID
;  EFIAPI
;  IoWriteFifo8 (
;    IN UINTN                  Port,
;    IN UINTN                  Size,
;    IN VOID                   *Buffer
;    );
;------------------------------------------------------------------------------
IoWriteFifo8 PROC

    mov     dx, [esp + 4]
    mov     ecx, [esp + 8]
    push    esi
    mov     esi, [esp + 16]
rep outsb
    pop     esi
    ret

IoWriteFifo8 ENDP

    END

