;------------------------------------------------------------------------------
; @file
; A minimal Int10h stub that allows the Windows 7 SP1 default VGA driver to
; 'swithc' to the 1024x768x32 video mode on MacBookAir7,2 and possibly other
; Apple laptops that do not have a VGA ROM / Int10h handler.

; Adapted from VbeShim.asm from the Qemu project.
;
; Copyright (C) 2014, Red Hat, Inc.
; Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
;
; This program and the accompanying materials are licensed and made available
; under the terms and conditions of the BSD License which accompanies this
; distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
; WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------


BITS 16
ORG 0


VbeInfo:
TIMES 256 nop							; this will be filled in by the efi shim


VbeModeInfo:
TIMES 256 nop							; this will be filled in by the efi shim


InterruptHandlerEntry:
  ; Main entry point for the interrupt handler
  ; http://wiki.osdev.org/BIOS#Common_functions
  cmp        ax, 0x4f00
  je         GetInfo
  
  cmp        ax, 0x4f01
  je         GetModeInfo
  
  cmp        ax, 0x4f02
  je         SetMode
  
  cmp        ax, 0x4f03
  je         GetMode
  
  cmp        ax, 0x4f10
  je         GetPmCapabilities
  
  cmp        ax, 0x4f15
  je         ReadEdid
  
  cmp        ah, 0x00
  je         SetModeLegacy

Hang:
  jmp        Hang


GetInfo:
  ; Function 00: Return Controller Information
  ; Inputs:
	;   AX    = 0x4f00
	;   ES:DI = pointer to VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK buffer
	; Outputs:
	;   AX    = return status
  push       es						; store registers on stack
  push       di
  push       ds
  push       si
  push       cx
  push       cs
  pop        ds						; load the code segment address to DS
  mov        si, VbeInfo	; load offset of VbeInfo from program start to SI
  mov        cx, 256			; we want to copy 256 bytes
  cld											; clear direction flag
  rep movsb								; move 256 bytes of VbeInfo at DS:SI to buffer at ES:DI
  pop        cx						; restore registers from stack
  pop        si
  pop        ds
  pop        di
  pop        es
  jmp        Success 			; ax=0x4f


GetModeInfo:
  ; Function 01: Return Mode Information
	; Inputs:
	;   AX    = 0x4f01
	;   CX    = mode number
	;   ES:DI = pointer to VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK buffer
	; Outputs:
	;   AX    = return status
  push       es						; store registers on stack
  push       di
  push       ds
  push       si
  push       cx
  and        cx, ~0x4000	; clear potentially set LFB (linear frame buffer) bit in mode number
  cmp        cx, 0x00f1		; offer information on only known mode
  je         GetKnownModeInfo1
  jmp        Hang
GetKnownModeInfo1:
  push       cs
  pop        ds						; load the code segment address to DS
  mov        si, VbeModeInfo ; load offset of VbeModeInfo from program start to SI
  mov        cx, 256			; we want to copy 256 bytes
  cld											; clear direction flag
  rep movsb								; move 256 bytes of VbeModeInfo at DS:SI to buffer at ES:DI
  pop        cx						; restore registers from stack
  pop        si
  pop        ds
  pop        di
  pop        es
  jmp        Success			; ax=0x4f


SetMode:
  ; Function 02: Set Mode
  ; Inputs:
	;   AX    = 0x4f02
	;   BX    = desired mode to set
	;   ES:DI = pointer to VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK structure
	; Outputs:
	;   AX    = return status
  push       dx						; store registers on stack
  push       ax
  cmp        bx, 0x40f1		; 0x40f1 mode
													; <15>=0 Clear display memory
													; <14>=1 Use linear/flat frame buffer model
													; <13:12>=00 Reserved (must be 0)
													; <11>=0 Use current default refresh rate
													; <10:9>00
													; <8:0>=011110001 Mode Number 241 ???
													; to identify available modes use sudo hwinfo --framebuffer
  je         SetKnownMode1
  jmp        Hang
SetKnownMode1:
  ; everything is done by the efi shim
  pop        ax						; restore registers from stack
  pop        dx
  jmp        Success 			; ax=0x4f


GetMode:
  ; Function 03: Return Current Mode
  ; Inputs:
	;   AX    = 0x4f03
	; Outputs:
	;   AX    = return status
	;   BX    = current mode
  mov        bx, 0x40f1
  jmp        Success ; ax=0x4f


GetPmCapabilities:
  ; Function 10: Get Power Management Capabilities
  ; Inputs:
  ;   AX    = 0x4f10
  ;   BL    = 0
  ;   ES:DI = null pointer
  ; Outputs:
  ;   AL    = 0x4f if function is supported
  ;   AH    = 0 if successful, else failed
  jmp        Unsupported 	; ax=0x014f


ReadEdid:
	; Function 15: implement VBE/DDC service
  ; Inputs:
	;   AX    = 0x4f15
	;   BL    = report VBE/DDC capabilities
	;   ES:DI = null pointer
	; Outputs:
	;   AX    = return status
	;   BH    = approx. time in seconds to transfer EDID block
	;   BL    = DDC level supported
  jmp        Unsupported	; ax=0x014f


SetModeLegacy:
  ; Inputs:
	;   AH    = 0x00
	;   AL    = video mode
	; Outputs:
	;   AL    = video mode flag (20h for mode > 7, 30h for 0-5,7 and 3Fh for 6)
	; Notes:
	;   We only pretend to do something here, don't set any modes.
  cmp        al, 0x03 		; 80x25 chars @ 720x400x16
  je         SetKnownModeLegacy1
  cmp        al, 0x12 		; 80x30 chars @ 640x480x256
  je         SetKnownModeLegacy2
  jmp        Hang
SetKnownModeLegacy1:
  mov        al, 0x30 		; return success value
  jmp        SetModeLegacyDone
SetKnownModeLegacy2:
  mov        al, 0x20 		; return success value
SetModeLegacyDone:
  iret


Success:
  mov        ax, 0x004f
  iret


Unsupported:
  mov        ax, 0x014f
  iret
