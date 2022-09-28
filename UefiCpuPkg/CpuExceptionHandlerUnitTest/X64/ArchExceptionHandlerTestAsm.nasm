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
  .Rbp:    resq    1
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

extern ASM_PFX(mOriginGeneralRegister)
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
;  UINTN  ReadOnlyAddr
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
    ; push original register
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

    ;
    ; modify [0]
    ;
    mov rdi, 0x1
    mov rsi, 0x2
    ;mov rbp, 0x3
    mov rbx, 0x4
    mov rdx, 0x5
    ;
    ; Do not modify rcx
    ;
    mov rax, 0x7
    mov r8,  0x8
    mov r9,  0x9
    mov r10, 0xa
    mov r11, 0xb
    mov r12, 0xc
    mov r13, 0xd
    mov r14, 0xe
    mov r15, 0xf

    cmp  rcx, 0xd
    jz   GPException
    cmp  rcx, 0xe
    jz   PFException
    jmp  INTnException

PFException:
    lea  rcx, [ASM_PFX(mIsDxeDriver)]
    cmp  byte[rcx], 0
    jz   PeiPFException
    jmp  DxePFException
PeiPFException:
    ;
    ; pop rdx(Pei StackBase) to rcx and restore stack
    ;
    pop  rcx
    push rcx
    jmp  PFExceptionInternal
DxePFException:
    mov  rcx, cr3
PFExceptionInternal:
    call ASM_PFX(TriggerPFException)
    jmp  AfterException

GPException:
    ;
    ; Prepare rcx value for GP
    ;
    mov  rcx, 0x8000
    call ASM_PFX(TriggerGPException)
    jmp  AfterException

INTnException:
    call ASM_PFX(TriggerINTnException)

AfterException:
    ;
    ; store [1]
    ;
    push rax
    lea  rax, [ASM_PFX(mOriginGeneralRegister)]
    add  rax, GENERAL_REGISTER_size
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
; TriggerStackOverflowbyCpuStackGuard (
;  VOID
;  );
;------------------------------------------------------------------------------
global ASM_PFX(TriggerStackOverflowbyCpuStackGuard)
ASM_PFX(TriggerStackOverflowbyCpuStackGuard):
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
