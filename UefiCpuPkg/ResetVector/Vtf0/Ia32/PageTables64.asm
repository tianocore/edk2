;------------------------------------------------------------------------------
; @file
; Sets the CR3 register for 64-bit paging
;
; Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

BITS    32

;
; Modified:  EAX
;
SetCr3ForPageTables64:

    ;
    ; These pages are built into the ROM image by Tools/FixupForRawSection.py
    ;
    mov     eax, ((ADDR_OF_START_OF_RESET_CODE & ~0xfff) - 0x1000)
    mov     cr3, eax

    OneTimeCallRet SetCr3ForPageTables64

