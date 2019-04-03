/** @file
  Basic serial IO abstaction for GDB

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/GdbSerialLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/OmapLib.h>
#include <Omap3530/Omap3530.h>

RETURN_STATUS
EFIAPI
GdbSerialLibConstructor (
  VOID
  )
{
  return RETURN_SUCCESS;
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
  return RETURN_SUCCESS;
}

BOOLEAN
EFIAPI
GdbIsCharAvailable (
  VOID
  )
{
  UINT32 LSR = UartBase(PcdGet32(PcdOmap35xxConsoleUart)) + UART_LSR_REG;

  if ((MmioRead8(LSR) & UART_LSR_RX_FIFO_E_MASK) == UART_LSR_RX_FIFO_E_NOT_EMPTY) {
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
  UINT32  LSR = UartBase(PcdGet32(PcdOmap35xxConsoleUart)) + UART_LSR_REG;
  UINT32  RBR = UartBase(PcdGet32(PcdOmap35xxConsoleUart)) + UART_RBR_REG;
  CHAR8   Char;

  while ((MmioRead8(LSR) & UART_LSR_RX_FIFO_E_MASK) == UART_LSR_RX_FIFO_E_EMPTY);
  Char = MmioRead8(RBR);

  return Char;
}

VOID
EFIAPI
GdbPutChar (
  IN  CHAR8   Char
  )
{
  UINT32  LSR = UartBase(PcdGet32(PcdOmap35xxConsoleUart)) + UART_LSR_REG;
  UINT32  THR = UartBase(PcdGet32(PcdOmap35xxConsoleUart)) + UART_THR_REG;

  while ((MmioRead8(LSR) & UART_LSR_TX_FIFO_E_MASK) == UART_LSR_TX_FIFO_E_NOT_EMPTY);
  MmioWrite8(THR, Char);
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




