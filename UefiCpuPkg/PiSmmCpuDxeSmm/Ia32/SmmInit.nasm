;------------------------------------------------------------------------------ ;
; Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   SmmInit.nasm
;
; Abstract:
;
;   Functions for relocating SMBASE's for all processors
;
;-------------------------------------------------------------------------------

extern ASM_PFX(SmmInitHandler)
extern ASM_PFX(mRebasedFlag)
extern ASM_PFX(mSmmRelocationOriginalAddress)

global ASM_PFX(gSmmCr3)
global ASM_PFX(gSmmCr4)
global ASM_PFX(gSmmCr0)
global ASM_PFX(gSmmJmpAddr)
global ASM_PFX(gSmmInitStack)
global ASM_PFX(gcSmiInitGdtr)
global ASM_PFX(gcSmmInitSize)
global ASM_PFX(gcSmmInitTemplate)

%define PROTECT_MODE_CS 0x8
%define PROTECT_MODE_DS 0x20

    SECTION .text

ASM_PFX(gcSmiInitGdtr):
            DW      0
            DQ      0

global ASM_PFX(SmmStartup)

BITS 16
ASM_PFX(SmmStartup):
    mov     eax, 0x80000001             ; read capability
    cpuid
    mov     ebx, edx                    ; rdmsr will change edx. keep it in ebx.
    DB      0x66, 0xb8                  ; mov eax, imm32
ASM_PFX(gSmmCr3): DD 0
    mov     cr3, eax
o32 lgdt    [cs:ebp + (ASM_PFX(gcSmiInitGdtr) - ASM_PFX(SmmStartup))]
    DB      0x66, 0xb8                  ; mov eax, imm32
ASM_PFX(gSmmCr4): DD 0
    mov     cr4, eax
    mov     ecx, 0xc0000080             ; IA32_EFER MSR
    rdmsr
    test    ebx, BIT20                  ; check NXE capability
    jz      .1
    or      ah, BIT3                    ; set NXE bit
    wrmsr
.1:
    DB      0x66, 0xb8                  ; mov eax, imm32
ASM_PFX(gSmmCr0): DD 0
    mov     di, PROTECT_MODE_DS
    mov     cr0, eax
    DB      0x66, 0xea                  ; jmp far [ptr48]
ASM_PFX(gSmmJmpAddr):
    DD      @32bit
    DW      PROTECT_MODE_CS

BITS 32
@32bit:
    mov     ds, edi
    mov     es, edi
    mov     fs, edi
    mov     gs, edi
    mov     ss, edi
    DB      0xbc                        ; mov esp, imm32
ASM_PFX(gSmmInitStack): DD 0
    call    ASM_PFX(SmmInitHandler)
    rsm

BITS 16
ASM_PFX(gcSmmInitTemplate):
    mov ebp, ASM_PFX(SmmStartup)
    sub ebp, 0x30000
    jmp ebp

ASM_PFX(gcSmmInitSize): DW $ - ASM_PFX(gcSmmInitTemplate)

BITS 32
global ASM_PFX(SmmRelocationSemaphoreComplete)
ASM_PFX(SmmRelocationSemaphoreComplete):
    push    eax
    mov     eax, [ASM_PFX(mRebasedFlag)]
    mov     byte [eax], 1
    pop     eax
    jmp     [ASM_PFX(mSmmRelocationOriginalAddress)]

global ASM_PFX(PiSmmCpuSmmInitFixupAddress)
ASM_PFX(PiSmmCpuSmmInitFixupAddress):
    ret
