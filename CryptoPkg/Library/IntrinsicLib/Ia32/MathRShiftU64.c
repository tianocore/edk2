/** @file
  64-bit Math Worker Function.
  The 32-bit versions of C compiler generate calls to library routines
  to handle 64-bit math. These functions use non-standard calling conventions.

Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


/*
 * Shifts a 64-bit unsigned value right by a certain number of bits.
 */
__declspec(naked) void __cdecl _aullshr (void)
{
  _asm {
    ;
    ; Checking: Only handle 64bit shifting or more
    ;
    cmp     cl, 64
    jae     _Exit

    ;
    ; Handle shifting between 0 and 31 bits
    ;
    cmp     cl, 32
    jae     More32
    shrd    eax, edx, cl
    shr     edx, cl
    ret

    ;
    ; Handle shifting of 32-63 bits
    ;
More32:
    mov     eax, edx
    xor     edx, edx
    and     cl, 31
    shr     eax, cl
    ret

    ;
    ; Invalid number (less then 32bits), return 0
    ;
_Exit:
    xor     eax, eax
    xor     edx, edx
    ret
  }
}
