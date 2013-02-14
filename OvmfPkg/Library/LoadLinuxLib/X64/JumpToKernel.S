#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(JumpToKernel)
ASM_GLOBAL ASM_PFX(JumpToUefiKernel)

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# JumpToKernel (
#   VOID *KernelStart,         // %rcx
#   VOID *KernelBootParams     // %rdx
#   );
#------------------------------------------------------------------------------
ASM_PFX(JumpToKernel):

    // Set up for executing kernel. BP in %esi, entry point on the stack
    // (64-bit when the 'ret' will use it as 32-bit, but we're little-endian)
    movq    %rdx, %rsi
    pushq   %rcx

    // Jump into the compatibility mode CS
    pushq   $0x10
    leaq    1f(%rip), %rax
    pushq   %rax
    .byte   0x48, 0xcb                      // retfq

1:  // Now in compatibility mode
.code32
    movl    $0x18, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    %eax, %ss

    // Disable paging
    movl    %cr0, %eax
    btcl    $31, %eax
    movl    %eax, %cr0

    // Disable long mode in EFER
    movl    $0x0c0000080, %ecx
    rdmsr
    btcl    $8, %eax
    wrmsr

    // Disable PAE
    movl    %cr4, %eax
    btcl    $5, %eax
    movl    %eax, %cr4

    // Zero registers and 'return' to kernel
    xorl    %ebp, %ebp
    xorl    %edi, %edi
    xorl    %ebx, %ebx
    ret
.code64

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# JumpToUefiKernel (
#   EFI_HANDLE ImageHandle,        // rcx
#   EFI_SYSTEM_TABLE *SystemTable, // rdx
#   VOID *KernelBootParams,        // r8
#   VOID *KernelStart              // r9
#   );
#------------------------------------------------------------------------------
ASM_PFX(JumpToUefiKernel):
    movq    %rcx, %rdi
    movq    %rdx, %rsi
    movq    %r8, %rdx
    xor     %rax, %rax
    movl    0x264(%r8), %eax
    addq    %rax, %r9
    addq    $0x200, %r9
    callq   %r9
    ret

