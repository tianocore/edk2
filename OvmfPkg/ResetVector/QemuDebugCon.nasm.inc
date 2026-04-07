;------------------------------------------------------------------------------
; @file
; qemu debug console support macros (based on serial port macros)
;
; Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
; Copyright (c) 2024, Red Hat, Inc.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

%macro  debugShowCharacter 1
    mov     dx, 0x402
    mov     al, %1
    out     dx, al
%endmacro

%macro  debugShowHexDigit 1
  %if (%1 < 0xa)
    debugShowCharacter BYTE ('0' + (%1))
  %else
    debugShowCharacter BYTE ('a' + ((%1) - 0xa))
  %endif
%endmacro

%macro  debugShowPostCode 1
    debugShowHexDigit (((%1) >> 4) & 0xf)
    debugShowHexDigit ((%1) & 0xf)
    debugShowCharacter `\r`
    debugShowCharacter `\n`
%endmacro

BITS    16

%macro  debugInitialize 0
    ; not required
%endmacro
