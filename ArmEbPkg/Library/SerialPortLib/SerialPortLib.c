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

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>

#include <ArmEb/ArmEb.h>

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
  UINT32  Base = PcdGet32 (PcdConsoleUart);
  
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
  UINT32 FR = PcdGet32(PcdConsoleUart) + UARTFR;
  UINT32 DR = PcdGet32(PcdConsoleUart) + UARTDR;
  UINTN  Count;
    
  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while ((MmioRead32 (FR) & UART_TX_EMPTY_FLAG_MASK) != 0);
    MmioWrite8 (DR, *Buffer);
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
  UINT32  FR = PcdGet32(PcdConsoleUart) + UARTFR;
  UINT32  DR = PcdGet32(PcdConsoleUart) + UARTDR;
  UINTN   Count;
    
  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while ((MmioRead32 (FR) & UART_RX_EMPTY_FLAG_MASK) == 0);
    *Buffer = MmioRead8 (DR);
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
  UINT32 FR = PcdGet32(PcdConsoleUart) + UARTFR;

  if ((MmioRead32 (FR) & UART_RX_EMPTY_FLAG_MASK) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

