;------------------------------------------------------------------------------
; @file
; Main routine of the pre-SEC code up through the jump into SEC
;
; Copyright (c) 2008 - 2009, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------


BITS    16

;
; Modified:  EBX, ECX, EDX, EBP
;
; @param[in,out]  RAX/EAX  Initial value of the EAX register
;                          (BIST: Built-in Self Test)
; @param[in,out]  DI       'BP': boot-strap processor, or
;                          'AP': application processor
; @param[out]     RBP/EBP  Address of Boot Firmware Volume (BFV)
;
; @return         None  This routine jumps to SEC and does not return
;
Main16:
    OneTimeCall EarlyInit16

    ;
    ; Transition the processor from 16-bit real mode to 32-bit flat mode
    ;
    OneTimeCall TransitionFromReal16To32BitFlat

BITS    32

    ;
    ; Search for the Boot Firmware Volume (BFV)
    ;
    OneTimeCall Flat32SearchForBfvBase

    ;
    ; EBP - Start of BFV
    ;

    ;
    ; Search for the SEC entry point
    ;
    OneTimeCall Flat32SearchForSecEntryPoint

    ;
    ; ESI - SEC Core entry point
    ; EBP - Start of BFV
    ;

%ifdef ARCH_IA32

    ;
    ; Restore initial EAX value into the EAX register
    ;
    mov     eax, esp

    ;
    ; Jump to the 32-bit SEC entry point
    ;
    jmp     esi

%else

    ;
    ; Transition the processor from 32-bit flat mode to 64-bit flat mode
    ;
    OneTimeCall Transition32FlatTo64Flat

BITS    64

    ;
    ; Some values were calculated in 32-bit mode.  Make sure the upper
    ; 32-bits of 64-bit registers are zero for these values.
    ;
    mov     rax, 0x00000000ffffffff
    and     rsi, rax
    and     rbp, rax
    and     rsp, rax

    ;
    ; RSI - SEC Core entry point
    ; RBP - Start of BFV
    ;

    ;
    ; Restore initial EAX value into the RAX register
    ;
    mov     rax, rsp

    ;
    ; Jump to the 64-bit SEC entry point
    ;
    jmp     rsi

%endif


