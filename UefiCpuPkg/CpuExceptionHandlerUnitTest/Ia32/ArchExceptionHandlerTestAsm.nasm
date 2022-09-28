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
  .Ebp:    resd    1
  .Ebx:    resd    1
  .Edx:    resd    1
  .Ecx:    resd    1
  .Eax:    resd    1

endstruc

extern ASM_PFX(mOriginIa32GeneralRegister)
extern ASM_PFX(mFaultInstructionLength)
extern ASM_PFX(mIsDxeDriver)

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
;  UINTN  ReadOnlyAddr
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
;VOID
;EFIAPI
;AsmTestConsistencyOfCpuContext (
;  IN  EFI_EXCEPTION_TYPE ExceptionType
;  IN  UINTN              OptionalPeiStackBase   OPTIONAL
;  );
;------------------------------------------------------------------------------
global ASM_PFX(AsmTestConsistencyOfCpuContext)
ASM_PFX(AsmTestConsistencyOfCpuContext):
    ;
    ; push 7 general register plus 4 bytes
    ;
    pushad

    ;
    ; modify [0]
    ;
    mov edi, 0x1
    mov esi, 0x2
    mov ebx, 0x4
    mov edx, 0x5
    ;
    ; Set ecx to first input value which is align to X64 code
    ;
    mov ecx, dword [esp + 0x24]
    mov eax, 0x7

    cmp  ecx, 0xd
    jz   GPException
    cmp  ecx, 0xe
    jz   PFException
    jmp  INTnException

PFException:
    lea  ecx, [ASM_PFX(mIsDxeDriver)]
    cmp  byte[ecx], 0
    jz   PeiPFException
    jmp  DxePFException

    ;
    ; Push ReadOnlyAddr into stack
    ;
PeiPFException:
    push dword [esp + 0x28]
    jmp  PFExceptionInternal
DxePFException:
    mov  ecx, cr3
    push ecx
PFExceptionInternal:
    call ASM_PFX(TriggerPFException)
    jmp  AfterException

GPException:
    ;
    ; Push GP_CR4_RESERVED_BIT into stack
    ;
    push 0x8000
    call ASM_PFX(TriggerGPException)
    jmp  AfterException

INTnException:
    ;
    ; Push exception index into stack
    ;
    push ecx
    call ASM_PFX(TriggerINTnException)

AfterException:
    ;
    ; Store [1]. Esp and ebp will not be synced to the value in SystemContext
    ; Do not test esp and ebp
    ;
    push eax
    lea  eax, [ASM_PFX(mOriginIa32GeneralRegister)]
    add  eax, GENERAL_REGISTER_IA32_size
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
; TriggerStackOverflowbyCpuStackGuard (
;  VOID
;  );
;------------------------------------------------------------------------------
global ASM_PFX(TriggerStackOverflowbyCpuStackGuard)
ASM_PFX(TriggerStackOverflowbyCpuStackGuard):
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
