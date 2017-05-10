;------------------------------------------------------------------------------ ;
; Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
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
;   SmiException.asm
;
; Abstract:
;
;   Exception handlers used in SM mode
;
;-------------------------------------------------------------------------------

    .686p
    .model  flat,C

EXTERNDEF   gcStmPsd:BYTE

EXTERNDEF   SmmStmExceptionHandler:PROC
EXTERNDEF   SmmStmSetup:PROC
EXTERNDEF   SmmStmTeardown:PROC
EXTERNDEF   gStmXdSupported:BYTE

CODE_SEL    = 08h
DATA_SEL    = 20h
TSS_SEL     = 40h

MSR_IA32_MISC_ENABLE  EQU     1A0h
MSR_EFER              EQU     0c0000080h
MSR_EFER_XD           EQU     0800h

    .data

gcStmPsd     LABEL   BYTE
            DB      'TXTPSSIG'
            DW      PSD_SIZE
            DW      1              ; Version
            DD      0              ; LocalApicId
            DB      05h            ; Cr4Pse;Cr4Pae;Intel64Mode;ExecutionDisableOutsideSmrr
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
            DQ      _OnStmSetup
            DQ      _OnStmTeardown
            DQ      0              ; SmmSmiHandlerRip - SMM guest entrypoint
            DQ      0              ; SmmSmiHandlerRsp
            DQ      0
            DD      0
            DD      80010100h      ; RequiredStmSmmRevId
            DQ      _OnException
            DQ      0              ; ExceptionStack
            DW      DATA_SEL
            DW      01Fh           ; ExceptionFilter
            DD      0
            DQ      0
            DQ      0              ; BiosHwResourceRequirementsPtr
            DQ      0              ; AcpiRsdp
            DB      0              ; PhysicalAddressBits
PSD_SIZE  = $ - offset gcStmPsd

    .code
;------------------------------------------------------------------------------
; SMM Exception handlers
;------------------------------------------------------------------------------
_OnException       PROC
    mov  ecx, esp
    push ecx
    call SmmStmExceptionHandler
    add  esp, 4

    mov  ebx, eax
    mov  eax, 4
    DB  0fh, 01h, 0c1h ; VMCALL
    jmp $
_OnException       ENDP

_OnStmSetup PROC
;
; Check XD disable bit
;
    xor     esi, esi
    mov     eax, offset gStmXdSupported
    mov     al, [eax]
    cmp     al, 0
    jz      @StmXdDone1
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    mov     esi, edx                   ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      @f
    and     dx, 0FFFBh                 ; clear XD Disable bit if it is set
    wrmsr
@@:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD             ; enable NXE
    wrmsr
@StmXdDone1:
    push    esi

  call SmmStmSetup

    mov     eax, offset gStmXdSupported
    mov     al, [eax]
    cmp     al, 0
    jz      @f
    pop     edx                       ; get saved MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2
    jz      @f
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    or      dx, BIT2                  ; set XD Disable bit if it was set before entering into SMM
    wrmsr
@@:

  rsm
_OnStmSetup ENDP

_OnStmTeardown PROC
;
; Check XD disable bit
;
    xor     esi, esi
    mov     eax, offset gStmXdSupported
    mov     al, [eax]
    cmp     al, 0
    jz      @StmXdDone2
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    mov     esi, edx                   ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      @f
    and     dx, 0FFFBh                 ; clear XD Disable bit if it is set
    wrmsr
@@:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD             ; enable NXE
    wrmsr
@StmXdDone2:
    push    esi

  call SmmStmTeardown

    mov     eax, offset gStmXdSupported
    mov     al, [eax]
    cmp     al, 0
    jz      @f
    pop     edx                       ; get saved MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2
    jz      @f
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    or      dx, BIT2                  ; set XD Disable bit if it was set before entering into SMM
    wrmsr
@@:

  rsm
_OnStmTeardown ENDP

    END
