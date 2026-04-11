;/** @file
;
;    This code provides low level routines that support the Virtual Machine.
;    for option ROMs.
;
;  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
;  Copyright (c) 2014 Hewlett-Packard Development Company, L.P.<BR>
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/

;---------------------------------------------------------------------------
; Equate files needed.
;---------------------------------------------------------------------------

DEFAULT REL
SECTION .text

extern ASM_PFX(CopyMem)
extern ASM_PFX(EbcInterpret)
extern ASM_PFX(ExecuteEbcImageEntryPoint)

;****************************************************************************
; EbcLLCALLEX
;
; This function is called to execute an EBC CALLEX instruction.
; This instruction requires that we thunk out to external native
; code. For x64, we switch stacks, copy the arguments to the stack
; and jump to the specified function.
; On return, we restore the stack pointer to its original location.
;
; Destroys no working registers.
;****************************************************************************
; INT64 EbcLLCALLEXNative(UINTN FuncAddr, UINTN NewStackPointer, VOID *FramePtr)
global ASM_PFX(EbcLLCALLEXNative)
ASM_PFX(EbcLLCALLEXNative):
      push   rbp
      push   rbx
      mov    rbp, rsp
      ; Function prolog

      ; Copy FuncAddr to a preserved register.
      mov    rbx, rcx

      ; Set stack pointer to new value
      sub    r8,  rdx

      ;
      ; Fix X64 native function call prolog. Prepare space for at least 4 arguments,
      ; even if the native function's arguments are less than 4.
      ;
      ; From MSDN x64 Software Conventions, Overview of x64 Calling Conventions:
      ;   "The caller is responsible for allocating space for parameters to the
      ;   callee, and must always allocate sufficient space for the 4 register
      ;   parameters, even if the callee doesn't have that many parameters.
      ;   This aids in the simplicity of supporting C unprototyped functions,
      ;   and vararg C/C++ functions."
      ;
      cmp    r8, 0x20
      jae    skip_expansion
      mov    r8, dword 0x20
skip_expansion:

      sub    rsp, r8

      ;
      ; Fix X64 native function call 16-byte alignment.
      ;
      ; From MSDN x64 Software Conventions, Stack Usage:
      ;   "The stack will always be maintained 16-byte aligned, except within
      ;   the prolog (for example, after the return address is pushed)."
      ;
      and    rsp, ~ 0xf

      mov    rcx, rsp
      sub    rsp, 0x20
      call   ASM_PFX(CopyMem)
      add    rsp, 0x20

      ; Considering the worst case, load 4 potiential arguments
      ; into registers.
      mov    rcx, qword [rsp]
      mov    rdx, qword [rsp+0x8]
      mov    r8,  qword [rsp+0x10]
      mov    r9,  qword [rsp+0x18]

      ; Now call the external routine
      call  rbx

      ; Function epilog
      mov      rsp, rbp
      pop      rbx
      pop      rbp
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
    ;; mov rax, ca112ebccall2ebch
    ;; mov r10, EbcEntryPoint
    ;; mov r11, EbcLLEbcInterpret
    ;; jmp r11
    ;
    ; Caller uses above instruction to jump here
    ; The stack is below:
    ; +-----------+
    ; |  RetAddr  |
    ; +-----------+
    ; |EntryPoint | (R10)
    ; +-----------+
    ; |   Arg1    | <- RDI
    ; +-----------+
    ; |   Arg2    |
    ; +-----------+
    ; |   ...     |
    ; +-----------+
    ; |   Arg16   |
    ; +-----------+
    ; |   Dummy   |
    ; +-----------+
    ; |   RDI     |
    ; +-----------+
    ; |   RSI     |
    ; +-----------+
    ; |   RBP     | <- RBP
    ; +-----------+
    ; |  RetAddr  | <- RSP is here
    ; +-----------+
    ; |  Scratch1 | (RCX) <- RSI
    ; +-----------+
    ; |  Scratch2 | (RDX)
    ; +-----------+
    ; |  Scratch3 | (R8)
    ; +-----------+
    ; |  Scratch4 | (R9)
    ; +-----------+
    ; |   Arg5    |
    ; +-----------+
    ; |   Arg6    |
    ; +-----------+
    ; |   ...     |
    ; +-----------+
    ; |   Arg16   |
    ; +-----------+
    ;

    ; save old parameter to stack
    mov  [rsp + 0x8], rcx
    mov  [rsp + 0x10], rdx
    mov  [rsp + 0x18], r8
    mov  [rsp + 0x20], r9

    ; Construct new stack
    push rbp
    mov  rbp, rsp
    push rsi
    push rdi
    push rbx
    sub  rsp, 0x80
    push r10
    mov  rsi, rbp
    add  rsi, 0x10
    mov  rdi, rsp
    add  rdi, 8
    mov  rcx, dword 16
    rep  movsq

    ; build new paramater calling convention
    mov  r9,  [rsp + 0x18]
    mov  r8,  [rsp + 0x10]
    mov  rdx, [rsp + 0x8]
    mov  rcx, r10

    ; call C-code
    call ASM_PFX(EbcInterpret)
    add  rsp, 0x88
    pop  rbx
    pop  rdi
    pop  rsi
    pop  rbp
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
    ;; mov rax, ca112ebccall2ebch
    ;; mov r10, EbcEntryPoint
    ;; mov r11, EbcLLExecuteEbcImageEntryPoint
    ;; jmp r11
    ;
    ; Caller uses above instruction to jump here
    ; The stack is below:
    ; +-----------+
    ; |  RetAddr  |
    ; +-----------+
    ; |EntryPoint | (R10)
    ; +-----------+
    ; |ImageHandle|
    ; +-----------+
    ; |SystemTable|
    ; +-----------+
    ; |   Dummy   |
    ; +-----------+
    ; |   Dummy   |
    ; +-----------+
    ; |  RetAddr  | <- RSP is here
    ; +-----------+
    ; |ImageHandle| (RCX)
    ; +-----------+
    ; |SystemTable| (RDX)
    ; +-----------+
    ;

    ; build new paramater calling convention
    mov  r8, rdx
    mov  rdx, rcx
    mov  rcx, r10

    ; call C-code
    sub  rsp, 0x28
    call ASM_PFX(ExecuteEbcImageEntryPoint)
    add  rsp, 0x28
    ret

