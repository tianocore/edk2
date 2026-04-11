;***
;llshr.asm - long shift right
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;       SPDX-License-Identifier: BSD-2-Clause-Patent
;
;Purpose:
;       define signed long shift right routine
;           __allshr
;
;Original Implemenation: MSVC 14.12.25827
;
;*******************************************************************************
    .686
    .model  flat,C
    .code



;***
;llshr - long shift right
;
;Purpose:
;       Does a signed Long Shift Right
;       Shifts a long right any number of bits.
;
;Entry:
;       EDX:EAX - long value to be shifted
;       CL    - number of bits to shift by
;
;Exit:
;       EDX:EAX - shifted value
;
;Uses:
;       CL is destroyed.
;
;Exceptions:
;
;*******************************************************************************
_allshr PROC NEAR

;
; Handle shifts of 64 bits or more (if shifting 64 bits or more, the result
; depends only on the high order bit of edx).
;
        cmp     cl,64
        jae     short RETSIGN

;
; Handle shifts of between 0 and 31 bits
;
        cmp     cl, 32
        jae     short MORE32
        shrd    eax,edx,cl
        sar     edx,cl
        ret

;
; Handle shifts of between 32 and 63 bits
;
MORE32:
        mov     eax,edx
        sar     edx,31
        and     cl,31
        sar     eax,cl
        ret

;
; Return double precision 0 or -1, depending on the sign of edx
;
RETSIGN:
        sar     edx,31
        mov     eax,edx
        ret

_allshr ENDP

        end
