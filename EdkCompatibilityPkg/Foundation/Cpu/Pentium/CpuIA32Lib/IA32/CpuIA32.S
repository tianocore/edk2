#/*++
#
#Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
#This program and the accompanying materials                          
#are licensed and made available under the terms and conditions of the BSD License         
#which accompanies this distribution.  The full text of the license may be found at        
#http://opensource.org/licenses/bsd-license.php                                            
#                                                                                          
#THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#
#Module Name:
#
#  CpuIA32.c
#
#Abstract:
#
#--*/

##include "CpuIA32.h"
#include "EfiBind.h"

#---------------------------------------------------------------------------
    .586p: 
    #.MODEL flat,C
    .code: 

#---------------------------------------------------------------------------

.globl ASM_PFX(EfiHalt)
.globl ASM_PFX(EfiWbinvd)
.globl ASM_PFX(EfiInvd)
.globl ASM_PFX(EfiCpuid)
.globl ASM_PFX(EfiReadMsr)
.globl ASM_PFX(EfiWriteMsr)
.globl ASM_PFX(EfiReadTsc)
.globl ASM_PFX(EfiDisableCache)
.globl ASM_PFX(EfiEnableCache)
.globl ASM_PFX(EfiGetEflags)
.globl ASM_PFX(EfiDisableInterrupts)
.globl ASM_PFX(EfiEnableInterrupts)
.globl ASM_PFX(EfiCpuidExt)


#VOID
#EfiHalt (
#  VOID
#)
ASM_PFX(EfiHalt):
    hlt
    ret
#EfiHalt ENDP

#VOID
#EfiWbinvd (
#  VOID
#)
ASM_PFX(EfiWbinvd):
    wbinvd
    ret
#EfiWbinvd ENDP

#VOID
#EfiInvd (
# VOID
#)
ASM_PFX(EfiInvd):
    invd
    ret
#EfiInvd ENDP

#VOID
#EfiCpuid (IN UINT32 RegisterInEax,
#          OUT EFI_CPUID_REGISTER *Reg OPTIONAL)
ASM_PFX(EfiCpuid):
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %esi
    pushl %edi
    pushal

    movl   8(%ebp), %eax           #RegisterInEax
    cpuid
    cmpl   $0, 0xC(%ebp)           # Reg
    je     L1
    movl        0xC(%ebp), %edi         # Reg 

    movl        %eax, (%edi)        # Reg->RegEax
    movl        %ebx, 4(%edi)         # Reg->RegEbx
    movl        %ecx, 8(%edi)         # Reg->RegEcx
    movl        %edx, 0xC(%edi)         # Reg->RegEdx

L1: 
    popal
    popl        %edi
    popl        %esi
    popl        %ebx
    popl        %ebp

    ret
#EfiCpuid ENDP


#UINT64
#EfiReadMsr (
#  IN UINT32 Index
#  );
ASM_PFX(EfiReadMsr):
    movl   4(%esp), %ecx           # Index
    rdmsr
    ret
#EfiReadMsr ENDP

#VOID
#EfiWriteMsr (
#  IN   UINT32  Index,
#  IN   UINT64  Value
#  );
ASM_PFX(EfiWriteMsr):
    movl   4(%esp), %ecx         # Index
    movl   8(%esp), %eax         # DWORD PTR Value[0]
    movl   0xC(%esp), %edx         # DWORD PTR Value[4]
    wrmsr
    ret
#EfiWriteMsr ENDP

#UINT64
#EfiReadTsc (
#  VOID
#  )
ASM_PFX(EfiReadTsc):
    rdtsc
    ret
#EfiReadTsc ENDP

#VOID
#EfiDisableCache (
#  VOID
#)
ASM_PFX(EfiDisableCache):
    movl  %cr0, %eax
    bswapl %eax
    andb  $0x60, %al
    cmpb  $0x60, %al
    je    L2
    movl  %cr0, %eax
    orl   $0x60000000, %eax
    movl  %eax, %cr0
    wbinvd
L2: 
    ret
#EfiDisableCache ENDP

#VOID
#EfiEnableCache (
#  VOID
#  )
ASM_PFX(EfiEnableCache):
    wbinvd
    movl  %cr0, %eax
    andl  $0x9fffffff, %eax
    movl  %eax, %cr0
    ret
#EfiEnableCache ENDP

#UINT32
#EfiGetEflags (
#  VOID
#  )
ASM_PFX(EfiGetEflags):
    pushfl
    popl %eax
    ret
#EfiGetEflags ENDP

#VOID
#EfiDisableInterrupts (
#  VOID
#  )
ASM_PFX(EfiDisableInterrupts):
    cli
    ret
#EfiDisableInterrupts ENDP

#VOID
#EfiEnableInterrupts (
#  VOID
#  )
ASM_PFX(EfiEnableInterrupts):
    sti
    ret
#EfiEnableInterrupts ENDP

#VOID
#EfiCpuidExt (
#  IN   UINT32              RegisterInEax,
#  IN   UINT32              CacheLevel,
#  OUT  EFI_CPUID_REGISTER  *Regs              
#  )
ASM_PFX(EfiCpuidExt):
    push   %ebx
    push   %edi
    push   %esi
    pushal

    movl   0x30(%esp), %eax           # RegisterInEax
    movl   0x34(%esp), %ecx           # CacheLevel
    cpuid
    movl   0x38(%esp), %edi           # DWORD PTR Regs 

    movl   %eax, (%edi)                 # Reg->RegEax
    movl   %ebx, 4(%edi)                # Reg->RegEbx
    movl   %ecx, 8(%edi)                # Reg->RegEcx
    movl   %edx, 0xC(%edi)              # Reg->RegEdx

    popal
    pop    %esi
    pop    %edi
    pop    %ebx
    ret
#EfiCpuidExt ENDP



