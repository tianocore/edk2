/** @file
  64-bit Math Worker Function.
  The 32-bit versions of C compiler generate calls to library routines
  to handle 64-bit math. These functions use non-standard calling conventions.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


/*
 * Shifts a 64-bit signed value right by a particular number of bits.
 */
__declspec(naked) void __cdecl _allshr (void)
{
  _asm {
    ;
    ; Handle shifts of 64 bits or more (if shifting 64 bits or more, the result
    ; depends only on the high order bit of edx).
      ;
    cmp     cl,64
    jae     short SIGNRETURN

    ;
    ; Handle shifts of between 0 and 31 bits
    ;
    cmp     cl, 32
    jae     short MORE32
    shrd    eax,edx,cl
    sar     edx,cl
    ret

    ;
    ; Handle shifts of between 32 and 63 bits
    ;
MORE32:
    mov     eax,edx
    sar     edx,31
    and     cl,31
    sar     eax,cl
    ret

    ;
    ; Return double precision 0 or -1, depending on the sign of edx
    ;
SIGNRETURN:
    sar     edx,31
    mov     eax,edx
    ret
  }
}
