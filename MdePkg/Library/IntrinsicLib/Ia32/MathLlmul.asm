;***
;llmul.asm - long multiply routine
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;       SPDX-License-Identifier: BSD-2-Clause-Patent
;
;Purpose:
;       Defines long multiply routine
;       Both signed and unsigned routines are the same, since multiply's
;       work out the same in 2's complement
;       creates the following routine:
;           __allmul
;
;Original Implemenation: MSVC 14.12.25827
;
;*******************************************************************************
    .686
    .model  flat,C
    .code


;***
;llmul - long multiply routine
;
;Purpose:
;       Does a long multiply (same for signed/unsigned)
;       Parameters are not changed.
;
;Entry:
;       Parameters are passed on the stack:
;               1st pushed: multiplier (QWORD)
;               2nd pushed: multiplicand (QWORD)
;
;Exit:
;       EDX:EAX - product of multiplier and multiplicand
;       NOTE: parameters are removed from the stack
;
;Uses:
;       ECX
;
;Exceptions:
;
;*******************************************************************************
_allmul PROC NEAR

A       EQU     [esp + 4]       ; stack address of a
B       EQU     [esp + 12]      ; stack address of b

HIGH_PART  EQU     [4]             ;
LOW_PART   EQU     [0]

;
;       AHI, BHI : upper 32 bits of A and B
;       ALO, BLO : lower 32 bits of A and B
;
;             ALO * BLO
;       ALO * BHI
; +     BLO * AHI
; ---------------------
;

        mov     eax,HIGH_PART(A)
        mov     ecx,HIGH_PART(B)
        or      ecx,eax         ;test for both high dwords zero.
        mov     ecx,LOW_PART(B)
        jnz     short hard      ;both are zero, just mult ALO and BLO

        mov     eax,LOW_PART(A)
        mul     ecx

        ret     16              ; callee restores the stack

hard:
        push    ebx

; must redefine A and B since esp has been altered

A2      EQU     [esp + 8]       ; stack address of a
B2      EQU     [esp + 16]      ; stack address of b

        mul     ecx             ;eax has AHI, ecx has BLO, so AHI * BLO
        mov     ebx,eax         ;save result

        mov     eax,LOW_PART(A2)
        mul     dword ptr HIGH_PART(B2) ;ALO * BHI
        add     ebx,eax         ;ebx = ((ALO * BHI) + (AHI * BLO))

        mov     eax,LOW_PART(A2);ecx = BLO
        mul     ecx             ;so edx:eax = ALO*BLO
        add     edx,ebx         ;now edx has all the LO*HI stuff

        pop     ebx

        ret     16              ; callee restores the stack

_allmul ENDP

        end
