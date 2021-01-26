;------------------------------------------------------------------------------
;
; Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   This file provides macro definitions for NASM files.
;
;------------------------------------------------------------------------------

%macro SAVEPREVSSP     0
    DB 0xF3, 0x0F, 0x01, 0xEA
%endmacro

%macro CLRSSBSY_EAX    0
    DB 0x67, 0xF3, 0x0F, 0xAE, 0x30
%endmacro

%macro RSTORSSP_EAX    0
    DB 0x67, 0xF3, 0x0F, 0x01, 0x28
%endmacro

%macro SETSSBSY        0
    DB 0xF3, 0x0F, 0x01, 0xE8
%endmacro

%macro READSSP_EAX     0
    DB 0xF3, 0x0F, 0x1E, 0xC8
%endmacro

%macro INCSSP_EAX      0
    DB 0xF3, 0x0F, 0xAE, 0xE8
%endmacro
