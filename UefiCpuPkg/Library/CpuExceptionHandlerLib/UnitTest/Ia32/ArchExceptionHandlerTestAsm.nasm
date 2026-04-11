;------------------------------------------------------------------------------
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   ArchExceptionHandlerTestAsm.nasm
;
; Abstract:
;
;   ia32 CPU Exception Handler Lib Unit test
;
;------------------------------------------------------------------------------

    SECTION .text

struc GENERAL_REGISTER_IA32
  .Edi:    resd    1
  .Esi:    resd    1
  .Ebx:    resd    1
  .Edx:    resd    1
  .Ecx:    resd    1
  .Eax:    resd    1

endstruc

extern ASM_PFX(mExpectedContextInHandler)
extern ASM_PFX(mActualContextAfterException)
extern ASM_PFX(mFaultInstructionLength)

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; TriggerGPException (
;  UINTN  Cr4ReservedBit
;  );
;------------------------------------------------------------------------------
global ASM_PFX(TriggerGPException)
ASM_PFX(TriggerGPException):
    ;
    ; Set reserved bit 15 of cr4 to 1
    ;
    lea  ecx, [ASM_PFX(mFaultInstructionLength)]
    mov  dword[ecx], TriggerGPExceptionAfter - TriggerGPExceptionBefore
    mov  ecx, dword [esp + 0x4]
TriggerGPExceptionBefore:
    mov  cr4, ecx
TriggerGPExceptionAfter:
    ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; TriggerPFException (
;  UINTN  PfAddress
;  );
;------------------------------------------------------------------------------
global ASM_PFX(TriggerPFException)
ASM_PFX(TriggerPFException):
    lea  ecx, [ASM_PFX(mFaultInstructionLength)]
    mov  dword[ecx], TriggerPFExceptionAfter - TriggerPFExceptionBefore
    mov  ecx, dword [esp + 0x4]
TriggerPFExceptionBefore:
    mov  dword[ecx], 0x1
TriggerPFExceptionAfter:
    ret

;------------------------------------------------------------------------------
; ModifyEcxInGlobalBeforeException;
; This function is writed by assebly code because it's only called in this file.
; It's used to set Ecx in mExpectedContextInHandler for different exception.
;------------------------------------------------------------------------------
global ASM_PFX(ModifyEcxInGlobalBeforeException)
ASM_PFX(ModifyEcxInGlobalBeforeException):
    push eax
    lea  eax, [ASM_PFX(mExpectedContextInHandler)]
    mov  [eax + GENERAL_REGISTER_IA32.Ecx], ecx
    pop  eax
    ret

;------------------------------------------------------------------------------
;VOID
;EFIAPI
;AsmTestConsistencyOfCpuContext (
;  IN  EFI_EXCEPTION_TYPE ExceptionType
;  IN  UINTN              FaultParameter   OPTIONAL
;  );
;------------------------------------------------------------------------------
global ASM_PFX(AsmTestConsistencyOfCpuContext)
ASM_PFX(AsmTestConsistencyOfCpuContext):
    ;
    ; push 7 general register plus 4 bytes
    ;
    pushad

    ;
    ; Modify register to mExpectedContextInHandler. Do not handle Esp and Ebp.
    ; CpuExceptionHandlerLib doesn't set Esp and Esp register to the value in SystemContext.
    ;
    lea eax, [ASM_PFX(mExpectedContextInHandler)]
    mov edi, [eax + GENERAL_REGISTER_IA32.Edi]
    mov esi, [eax + GENERAL_REGISTER_IA32.Esi]
    mov ebx, [eax + GENERAL_REGISTER_IA32.Ebx]
    mov edx, [eax + GENERAL_REGISTER_IA32.Edx]
    ;
    ; Set ecx to ExceptionType
    ;
    mov ecx, dword [esp + 0x24]
    mov eax, [eax + GENERAL_REGISTER_IA32.Eax]

    cmp  ecx, 0xd
    jz   GPException
    cmp  ecx, 0xe
    jz   PFException
    jmp  INTnException

PFException:
    mov  ecx, dword [esp + 0x28]                    ; Set ecx to PFAddress.
    call ASM_PFX(ModifyEcxInGlobalBeforeException)  ; Set mExpectedContextInHandler.Ecx to PFAddress.
    push ecx                                        ; Push PfAddress into stack.
    call ASM_PFX(TriggerPFException)
    jmp  AfterException

GPException:
    mov  ecx, dword [esp + 0x28]                    ; Set ecx to CR4_RESERVED_BIT.
    call ASM_PFX(ModifyEcxInGlobalBeforeException)  ; Set mExpectedContextInHandler.Ecx to CR4_RESERVED_BIT.
    push ecx                                        ; Push CR4_RESERVED_BIT into stack.
    call ASM_PFX(TriggerGPException)
    jmp  AfterException

INTnException:
    call ASM_PFX(ModifyEcxInGlobalBeforeException)  ; Set mExpectedContextInHandler.Ecx to ExceptionType.
    push ecx                                        ; Push ExceptionType into stack.
    call ASM_PFX(TriggerINTnException)

AfterException:
    ;
    ; Save register in mActualContextAfterException.
    ;
    push eax
    lea  eax, [ASM_PFX(mActualContextAfterException)]
    mov  [eax + GENERAL_REGISTER_IA32.Edi], edi
    mov  [eax + GENERAL_REGISTER_IA32.Esi], esi
    mov  [eax + GENERAL_REGISTER_IA32.Ebx], ebx
    mov  [eax + GENERAL_REGISTER_IA32.Edx], edx
    mov  [eax + GENERAL_REGISTER_IA32.Ecx], ecx
    pop  ecx
    mov  [eax + GENERAL_REGISTER_IA32.Eax], ecx
    add  esp, 4

    ;
    ; restore original register
    ;
    popad
    ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; TriggerStackOverflow (
;  VOID
;  );
;------------------------------------------------------------------------------
global ASM_PFX(TriggerStackOverflow)
ASM_PFX(TriggerStackOverflow):
    lea  ecx, [ASM_PFX(mFaultInstructionLength)]
    mov  dword[ecx], TriggerCpuStackGuardAfter - TriggerCpuStackGuardBefore
TriggerCpuStackGuardBefore:
    ;
    ; Clear CR0.TS since it is set after return from a nested DF
    ;
    call TriggerCpuStackGuardBefore
    clts
TriggerCpuStackGuardAfter:
    ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; TriggerINTnException (
;  IN  EFI_EXCEPTION_TYPE ExceptionType
;  );
;------------------------------------------------------------------------------
global ASM_PFX(TriggerINTnException)
ASM_PFX(TriggerINTnException):
    push eax
    push edx
    lea  eax, [AsmTriggerException1 - AsmTriggerException0]
    mov  ecx, dword [esp + 0xc]
    push ecx
    mul  ecx
    mov  ecx, AsmTriggerException0
    add  eax, ecx
    pop  ecx
    pop  edx
    jmp  eax
    ;
    ; eax = AsmTriggerException0 + (AsmTriggerException1 - AsmTriggerException0) * ecx
    ;
%assign Vector 0
%rep  22
AsmTriggerException %+ Vector:
    pop eax
    INT Vector
    ret
%assign Vector Vector+1
%endrep
