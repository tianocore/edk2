;------------------------------------------------------------------------------
; @file
; PML5 page table creation.
;
; Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

    ;
    ; PML5 table Pointers
    ; Assume page table is create from bottom to top, and only one PML4 table there.
    ;
    DQ      ($ - 0x1000 + PAGE_PDP_ATTR)

    TIMES   (0x1000 - 0x8) DB 0

