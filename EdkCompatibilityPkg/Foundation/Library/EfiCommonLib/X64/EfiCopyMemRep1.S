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
#   EfiCopyMemRep1.S
#
# Abstract:
#
#   CopyMem function
#
# Notes:
#
#------------------------------------------------------------------------------
#include <EfiBind.h>

    .code: 

.globl ASM_PFX(EfiCommonLibCopyMem)

#------------------------------------------------------------------------------
# VOID
# EfiCommonLibCopyMem (
#   OUT     VOID                      *Destination,
#   IN      VOID                      *Source,
#   IN      UINTN                     Count
#   );
#------------------------------------------------------------------------------
ASM_PFX(EfiCommonLibCopyMem):
      push   %rsi
      push   %rdi
      cmp    %rcx,%rdx
      je     CopyMemDone
      cmp    $0x0,%r8
      je     CopyMemDone
      mov    %rdx,%rsi
      mov    %rcx,%rdi
      lea    -1(%r8,%rsi,1),%r9
      cmp    %rdi,%rsi
      jae    CopyBytes
      cmp    %rdi,%r9
      jb     CopyBytes
      mov    %r9,%rsi
      lea    -1(%r8,%rdi,1),%rdi
      std    

CopyBytes:
      mov    %r8,%rcx
      rep movsb %ds:(%rsi),%es:(%rdi)
      cld    

CopyMemDone:
      pop    %rdi
      pop    %rsi
      retq   



