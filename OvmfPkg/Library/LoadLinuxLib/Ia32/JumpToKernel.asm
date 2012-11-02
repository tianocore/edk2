;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
;
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

  .586p
  .model flat,C
  .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; JumpToKernel (
;   VOID *KernelStart,
;   VOID *KernelBootParams
;   );
;------------------------------------------------------------------------------
JumpToKernel PROC

    mov     esi, [esp + 8]
    call    DWORD PTR [esp + 4]
    ret

JumpToKernel ENDP

END
