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
#   EfiSetMemRep4.S
#
# Abstract:
#
#   SetMem function
#
# Notes:
#
#------------------------------------------------------------------------------
#include <EfiBind.h>

    .code: 

.globl ASM_PFX(EfiCommonLibCopyMem)

#------------------------------------------------------------------------------
# VOID
# EfiCommonLibSetMem (
#   OUT     VOID                      *Buffer,
#   IN      UINTN                     Size,
#   IN      UINT8                     Value
#   );
#------------------------------------------------------------------------------
ASM_PFX(EfiCommonLibSetMem):
      push   %rdi
      cmp    $0x0,%rdx
      je     SetDone
      mov    %rcx,%rdi
      mov    %r8b,%al
      mov    %al,%ah
      shrd   $0x10,%eax,%ecx
      shld   $0x10,%ecx,%eax
      mov    %rdx,%rcx
      shr    $0x2,%rcx
      rep stos %eax,%es:(%rdi)
      mov    %rdx,%rcx
      and    $0x3,%rcx
      rep stos %al,%es:(%rdi)
SetDone:
      pop    %rdi
      retq   
