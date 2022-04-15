;; @file
;  This is the code that goes from real-mode to protected mode.
;  It consumes the reset vector, configures the stack.
;
; Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

;
; Define assembler characteristics
;

extern   ASM_PFX(TempRamInitApi)

SECTION .text

%macro RET_ESI  0

  movd    esi, mm7                      ; restore EIP from MM7
  jmp     esi

%endmacro

;
; Perform early platform initialization
;
global ASM_PFX(SecPlatformInit)
ASM_PFX(SecPlatformInit):

  RET_ESI

;
; Protected mode portion initializes stack, configures cache, and calls C entry point
;

;----------------------------------------------------------------------------
;
; Procedure:    ProtectedModeEntryPoint
;
; Input:        Executing in 32 Bit Protected (flat) mode
;               cs: 0-4GB
;               ds: 0-4GB
;               es: 0-4GB
;               fs: 0-4GB
;               gs: 0-4GB
;               ss: 0-4GB
;
; Output:       This function never returns
;
; Destroys:
;               ecx
;               edi
;               esi
;               esp
;
; Description:
;               Perform any essential early platform initialisation
;               Setup a stack
;
;----------------------------------------------------------------------------
global ASM_PFX(ProtectedModeEntryPoint)
ASM_PFX(ProtectedModeEntryPoint):
  ;
  ; Dummy function. Consume 2 API to make sure they can be linked.
  ;
  mov  eax, ASM_PFX(TempRamInitApi)

  ; Should never return
  jmp  $

;
; ROM-based Global-Descriptor Table for the PEI Phase
;
align 16
global  ASM_PFX(BootGdtTable)

;
; GDT[0]: 0x00: Null entry, never used.
;
NULL_SEL        equ     $ - GDT_BASE        ; Selector [0]
GDT_BASE:
ASM_PFX(BootGdtTable):    DD      0
                          DD      0
;
; Linear code segment descriptor
;
LINEAR_CODE_SEL equ     $ - GDT_BASE        ; Selector [0x8]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      09Bh                        ; present, ring 0, data, expand-up, not-writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0
;
; System data segment descriptor
;
SYS_DATA_SEL    equ     $ - GDT_BASE        ; Selector [0x10]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      093h                        ; present, ring 0, data, expand-up, not-writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0

GDT_SIZE        EQU     $ - GDT_BASE        ; Size, in bytes

;
; GDT Descriptor
;
GdtDesc:                                    ; GDT descriptor
        DW      GDT_SIZE - 1                ; GDT limit
        DD      GDT_BASE                    ; GDT base address

global ASM_PFX(ProtectedModeEntryLinearAddress)
global ASM_PFX(ProtectedModeEntryLinearOffset)

ASM_PFX(ProtectedModeEntryLinearAddress):
ASM_PFX(ProtectedModeEntryLinearOffset):
  DD      ASM_PFX(ProtectedModeEntryPoint)  ; Offset of our 32 bit code
  DW      LINEAR_CODE_SEL

