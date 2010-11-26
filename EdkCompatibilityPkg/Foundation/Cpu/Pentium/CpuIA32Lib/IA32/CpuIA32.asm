;/*++
;
;Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
;This program and the accompanying materials                          
;are licensed and made available under the terms and conditions of the BSD License         
;which accompanies this distribution.  The full text of the license may be found at        
;http://opensource.org/licenses/bsd-license.php                                            
;                                                                                          
;THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;
;Module Name:
;
;  CpuIA32.c
;
;Abstract:
;
;--*/

;#include "CpuIA32.h"

;---------------------------------------------------------------------------
    .586p
    .model  flat,C
    .code

;---------------------------------------------------------------------------
;VOID
;EfiHalt (
;  VOID
;)
EfiHalt PROC C PUBLIC
    hlt
    ret
EfiHalt ENDP    

;VOID
;EfiWbinvd (
;  VOID
;)
EfiWbinvd PROC C PUBLIC
    wbinvd
    ret
EfiWbinvd ENDP

;VOID
;EfiInvd (
; VOID
;)
EfiInvd PROC C PUBLIC
    invd
    ret
EfiInvd ENDP

;VOID
;EfiCpuid (IN UINT32 RegisterInEax,
;          OUT EFI_CPUID_REGISTER *Reg OPTIONAL)
EfiCpuid PROC C PUBLIC
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi
    pushad

    mov    eax, dword ptr[ebp + 8] ;egisterInEax
    cpuid
    cmp    dword ptr[ebp + 0Ch], 0 ; Reg
    je     @F
    mov         edi,dword ptr [ebp+0Ch] ; Reg 

    mov         dword ptr [edi],eax ; Reg->RegEax
    mov         dword ptr [edi+4],ebx ; Reg->RegEbx
    mov         dword ptr [edi+8],ecx ; Reg->RegEcx
    mov         dword ptr [edi+0Ch],edx ; Reg->RegEdx

@@:
    popad
    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
    
    ret
EfiCpuid ENDP


;UINT64
;EfiReadMsr (
;  IN UINT32 Index
;  );
EfiReadMsr PROC C PUBLIC
    mov    ecx, dword ptr [esp + 4]; Index
    rdmsr
    ret
EfiReadMsr ENDP

;VOID
;EfiWriteMsr (
;  IN   UINT32  Index,
;  IN   UINT64  Value
;  );
EfiWriteMsr PROC C PUBLIC
    mov    ecx, dword ptr [esp+4]; Index
    mov    eax, dword ptr [esp+8]; DWORD PTR Value[0]
    mov    edx, dword ptr [esp+0Ch]; DWORD PTR Value[4]
    wrmsr            
    ret
EfiWriteMsr  ENDP

;UINT64
;EfiReadTsc (
;  VOID
;  )
EfiReadTsc PROC C PUBLIC
    rdtsc
    ret
EfiReadTsc  ENDP

;VOID
;EfiDisableCache (
;  VOID
;)
EfiDisableCache PROC C PUBLIC
    mov   eax, cr0
    bswap eax
    and   al, 60h
    cmp   al, 60h
    je    @F
    mov   eax, cr0
    or    eax, 060000000h     
    mov   cr0, eax
    wbinvd
@@:
    ret
EfiDisableCache  ENDP

;VOID
;EfiEnableCache (
;  VOID
;  )
EfiEnableCache PROC C PUBLIC
    wbinvd
    mov   eax, cr0
    and   eax, 09fffffffh         
    mov   cr0, eax
    ret
EfiEnableCache  ENDP

;UINT32
;EfiGetEflags (
;  VOID
;  )
EfiGetEflags PROC C PUBLIC
    pushfd
    pop  eax
    ret
EfiGetEflags  ENDP

;VOID
;EfiDisableInterrupts (
;  VOID
;  )
EfiDisableInterrupts PROC C PUBLIC
    cli
    ret
EfiDisableInterrupts  ENDP

;VOID
;EfiEnableInterrupts (
;  VOID
;  )
EfiEnableInterrupts  PROC C PUBLIC
    sti
    ret
EfiEnableInterrupts   ENDP

;VOID
;EfiCpuidExt (
;  IN   UINT32              RegisterInEax,
;  IN   UINT32              CacheLevel,
;  OUT  EFI_CPUID_REGISTER  *Regs              
;  )
EfiCpuidExt PROC C PUBLIC USES ebx edi esi
    pushad

    mov    eax, dword ptr [esp + 30h] ; RegisterInEax
    mov    ecx, dword ptr [esp + 34h] ; CacheLevel
    cpuid
    mov    edi, dword ptr [esp + 38h] ; DWORD PTR Regs 

    mov    dword ptr [edi], eax   	; Reg->RegEax
    mov    dword ptr [edi + 4], ebx   	; Reg->RegEbx
    mov    dword ptr [edi + 8], ecx   	; Reg->RegEcx
    mov    dword ptr [edi + 0Ch], edx   ; Reg->RegEdx

    popad
    ret
EfiCpuidExt  ENDP

	END
	
