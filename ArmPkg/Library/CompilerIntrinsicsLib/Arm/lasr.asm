//------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------



    INCLUDE AsmMacroExport.inc

;
;UINT32
;EFIAPI
;__aeabi_lasr (
;  IN UINT32  Dividen
;  IN UINT32  Divisor
;  );
;
 RVCT_ASM_EXPORT __aeabi_lasr
    SUBS     r3,r2,#0x20
    BPL      {pc} + 0x18  ; 0x1c
    RSB      r3,r2,#0x20
    LSR      r0,r0,r2
    ORR      r0,r0,r1,LSL r3
    ASR      r1,r1,r2
    BX       lr
    ASR      r0,r1,r3
    ASR      r1,r1,#31
    BX       lr

    END

