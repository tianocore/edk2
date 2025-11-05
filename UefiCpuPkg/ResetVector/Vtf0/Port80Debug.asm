;------------------------------------------------------------------------------
; @file
; Port 0x80 debug support macros
;
; Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    16

%macro  debugInitialize 0
    ;
    ; No initialization is required
    ;
%endmacro

%macro  debugShowPostCode 1
    mov     al, %1
    out     0x80, al
%endmacro

