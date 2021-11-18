/** @file
  Basic serial IO abstraction for GDB

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/GdbSerialLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>

// ---------------------------------------------
// UART Register Offsets
// ---------------------------------------------
#define BAUD_LOW_OFFSET    0x00
#define BAUD_HIGH_OFFSET   0x01
#define IER_OFFSET         0x01
#define LCR_SHADOW_OFFSET  0x01
#define FCR_SHADOW_OFFSET  0x02
#define IR_CONTROL_OFFSET  0x02
#define FCR_OFFSET         0x02
#define EIR_OFFSET         0x02
#define BSR_OFFSET         0x03
#define LCR_OFFSET         0x03
#define MCR_OFFSET         0x04
#define LSR_OFFSET         0x05
#define MSR_OFFSET         0x06

// ---------------------------------------------
// UART Register Bit Defines
// ---------------------------------------------
#define LSR_TXRDY    0x20U
#define LSR_RXDA     0x01U
#define DLAB         0x01U
#define ENABLE_FIFO  0x01U
#define CLEAR_FIFOS  0x06U

// IO Port Base for the UART
UINTN  gPort;

/**
  The constructor function initializes the UART.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
RETURN_STATUS
EFIAPI
GdbSerialLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT64  BaudRate;
  UINT8   DataBits;
  UINT8   Parity;
  UINT8   StopBits;

  gPort = (UINTN)PcdGet32 (PcdGdbUartPort);

  BaudRate = PcdGet64 (PcdGdbBaudRate);
  Parity   = PcdGet8 (PcdGdbParity);
  DataBits = PcdGet8 (PcdGdbDataBits);
  StopBits = PcdGet8 (PcdGdbStopBits);

  return GdbSerialInit (BaudRate, Parity, DataBits, StopBits);
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data buts, and stop bits on a serial device. This call is optional as the serial
  port will be set up with defaults base on PCD values.

  @param  BaudRate         The requested baud rate. A BaudRate value of 0 will use the the
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
  )
{
  UINTN  Divisor;
  UINT8  OutputData;
  UINT8  Data;
  UINT8  BreakSet = 0;

  //
  // We assume the UART has been turned on to decode gPort address range
  //

  //
  // Map 5..8 to 0..3
  //
  Data = (UINT8)(DataBits - (UINT8)5);

  //
  // Calculate divisor for baud generator
  //
  Divisor = 115200/(UINTN)BaudRate;

  //
  // Set communications format
  //
  OutputData = (UINT8)((DLAB << 7) | ((BreakSet << 6) | ((Parity << 3) | ((StopBits << 2) | Data))));
  IoWrite8 (gPort + LCR_OFFSET, OutputData);

  //
  // Configure baud rate
  //
  IoWrite8 (gPort + BAUD_HIGH_OFFSET, (UINT8)(Divisor >> 8));
  IoWrite8 (gPort + BAUD_LOW_OFFSET, (UINT8)(Divisor & 0xff));

  //
  // Switch back to bank 0
  //
  OutputData = (UINT8)((~DLAB<<7)|((BreakSet<<6)|((Parity<<3)|((StopBits<<2)| Data))));
  IoWrite8 (gPort + LCR_OFFSET, OutputData);

  // Not sure this is the right place to enable the FIFOs....
  // We probably need the FIFO enabled to not drop input
  IoWrite8 (gPort + FCR_SHADOW_OFFSET, ENABLE_FIFO);

  // Configure the UART hardware here
  return RETURN_SUCCESS;
}

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
  )
{
  UINT8  Data;

  Data = IoRead8 (gPort + LSR_OFFSET);

  return ((Data & LSR_RXDA) == LSR_RXDA);
}

/**
  Get a character from GDB. This function must be able to run in interrupt context.

  @return A character from GDB

**/
CHAR8
EFIAPI
GdbGetChar (
  VOID
  )
{
  UINT8  Data;
  CHAR8  Char;

  // Wait for the serial port to be ready
  do {
    Data = IoRead8 (gPort + LSR_OFFSET);
  } while ((Data & LSR_RXDA) == 0);

  Char = IoRead8 (gPort);

  // Make this an DEBUG_INFO after we get everything debugged.
  DEBUG ((DEBUG_ERROR, "<%c<", Char));
  return Char;
}

/**
  Send a character to GDB. This function must be able to run in interrupt context.


  @param  Char    Send a character to GDB

**/
VOID
EFIAPI
GdbPutChar (
  IN  CHAR8  Char
  )
{
  UINT8  Data;

  // Make this an DEBUG_INFO after we get everything debugged.
  DEBUG ((DEBUG_ERROR, ">%c>", Char));

  // Wait for the serial port to be ready
  do {
    Data = IoRead8 (gPort + LSR_OFFSET);
  } while ((Data & LSR_TXRDY) == 0);

  IoWrite8 (gPort, Char);
}

/**
  Send an ASCII string to GDB. This function must be able to run in interrupt context.


  @param  String    Send a string to GDB

**/
VOID
GdbPutString (
  IN CHAR8  *String
  )
{
  while (*String != '\0') {
    GdbPutChar (*String);
    String++;
  }
}
