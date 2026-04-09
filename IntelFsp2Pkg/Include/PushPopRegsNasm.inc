;------------------------------------------------------------------------------
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   Provide macro to push/pop registers in X64
;
;------------------------------------------------------------------------------

;-----------------------------------------------------------------------------
;  Macro:        PUSHA_64
;
;  Description:  Saves all registers on stack
;
;  Input:        None
;
;  Output:       None
;-----------------------------------------------------------------------------
%macro PUSHA_64   0
  push    r8
  push    r9
  push    r10
  push    r11
  push    r12
  push    r13
  push    r14
  push    r15
  push    rax
  push    rcx
  push    rdx
  push    rbx
  push    rsp
  push    rbp
  push    rsi
  push    rdi
%endmacro

;-----------------------------------------------------------------------------
;  Macro:        POPA_64
;
;  Description:  Restores all registers from stack
;
;  Input:        None
;
;  Output:       None
;-----------------------------------------------------------------------------
%macro POPA_64   0
  pop    rdi
  pop    rsi
  pop    rbp
  pop    rsp
  pop    rbx
  pop    rdx
  pop    rcx
  pop    rax
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  pop    r11
  pop    r10
  pop    r9
  pop    r8
%endmacro

