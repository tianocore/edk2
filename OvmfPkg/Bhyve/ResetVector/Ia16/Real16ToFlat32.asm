;------------------------------------------------------------------------------
; @file
; Transition from 16 bit real mode into 32 bit flat protected mode
;
; Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>. All rights reserved.<BR>
; Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

%define SEC_DEFAULT_CR0  0x00000023
%define SEC_DEFAULT_CR4  0x640

BITS    16

;
; Modified:  EAX, EBX
;
; @param[out]     DS       Selector allowing flat access to all addresses
; @param[out]     ES       Selector allowing flat access to all addresses
; @param[out]     FS       Selector allowing flat access to all addresses
; @param[out]     GS       Selector allowing flat access to all addresses
; @param[out]     SS       Selector allowing flat access to all addresses
;
TransitionFromReal16To32BitFlat:

    debugShowPostCode POSTCODE_16BIT_MODE

    cli

    mov     bx, 0xf000
    mov     ds, bx

    mov     bx, ADDR16_OF(gdtr)

o32 lgdt    [cs:bx]

    mov     eax, SEC_DEFAULT_CR0
    mov     cr0, eax

    jmp     LINEAR_CODE_SEL:dword ADDR_OF(jumpTo32BitAndLandHere)
BITS    32
jumpTo32BitAndLandHere:

    mov     eax, SEC_DEFAULT_CR4
    mov     cr4, eax

    debugShowPostCode POSTCODE_32BIT_MODE

    mov     ax, LINEAR_SEL
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    OneTimeCallRet TransitionFromReal16To32BitFlat

ALIGN   2

gdtr:
    dw      GDT_END - GDT_BASE - 1   ; GDT limit
    dd      ADDR_OF(GDT_BASE)

ALIGN   16

;
; Macros for GDT entries
;

%define  PRESENT_FLAG(p) (p << 7)
%define  DPL(dpl) (dpl << 5)
%define  SYSTEM_FLAG(s) (s << 4)
%define  DESC_TYPE(t) (t)

; Type: data, expand-up, writable, accessed
%define  DATA32_TYPE 3

; Type: execute, readable, expand-up, accessed
%define  CODE32_TYPE 0xb

; Type: execute, readable, expand-up, accessed
%define  CODE64_TYPE 0xb

%define  GRANULARITY_FLAG(g) (g << 7)
%define  DEFAULT_SIZE32(d) (d << 6)
%define  CODE64_FLAG(l) (l << 5)
%define  UPPER_LIMIT(l) (l)

;
; The Global Descriptor Table (GDT)
;

GDT_BASE:
; null descriptor
NULL_SEL            equ $-GDT_BASE
    DW      0            ; limit 15:0
    DW      0            ; base 15:0
    DB      0            ; base 23:16
    DB      0            ; sys flag, dpl, type
    DB      0            ; limit 19:16, flags
    DB      0            ; base 31:24

; linear data segment descriptor
LINEAR_SEL          equ $-GDT_BASE
    DW      0xffff       ; limit 15:0
    DW      0            ; base 15:0
    DB      0            ; base 23:16
    DB      PRESENT_FLAG(1)|DPL(0)|SYSTEM_FLAG(1)|DESC_TYPE(DATA32_TYPE)
    DB      GRANULARITY_FLAG(1)|DEFAULT_SIZE32(1)|CODE64_FLAG(0)|UPPER_LIMIT(0xf)
    DB      0            ; base 31:24

; linear code segment descriptor
LINEAR_CODE_SEL     equ $-GDT_BASE
    DW      0xffff       ; limit 15:0
    DW      0            ; base 15:0
    DB      0            ; base 23:16
    DB      PRESENT_FLAG(1)|DPL(0)|SYSTEM_FLAG(1)|DESC_TYPE(CODE32_TYPE)
    DB      GRANULARITY_FLAG(1)|DEFAULT_SIZE32(1)|CODE64_FLAG(0)|UPPER_LIMIT(0xf)
    DB      0            ; base 31:24

%ifdef ARCH_X64
; linear code (64-bit) segment descriptor
LINEAR_CODE64_SEL   equ $-GDT_BASE
    DW      0xffff       ; limit 15:0
    DW      0            ; base 15:0
    DB      0            ; base 23:16
    DB      PRESENT_FLAG(1)|DPL(0)|SYSTEM_FLAG(1)|DESC_TYPE(CODE64_TYPE)
    DB      GRANULARITY_FLAG(1)|DEFAULT_SIZE32(0)|CODE64_FLAG(1)|UPPER_LIMIT(0xf)
    DB      0            ; base 31:24
%endif

; linear code segment descriptor
LINEAR_CODE16_SEL     equ $-GDT_BASE
    DW      0xffff       ; limit 15:0
    DW      0            ; base 15:0
    DB      0            ; base 23:16
    DB      PRESENT_FLAG(1)|DPL(0)|SYSTEM_FLAG(1)|DESC_TYPE(CODE32_TYPE)
    DB      GRANULARITY_FLAG(1)|DEFAULT_SIZE32(0)|CODE64_FLAG(0)|UPPER_LIMIT(0xf)
    DB      0            ; base 31:24

GDT_END:

