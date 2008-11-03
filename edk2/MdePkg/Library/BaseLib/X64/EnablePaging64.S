#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation
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
#   IN      UINT16                    Cs,
#   IN      UINT64                    EntryPoint,
#   IN      UINT64                    Context1,  OPTIONAL
#   IN      UINT64                    Context2,  OPTIONAL
#   IN      UINT64                    NewStack
#   );
#------------------------------------------------------------------------------
.global ASM_PFX(InternalX86EnablePaging64)
ASM_PFX(InternalX86EnablePaging64):
    cli    
    pop    %rax                      # skip the return address
    callq  Base
Base:
    addl   $(L1-Base),(%rsp)         # offset for ret, seg is the 1st arg
    mov    %cr4,%rax
    or     $0x20,%al
    mov    %rax,%cr4                 # enable PAE
    mov    $0xc0000080,%ecx
    rdmsr  
    or     $0x1,%ah                  # set LME
    wrmsr  
    mov    %cr0,%rax
    bts    $0x1f,%eax
    mov    %rax,%cr0                 # enable paging
    lret   
L1:                                  # long mode starts here
    addr32 mov (%esp),%rbx
    addr32 mov 0x8(%esp),%rcx
    addr32 mov 0x10(%esp),%rdx
    addr32 mov 0x18(%esp),%rsp
    add    $-0x20,%rsp
    callq  *%rbx
    jmp    .                         # dead loop if EntryPoint() returned
