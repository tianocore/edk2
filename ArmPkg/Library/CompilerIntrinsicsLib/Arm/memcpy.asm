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


    EXPORT  __aeabi_memcpy

    AREA    Memcpy, CODE, READONLY

;
;VOID
;EFIAPI
;__aeabi_memcpy (
; IN  VOID    *Destination,
; IN  VOID    *Source,
; IN  UINT32  Size
; );
;
__aeabi_memcpy
    CMP     r2, #0
    BXEQ    r14
loop
    LDRB    r3, [r1], #1
    STRB    r3, [r0], #1
    SUBS    r2, r2, #1
    BXEQ    r14
    B       loop
        
    END

