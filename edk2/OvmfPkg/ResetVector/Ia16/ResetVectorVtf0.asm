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
;   Reset-16Bit-vft0.asm
;
; Abstract:
;
;   First code exectuted by processor after resetting.
;
;------------------------------------------------------------------------------

BITS	16

earlyInit_Real16:

	jmp	real16InitSerialPort
real16SerialPortInitReturn:

	jmp	to32BitFlat

ALIGN	16

	DD	0, 0, 0

;
; The VTF signature
;
; VTF-0 means that the VTF (Volume Top File) code does not require
; any fixups.
;
vtfSignature:
	DB	'V', 'T', 'F', 0

;
; Reset Vector
;
; This is where the processor will begin execution
;
	jmp	short earlyInit_Real16

ALIGN	16

fourGigabytes:

