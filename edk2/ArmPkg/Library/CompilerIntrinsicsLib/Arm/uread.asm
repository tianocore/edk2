//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008-2009 Apple Inc. All rights reserved.
//
// All rights reserved. This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------



    EXPORT  __aeabi_uread4

    AREA  Uread4, CODE, READONLY

;
;UINT32
;EFIAPI
;__aeabi_uread4 (
;  IN VOID   *Pointer
;  );
;
__aeabi_uread4
    ldrb    r2, [r0, #1]
    ldrb    r1, [r0]
    ldrb    r3, [r0, #2]
    ldrb    r0, [r0, #3]
    orr     r1, r1, r2, lsl #8
    orr     r1, r1, r3, lsl #16
    orr     r0, r1, r0, lsl #24
    bx      lr

    END
