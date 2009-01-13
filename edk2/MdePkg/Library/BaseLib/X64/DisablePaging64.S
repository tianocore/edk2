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
#   IN      UINT16                    Cs,
#   IN      UINT64                    EntryPoint,
#   IN      UINT64                    Context1,  OPTIONAL
#   IN      UINT32                    Context2,  OPTIONAL
#   IN      UINT64                    NewStack
#   );
#------------------------------------------------------------------------------

.global ASM_PFX(InternalX86DisablePaging64)
ASM_PFX(InternalX86DisablePaging64):
    cli    
    shl    $0x20,%rcx                     # rcx[32..47] <- Cs
    lea    L1, %eax
    mov    %r8d, %esi 
    or     %rax, %rcx                     # rcx[0..47] <- Cs:@F
    mov    %r9d, %edi 
    mov    0x28(%rsp), %eax               # eax <- New Stack
    push   %rcx
    ret                                   # switch to compatibility mode
L1:
    mov    %eax,%esp                      # set up new stack
    mov    %cr0,%rax
    btr    $0x1f,%eax
    mov    %rax,%cr0                      # disable paging

    mov    %rdx,%rbx                      # save EntryPoint to rbx, for rdmsr will overwrite rdx
    mov    $0xc0000080,%ecx
    rdmsr  
    and    $0xfe,%ah                      # clear LME
    wrmsr  
    mov    %cr4,%rax
    and    $0xdf,%al                      # clear PAE
    mov    %rax,%cr4
    push   %rdi                           # push Context2
    push   %rsi                           # push Context1
    callq  *%rbx                          # transfer control to EntryPoint
    jmp    .                              # no one should get here

