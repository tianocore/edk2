;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

  DEFAULT REL
  SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; JumpToKernel (
;   VOID *KernelStart,         // rcx
;   VOID *KernelBootParams     // rdx
;   );
;------------------------------------------------------------------------------
global ASM_PFX(JumpToKernel)
ASM_PFX(JumpToKernel):

    ; Set up for executing kernel. BP in %esi, entry point on the stack
    ; (64-bit when the 'ret' will use it as 32-bit, but we're little-endian)
    mov    rsi, rdx
    push   rcx

    ; Jump into the compatibility mode CS
    push    0x10
    lea     rax, [.0]
    push    rax
    DB 0x48, 0xcb                      ; retfq

.0:
    ; Now in compatibility mode.

    DB 0xb8, 0x18, 0x0, 0x0, 0x0    ; movl    $0x18, %eax
    DB 0x8e, 0xd8                      ; movl    %eax, %ds
    DB 0x8e, 0xc0                      ; movl    %eax, %es
    DB 0x8e, 0xe0                      ; movl    %eax, %fs
    DB 0x8e, 0xe8                      ; movl    %eax, %gs
    DB 0x8e, 0xd0                      ; movl    %eax, %ss

    ; Disable paging
    DB 0xf, 0x20, 0xc0                ; movl    %cr0, %eax
    DB 0xf, 0xba, 0xf8, 0x1f          ; btcl    $31, %eax
    DB 0xf, 0x22, 0xc0                ; movl    %eax, %cr0

    ; Disable long mode in EFER
    DB 0xb9, 0x80, 0x0, 0x0, 0xc0    ; movl    $0x0c0000080, %ecx
    DB 0xf, 0x32                      ; rdmsr
    DB 0xf, 0xba, 0xf8, 0x8          ; btcl    $8, %eax
    DB 0xf, 0x30                      ; wrmsr

    ; Disable PAE
    DB 0xf, 0x20, 0xe0                ; movl    %cr4, %eax
    DB 0xf, 0xba, 0xf8, 0x5          ; btcl    $5, %eax
    DB 0xf, 0x22, 0xe0                ; movl    %eax, %cr4

    DB 0x31, 0xed                      ; xor     %ebp, %ebp
    DB 0x31, 0xff                      ; xor     %edi, %edi
    DB 0x31, 0xdb                      ; xor     %ebx, %ebx
    DB 0xc3                            ; ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; JumpToUefiKernel (
;   EFI_HANDLE ImageHandle,        // rcx
;   EFI_SYSTEM_TABLE *SystemTable, // rdx
;   VOID *KernelBootParams         // r8
;   VOID *KernelStart,             // r9
;   );
;------------------------------------------------------------------------------
global ASM_PFX(JumpToUefiKernel)
ASM_PFX(JumpToUefiKernel):

    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    xor     rax, rax
    mov     eax, [r8 + 0x264]
    add     r9, rax
    add     r9, 0x200
    call    r9
    ret

