;------------------------------------------------------------------------------
; @file
; Serial port debug support macros
;
; Copyright (c) 2008 - 2009, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

;//---------------------------------------------
;// UART Register Offsets
;//---------------------------------------------
%define BAUD_LOW_OFFSET         0x00
%define BAUD_HIGH_OFFSET        0x01
%define IER_OFFSET              0x01
%define LCR_SHADOW_OFFSET       0x01
%define FCR_SHADOW_OFFSET       0x02
%define IR_CONTROL_OFFSET       0x02
%define FCR_OFFSET              0x02
%define EIR_OFFSET              0x02
%define BSR_OFFSET              0x03
%define LCR_OFFSET              0x03
%define MCR_OFFSET              0x04
%define LSR_OFFSET              0x05
%define MSR_OFFSET              0x06

;//---------------------------------------------
;// UART Register Bit Defines
;//---------------------------------------------
%define LSR_TXRDY               0x20
%define LSR_RXDA                0x01
%define DLAB                    0x01

; UINT16  gComBase = 0x3f8;
; UINTN   gBps = 115200;
; UINT8   gData = 8;
; UINT8   gStop = 1;
; UINT8   gParity = 0;
; UINT8   gBreakSet = 0;

%define DEFAULT_COM_BASE 0x3f8
%define DEFAULT_BPS 115200
%define DEFAULT_DATA 8
%define DEFAULT_STOP 1
%define DEFAULT_PARITY 0
%define DEFAULT_BREAK_SET 0

%define SERIAL_DEFAULT_LCR ( \
     (DEFAULT_BREAK_SET << 6) | \
     (DEFAULT_PARITY << 3) | \
     (DEFAULT_STOP << 2) | \
     (DEFAULT_DATA - 5) \
    )

%define SERIAL_PORT_IO_BASE_ADDRESS DEFAULT_COM_BASE

%macro  inFromSerialPort 1
    mov     dx, (SERIAL_PORT_IO_BASE_ADDRESS + %1)
    in      al, dx
%endmacro

%macro  waitForSerialTxReady 0

%%waitingForTx:
    inFromSerialPort LSR_OFFSET
    test    al, LSR_TXRDY
    jz      %%waitingForTx

%endmacro

%macro  outToSerialPort 2
    mov     dx, (SERIAL_PORT_IO_BASE_ADDRESS + %1)
    mov     al, %2
    out     dx, al
%endmacro

%macro  debugShowCharacter 1
    waitForSerialTxReady
    outToSerialPort 0, %1
%endmacro

%macro  debugShowHexDigit 1
  %if (%1 < 0xa)
    debugShowCharacter BYTE ('0' + (%1))
  %else
    debugShowCharacter BYTE ('a' + ((%1) - 0xa))
  %endif
%endmacro

%macro  debugNewline 0
    debugShowCharacter `\r`
    debugShowCharacter `\n`
%endmacro

%macro  debugShowPostCode 1
    debugShowHexDigit (((%1) >> 4) & 0xf)
    debugShowHexDigit ((%1) & 0xf)
    debugNewline
%endmacro

BITS    16

%macro  debugInitialize 0
	jmp	real16InitDebug
real16InitDebugReturn:
%endmacro

real16InitDebug:
    ;
    ; Set communications format
    ;
    outToSerialPort LCR_OFFSET, ((DLAB << 7) | SERIAL_DEFAULT_LCR)

    ;
    ; Configure baud rate
    ;
    outToSerialPort BAUD_HIGH_OFFSET, ((115200 / DEFAULT_BPS) >> 8)
    outToSerialPort BAUD_LOW_OFFSET, ((115200 / DEFAULT_BPS) & 0xff)

    ;
    ; Switch back to bank 0
    ;
    outToSerialPort LCR_OFFSET, SERIAL_DEFAULT_LCR

    jmp     real16InitDebugReturn

