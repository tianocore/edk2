//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------


    EXPORT  __aeabi_memcpy4

    AREA    Memcpy4, CODE, READONLY

;
;VOID
;EFIAPI
;__aeabi_memcpy (
; IN  VOID    *Destination,
; IN  VOID    *Source,
; IN  UINT32  Size
; );
;
__aeabi_memcpy4
    stmdb   sp!, {r4, lr}
    subs    r2, r2, #32     ; 0x20
    bcc     memcpy4_label2
memcpy4_label1
    ldmcsia r1!, {r3, r4, ip, lr}
    stmcsia r0!, {r3, r4, ip, lr}
    ldmcsia r1!, {r3, r4, ip, lr}
    stmcsia r0!, {r3, r4, ip, lr}
    subcss  r2, r2, #32     ; 0x20
    bcs     memcpy4_label1
memcpy4_label2
    movs    ip, r2, lsl #28
    ldmcsia r1!, {r3, r4, ip, lr}
    stmcsia r0!, {r3, r4, ip, lr}
    ldmmiia r1!, {r3, r4}
    stmmiia r0!, {r3, r4}
    ldmia   sp!, {r4, lr}
    movs    ip, r2, lsl #30
    ldrcs   r3, [r1], #4
    strcs   r3, [r0], #4
    bxeq    lr

_memcpy4_lastbytes_aligned
    movs    r2, r2, lsl #31
    ldrcsh  r3, [r1], #2
    ldrmib  r2, [r1], #1
    strcsh  r3, [r0], #2
    strmib  r2, [r0], #1
    bx      lr

    END

