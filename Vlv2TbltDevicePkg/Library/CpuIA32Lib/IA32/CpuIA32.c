/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  CpuIA32.c

Abstract:

--*/

#include <Library/CpuIA32.h>

VOID
EfiHalt (VOID)
{
  __asm {
    hlt
  }
}

VOID
EfiWbinvd (VOID)
{
  __asm {
    wbinvd
  }
}

VOID
EfiInvd (VOID)
{
  __asm {
    invd
  }
}

VOID
EfiCpuid (IN UINT32 RegisterInEax,
          OUT EFI_CPUID_REGISTER *Reg OPTIONAL)
{
  __asm {
    pushad

    mov    eax, RegisterInEax
    cpuid
    cmp    Reg, 0
    je     _Exit
    mov    edi, DWORD PTR Reg

    mov    DWORD PTR [edi].RegEax, eax   ; Reg->RegEax
    mov    DWORD PTR [edi].RegEbx, ebx   ; Reg->RegEbx
    mov    DWORD PTR [edi].RegEcx, ecx   ; Reg->RegEcx
    mov    DWORD PTR [edi].RegEdx, edx   ; Reg->RegEdx

_Exit:
     popad
  }
}

UINT64
EfiReadMsr (IN UINT32 Index)
{
  __asm {
    mov    ecx, Index
    rdmsr
  }
}

VOID
EfiWriteMsr (
  IN   UINT32  Index,
  IN   UINT64  Value
  )
{
  __asm {
    mov    ecx, Index
    mov    eax, DWORD PTR Value[0]
    mov    edx, DWORD PTR Value[4]
    wrmsr
  }
}

UINT64
EfiReadTsc (VOID)
{
  __asm {
    rdtsc
  }
}

VOID
EfiDisableCache (VOID)
{
  __asm {
    mov   eax, cr0
    bswap eax
    and   al, 60h
    cmp   al, 60h
    je    Exit
    mov   eax, cr0
    or    eax, 060000000h
    mov   cr0, eax
    wbinvd
Exit:
  }
}

VOID
EfiEnableCache (VOID)
{
  __asm {
    wbinvd
    mov   eax, cr0
    and   eax, 09fffffffh
    mov   cr0, eax
  }
}

UINT32
EfiGetEflags (
  VOID
  )
{
  __asm {
    pushfd
    pop  eax
  }
}

VOID
EfiDisableInterrupts (VOID)
{
  __asm {
    cli
  }
}

VOID
EfiEnableInterrupts (
  VOID
  )
{
  __asm {
    sti
  }
}

VOID
EfiCpuidExt (
  IN   UINT32              RegisterInEax,
  IN   UINT32              CacheLevel,
  OUT  EFI_CPUID_REGISTER  *Regs
  )
{
  __asm {
    pushad

    mov    eax, RegisterInEax
    mov    ecx, CacheLevel
    cpuid
    mov    edi, DWORD PTR Regs

    mov    DWORD PTR [edi].RegEax, eax   ; Reg->RegEax
    mov    DWORD PTR [edi].RegEbx, ebx   ; Reg->RegEbx
    mov    DWORD PTR [edi].RegEcx, ecx   ; Reg->RegEcx
    mov    DWORD PTR [edi].RegEdx, edx   ; Reg->RegEdx

    popad
  }
}
