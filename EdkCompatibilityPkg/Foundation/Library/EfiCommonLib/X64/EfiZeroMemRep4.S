#------------------------------------------------------------------------------
#
# Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   EfiZeroMemRep4.S
#
# Abstract:
#
#   ZeroMem function
#
# Notes:
#
#------------------------------------------------------------------------------
#include <EfiBind.h>

    .code: 

.globl ASM_PFX(EfiCommonLibZeroMem)
#------------------------------------------------------------------------------
#  VOID
#  EfiCommonLibZeroMem (
#    IN VOID   *Buffer,
#    IN UINTN  Size
#    );
#------------------------------------------------------------------------------
ASM_PFX(EfiCommonLibZeroMem):
      push   %rdi
      xor    %rax,%rax
      mov    %rcx,%rdi
      mov    %rdx,%rcx
      shr    $0x2,%rcx
      and    $0x3,%rdx
      rep stos %eax,%es:(%rdi)
      mov    %rdx,%rcx
      rep stos %al,%es:(%rdi)
      pop    %rdi
      retq   