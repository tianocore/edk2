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
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DebugPort.h>


EFI_DEBUGPORT_PROTOCOL  *gDebugPort = NULL;
UINTN                   gTimeOut = 0;

/**
  The constructor function initializes the UART.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
RETURN_STATUS
EFIAPI
GdbSerialLibDebugPortConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;

  Status = gBS->LocateProtocol (&gEfiDebugPortProtocolGuid, NULL, (VOID **)&gDebugPort);
  if (!EFI_ERROR (Status)) {
    gTimeOut = PcdGet32 (PcdGdbMaxPacketRetryCount);
    gDebugPort->Reset (gDebugPort);
  }

  return Status;
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
  IN UINT64     BaudRate,
  IN UINT8      Parity,
  IN UINT8      DataBits,
  IN UINT8      StopBits
  )
{
  EFI_STATUS  Status;

  Status = gDebugPort->Reset (gDebugPort);
  return Status;
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
  EFI_STATUS  Status;

  Status = gDebugPort->Poll (gDebugPort);

  return (Status == EFI_SUCCESS ? TRUE : FALSE);
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
  EFI_STATUS  Status;
  CHAR8       Char;
  UINTN       BufferSize;

  do {
    BufferSize = sizeof (Char);
    Status = gDebugPort->Read (gDebugPort, gTimeOut, &BufferSize, &Char);
  } while (EFI_ERROR (Status) || BufferSize != sizeof (Char));

  return Char;
}


/**
  Send a character to GDB. This function must be able to run in interrupt context.


  @param  Char    Send a character to GDB

**/

VOID
EFIAPI
GdbPutChar (
  IN  CHAR8   Char
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  do {
    BufferSize = sizeof (Char);
    Status = gDebugPort->Write (gDebugPort, gTimeOut, &BufferSize, &Char);
  } while (EFI_ERROR (Status) || BufferSize != sizeof (Char));

  return;
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
 // We could performance enhance this function by calling gDebugPort->Write ()
  while (*String != '\0') {
    GdbPutChar (*String);
    String++;
  }
}




