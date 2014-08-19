/** @file
  Basic serial IO abstaction for GDB

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __GDB_SERIAL_LIB_H__
#define __GDB_SERIAL_LIB_H__



/**
  Sets the baud rate, receive FIFO depth, transmit/receice time out, parity,
  data buts, and stop bits on a serial device. This call is optional as the serial
  port will be set up with defaults base on PCD values.

  @param  BaudRate         The requested baud rate. A BaudRate value of 0 will use the the
                           device's default interface speed.
  @param  Parity           The type of parity to use on this serial device. A Parity value of
                           DefaultParity will use the device's default parity value.
  @param  DataBits         The number of data bits to use on the serial device. A DataBits
                           vaule of 0 will use the device's default data bit setting.
  @param  StopBits         The number of stop bits to use on this serial device. A StopBits
                           value of DefaultStopBits will use the device's default number of
                           stop bits.

  @retval EFI_SUCCESS      The device was configured.
  @retval EFI_DEVICE_ERROR The serial device could not be coonfigured.

**/
RETURN_STATUS
EFIAPI
GdbSerialInit (
  IN UINT64     BaudRate,
  IN UINT8      Parity,
  IN UINT8      DataBits,
  IN UINT8      StopBits
  );


/**
  Check to see if a character is available from GDB. Do not read the character as that is
  done via GdbGetChar().

  @return TRUE  - Character availible
  @return FALSE - Character not availible

**/
BOOLEAN
EFIAPI
GdbIsCharAvailable (
  VOID
  );

/**
  Get a character from GDB. This function must be able to run in interrupt context.

  @return A character from GDB

**/
CHAR8
EFIAPI
GdbGetChar (
  VOID
  );


/**
  Send a character to GDB. This function must be able to run in interrupt context.


  @param  Char    Send a character to GDB

**/

VOID
EFIAPI
GdbPutChar (
  IN  CHAR8   Char
  );


/**
  Send an ASCII string to GDB. This function must be able to run in interrupt context.


  @param  String    Send a string to GDB

**/

VOID
GdbPutString (
  IN CHAR8  *String
  );


#endif

