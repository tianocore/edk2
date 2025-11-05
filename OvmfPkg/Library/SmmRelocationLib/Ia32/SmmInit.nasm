;------------------------------------------------------------------------------ ;
; Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
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

%include "StuffRsbNasm.inc"

extern ASM_PFX(SmmInitHandler)
extern ASM_PFX(mRebasedFlag)
extern ASM_PFX(mSmmRelocationOriginalAddress)

global ASM_PFX(gPatchSmmInitCr3)
global ASM_PFX(gPatchSmmInitCr4)
global ASM_PFX(gPatchSmmInitCr0)
global ASM_PFX(gPatchSmmInitStack)
global ASM_PFX(gcSmmInitGdtr)
global ASM_PFX(gcSmmInitSize)
global ASM_PFX(gcSmmInitTemplate)

%define PROTECT_MODE_CS 0x8
%define PROTECT_MODE_DS 0x20

    SECTION .data

NullSeg: DQ 0                   ; reserved by architecture
CodeSeg32:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x9b
            DB      0xcf                ; LimitHigh
            DB      0                   ; BaseHigh
ProtModeCodeSeg32:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x9b
            DB      0xcf                ; LimitHigh
            DB      0                   ; BaseHigh
ProtModeSsSeg32:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x93
            DB      0xcf                ; LimitHigh
            DB      0                   ; BaseHigh
DataSeg32:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x93
            DB      0xcf                ; LimitHigh
            DB      0                   ; BaseHigh
CodeSeg16:
            DW      -1
            DW      0
            DB      0
            DB      0x9b
            DB      0x8f
            DB      0
DataSeg16:
            DW      -1
            DW      0
            DB      0
            DB      0x93
            DB      0x8f
            DB      0
CodeSeg64:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x9b
            DB      0xaf                ; LimitHigh
            DB      0                   ; BaseHigh
GDT_SIZE equ $ - NullSeg

ASM_PFX(gcSmmInitGdtr):
    DW      GDT_SIZE - 1
    DD      NullSeg


    SECTION .text

global ASM_PFX(SmmStartup)

BITS 16
ASM_PFX(SmmStartup):
    mov     eax, 0x80000001             ; read capability
    cpuid
    mov     ebx, edx                    ; rdmsr will change edx. keep it in ebx.
    and     ebx, BIT20                  ; extract NX capability bit
    shr     ebx, 9                      ; shift bit to IA32_EFER.NXE[BIT11] position
    mov     eax, strict dword 0         ; source operand will be patched
ASM_PFX(gPatchSmmInitCr3):
    mov     cr3, eax
o32 lgdt    [cs:ebp + (ASM_PFX(gcSmmInitGdtr) - ASM_PFX(SmmStartup))]
    mov     eax, strict dword 0         ; source operand will be patched
ASM_PFX(gPatchSmmInitCr4):
    mov     cr4, eax
    mov     ecx, 0xc0000080             ; IA32_EFER MSR
    rdmsr
    or      eax, ebx                    ; set NXE bit if NX is available
    wrmsr
    mov     eax, strict dword 0         ; source operand will be patched
ASM_PFX(gPatchSmmInitCr0):
    mov     di, PROTECT_MODE_DS
    mov     cr0, eax
    jmp     PROTECT_MODE_CS : dword @32bit

BITS 32
@32bit:
    mov     ds, edi
    mov     es, edi
    mov     fs, edi
    mov     gs, edi
    mov     ss, edi
    mov     esp, strict dword 0         ; source operand will be patched
ASM_PFX(gPatchSmmInitStack):
    call    ASM_PFX(SmmInitHandler)
    StuffRsb32
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

global ASM_PFX(SmmInitFixupAddress)
ASM_PFX(SmmInitFixupAddress):
    ret
