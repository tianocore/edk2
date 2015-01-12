#
#
# Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
#                                                                                  
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.  
# The full text of the license may be found at                                     
# http://opensource.org/licenses/bsd-license.php.                                  
#                                                                                  
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
#                                                                                  
#
#
#*   Module Name:
#*
#*    Cpu.asm
#*
#*   Abstract:
#*
#------------------------------------------------------------------------------
##include <EfiBind.h>

.globl ASM_PFX(EfiHalt)
.globl ASM_PFX(EfiWbinvd)
.globl ASM_PFX(EfiInvd)
.globl ASM_PFX(EfiCpuid)
.globl ASM_PFX(EfiReadTsc)
.globl ASM_PFX(EfiDisableCache)
.globl ASM_PFX(EfiEnableCache)
.globl ASM_PFX(EfiReadMsr)
.globl ASM_PFX(EfiWriteMsr)
.globl ASM_PFX(EfiGetEflags)
.globl ASM_PFX(EfiDisableInterrupts)
.globl ASM_PFX(EfiEnableInterrupts)
.globl ASM_PFX(EfiCpuidExt)

.text


#------------------------------------------------------------------------------
#  VOID
#  EfiHalt (
#    VOID
#    )
#------------------------------------------------------------------------------
ASM_PFX(EfiHalt):
    hlt
    retq


#------------------------------------------------------------------------------
#  VOID
#  EfiWbinvd (
#    VOID
#    )
#------------------------------------------------------------------------------
ASM_PFX(EfiWbinvd):
    wbinvd
    retq


#------------------------------------------------------------------------------
#  VOID
#  EfiInvd (
#    VOID
#    )
#------------------------------------------------------------------------------
ASM_PFX(EfiInvd):
    invd
    retq

#------------------------------------------------------------------------------
#  VOID
#  EfiCpuid (
#    IN   UINT32              RegisterInEax,          // rcx
#    OUT  EFI_CPUID_REGISTER  *Reg           OPTIONAL // rdx
#    )
#------------------------------------------------------------------------------
ASM_PFX(EfiCpuid):
      push   %rbx
      mov    %rdx,%r8
      mov    %rcx,%rax
      cpuid
      cmp    $0x0,%r8
      je     _Exit
      mov    %eax,(%r8)
      mov    %ebx,0x4(%r8)
      mov    %ecx,0x8(%r8)
      mov    %edx,0xc(%r8)
_Exit:
      pop    %rbx
      retq

#------------------------------------------------------------------------------
#  UINT64
#  EfiReadMsr (
#    IN   UINT32  Index,  // rcx
#    )
#------------------------------------------------------------------------------
ASM_PFX(EfiReadMsr):
      rdmsr
      shl    $0x20,%rdx
      or     %rdx,%rax
      retq

#------------------------------------------------------------------------------
#  VOID
#  EfiWriteMsr (
#    IN   UINT32  Index,  // rcx
#    IN   UINT64  Value   // rdx
#    )
#------------------------------------------------------------------------------
ASM_PFX(EfiWriteMsr):
      mov    %rdx,%rax
      sar    $0x20,%rdx
      wrmsr
      retq

#------------------------------------------------------------------------------
# UINT64
# EfiReadTsc (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(EfiReadTsc):
      rdtsc
      shl    $0x20,%rax
      shrd   $0x20,%rdx,%rax
      retq

#------------------------------------------------------------------------------
# VOID
# EfiDisableCache (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(EfiDisableCache):
# added a check to see if cache is already disabled. If it is, then skip.
      mov    %cr0,%rax
      and    $0x60000000,%rax
      cmp    $0x0,%rax
      jne    1f
      mov    %cr0,%rax
      or     $0x60000000,%rax
      mov    %rax,%cr0
      wbinvd
1:
      retq

#------------------------------------------------------------------------------
# VOID
# EfiEnableCache (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(EfiEnableCache):
      wbinvd
      mov    %cr0,%rax
      and    $0xffffffff9fffffff,%rax
      mov    %rax,%cr0
      retq

#------------------------------------------------------------------------------
# UINTN
# EfiGetEflags (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(EfiGetEflags):
      pushfq
      pop    %rax
      retq

#------------------------------------------------------------------------------
# VOID
# EfiDisableInterrupts (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(EfiDisableInterrupts):
    cli
    ret

#------------------------------------------------------------------------------
# VOID
# EfiEnableInterrupts (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(EfiEnableInterrupts):
    sti
    ret
#------------------------------------------------------------------------------
#  VOID
#  EfiCpuidExt (
#    IN   UINT32              RegisterInEax,
#    IN   UINT32              CacheLevel,
#    OUT  EFI_CPUID_REGISTER  *Regs
#    )
#------------------------------------------------------------------------------
ASM_PFX(EfiCpuidExt):
      push   %rbx
      mov    %rcx,%rax
      mov    %rdx,%rcx
      cpuid
      mov    %eax,(%r8)
      mov    %ebx,0x4(%r8)
      mov    %ecx,0x8(%r8)
      mov    %edx,0xc(%r8)
      pop    %rbx
      retq
