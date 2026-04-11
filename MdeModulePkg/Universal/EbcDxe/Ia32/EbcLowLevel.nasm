;/** @file
;
;    This code provides low level routines that support the Virtual Machine
;    for option ROMs.
;
;  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/

;---------------------------------------------------------------------------
; Equate files needed.
;---------------------------------------------------------------------------

;---------------------------------------------------------------------------
; Assembler options
;---------------------------------------------------------------------------

SECTION .text
extern ASM_PFX(CopyMem)
extern ASM_PFX(EbcInterpret)
extern ASM_PFX(ExecuteEbcImageEntryPoint)

;****************************************************************************
; EbcLLCALLEXNative
;
; This function is called to execute an EBC CALLEX instruction
; to native code.
; This instruction requires that we thunk out to external native
; code. For IA32, we simply switch stacks and jump to the
; specified function. On return, we restore the stack pointer
; to its original location.
;
; Destroys no working registers.
;****************************************************************************
; INT64 EbcLLCALLEXNative(UINTN FuncAddr, UINTN NewStackPointer, VOID *FramePtr)
global ASM_PFX(EbcLLCALLEXNative)
ASM_PFX(EbcLLCALLEXNative):
      push   ebp
      push   ebx
      mov    ebp, esp              ; standard function prolog

      ; Get function address in a register
      ; mov ecx, FuncAddr => mov ecx, dword ptr [FuncAddr]
      mov    ecx, dword [esp + 0xC]

      ; Set stack pointer to new value
      ; mov eax, NewStackPointer => mov eax, dword ptr [NewSp]
      mov    eax, dword [esp + 0x14]
      mov    edx, dword [esp + 0x10]
      sub    eax, edx
      sub    esp, eax
      mov    ebx, esp
      push   ecx
      push   eax
      push   edx
      push   ebx
      call   ASM_PFX(CopyMem)
      pop    eax
      pop    eax
      pop    eax
      pop    ecx

      ; Now call the external routine
      call  ecx

      ; ebp is preserved by the callee. In this function it
      ; equals the original esp, so set them equal
      mov    esp, ebp

      ; Standard function epilog
      mov      esp, ebp
      pop      ebx
      pop      ebp
      ret

;****************************************************************************
; EbcLLEbcInterpret
;
; Begin executing an EBC image.
;****************************************************************************
; UINT64 EbcLLEbcInterpret(VOID)
global ASM_PFX(EbcLLEbcInterpret)
ASM_PFX(EbcLLEbcInterpret):
    ;
    ;; mov eax, 0xca112ebc
    ;; mov eax, EbcEntryPoint
    ;; mov ecx, EbcLLEbcInterpret
    ;; jmp ecx
    ;
    ; Caller uses above instruction to jump here
    ; The stack is below:
    ; +-----------+
    ; |  RetAddr  |
    ; +-----------+
    ; |EntryPoint | (EAX)
    ; +-----------+
    ; |   Arg1    | <- EDI
    ; +-----------+
    ; |   Arg2    |
    ; +-----------+
    ; |   ...     |
    ; +-----------+
    ; |   Arg16   |
    ; +-----------+
    ; |   EDI     |
    ; +-----------+
    ; |   ESI     |
    ; +-----------+
    ; |   EBP     | <- EBP
    ; +-----------+
    ; |  RetAddr  | <- ESP is here
    ; +-----------+
    ; |   Arg1    | <- ESI
    ; +-----------+
    ; |   Arg2    |
    ; +-----------+
    ; |   ...     |
    ; +-----------+
    ; |   Arg16   |
    ; +-----------+
    ;

    ; Construct new stack
    push ebp
    mov  ebp, esp
    push esi
    push edi
    sub  esp, 0x40
    push eax
    mov  esi, ebp
    add  esi, 8
    mov  edi, esp
    add  edi, 4
    mov  ecx, 16
    rep  movsd

    ; call C-code
    call ASM_PFX(EbcInterpret)
    add  esp, 0x44
    pop  edi
    pop  esi
    pop  ebp
    ret

;****************************************************************************
; EbcLLExecuteEbcImageEntryPoint
;
; Begin executing an EBC image.
;****************************************************************************
; UINT64 EbcLLExecuteEbcImageEntryPoint(VOID)
global ASM_PFX(EbcLLExecuteEbcImageEntryPoint)
ASM_PFX(EbcLLExecuteEbcImageEntryPoint):
    ;
    ;; mov eax, 0xca112ebc
    ;; mov eax, EbcEntryPoint
    ;; mov ecx, EbcLLExecuteEbcImageEntryPoint
    ;; jmp ecx
    ;
    ; Caller uses above instruction to jump here
    ; The stack is below:
    ; +-----------+
    ; |  RetAddr  |
    ; +-----------+
    ; |EntryPoint | (EAX)
    ; +-----------+
    ; |ImageHandle|
    ; +-----------+
    ; |SystemTable|
    ; +-----------+
    ; |  RetAddr  | <- ESP is here
    ; +-----------+
    ; |ImageHandle|
    ; +-----------+
    ; |SystemTable|
    ; +-----------+
    ;

    ; Construct new stack
    mov  [esp - 0xC], eax
    mov  eax, [esp + 0x4]
    mov  [esp - 0x8], eax
    mov  eax, [esp + 0x8]
    mov  [esp - 0x4], eax

    ; call C-code
    sub  esp, 0xC
    call ASM_PFX(ExecuteEbcImageEntryPoint)
    add  esp, 0xC
    ret

