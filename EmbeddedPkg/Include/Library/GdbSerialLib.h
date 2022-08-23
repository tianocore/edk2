/** @file
  Basic serial IO abstraction for GDB

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __GDB_SERIAL_LIB_H__
#define __GDB_SERIAL_LIB_H__

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data buts, and stop bits on a serial device. This call is optional as the serial
  port will be set up with defaults base on PCD values.

  @param  BaudRate         The requested baud rate. A BaudRate value of 0 will use the
                           device's default interface speed.
  @param  Parity           The type of parity to use on this serial device. A Parity value of
                           DefaultParity will use the device's default parity value.
  @param  DataBits         The number of data bits to use on the serial device. A DataBits
                           value of 0 will use the device's default data bit setting.
  @param  StopBits         The number of stop bits to use on this serial device. A StopBits
                           value of DefaultStopBits will use the device's default number of
                           stop bits.

  @retval EFI_SUCCESS      The device was configured.
  @retval EFI_DEVICE_ERROR The serial device could not be configured.

**/
RETURN_STATUS
EFIAPI
GdbSerialInit (
  IN UINT64  BaudRate,
  IN UINT8   Parity,
  IN UINT8   DataBits,
  IN UINT8   StopBits
  );

/**
  Check to see if a character is available from GDB. Do not read the character as that is
  done via GdbGetChar().

  @return TRUE  - Character available
  @return FALSE - Character not available

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
  IN  CHAR8  Char
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
