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
;   SmiEntry.nasm
;
; Abstract:
;
;   Code template of the SMI handler for a particular processor
;
;-------------------------------------------------------------------------------

%define MSR_IA32_MISC_ENABLE 0x1A0
%define MSR_EFER      0xc0000080
%define MSR_EFER_XD   0x800

;
; Constants relating to PROCESSOR_SMM_DESCRIPTOR
;
%define DSC_OFFSET 0xfb00
%define DSC_GDTPTR 0x30
%define DSC_GDTSIZ 0x38
%define DSC_CS 14
%define DSC_DS 16
%define DSC_SS 18
%define DSC_OTHERSEG 20

%define PROTECT_MODE_CS 0x8
%define PROTECT_MODE_DS 0x20
%define TSS_SEGMENT 0x40

extern ASM_PFX(SmiRendezvous)
extern ASM_PFX(FeaturePcdGet (PcdCpuSmmStackGuard))
extern ASM_PFX(CpuSmmDebugEntry)
extern ASM_PFX(CpuSmmDebugExit)

global ASM_PFX(gcSmiHandlerTemplate)
global ASM_PFX(gcSmiHandlerSize)
global ASM_PFX(gPatchSmiCr3)
global ASM_PFX(gPatchSmiStack)
global ASM_PFX(gPatchSmbase)
global ASM_PFX(mXdSupported)
extern ASM_PFX(gSmiHandlerIdtr)

    SECTION .text

BITS 16
ASM_PFX(gcSmiHandlerTemplate):
_SmiEntryPoint:
    mov     bx, _GdtDesc - _SmiEntryPoint + 0x8000
    mov     ax,[cs:DSC_OFFSET + DSC_GDTSIZ]
    dec     ax
    mov     [cs:bx], ax
    mov     eax, [cs:DSC_OFFSET + DSC_GDTPTR]
    mov     [cs:bx + 2], eax
    mov     ebp, eax                      ; ebp = GDT base
o32 lgdt    [cs:bx]                       ; lgdt fword ptr cs:[bx]
    mov     ax, PROTECT_MODE_CS
    mov     [cs:bx-0x2],ax
    mov     edi, strict dword 0           ; source operand will be patched
ASM_PFX(gPatchSmbase):
    lea     eax, [edi + (@32bit - _SmiEntryPoint) + 0x8000]
    mov     [cs:bx-0x6],eax
    mov     ebx, cr0
    and     ebx, 0x9ffafff3
    or      ebx, 0x23
    mov     cr0, ebx
    jmp     dword 0x0:0x0
_GdtDesc:
    DW 0
    DD 0

BITS 32
@32bit:
    mov     ax, PROTECT_MODE_DS
o16 mov     ds, ax
o16 mov     es, ax
o16 mov     fs, ax
o16 mov     gs, ax
o16 mov     ss, ax
    mov esp, strict dword 0               ; source operand will be patched
ASM_PFX(gPatchSmiStack):
    mov     eax, ASM_PFX(gSmiHandlerIdtr)
    lidt    [eax]
    jmp     ProtFlatMode

ProtFlatMode:
    mov eax, strict dword 0               ; source operand will be patched
ASM_PFX(gPatchSmiCr3):
    mov     cr3, eax
;
; Need to test for CR4 specific bit support
;
    mov     eax, 1
    cpuid                               ; use CPUID to determine if specific CR4 bits are supported
    xor     eax, eax                    ; Clear EAX
    test    edx, BIT2                   ; Check for DE capabilities
    jz      .0
    or      eax, BIT3
.0:
    test    edx, BIT6                   ; Check for PAE capabilities
    jz      .1
    or      eax, BIT5
.1:
    test    edx, BIT7                   ; Check for MCE capabilities
    jz      .2
    or      eax, BIT6
.2:
    test    edx, BIT24                  ; Check for FXSR capabilities
    jz      .3
    or      eax, BIT9
.3:
    test    edx, BIT25                  ; Check for SSE capabilities
    jz      .4
    or      eax, BIT10
.4:                                     ; as cr4.PGE is not set here, refresh cr3
    mov     cr4, eax                    ; in PreModifyMtrrs() to flush TLB.

    cmp     byte [dword ASM_PFX(FeaturePcdGet (PcdCpuSmmStackGuard))], 0
    jz      .6
; Load TSS
    mov     byte [ebp + TSS_SEGMENT + 5], 0x89 ; clear busy flag
    mov     eax, TSS_SEGMENT
    ltr     ax
.6:

; enable NXE if supported
    DB      0b0h                        ; mov al, imm8
ASM_PFX(mXdSupported):     DB      1
    cmp     al, 0
    jz      @SkipXd
;
; Check XD disable bit
;
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    push    edx                        ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      .5
    and     dx, 0xFFFB                 ; clear XD Disable bit if it is set
    wrmsr
.5:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD             ; enable NXE
    wrmsr
    jmp     @XdDone
@SkipXd:
    sub     esp, 4
@XdDone:

    mov     ebx, cr0
    or      ebx, 0x80010023             ; enable paging + WP + NE + MP + PE
    mov     cr0, ebx
    lea     ebx, [edi + DSC_OFFSET]
    mov     ax, [ebx + DSC_DS]
    mov     ds, eax
    mov     ax, [ebx + DSC_OTHERSEG]
    mov     es, eax
    mov     fs, eax
    mov     gs, eax
    mov     ax, [ebx + DSC_SS]
    mov     ss, eax

;   jmp     _SmiHandler                 ; instruction is not needed

global ASM_PFX(SmiHandler)
ASM_PFX(SmiHandler):
    mov     ebx, [esp + 4]                  ; CPU Index
    push    ebx
    mov     eax, ASM_PFX(CpuSmmDebugEntry)
    call    eax
    add     esp, 4

    push    ebx
    mov     eax, ASM_PFX(SmiRendezvous)
    call    eax
    add     esp, 4

    push    ebx
    mov     eax, ASM_PFX(CpuSmmDebugExit)
    call    eax
    add     esp, 4

    mov     eax, ASM_PFX(mXdSupported)
    mov     al, [eax]
    cmp     al, 0
    jz      .7
    pop     edx                       ; get saved MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2
    jz      .7
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    or      dx, BIT2                  ; set XD Disable bit if it was set before entering into SMM
    wrmsr

.7:
    rsm

ASM_PFX(gcSmiHandlerSize): DW $ - _SmiEntryPoint

global ASM_PFX(PiSmmCpuSmiEntryFixupAddress)
ASM_PFX(PiSmmCpuSmiEntryFixupAddress):
    ret
