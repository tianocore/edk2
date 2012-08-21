#------------------------------------------------------------------------------
#
# Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   GasketHypercall.S
#
# Abstract:
#
#   Hypercall function
#
# Notes:
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
#  UINTN
#  EFIAPI
#  Hypercall (
#    IN  VOID   *HypercallPageAddress,  // passed in %rcx
#    IN  UINTN  HypercallOffset,        // passed in %rdx
#    IN  UINTN  Arg1,                   // passed in %r8
#    IN  UINTN  Arg2,                   // passed in %r9
#    IN  UINTN  Arg3,                   // passed in stack
#    IN  UINTN  Arg4,                   // passed in stack
#    IN  UINTN  Arg5,                   // passed in stack
#    )
#------------------------------------------------------------------------------

  .text

ASM_GLOBAL ASM_PFX(Hypercall)
ASM_PFX(Hypercall):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp
  subq    $16, %rsp       // Allocate space for args on the stack

  pushq   %rsi            // %rsi & %rdi are callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rax      // Swizzle args
  addq    %rdx, %rax      // Address of hypercall is in %rax

  movq    %r8,  %rdi      // Move parameters into %rdi,
  movq    %r9,  %rsi      // %rsi,
  movq    48(%rbp), %rdx  // %rdx,
  movq    56(%rbp), %r10  // %r10,
  movq    64(%rbp), %r8   // %r8
  movq    %r8,     (%rsp) //

  call    *%rax
  addq    $16, %rsp

  popq    %rdi            // restore state
  popq    %rsi
  popq    %rbp
  ret                     // return value is in %rax


//
// Hypercall2 is not used.
//
ASM_GLOBAL ASM_PFX(Hypercall2)
ASM_PFX(Hypercall2):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi            // %rsi & %rdi are callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rax      // Swizzle args
  addq    %rdx, %rax      // Address of hypercall is in %rax

  movq    %r8,  %rdi      // Move parameters into %rdi, %rsi
  movq    %r9,  %rsi

  call    *%rax

  popq    %rdi            // restore state
  popq    %rsi
  popq    %rbp
  ret                     // return value is in %rax


