;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

  SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; JumpToKernel (
;   VOID *KernelStart,
;   VOID *KernelBootParams
;   );
;------------------------------------------------------------------------------
global ASM_PFX(JumpToKernel)
ASM_PFX(JumpToKernel):

    mov     esi, [esp + 8]
    call    DWORD [esp + 4]
    ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; JumpToUefiKernel (
;   EFI_HANDLE ImageHandle,
;   EFI_SYSTEM_TABLE *SystemTable,
;   VOID *KernelBootParams,
;   VOID *KernelStart
;   );
;------------------------------------------------------------------------------
global ASM_PFX(JumpToUefiKernel)
ASM_PFX(JumpToUefiKernel):

    mov     eax, [esp + 12]
    mov     eax, [eax + 0x264]
    add     eax, [esp + 16]
    jmp     eax

