/** @file
  Serial I/O Port library functions with no library constructor/destructor


  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/OmapLib.h>
#include <Omap3530/Omap3530.h>

/*

  Programmed hardware of Serial port.

  @return    Always return EFI_UNSUPPORTED.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  // assume assembly code at reset vector has setup UART
  return RETURN_SUCCESS;
}

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes writed to serial device.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
)
{
  UINT32  LSR = UartBase(PcdGet32(PcdOmap35xxConsoleUart)) + UART_LSR_REG;
  UINT32  THR = UartBase(PcdGet32(PcdOmap35xxConsoleUart)) + UART_THR_REG;
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while ((MmioRead8(LSR) & UART_LSR_TX_FIFO_E_MASK) == UART_LSR_TX_FIFO_E_NOT_EMPTY);
    MmioWrite8(THR, *Buffer);
  }

  return NumberOfBytes;
}


/**
  Read data from serial device and save the datas in buffer.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Aactual number of bytes read from serial device.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
)
{
  UINT32  LSR = UartBase(PcdGet32(PcdOmap35xxConsoleUart)) + UART_LSR_REG;
  UINT32  RBR = UartBase(PcdGet32(PcdOmap35xxConsoleUart)) + UART_RBR_REG;
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while ((MmioRead8(LSR) & UART_LSR_RX_FIFO_E_MASK) == UART_LSR_RX_FIFO_E_EMPTY);
    *Buffer = MmioRead8(RBR);
  }

  return NumberOfBytes;
}


/**
  Check to see if any data is avaiable to be read from the debug device.

  @retval EFI_SUCCESS       At least one byte of data is avaiable to be read
  @retval EFI_NOT_READY     No data is avaiable to be read
  @retval EFI_DEVICE_ERROR  The serial device is not functioning properly

**/
BOOLEAN
EFIAPI
SerialPortPoll (
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

