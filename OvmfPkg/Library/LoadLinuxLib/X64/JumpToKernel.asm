;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
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

  .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; JumpToKernel (
;   VOID *KernelStart,         // rcx
;   VOID *KernelBootParams     // rdx
;   );
;------------------------------------------------------------------------------
JumpToKernel PROC

    ; Set up for executing kernel. BP in %esi, entry point on the stack
    ; (64-bit when the 'ret' will use it as 32-bit, but we're little-endian)
    mov    rsi, rdx
    push   rcx

    ; Jump into the compatibility mode CS
    push    10h
    lea     rax, @F
    push    rax
    DB 048h, 0cbh                      ; retfq

@@:
    ; Now in compatibility mode.

    DB 0b8h, 018h, 000h, 000h, 000h    ; movl    $0x18, %eax
    DB 08eh, 0d8h                      ; movl    %eax, %ds
    DB 08eh, 0c0h                      ; movl    %eax, %es
    DB 08eh, 0e0h                      ; movl    %eax, %fs
    DB 08eh, 0e8h                      ; movl    %eax, %gs
    DB 08eh, 0d0h                      ; movl    %eax, %ss

    ; Disable paging
    DB 00fh, 020h, 0c0h                ; movl    %cr0, %eax
    DB 00fh, 0bah, 0f8h, 01fh          ; btcl    $31, %eax
    DB 00fh, 022h, 0c0h                ; movl    %eax, %cr0

    ; Disable long mode in EFER
    DB 0b9h, 080h, 000h, 000h, 0c0h    ; movl    $0x0c0000080, %ecx
    DB 00fh, 032h                      ; rdmsr
    DB 00fh, 0bah, 0f8h, 008h          ; btcl    $8, %eax
    DB 00fh, 030h                      ; wrmsr

    ; Disable PAE
    DB 00fh, 020h, 0e0h                ; movl    %cr4, %eax
    DB 00fh, 0bah, 0f8h, 005h          ; btcl    $5, %eax
    DB 00fh, 022h, 0e0h                ; movl    %eax, %cr4

    DB 031h, 0edh                      ; xor     %ebp, %ebp
    DB 031h, 0ffh                      ; xor     %edi, %edi
    DB 031h, 0dbh                      ; xor     %ebx, %ebx
    DB 0c3h                            ; ret

JumpToKernel ENDP

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
JumpToUefiKernel PROC

    mov     rdi, rcx
    mov     rsi, rdx
    mov     rdx, r8
    xor     rax, rax
    mov     eax, [r8 + 264h]
    add     r9, rax
    add     r9, 200h
    call    r9
    ret

JumpToUefiKernel ENDP

END
