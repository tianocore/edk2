;; @file
;  Interrupt Redirection Template
;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;;

    DEFAULT REL
    SECTION .text

;----------------------------------------------------------------------------
; Procedure:    InterruptRedirectionTemplate: Redirects interrupts 0x68-0x6F
;
; Input:        None
;
; Output:       None
;
; Prototype:    VOID
;               InterruptRedirectionTemplate (
;                                VOID
;                                );
;
; Saves:        None
;
; Modified:     None
;
; Description:  Contains the code that is copied into low memory (below 640K).
;               This code reflects interrupts 0x68-0x6f to interrupts 0x08-0x0f.
;               This template must be copied into low memory, and the IDT entries
;               0x68-0x6F must be point to the low memory copy of this code.  Each
;               entry is 4 bytes long, so IDT entries 0x68-0x6F can be easily
;               computed.
;
;----------------------------------------------------------------------------

global ASM_PFX(InterruptRedirectionTemplate)
ASM_PFX(InterruptRedirectionTemplate):
  int     0x8
  DB      0xcf          ; IRET
  nop
  int     0x9
  DB      0xcf          ; IRET
  nop
  int     0xa
  DB      0xcf          ; IRET
  nop
  int     0xb
  DB      0xcf          ; IRET
  nop
  int     0xc
  DB      0xcf          ; IRET
  nop
  int     0xd
  DB      0xcf          ; IRET
  nop
  int     0xe
  DB      0xcf          ; IRET
  nop
  int     0xf
  DB      0xcf          ; IRET
  nop

