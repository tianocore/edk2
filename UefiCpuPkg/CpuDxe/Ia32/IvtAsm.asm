      TITLE   IvtAsm.asm:
;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2008 - 2009, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials
;*   are licensed and made available under the terms and conditions of the BSD License
;*   which accompanies this distribution.  The full text of the license may be found at
;*   http://opensource.org/licenses/bsd-license.php
;*
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;*
;*    IvtAsm.asm
;*
;*   Abstract:
;*
;------------------------------------------------------------------------------

#include <Base.h>

#ifdef MDE_CPU_IA32
    .686
    .model  flat,C
#endif
    .code

;------------------------------------------------------------------------------
;  Generic IDT Vector Handlers for the Host. They are all the same so they
;  will compress really well.
;
;  By knowing the return address for Vector 00 you can can calculate the
;  vector number by looking at the call CommonInterruptEntry return address.
;  (return address - (AsmIdtVector00 + 5))/8 == IDT index
;
;------------------------------------------------------------------------------

EXTRN CommonInterruptEntry:PROC

ALIGN   8

PUBLIC	AsmIdtVector00

AsmIdtVector00 LABEL BYTE
REPEAT  256
    call    CommonInterruptEntry
    dw      ($ - AsmIdtVector00 - 5) / 8 ; vector number
    nop
ENDM

END

