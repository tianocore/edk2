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


    EXPORT  __aeabi_uwrite4

    AREA  Uwrite4, CODE, READONLY

;
;UINT32
;EFIAPI
;__aeabi_uwrite4 (
;  IN UINT32 Data,
;  IN VOID   *Pointer
;  );
;
;
__aeabi_uwrite4
    mov     r2, r0, lsr #8
    strb    r0, [r1]
    strb    r2, [r1, #1]
    mov     r2, r0, lsr #16
    strb    r2, [r1, #2]
    mov     r2, r0, lsr #24
    strb    r2, [r1, #3]
    bx      lr
 
    END

