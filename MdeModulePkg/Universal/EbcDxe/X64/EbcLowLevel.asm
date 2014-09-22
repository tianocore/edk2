;/** @file
;  
;    This code provides low level routines that support the Virtual Machine.
;    for option ROMs.
;  
;  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
;  Copyright (c) 2014 Hewlett-Packard Development Company, L.P.<BR>
;  This program and the accompanying materials
;  are licensed and made available under the terms and conditions of the BSD License
;  which accompanies this distribution.  The full text of the license may be found at
;  http://opensource.org/licenses/bsd-license.php
;  
;  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;  
;**/

  page    ,132
  title   VM ASSEMBLY LANGUAGE ROUTINES

;---------------------------------------------------------------------------
; Equate files needed.
;---------------------------------------------------------------------------

.CODE

CopyMem  PROTO  Destination:PTR DWORD, Source:PTR DWORD, Count:DWORD
EbcInterpret               PROTO
ExecuteEbcImageEntryPoint  PROTO

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
EbcLLCALLEXNative        PROC    PUBLIC
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
      cmp    r8, 20h
      jae    skip_expansion
      mov    r8, 20h
skip_expansion:
      
      sub    rsp, r8

      ;
      ; Fix X64 native function call 16-byte alignment.
      ;
      ; From MSDN x64 Software Conventions, Stack Usage:
      ;   "The stack will always be maintained 16-byte aligned, except within 
      ;   the prolog (for example, after the return address is pushed)."
      ;
      and    rsp, NOT 0fh

      mov    rcx, rsp
      sub    rsp, 20h
      call   CopyMem
      add    rsp, 20h

      ; Considering the worst case, load 4 potiential arguments
      ; into registers.
      mov    rcx, qword ptr [rsp]
      mov    rdx, qword ptr [rsp+8h]
      mov    r8,  qword ptr [rsp+10h]
      mov    r9,  qword ptr [rsp+18h]

      ; Now call the external routine
      call  rbx

      ; Function epilog
      mov      rsp, rbp
      pop      rbx
      pop      rbp
      ret
EbcLLCALLEXNative    ENDP

;****************************************************************************
; EbcLLEbcInterpret
;
; Begin executing an EBC image.
;****************************************************************************
; UINT64 EbcLLEbcInterpret(VOID)
EbcLLEbcInterpret PROC PUBLIC
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
    mov  [rsp + 08h], rcx
    mov  [rsp + 10h], rdx
    mov  [rsp + 18h], r8
    mov  [rsp + 20h], r9

    ; Construct new stack
    push rbp
    mov  rbp, rsp
    push rsi
    push rdi
    push rbx
    sub  rsp, 80h
    push r10
    mov  rsi, rbp
    add  rsi, 10h
    mov  rdi, rsp
    add  rdi, 8
    mov  rcx, 16
    rep  movsq
    
    ; build new paramater calling convention
    mov  r9,  [rsp + 18h]
    mov  r8,  [rsp + 10h]
    mov  rdx, [rsp + 08h]
    mov  rcx, r10

    ; call C-code
    call EbcInterpret
    add  rsp, 88h
    pop  rbx
    pop  rdi
    pop  rsi
    pop  rbp
    ret
EbcLLEbcInterpret ENDP

;****************************************************************************
; EbcLLExecuteEbcImageEntryPoint
;
; Begin executing an EBC image.
;****************************************************************************
; UINT64 EbcLLExecuteEbcImageEntryPoint(VOID)
EbcLLExecuteEbcImageEntryPoint PROC PUBLIC
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
    sub  rsp, 28h
    call ExecuteEbcImageEntryPoint
    add  rsp, 28h
    ret
EbcLLExecuteEbcImageEntryPoint ENDP

END

