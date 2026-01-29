;------------------------------------------------------------------------------
; @file
; First code executed by processor after resetting.
;
; Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
; Copyright (c) 2019, Citrix Systems, Inc.
;
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
    TIMES (0x1000 - ($ - EndOfPageTables) - (fourGigabytes - xenPVHEntryPoint)) DB 0
%endif

BITS    32
xenPVHEntryPoint:
;
; Entry point to use when running as a Xen PVH guest. (0xffffffd0)
;
; Description of the expected state of the machine when this entry point is
; used can be found at:
; https://xenbits.xenproject.org/docs/unstable/misc/pvh.html
;
    jmp     xenPVHMain

BITS    16
ALIGN   16
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
    jmp     EarlyBspInitReal16

ALIGN   16

fourGigabytes:

