
TITLE   Cpu.asm: Assembly code for the x64 resources

;
; This file contains an 'Intel Sample Driver' and is
; licensed for Intel CPUs and chipsets under the terms of your
; license agreement with Intel or your vendor.  This file may
; be modified by the user, subject to additional terms of the
; license agreement
;
;
; Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
;                                                                                  
; This program and the accompanying materials are licensed and made available under
; the terms and conditions of the BSD License that accompanies this distribution.  
; The full text of the license may be found at                                     
; http://opensource.org/licenses/bsd-license.php.                                  
;                                                                                  
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
;                                                                                  
;
;
;
;
;*   Module Name:
;*
;*    Cpu.asm
;*
;*   Abstract:
;*
;------------------------------------------------------------------------------

text    SEGMENT


;------------------------------------------------------------------------------
;  VOID
;  EfiHalt (
;    VOID
;    )
;------------------------------------------------------------------------------
EfiHalt PROC    PUBLIC
    hlt
    ret
EfiHalt ENDP


;------------------------------------------------------------------------------
;  VOID
;  EfiWbinvd (
;    VOID
;    )
;------------------------------------------------------------------------------
EfiWbinvd PROC    PUBLIC
    wbinvd
    ret
EfiWbinvd ENDP


;------------------------------------------------------------------------------
;  VOID
;  EfiInvd (
;    VOID
;    )
;------------------------------------------------------------------------------
EfiInvd PROC    PUBLIC
    invd
    ret
EfiInvd ENDP

;------------------------------------------------------------------------------
;  VOID
;  EfiCpuid (
;    IN   UINT32              RegisterInEax,          // rcx
;    OUT  EFI_CPUID_REGISTER  *Reg           OPTIONAL // rdx
;    )
;------------------------------------------------------------------------------
EfiCpuid PROC   PUBLIC
    push  rbx

    mov   r8,   rdx         ; r8 = *Reg
    mov   rax,  rcx         ; RegisterInEax
    cpuid
    cmp   r8,   0
    je    _Exit
    mov   [r8 +  0], eax    ; Reg->RegEax
    mov   [r8 +  4], ebx    ; Reg->RegEbx
    mov   [r8 +  8], ecx    ; Reg->RegEcx
    mov   [r8 + 12], edx    ; Reg->RegEdx

_Exit:
    pop   rbx
    ret
EfiCpuid  ENDP

;------------------------------------------------------------------------------
;  UINT64
;  EfiReadMsr (
;    IN   UINT32  Index,  // rcx
;    )
;------------------------------------------------------------------------------
EfiReadMsr PROC  PUBLIC
    rdmsr
    sal     rdx, 32   ; edx:eax -> rax
    or      rax, rdx  ; rax = edx:eax
    ret
EfiReadMsr  ENDP

;------------------------------------------------------------------------------
;  VOID
;  EfiWriteMsr (
;    IN   UINT32  Index,  // rcx
;    IN   UINT64  Value   // rdx
;    )
;------------------------------------------------------------------------------
EfiWriteMsr PROC   PUBLIC
    mov     rax,  rdx   ; rdx = Value
    sar     rdx,  32    ; convert rdx to edx upper 32-bits
    wrmsr               ; wrmsr[ecx] result = edx:eax
    ret
EfiWriteMsr  ENDP


;------------------------------------------------------------------------------
; UINT64
; EfiReadTsc (
;   VOID
;   );
;------------------------------------------------------------------------------
EfiReadTsc PROC    PUBLIC
    rdtsc
    shl     rax, 32
    shrd    rax, rdx, 32
    ret
EfiReadTsc  ENDP

;------------------------------------------------------------------------------
; VOID
; EfiDisableCache (
;   VOID
;   );
;------------------------------------------------------------------------------
EfiDisableCache PROC    PUBLIC
; added a check to see if cache is already disabled. If it is, then skip.
    mov   rax, cr0
    and   rax, 060000000h
    cmp   rax, 0
    jne   @f
    mov   rax, cr0
    or    rax, 060000000h
    mov   cr0, rax
    wbinvd
@@:
    ret
EfiDisableCache ENDP

;------------------------------------------------------------------------------
; VOID
; EfiEnableCache (
;   VOID
;   );
;------------------------------------------------------------------------------
EfiEnableCache PROC    PUBLIC
    wbinvd
    mov   rax, cr0
    and   rax, 09fffffffh
    mov   cr0, rax
    ret
EfiEnableCache ENDP

;------------------------------------------------------------------------------
; UINTN
; EfiGetEflags (
;   VOID
;   );
;------------------------------------------------------------------------------
EfiGetEflags PROC    PUBLIC
    pushfq
    pop   rax
    ret
EfiGetEflags  ENDP

;------------------------------------------------------------------------------
; VOID
; EfiDisableInterrupts (
;   VOID
;   );
;------------------------------------------------------------------------------
EfiDisableInterrupts PROC    PUBLIC
    cli
    ret
EfiDisableInterrupts  ENDP

;------------------------------------------------------------------------------
; VOID
; EfiEnableInterrupts (
;   VOID
;   );
;------------------------------------------------------------------------------
EfiEnableInterrupts PROC    PUBLIC
    sti
    ret
EfiEnableInterrupts  ENDP
;------------------------------------------------------------------------------
;  VOID
;  EfiCpuidExt (
;    IN   UINT32              RegisterInEax,
;    IN   UINT32              CacheLevel,
;    OUT  EFI_CPUID_REGISTER  *Regs
;    )
;------------------------------------------------------------------------------
EfiCpuidExt PROC    PUBLIC
     push   rbx
     mov    rax, rcx          ; rax = RegisterInEax
     mov    rcx, rdx          ; rcx = CacheLevel

     cpuid
     mov    [r8 + 0 ],  eax   ; Reg->RegEax
     mov    [r8 + 4 ],  ebx   ; Reg->RegEbx
     mov    [r8 + 8 ],  ecx   ; Reg->RegEcx
     mov    [r8 + 12],  edx   ; Reg->RegEdx

     pop rbx
     ret
EfiCpuidExt  ENDP
END
