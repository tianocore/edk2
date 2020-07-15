;------------------------------------------------------------------------------ ;
; Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   SmiException.nasm
;
; Abstract:
;
;   Exception handlers used in SM mode
;
;-------------------------------------------------------------------------------

%include "StuffRsbNasm.inc"

global  ASM_PFX(gcStmPsd)

extern  ASM_PFX(SmmStmExceptionHandler)
extern  ASM_PFX(SmmStmSetup)
extern  ASM_PFX(SmmStmTeardown)
extern  ASM_PFX(gStmXdSupported)
extern  ASM_PFX(gStmSmiHandlerIdtr)

%define MSR_IA32_MISC_ENABLE 0x1A0
%define MSR_EFER      0xc0000080
%define MSR_EFER_XD   0x800

CODE_SEL          equ 0x08
DATA_SEL          equ 0x20
TSS_SEL           equ 0x40

    SECTION .data

ASM_PFX(gcStmPsd):
            DB      'TXTPSSIG'
            DW      PSD_SIZE
            DW      1              ; Version
            DD      0              ; LocalApicId
            DB      0x05           ; Cr4Pse;Cr4Pae;Intel64Mode;ExecutionDisableOutsideSmrr
            DB      0              ; BIOS to STM
            DB      0              ; STM to BIOS
            DB      0
            DW      CODE_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      TSS_SEL
            DW      0
            DQ      0              ; SmmCr3
            DD      ASM_PFX(OnStmSetup)
            DD      0
            DD      ASM_PFX(OnStmTeardown)
            DD      0
            DQ      0              ; SmmSmiHandlerRip - SMM guest entrypoint
            DQ      0              ; SmmSmiHandlerRsp
            DQ      0
            DD      0
            DD      0x80010100     ; RequiredStmSmmRevId
            DD      ASM_PFX(OnException)
            DD      0
            DQ      0              ; ExceptionStack
            DW      DATA_SEL
            DW      0x01F          ; ExceptionFilter
            DD      0
            DD      0
            DD      0
            DQ      0              ; BiosHwResourceRequirementsPtr
            DQ      0              ; AcpiRsdp
            DB      0              ; PhysicalAddressBits
PSD_SIZE  equ $ - ASM_PFX(gcStmPsd)

    SECTION .text
;------------------------------------------------------------------------------
; SMM Exception handlers
;------------------------------------------------------------------------------
global ASM_PFX(OnException)
ASM_PFX(OnException):
    mov  ecx, esp
    push ecx
    call ASM_PFX(SmmStmExceptionHandler)
    add  esp, 4

    mov  ebx, eax
    mov  eax, 4
    vmcall
    jmp $

global ASM_PFX(OnStmSetup)
ASM_PFX(OnStmSetup):
;
; Check XD disable bit
;
    xor     esi, esi
    mov     eax, ASM_PFX(gStmXdSupported)
    mov     al, [eax]
    cmp     al, 0
    jz      @StmXdDone1
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    mov     esi, edx                   ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      .51
    and     dx, 0xFFFB                 ; clear XD Disable bit if it is set
    wrmsr
.51:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD             ; enable NXE
    wrmsr
@StmXdDone1:
    push    esi

  call ASM_PFX(SmmStmSetup)

    mov     eax, ASM_PFX(gStmXdSupported)
    mov     al, [eax]
    cmp     al, 0
    jz      .71
    pop     edx                       ; get saved MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2
    jz      .71
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    or      dx, BIT2                  ; set XD Disable bit if it was set before entering into SMM
    wrmsr

.71:
    StuffRsb32
    rsm

global  ASM_PFX(OnStmTeardown)
ASM_PFX(OnStmTeardown):
;
; Check XD disable bit
;
    xor     esi, esi
    mov     eax, ASM_PFX(gStmXdSupported)
    mov     al, [eax]
    cmp     al, 0
    jz      @StmXdDone2
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    mov     esi, edx                   ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      .52
    and     dx, 0xFFFB                 ; clear XD Disable bit if it is set
    wrmsr
.52:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD             ; enable NXE
    wrmsr
@StmXdDone2:
    push    esi

  call ASM_PFX(SmmStmTeardown)

    mov     eax, ASM_PFX(gStmXdSupported)
    mov     al, [eax]
    cmp     al, 0
    jz      .72
    pop     edx                       ; get saved MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2
    jz      .72
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    or      dx, BIT2                  ; set XD Disable bit if it was set before entering into SMM
    wrmsr

.72:
    StuffRsb32
    rsm
