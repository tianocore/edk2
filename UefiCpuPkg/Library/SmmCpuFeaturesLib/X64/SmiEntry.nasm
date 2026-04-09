;------------------------------------------------------------------------------ ;
; Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
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

%include "StuffRsbNasm.inc"

;
; Variables referenced by C code
;

%define MSR_IA32_MISC_ENABLE 0x1A0
%define MSR_EFER      0xc0000080
%define MSR_EFER_XD   0x800

;
; Constants relating to TXT_PROCESSOR_SMM_DESCRIPTOR
;
%define DSC_OFFSET 0xfb00
%define DSC_GDTPTR 0x48
%define DSC_GDTSIZ 0x50
%define DSC_CS 0x14
%define DSC_DS 0x16
%define DSC_SS 0x18
%define DSC_OTHERSEG 0x1a
;
; Constants relating to CPU State Save Area
;
%define SSM_DR6 0xffd0
%define SSM_DR7 0xffc8

%define PROTECT_MODE_CS 0x8
%define PROTECT_MODE_DS 0x20
%define LONG_MODE_CS 0x38
%define TSS_SEGMENT 0x40
%define GDT_SIZE 0x50

extern ASM_PFX(SmiRendezvous)
extern ASM_PFX(gStmSmiHandlerIdtr)
extern ASM_PFX(CpuSmmDebugEntry)
extern ASM_PFX(CpuSmmDebugExit)

global ASM_PFX(gStmSmbase)
global ASM_PFX(gStmXdSupported)
global ASM_PFX(gStmSmiStack)
global ASM_PFX(gStmSmiCr3)
global ASM_PFX(gcStmSmiHandlerTemplate)
global ASM_PFX(gcStmSmiHandlerSize)
global ASM_PFX(gcStmSmiHandlerOffset)

ASM_PFX(gStmSmbase)      EQU StmSmbasePatch - 4
ASM_PFX(gStmSmiStack)    EQU StmSmiStackPatch - 4
ASM_PFX(gStmSmiCr3)      EQU StmSmiCr3Patch - 4
ASM_PFX(gStmXdSupported) EQU StmXdSupportedPatch - 1

    DEFAULT REL
    SECTION .text

BITS 16
ASM_PFX(gcStmSmiHandlerTemplate):
_StmSmiEntryPoint:
    mov     bx, _StmGdtDesc - _StmSmiEntryPoint + 0x8000
    mov     ax,[cs:DSC_OFFSET + DSC_GDTSIZ]
    dec     ax
    mov     [cs:bx], ax
    mov     eax, [cs:DSC_OFFSET + DSC_GDTPTR]
    mov     [cs:bx + 2], eax
o32 lgdt    [cs:bx]                       ; lgdt fword ptr cs:[bx]
    mov     ax, PROTECT_MODE_CS
    mov     [cs:bx-0x2],ax
o32 mov     edi, strict dword 0
StmSmbasePatch:
    lea     eax, [edi + (@ProtectedMode - _StmSmiEntryPoint) + 0x8000]
    mov     [cs:bx-0x6],eax
    mov     ebx, cr0
    and     ebx, 0x9ffafff3
    or      ebx, 0x23
    mov     cr0, ebx
    jmp     dword 0x0:0x0
_StmGdtDesc:
    DW 0
    DD 0

BITS 32
@ProtectedMode:
    mov     ax, PROTECT_MODE_DS
o16 mov     ds, ax
o16 mov     es, ax
o16 mov     fs, ax
o16 mov     gs, ax
o16 mov     ss, ax
    mov     esp, strict dword 0
StmSmiStackPatch:
    jmp     ProtFlatMode

BITS 64
ProtFlatMode:
    mov     eax, strict dword 0
StmSmiCr3Patch:
    mov     cr3, rax
    mov     eax, 0x668                   ; as cr4.PGE is not set here, refresh cr3
    mov     cr4, rax                    ; in PreModifyMtrrs() to flush TLB.
; Load TSS
    sub     esp, 8                      ; reserve room in stack
    sgdt    [rsp]
    mov     eax, [rsp + 2]              ; eax = GDT base
    add     esp, 8
    mov     dl, 0x89
    mov     [rax + TSS_SEGMENT + 5], dl ; clear busy flag
    mov     eax, TSS_SEGMENT
    ltr     ax

; enable NXE if supported
    mov al, strict byte 1
StmXdSupportedPatch:
    cmp     al, 0
    jz      @SkipXd
;
; Check XD disable bit
;
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    sub     esp, 4
    push    rdx                        ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      .0
    and     dx, 0xFFFB                 ; clear XD Disable bit if it is set
    wrmsr
.0:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD            ; enable NXE
    wrmsr
    jmp     @XdDone
@SkipXd:
    sub     esp, 8
@XdDone:

; Switch into @LongMode
    push    LONG_MODE_CS                ; push cs hardcore here
    call    Base                       ; push return address for retf later
Base:
    add     dword [rsp], @LongMode - Base; offset for far retf, seg is the 1st arg

    mov     ecx, MSR_EFER
    rdmsr
    or      ah, 1                      ; enable LME
    wrmsr
    mov     rbx, cr0
    or      ebx, 0x80010023            ; enable paging + WP + NE + MP + PE
    mov     cr0, rbx
    retf
@LongMode:                              ; long mode (64-bit code) starts here
    mov     rax, strict qword 0         ;  mov     rax, ASM_PFX(gStmSmiHandlerIdtr)
StmSmiEntrySmiHandlerIdtrAbsAddr:
    lidt    [rax]
    lea     ebx, [rdi + DSC_OFFSET]
    mov     ax, [rbx + DSC_DS]
    mov     ds, eax
    mov     ax, [rbx + DSC_OTHERSEG]
    mov     es, eax
    mov     fs, eax
    mov     gs, eax
    mov     ax, [rbx + DSC_SS]
    mov     ss, eax
    mov     rax, strict qword 0           ;   mov     rax, CommonHandler
StmSmiEntryCommonHandlerAbsAddr:
    jmp     rax
CommonHandler:
    mov     rbx, [rsp + 0x08]             ; rbx <- CpuIndex

    ;
    ; Save FP registers
    ;
    sub     rsp, 0x200
    fxsave64 [rsp]

    add     rsp, -0x20

    mov     rcx, rbx
    call    ASM_PFX(CpuSmmDebugEntry)

    mov     rcx, rbx
    call    ASM_PFX(SmiRendezvous)

    mov     rcx, rbx
    call    ASM_PFX(CpuSmmDebugExit)

    add     rsp, 0x20

    ;
    ; Restore FP registers
    ;
    fxrstor64 [rsp]

    add     rsp, 0x200

    lea     rax, [ASM_PFX(gStmXdSupported)]
    mov     al, [rax]
    cmp     al, 0
    jz      .1
    pop     rdx                       ; get saved MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2
    jz      .1
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    or      dx, BIT2                  ; set XD Disable bit if it was set before entering into SMM
    wrmsr

.1:
    StuffRsb64
    rsm

_StmSmiHandler:
;
; Check XD disable bit
;
    xor     r8, r8
    lea     rax, [ASM_PFX(gStmXdSupported)]
    mov     al, [rax]
    cmp     al, 0
    jz      @StmXdDone
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    mov     r8, rdx                    ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      .0
    and     dx, 0xFFFB                 ; clear XD Disable bit if it is set
    wrmsr
.0:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD            ; enable NXE
    wrmsr
@StmXdDone:
    push    r8

    ; below step is needed, because STM does not run above code.
    ; we have to run below code to set IDT/CR0/CR4
    mov     rax, strict qword 0        ;  mov     rax, ASM_PFX(gStmSmiHandlerIdtr)
StmSmiHandlerIdtrAbsAddr:
    lidt    [rax]

    mov     rax, cr0
    or      eax, 0x80010023            ; enable paging + WP + NE + MP + PE
    mov     cr0, rax
    mov     rax, cr4
    mov     eax, 0x668                 ; as cr4.PGE is not set here, refresh cr3
    mov     cr4, rax                   ; in PreModifyMtrrs() to flush TLB.
    ; STM init finish
    jmp     CommonHandler

ASM_PFX(gcStmSmiHandlerSize)   : DW      $ - _StmSmiEntryPoint
ASM_PFX(gcStmSmiHandlerOffset) : DW      _StmSmiHandler - _StmSmiEntryPoint

global ASM_PFX(SmmCpuFeaturesLibStmSmiEntryFixupAddress)
ASM_PFX(SmmCpuFeaturesLibStmSmiEntryFixupAddress):
    lea    rax, [ASM_PFX(gStmSmiHandlerIdtr)]
    lea    rcx, [StmSmiEntrySmiHandlerIdtrAbsAddr]
    mov    qword [rcx - 8], rax
    lea    rcx, [StmSmiHandlerIdtrAbsAddr]
    mov    qword [rcx - 8], rax

    lea    rax, [CommonHandler]
    lea    rcx, [StmSmiEntryCommonHandlerAbsAddr]
    mov    qword [rcx - 8], rax
    ret
