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
;   x64 CPU Exception Handler Lib Unit test
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

struc GENERAL_REGISTER
  .Rdi:    resq    1
  .Rsi:    resq    1
  .Rbx:    resq    1
  .Rdx:    resq    1
  .Rcx:    resq    1
  .Rax:    resq    1
  .R8:     resq    1
  .R9:     resq    1
  .R10:    resq    1
  .R11:    resq    1
  .R12:    resq    1
  .R13:    resq    1
  .R14:    resq    1
  .R15:    resq    1

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
    push rcx
    lea  rcx, [ASM_PFX(mFaultInstructionLength)]
    mov  qword[rcx], TriggerGPExceptionAfter - TriggerGPExceptionBefore
    pop  rcx
TriggerGPExceptionBefore:
    mov  cr4, rcx
TriggerGPExceptionAfter:
    ret

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; TriggerPFException (
;  UINTN  PFAddress
;  );
;------------------------------------------------------------------------------
global ASM_PFX(TriggerPFException)
ASM_PFX(TriggerPFException):
    push rcx
    lea  rcx, [ASM_PFX(mFaultInstructionLength)]
    mov  qword[rcx], TriggerPFExceptionAfter - TriggerPFExceptionBefore
    pop  rcx
TriggerPFExceptionBefore:
    mov  qword[rcx], 0x1
TriggerPFExceptionAfter:
    ret

;------------------------------------------------------------------------------
; ModifyRcxInGlobalBeforeException;
; This function is writed by assebly code because it's only called in this file.
; It's used to set Rcx in mExpectedContextInHandler for different exception.
;------------------------------------------------------------------------------
global ASM_PFX(ModifyRcxInGlobalBeforeException)
ASM_PFX(ModifyRcxInGlobalBeforeException):
    push rax
    lea  rax, [ASM_PFX(mExpectedContextInHandler)]
    mov  [rax + GENERAL_REGISTER.Rcx], rcx
    pop  rax
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
    ; Push original register
    ;
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rax
    push rcx
    push rbx
    push rsi
    push rdi
    push rdx
    push rdx

    ;
    ; Modify registers to mExpectedContextInHandler. Do not handle Rsp and Rbp.
    ; CpuExceptionHandlerLib doesn't set Rsp and Rsp register to the value in SystemContext.
    ;
    lea r15, [ASM_PFX(mExpectedContextInHandler)]
    mov rdi, [r15 + GENERAL_REGISTER.Rdi]
    mov rsi, [r15 + GENERAL_REGISTER.Rsi]
    mov rbx, [r15 + GENERAL_REGISTER.Rbx]
    mov rdx, [r15 + GENERAL_REGISTER.Rdx]
    mov rax, [r15 + GENERAL_REGISTER.Rax]
    mov r8,  [r15 + GENERAL_REGISTER.R8]
    mov r9,  [r15 + GENERAL_REGISTER.R9]
    mov r10, [r15 + GENERAL_REGISTER.R10]
    mov r11, [r15 + GENERAL_REGISTER.R11]
    mov r12, [r15 + GENERAL_REGISTER.R12]
    mov r13, [r15 + GENERAL_REGISTER.R13]
    mov r14, [r15 + GENERAL_REGISTER.R14]
    mov r15, [r15 + GENERAL_REGISTER.R15]

    cmp  rcx, 0xd
    jz   GPException
    cmp  rcx, 0xe
    jz   PFException
    jmp  INTnException

PFException:
    pop  rcx                                       ; Pop rdx(PFAddress) to rcx.
    call ASM_PFX(ModifyRcxInGlobalBeforeException) ; Set mExpectedContextInHandler.Rcx to PFAddress.
    call ASM_PFX(TriggerPFException)
    jmp  AfterException

GPException:
    pop  rcx                                       ; Pop rdx(Cr4ReservedBit) to rcx.
    call ASM_PFX(ModifyRcxInGlobalBeforeException) ; Set mExpectedContextInHandler.Rcx to Cr4ReservedBit.
    call ASM_PFX(TriggerGPException)
    jmp  AfterException

INTnException:
    ;
    ; Modify Rcx in mExpectedContextInHandler.
    ;
    add  Rsp, 8                                    ; Discard the extra Rdx in stack. Rcx is ExceptionType now.
    call ASM_PFX(ModifyRcxInGlobalBeforeException) ; Set mExpectedContextInHandler.Rcx to ExceptionType.
    call ASM_PFX(TriggerINTnException)

AfterException:
    ;
    ; Save registers in mActualContextAfterException
    ;
    push rax
    lea  rax, [ASM_PFX(mActualContextAfterException)]
    mov  [rax + GENERAL_REGISTER.Rdi], rdi
    mov  [rax + GENERAL_REGISTER.Rsi], rsi
    mov  [rax + GENERAL_REGISTER.Rbx], rbx
    mov  [rax + GENERAL_REGISTER.Rdx], rdx
    mov  [rax + GENERAL_REGISTER.Rcx], rcx
    pop  rcx
    mov  [rax + GENERAL_REGISTER.Rax], rcx
    mov  [rax + GENERAL_REGISTER.R8],  r8
    mov  [rax + GENERAL_REGISTER.R9],  r9
    mov  [rax + GENERAL_REGISTER.R10], r10
    mov  [rax + GENERAL_REGISTER.R11], r11
    mov  [rax + GENERAL_REGISTER.R12], r12
    mov  [rax + GENERAL_REGISTER.R13], r13
    mov  [rax + GENERAL_REGISTER.R14], r14
    mov  [rax + GENERAL_REGISTER.R15], r15

    ;
    ; restore original register
    ;
    pop rdx
    pop rdi
    pop rsi
    pop rbx
    pop rcx
    pop rax
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

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
    push rcx
    lea  rcx, [ASM_PFX(mFaultInstructionLength)]
    mov  qword[rcx], TriggerCpuStackGuardAfter - TriggerCpuStackGuardBefore
    pop  rcx
TriggerCpuStackGuardBefore:
    call TriggerCpuStackGuardBefore
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
    push rax
    push rdx
    push rcx
    lea  rax, [AsmTriggerException1 - AsmTriggerException0]
    mul  rcx
    mov  rcx, AsmTriggerException0
    add  rax, rcx
    pop  rcx
    pop  rdx
    jmp  rax
    ;
    ; rax = AsmTriggerException0 + (AsmTriggerException1 - AsmTriggerException0) * rcx
    ;
%assign Vector 0
%rep  22
AsmTriggerException %+ Vector:
    pop rax
    INT Vector
    ret
%assign Vector Vector+1
%endrep
