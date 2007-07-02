#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   DisablePaging64.S
#
# Abstract:
#
#   AsmDisablePaging64 function
#
# Notes:
#
#------------------------------------------------------------------------------

    

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# InternalX86DisablePaging64 (
#   IN      UINT16                    Cs,			%rdi
#   IN      UINT64                    EntryPoint,		%rsi
#   IN      UINT64                    Context1,  OPTIONAL	%rdx
#   IN      UINT32                    Context2,  OPTIONAL	%rcx
#   IN      UINT64                    NewStack			%r8
#   );
#------------------------------------------------------------------------------

.global _InternalX86DisablePaging64
_InternalX86DisablePaging64:
    cli    
    shl    $0x20,%rcx
    lea    (%rip), %eax
    mov    %eax,%ecx
    push   %rcx
    mov    %edx,%ebx
    mov    %r8d,%esi
    mov    %r9d,%edi
    mov    0x28(%rsp),%eax
    lret   
L1:
    mov    %eax,%esp
    mov    %cr0,%rax
    btr    $0x1f,%eax
    mov    %rax,%cr0
    mov    $0xc0000080,%ecx
    rdmsr  
    and    $0xfe,%ah
    wrmsr  
    mov    %cr4,%rax
    and    $0xdf,%al
    mov    %rax,%cr4
    push   %rdi
    push   %rsi
    callq  *%rbx
    jmp    .

