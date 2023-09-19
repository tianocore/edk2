;------------------------------------------------------------------------------
; @file
; First code executed by processor after resetting.
;
; Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    16

ALIGN   16

;
; Pad the image size to 4k when page tables are in VTF0
;
; If the VTF0 image has page tables built in, then we need to make
; sure the end of VTF0 is 4k above where the page tables end.
;
; This is required so the page tables will be 4k aligned when VTF0 is
; located just below 0x100000000 (4GB) in the firmware device.
;
%ifdef ALIGN_TOP_TO_4K_FOR_PAGING
    TIMES (0x1000 - ($ - EndOfPageTables)) DB 0
;
; Pad the VTF0 Reset code for Bsp & Ap to 4k aligned block.
; Some implementations may need to keep the initial Reset code
; to be separated out from rest of the code.
; This padding will make sure lower 4K region below 4 GB may
; only contains few jmp instructions and data.
;
    TIMES (0x1000 - 0x20) DB 0
%endif

;
; 0xffffffe0
;
    DD      0, 0, 0

;
; The VTF signature (0xffffffec)
;
; VTF-0 means that the VTF (Volume Top File) code does not require
; any fixups.
;
vtfSignature:
    DB      'V', 'T', 'F', 0

ALIGN   16

resetVector:
;
; Reset Vector
;
; This is where the processor will begin execution
;
    nop
    nop
    jmp     EarlyBspInitReal16

ALIGN   16

fourGigabytes:

