;------------------------------------------------------------------------------
;
; Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Abstract:
;
;   This file provides macro definitions for stuffing the Return Stack Buffer (RSB).
;
;------------------------------------------------------------------------------

%define RSB_STUFF_ENTRIES 0x20

;
; parameters:
; @param 1: register to use as counter (e.g. IA32:eax, X64:rax)
; @param 2: stack pointer to restore   (IA32:esp, X64:rsp)
; @param 3: the size of a stack frame  (IA32:4, X64:8)
;
%macro StuffRsb 3
      mov     %1, RSB_STUFF_ENTRIES / 2
  %%Unroll1:
      call    %%Unroll2
  %%SpecTrap1:
      pause
      lfence
      jmp     %%SpecTrap1
  %%Unroll2:
      call    %%StuffLoop
  %%SpecTrap2:
      pause
      lfence
      jmp     %%SpecTrap2
  %%StuffLoop:
      dec     %1
      jnz     %%Unroll1
      add     %2, RSB_STUFF_ENTRIES * %3 ; Restore the stack pointer
%endmacro

;
; RSB stuffing macros for IA32 and X64
;
%macro StuffRsb32 0
      StuffRsb     eax, esp, 4
%endmacro

%macro StuffRsb64 0
      StuffRsb     rax, rsp, 8
%endmacro
