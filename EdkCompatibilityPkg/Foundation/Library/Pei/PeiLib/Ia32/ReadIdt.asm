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
;   ReadIdtBase.Asm
;
; Abstract:
;
;   ReadIdtBase function
;
; Notes:
;
;------------------------------------------------------------------------------

    .586
    .model  flat,C
    .mmx
    .code

;------------------------------------------------------------------------------
; UINTN
; ReadIdtBase (
;   void
;   )
;
; Abstract: Returns physical address of IDTR
;
ReadIdtBase     PROC    C PUBLIC
                LOCAL   IdtrBuf:FWORD
                sidt    IdtrBuf
                mov     eax, DWORD PTR IdtrBuf + 2
                ret
ReadIdtBase     ENDP

;------------------------------------------------------------------------------
; UINT16
; ReadIdtLimit (
;   void
;   )
;
; Abstract: Returns Limit of IDTR
;
ReadIdtLimit    PROC    C PUBLIC
                LOCAL   IdtrBuf:FWORD
                sidt    IdtrBuf
                mov     ax, WORD PTR IdtrBuf
                ret
ReadIdtLimit    ENDP


    END
