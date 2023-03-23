;------------------------------------------------------------------------------
; @file
; Sets the CR3 register for 64-bit paging
;
; Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    32

;
; Modified:  EAX
;
SetCr3ForPageTables64:

    ;
    ; These pages are built into the ROM image in X64/PageTables.asm
    ; Highest level PageTable is at the highest address
    ;
    mov     eax, ADDR_OF(EndOfPageTables) - 0x1000
    mov     cr3, eax

    OneTimeCallRet SetCr3ForPageTables64

