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
#   EnablePaging64.S
#
# Abstract:
#
#   AsmEnablePaging64 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# VOID
# EFIAPI
# InternalX86EnablePaging64 (
#   IN      UINT16                    Cs,			%rdi
#   IN      UINT64                    EntryPoint,		%rsi
#   IN      UINT64                    Context1,  OPTIONAL	%rdx
#   IN      UINT64                    Context2,  OPTIONAL	%rcx
#   IN      UINT64                    NewStack			%r8
#   );
#------------------------------------------------------------------------------
.global _InternalX86EnablePaging64;
_InternalX86EnablePaging64:
    cli    
    pop    %rax
    callq  Base
Base:
    addl   $(L1-Base),(%rsp)
    mov    %cr4,%rax
    or     $0x20,%al
    mov    %rax,%cr4
    mov    $0xc0000080,%ecx
    rdmsr  
    or     $0x1,%ah
    wrmsr  
    mov    %cr0,%rax
    bts    $0x1f,%eax
    mov    %rax,%cr0
    lret   
L1:
    addr32 mov (%esp),%rbx
    addr32 mov 0x8(%esp),%rcx
    addr32 mov 0x10(%esp),%rdx
    addr32 mov 0x18(%esp),%rsp
    add    $-0x20,%rsp
    callq  *%rbx
    jmp    .
