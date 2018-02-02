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

global ASM_PFX(gPatchSmmCr3)
global ASM_PFX(gPatchSmmCr4)
global ASM_PFX(gSmmCr0)
global ASM_PFX(gSmmJmpAddr)
global ASM_PFX(gSmmInitStack)
global ASM_PFX(gcSmiInitGdtr)
global ASM_PFX(gcSmmInitSize)
global ASM_PFX(gcSmmInitTemplate)
global ASM_PFX(mRebasedFlagAddr32)
global ASM_PFX(mSmmRelocationOriginalAddressPtr32)

    DEFAULT REL
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
    mov     eax, strict dword 0         ; source operand will be patched
ASM_PFX(gPatchSmmCr3):
    mov     cr3, eax
o32 lgdt    [cs:ebp + (ASM_PFX(gcSmiInitGdtr) - ASM_PFX(SmmStartup))]
    mov     eax, strict dword 0         ; source operand will be patched
ASM_PFX(gPatchSmmCr4):
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
    DB      0x66, 0xb8                   ; mov eax, imm32
ASM_PFX(gSmmCr0): DD 0
    mov     cr0, eax                    ; enable protected mode & paging
    DB      0x66, 0xea                   ; far jmp to long mode
ASM_PFX(gSmmJmpAddr): DQ 0;@LongMode

BITS 64
@LongMode:                              ; long-mode starts here
    DB      0x48, 0xbc                   ; mov rsp, imm64
ASM_PFX(gSmmInitStack): DQ 0
    and     sp, 0xfff0                  ; make sure RSP is 16-byte aligned
    ;
    ; Accoring to X64 calling convention, XMM0~5 are volatile, we need to save
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
global ASM_PFX(SmmRelocationSemaphoreComplete32)
ASM_PFX(SmmRelocationSemaphoreComplete32):
    ;
    ; mov byte ptr [], 1
    ;
    db      0xc6, 0x5
ASM_PFX(mRebasedFlagAddr32): dd 0
    db      1
    ;
    ; jmp dword ptr []
    ;
    db      0xff, 0x25
ASM_PFX(mSmmRelocationOriginalAddressPtr32): dd 0

global ASM_PFX(PiSmmCpuSmmInitFixupAddress)
ASM_PFX(PiSmmCpuSmmInitFixupAddress):
    lea    rax, [@LongMode]
    lea    rcx, [ASM_PFX(gSmmJmpAddr)]
    mov    qword [rcx], rax

    lea    rax, [ASM_PFX(SmmStartup)]
    lea    rcx, [@L1]
    mov    qword [rcx], rax
    ret
