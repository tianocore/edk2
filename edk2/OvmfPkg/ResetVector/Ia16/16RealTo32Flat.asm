;------------------------------------------------------------------------------
;
; Copyright (c) 2008, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   16RealTo32Flat.asm
;
; Abstract:
;
;   Transition from 16 bit real mode into 32 bit flat protected mode
;
;------------------------------------------------------------------------------

%define SEC_DEFAULT_CR0  0x40000023
%define SEC_DEFAULT_CR4  0x640

BITS    16

to32BitFlat:

    writeToSerialPort '1'
    writeToSerialPort '6'
    writeToSerialPort ' '

    cli

    mov     bx, 0xf000
    mov     ds, bx

    mov     bx, ADDR16_OF(gdtr)

o32 lgdt    [bx]

    mov     eax, SEC_DEFAULT_CR0
    mov     cr0, eax

;    mov     eax, cr0
;    or      al, 1
;    mov     cr0, eax

    jmp     LINEAR_CODE_SEL:dword ADDR_OF(jumpTo32BitAndLandHere)
BITS    32
jumpTo32BitAndLandHere:

    mov     eax, SEC_DEFAULT_CR4
    mov     cr4, eax

    writeToSerialPort '3'
    writeToSerialPort '2'
    writeToSerialPort ' '

    mov ax, LINEAR_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp     TransitionFrom16RealTo32FlatComplete

ALIGN   2

gdtr:
    dw      GDT_END - GDT_BASE - 1   ; GDT limit
    dd      ADDR_OF(GDT_BASE)

ALIGN   16

GDT_BASE:
; null descriptor
NULL_SEL            equ $-GDT_BASE
        dw 0            ; limit 15:0
        dw 0            ; base 15:0
        db 0            ; base 23:16
        db 0            ; type
        db 0            ; limit 19:16, flags
        db 0            ; base 31:24

; linear data segment descriptor
LINEAR_SEL      equ $-GDT_BASE
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; linear code segment descriptor
LINEAR_CODE_SEL equ $-GDT_BASE
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; system data segment descriptor
SYS_DATA_SEL    equ $-GDT_BASE
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; system code segment descriptor
SYS_CODE_SEL    equ $-GDT_BASE
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; spare segment descriptor
LINEAR_CODE64_SEL  equ $-GDT_BASE
    DW      -1                  ; LimitLow
    DW      0                   ; BaseLow
    DB      0                   ; BaseMid
    DB      9bh
    DB      0afh                ; LimitHigh   (CS.L=1, CS.D=0)
    DB      0                   ; BaseHigh

; spare segment descriptor
SPARE4_SEL  equ $-GDT_BASE
        dw 0            ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 0            ; present, ring 0, data, expand-up, writable
        db 0            ; page-granular, 32-bit
        db 0

; spare segment descriptor
SPARE5_SEL  equ $-GDT_BASE
        dw 0            ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 0            ; present, ring 0, data, expand-up, writable
        db 0            ; page-granular, 32-bit
        db 0

GDT_END:

