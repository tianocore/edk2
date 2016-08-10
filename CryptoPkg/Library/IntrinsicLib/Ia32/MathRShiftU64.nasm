;------------------------------------------------------------------------------
;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   MathRShiftU64.nasm
;
; Abstract:
;
;   64-bit Math Worker Function.
;   Shifts a 64-bit unsigned value right by a certain number of bits.
;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
;
; void __cdecl __ashrdi3 (void)
;
;------------------------------------------------------------------------------
global ASM_PFX(__ashrdi3)
ASM_PFX(__ashrdi3):
    cmp cl,0x40
    jnc _Exit
    cmp cl,0x20
    jnc More32
    shrd eax,edx,cl
    shr edx,cl
    ret
More32:
    mov eax,edx
    xor edx,edx
    and cl,0x1f
    shr eax,cl
    ret
_Exit:
    xor eax,eax
    xor edx,edx
    ret

