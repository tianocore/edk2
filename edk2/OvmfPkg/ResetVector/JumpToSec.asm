;------------------------------------------------------------------------------
;
; Copyright (c) 2008, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   JumpToSec.asm
;
; Abstract:
;
;   Jump from the reset vector binary to SEC
;
;------------------------------------------------------------------------------

BITS    32

TransitionFrom16RealTo32FlatComplete:

    OneTimeCall Flat32SearchForBfvBase

    OneTimeCall Flat32SearchForSecAndPeiEntries

    ;
    ; ESI - SEC Core entry point
    ; EDI - PEI Core entry point
    ; EBP - Start of BFV
    ;
    ; Jump to SEC Core entry point
    ;

%ifdef ARCH_IA32

    jmp     esi

%else

    OneTimeCall Transition32FlatTo64Flat
BITS    64

    mov     rax, 0x00000000ffffffff
    and     rsi, rax
    and     rdi, rax
    and     rbp, rax
    ;
    ; RSI - SEC Core entry point
    ; RDI - PEI Core entry point
    ; RBP - Start of BFV
    ;
    ; Jump to SEC Core entry point
    ;

    jmp     rsi

%endif


