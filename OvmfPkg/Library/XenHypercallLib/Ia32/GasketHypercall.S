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
#    IN  VOID   *HypercallPageAddress,  // passed in stack 8(%ebp)
#    IN  UINTN  HypercallOffset,        // passed in stack 12(%ebp)
#    IN  UINTN  Arg1,                   // passed in stack 16(%ebp)
#    IN  UINTN  Arg2,                   // passed in stack 20(%ebp)
#    IN  UINTN  Arg3,                   // passed in stack 24(%ebp)
#    IN  UINTN  Arg4,                   // passed in stack 28(%ebp)
#    IN  UINTN  Arg5,                   // passed in stack 32(%ebp)
#    )
#------------------------------------------------------------------------------

  .text

ASM_GLOBAL ASM_PFX(Hypercall)
ASM_PFX(Hypercall):
  pushl %ebp
  movl  %esp, %ebp

  pushl %ebx
  pushl %ecx
  pushl %edx
  pushl %esi
  pushl %edi  

  movl 8(%ebp), %eax     // Address of hypercall page
  addl 12(%ebp), %eax    // Address of hypercall is in %rax

  movl 32(%ebp), %edi    // Swizzle args
  movl 28(%ebp), %esi
  movl 24(%ebp), %edx
  movl 20(%ebp), %ecx
  movl 16(%ebp), %ebx

  call  *%eax

  popl %edi
  popl %esi
  popl %edx
  popl %ecx
  popl %ebx

  movl %ebp, %esp
  popl %ebp
  ret


//
// Hypercall2 is not used.
//
ASM_GLOBAL ASM_PFX(Hypercall2)
ASM_PFX(Hypercall2):
  pushl	%ebp
  movl	%esp, %ebp

  pushl %ecx

  movl 8(%ebp), %eax     // Address of hypercall page
  addl 12(%ebp), %eax    // Address of hypercall is in %rax

  movl 20(%ebp), %ecx    // Swizzle args
  movl 16(%ebp), %ebx
 
  call *%eax

  popl %ecx
  movl %ebp, %esp
  popl %ebp
  ret

