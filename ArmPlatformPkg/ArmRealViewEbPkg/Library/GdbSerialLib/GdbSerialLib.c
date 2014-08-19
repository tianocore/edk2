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

#include <Uefi.h>
#include <Library/GdbSerialLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Drivers/PL011Uart.h>

RETURN_STATUS
EFIAPI
GdbSerialLibConstructor (
  VOID
  )
{
  return GdbSerialInit (115200, 0, 8, 1);
}

RETURN_STATUS
EFIAPI
GdbSerialInit (
  IN UINT64     BaudRate,
  IN UINT8      Parity,
  IN UINT8      DataBits,
  IN UINT8      StopBits
  )
{
  if ((Parity != 0) || (DataBits != 8) || (StopBits != 1)) {
    return RETURN_UNSUPPORTED;
  }

  if (BaudRate != 115200) {
    // Could add support for different Baud rates....
    return RETURN_UNSUPPORTED;
  }

  UINT32  Base = PcdGet32 (PcdGdbUartBase);

  // initialize baud rate generator to 115200 based on EB clock REFCLK24MHZ
  MmioWrite32 (Base + UARTIBRD, UART_115200_IDIV);
  MmioWrite32 (Base + UARTFBRD, UART_115200_FDIV);

  // no parity, 1 stop, no fifo, 8 data bits
  MmioWrite32 (Base + UARTLCR_H, 0x60);

  // clear any pending errors
  MmioWrite32 (Base + UARTECR, 0);

  // enable tx, rx, and uart overall
  MmioWrite32 (Base + UARTCR, 0x301);

  return RETURN_SUCCESS;
}

BOOLEAN
EFIAPI
GdbIsCharAvailable (
  VOID
  )
{
  UINT32 FR = PcdGet32 (PcdGdbUartBase) + UARTFR;

  if ((MmioRead32 (FR) & UART_RX_EMPTY_FLAG_MASK) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

CHAR8
EFIAPI
GdbGetChar (
  VOID
  )
{
  UINT32  FR = PcdGet32 (PcdGdbUartBase) + UARTFR;
  UINT32  DR = PcdGet32 (PcdGdbUartBase) + UARTDR;

  while ((MmioRead32 (FR) & UART_RX_EMPTY_FLAG_MASK) == 0);
  return MmioRead8 (DR);
}

VOID
EFIAPI
GdbPutChar (
  IN  CHAR8   Char
  )
{
  UINT32 FR = PcdGet32 (PcdGdbUartBase) + UARTFR;
  UINT32 DR = PcdGet32 (PcdGdbUartBase) + UARTDR;

  while ((MmioRead32 (FR) & UART_TX_EMPTY_FLAG_MASK) != 0);
  MmioWrite8 (DR, Char);
  return;
}

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
