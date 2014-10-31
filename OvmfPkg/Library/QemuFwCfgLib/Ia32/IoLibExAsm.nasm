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

    SECTION .text

;------------------------------------------------------------------------------
;  VOID
;  EFIAPI
;  IoReadFifo8 (
;    IN UINTN                  Port,
;    IN UINTN                  Size,
;    IN VOID                   *Buffer
;    );
;------------------------------------------------------------------------------
global ASM_PFX(IoReadFifo8)
ASM_PFX(IoReadFifo8):

    mov     dx, [esp + 4]
    mov     ecx, [esp + 8]
    push    edi
    mov     edi, [esp + 16]
rep insb
    pop     edi
    ret

;------------------------------------------------------------------------------
;  VOID
;  EFIAPI
;  IoWriteFifo8 (
;    IN UINTN                  Port,
;    IN UINTN                  Size,
;    IN VOID                   *Buffer
;    );
;------------------------------------------------------------------------------
global ASM_PFX(IoWriteFifo8)
ASM_PFX(IoWriteFifo8):

    mov     dx, [esp + 4]
    mov     ecx, [esp + 8]
    push    esi
    mov     esi, [esp + 16]
rep outsb
    pop     esi
    ret

