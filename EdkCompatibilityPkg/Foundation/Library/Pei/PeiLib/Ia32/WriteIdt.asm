;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   WriteIdt.Asm
;
; Abstract:
;
;   SetIdtBase function
;
; Notes:
;
;------------------------------------------------------------------------------

    .586p
    .model  flat,C
    .mmx
    .code

;------------------------------------------------------------------------------
; void
; SetIdtBase (
;   UINT32  IdtBase,
;   UINT16  IdtLimit 
;   )
;
; Abstract: Set IDTR with the given physical address
;
SetIdtBase      PROC    C PUBLIC IdtBase:DWORD, IdtLimit:WORD
                LOCAL   IdtrBuf:FWORD

                mov     eax, IdtBase
                mov     cx, IdtLimit
                mov     DWORD PTR IdtrBuf + 2, eax                  ; write IDT base address
                mov     WORD  PTR IdtrBuf, cx                        ; write ITD limit
                lidt    FWORD PTR IdtrBuf
                ret
SetIdtBase      ENDP

     END