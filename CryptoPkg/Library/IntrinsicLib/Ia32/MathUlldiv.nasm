;***
;ulldiv.nasm - unsigned long divide routine
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;       Copyright (c) 2025, Intel Corporation. All rights reserved.
;       SPDX-License-Identifier: BSD-2-Clause-Patent
;
;Purpose:
;       defines the unsigned long divide routine
;           __aulldiv
;
;Original Implementation: MSVC 14.29.30133
;
;*******************************************************************************

    SECTION .text

;***
;ulldiv - unsigned long divide
;
;Purpose:
;       Does a unsigned long divide of the arguments.  Arguments are
;       not changed.
;
;Entry:
;       Arguments are passed on the stack:
;               1st pushed: divisor (QWORD)
;               2nd pushed: dividend (QWORD)
;
;Exit:
;       EDX:EAX contains the quotient (dividend/divisor)
;       NOTE: this routine removes the parameters from the stack.
;
;Uses:
;       ECX
;
;Exceptions:
;
;*******************************************************************************
global ASM_PFX(_aulldiv)
ASM_PFX(_aulldiv):

%define HIWORD_OFFSET 4
%define LOWORD_OFFSET 0

        push    ebx
        push    esi

; Set up the local stack and save the index registers.  When this is done
; the stack frame will look as follows (assuming that the expression a/b will
; generate a call to uldiv(a, b)):
;
;               -----------------
;               |               |
;               |---------------|
;               |               |
;               |--divisor (b)--|
;               |               |
;               |---------------|
;               |               |
;               |--dividend (a)-|
;               |               |
;               |---------------|
;               | return addr** |
;               |---------------|
;               |      EBX      |
;               |---------------|
;       ESP---->|      ESI      |
;               -----------------
;

; stack offset of dividend (a)
%define DVND_OFFSET 12
; stack offset of divisor (b)
%define DVSR_OFFSET 20

;
; Now do the divide.  First look to see if the divisor is less than 4194304K.
; If so, then we can use a simple algorithm with word divides, otherwise
; things get a little more complex.
;

        mov     eax, dword [esp + DVSR_OFFSET + HIWORD_OFFSET] ; check to see if divisor < 4194304K
        or      eax,eax
        jnz     short L1        ; nope, gotta do this the hard way
        mov     ecx, dword [esp + DVSR_OFFSET + LOWORD_OFFSET] ; load divisor
        mov     eax, dword [esp + DVND_OFFSET + HIWORD_OFFSET] ; load high word of dividend
        xor     edx,edx
        div     ecx             ; get high order bits of quotient
        mov     ebx,eax         ; save high bits of quotient
        mov     eax, dword [esp + DVND_OFFSET + LOWORD_OFFSET] ; edx:eax <- remainder:lo word of dividend
        div     ecx             ; get low order bits of quotient
        mov     edx,ebx         ; edx:eax <- quotient hi:quotient lo
        jmp     short L2        ; restore stack and return

;
; Here we do it the hard way.  Remember, eax contains DVSRHI
;

L1:
        mov     ecx,eax         ; ecx:ebx <- divisor
        mov     ebx, dword [esp + DVSR_OFFSET + LOWORD_OFFSET]
        mov     edx, dword [esp + DVND_OFFSET + HIWORD_OFFSET] ; edx:eax <- dividend
        mov     eax, dword [esp + DVND_OFFSET + LOWORD_OFFSET]
L3:
        shr     ecx,1           ; shift divisor right one bit; hi bit <- 0
        rcr     ebx,1
        shr     edx,1           ; shift dividend right one bit; hi bit <- 0
        rcr     eax,1
        or      ecx,ecx
        jnz     short L3        ; loop until divisor < 4194304K
        div     ebx             ; now divide, ignore remainder
        mov     esi,eax         ; save quotient

;
; We may be off by one, so to check, we will multiply the quotient
; by the divisor and check the result against the original dividend
; Note that we must also check for overflow, which can occur if the
; dividend is close to 2**64 and the quotient is off by 1.
;

        mul     dword [esp + DVSR_OFFSET + HIWORD_OFFSET] ; QUOT * HIWORD(DVSR)
        mov     ecx,eax
        mov     eax, dword [esp + DVSR_OFFSET + LOWORD_OFFSET]
        mul     esi             ; QUOT * LOWORD(DVSR)
        add     edx,ecx         ; EDX:EAX = QUOT * DVSR
        jc      short L4        ; carry means Quotient is off by 1

;
; do long compare here between original dividend and the result of the
; multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
; subtract one (1) from the quotient.
;

        cmp     edx, dword [esp + DVND_OFFSET + HIWORD_OFFSET] ; compare hi words of result and original
        ja      short L4        ; if result > original, do subtract
        jb      short L5        ; if result < original, we are ok
        cmp     eax, dword [esp + DVND_OFFSET + LOWORD_OFFSET] ; hi words are equal, compare lo words
        jbe     short L5        ; if less or equal we are ok, else subtract
L4:
        dec     esi             ; subtract 1 from quotient
L5:
        xor     edx,edx         ; edx:eax <- quotient
        mov     eax,esi

;
; Just the cleanup left to do.  edx:eax contains the quotient.
; Restore the saved registers and return.
;

L2:

        pop     esi
        pop     ebx

        ret     16
