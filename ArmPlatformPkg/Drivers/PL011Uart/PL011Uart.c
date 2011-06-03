/** @file
  Serial I/O Port library functions with no library constructor/destructor


  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Include/Uefi.h>

#include <Library/IoLib.h>

#include <Drivers/PL011Uart.h>

/*

  Programmed hardware of Serial port.

  @return    Always return EFI_UNSUPPORTED.

**/
RETURN_STATUS
EFIAPI
PL011UartInitialize (
  IN  UINTN       UartBase,
  IN  UINTN       BaudRate,
  IN  UINTN       LineControl
  )
{
	if (BaudRate == 115200) {
		// Initialize baud rate generator
		MmioWrite32 (UartBase + UARTIBRD, UART_115200_IDIV);
		MmioWrite32 (UartBase + UARTFBRD, UART_115200_FDIV);
	} else if (BaudRate == 38400) {
		// Initialize baud rate generator
		MmioWrite32 (UartBase + UARTIBRD, UART_38400_IDIV);
		MmioWrite32 (UartBase + UARTFBRD, UART_38400_FDIV);
	} else if (BaudRate == 19200) {
		// Initialize baud rate generator
		MmioWrite32 (UartBase + UARTIBRD, UART_19200_IDIV);
		MmioWrite32 (UartBase + UARTFBRD, UART_19200_FDIV);
	} else {
		return EFI_INVALID_PARAMETER;
	}

  // No parity, 1 stop, no fifo, 8 data bits
  MmioWrite32 (UartBase + UARTLCR_H, LineControl);

  // Clear any pending errors
  MmioWrite32 (UartBase + UARTECR, 0);

  // Enable tx, rx, and uart overall
  MmioWrite32 (UartBase + UARTCR, PL011_UARTCR_RXE | PL011_UARTCR_TXE | PL011_UARTCR_UARTEN);

  return EFI_SUCCESS;
}

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes written to serial device.

**/
UINTN
EFIAPI
PL011UartWrite (
  IN  UINTN    UartBase,
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
	UINTN  Count;

	for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
		while ((MmioRead32 (UartBase + UARTFR) & UART_TX_EMPTY_FLAG_MASK) == 0);
		MmioWrite8 (UartBase + UARTDR, *Buffer);
	}

	return NumberOfBytes;
}

/**
  Read data from serial device and save the data in buffer.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Actual number of bytes read from serial device.

**/
UINTN
EFIAPI
PL011UartRead (
  IN  UINTN     UartBase,
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
  )
{
  UINTN   Count;

	for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
		while ((MmioRead32 (UartBase + UARTFR) & UART_RX_EMPTY_FLAG_MASK) != 0);
		*Buffer = MmioRead8 (UartBase + UARTDR);
	}

	return NumberOfBytes;
}

/**
  Check to see if any data is available to be read from the debug device.

  @retval EFI_SUCCESS       At least one byte of data is available to be read
  @retval EFI_NOT_READY     No data is available to be read
  @retval EFI_DEVICE_ERROR  The serial device is not functioning properly

**/
BOOLEAN
EFIAPI
PL011UartPoll (
  IN  UINTN     UartBase
  )
{
  return ((MmioRead32 (UartBase + UARTFR) & UART_RX_EMPTY_FLAG_MASK) == 0);
}
