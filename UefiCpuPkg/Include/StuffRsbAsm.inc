;------------------------------------------------------------------------------ ;
; Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
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
;   StuffRsbAsm.inc
;
; Abstract:
;
;   This file provides macro definitions for stuffing the Return Stack Buffer (RSB)
;   for .ASM files.
;
;-------------------------------------------------------------------------------

RSB_STUFF_ENTRIES    Equ    20h

;
; parameters:
; @param 1: register to use as counter (e.g. IA32:eax, X64:rax)
; @param 2: stack pointer to restore   (IA32:esp, X64:rsp)
; @param 3: the size of a stack frame  (IA32:4, X64:8)
;
StuffRsb     MACRO   Reg, StackPointer, Size
             local   Unroll1, Unroll2, SpecTrap1, SpecTrap2, StuffLoop
             mov     Reg, RSB_STUFF_ENTRIES / 2
Unroll1:
             call    Unroll2
SpecTrap1:
             pause
             lfence
             jmp     SpecTrap1
Unroll2:
             call    StuffLoop
SpecTrap2:
             pause
             lfence
             jmp     SpecTrap2
StuffLoop:
             dec     Reg
             jnz     Unroll1
             add     StackPointer, RSB_STUFF_ENTRIES * Size ; Restore the stack pointer
             ENDM

;
; RSB stuffing macros for IA32 and X64
;
StuffRsb32   MACRO
             StuffRsb    eax, esp, 4
             ENDM

StuffRsb64   MACRO
             StuffRsb    rax, rsp, 8
             ENDM
