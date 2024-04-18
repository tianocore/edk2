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
global ASM_PFX(gPatchRebasedFlagAddr32)
global ASM_PFX(gPatchSmmRelocationOriginalAddressPtr32)

%define LONG_MODE_CS 0x38

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
GDT_SIZE equ $ -   NullSeg

ASM_PFX(gcSmmInitGdtr):
    DW      GDT_SIZE - 1
    DQ      NullSeg


    DEFAULT REL
    SECTION .text

global ASM_PFX(SmmStartup)

BITS 16
ASM_PFX(SmmStartup):
    mov     eax, 0x80000001             ; read capability
    cpuid
    mov     ebx, edx                    ; rdmsr will change edx. keep it in ebx.
    mov     eax, strict dword 0         ; source operand will be patched
ASM_PFX(gPatchSmmInitCr3):
    mov     cr3, eax
o32 lgdt    [cs:ebp + (ASM_PFX(gcSmmInitGdtr) - ASM_PFX(SmmStartup))]
    mov     eax, strict dword 0         ; source operand will be patched
ASM_PFX(gPatchSmmInitCr4):
    or      ah,  2                      ; enable XMM registers access
    mov     cr4, eax
    mov     ecx, 0xc0000080             ; IA32_EFER MSR
    rdmsr
    or      ah, BIT0                    ; set LME bit
    test    ebx, BIT20                  ; check NXE capability
    jz      .1
    or      ah, BIT3                    ; set NXE bit
.1:
    wrmsr
    mov     eax, strict dword 0         ; source operand will be patched
ASM_PFX(gPatchSmmInitCr0):
    mov     cr0, eax                    ; enable protected mode & paging
    jmp     LONG_MODE_CS : dword 0      ; offset will be patched to @LongMode
@PatchLongModeOffset:

BITS 64
@LongMode:                              ; long-mode starts here
    mov     rsp, strict qword 0         ; source operand will be patched
ASM_PFX(gPatchSmmInitStack):
    and     sp, 0xfff0                  ; make sure RSP is 16-byte aligned
    ;
    ; According to X64 calling convention, XMM0~5 are volatile, we need to save
    ; them before calling C-function.
    ;
    sub     rsp, 0x60
    movdqa  [rsp], xmm0
    movdqa  [rsp + 0x10], xmm1
    movdqa  [rsp + 0x20], xmm2
    movdqa  [rsp + 0x30], xmm3
    movdqa  [rsp + 0x40], xmm4
    movdqa  [rsp + 0x50], xmm5

    add     rsp, -0x20
    call    ASM_PFX(SmmInitHandler)
    add     rsp, 0x20

    ;
    ; Restore XMM0~5 after calling C-function.
    ;
    movdqa  xmm0, [rsp]
    movdqa  xmm1, [rsp + 0x10]
    movdqa  xmm2, [rsp + 0x20]
    movdqa  xmm3, [rsp + 0x30]
    movdqa  xmm4, [rsp + 0x40]
    movdqa  xmm5, [rsp + 0x50]

    StuffRsb64
    rsm

BITS 16
ASM_PFX(gcSmmInitTemplate):
    mov ebp, [cs:@L1 - ASM_PFX(gcSmmInitTemplate) + 0x8000]
    sub ebp, 0x30000
    jmp ebp
@L1:
    DQ     0; ASM_PFX(SmmStartup)

ASM_PFX(gcSmmInitSize): DW $ - ASM_PFX(gcSmmInitTemplate)

BITS 64
global ASM_PFX(SmmRelocationSemaphoreComplete)
ASM_PFX(SmmRelocationSemaphoreComplete):
    push    rax
    mov     rax, [ASM_PFX(mRebasedFlag)]
    mov     byte [rax], 1
    pop     rax
    jmp     [ASM_PFX(mSmmRelocationOriginalAddress)]

;
; Semaphore code running in 32-bit mode
;
BITS 32
global ASM_PFX(SmmRelocationSemaphoreComplete32)
ASM_PFX(SmmRelocationSemaphoreComplete32):
    push    eax
    mov     eax, strict dword 0                ; source operand will be patched
ASM_PFX(gPatchRebasedFlagAddr32):
    mov     byte [eax], 1
    pop     eax
    jmp     dword [dword 0]                    ; destination will be patched
ASM_PFX(gPatchSmmRelocationOriginalAddressPtr32):

BITS 64
global ASM_PFX(SmmInitFixupAddress)
ASM_PFX(SmmInitFixupAddress):
    lea    rax, [@LongMode]
    lea    rcx, [@PatchLongModeOffset - 6]
    mov    dword [rcx], eax

    lea    rax, [ASM_PFX(SmmStartup)]
    lea    rcx, [@L1]
    mov    qword [rcx], rax
    ret
