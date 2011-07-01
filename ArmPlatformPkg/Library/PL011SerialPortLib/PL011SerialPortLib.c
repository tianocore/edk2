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
#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>

#include <Drivers/PL011Uart.h>


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
  // No parity, 1 stop, no fifo, 8 data bits
  return PL011UartInitialize (
      (UINTN)PcdGet64 (PcdSerialRegisterBase),
      (UINTN)PcdGet64 (PcdUartDefaultBaudRate),
      PL011_UARTLCR_H_WLEN_8);
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
  return PL011UartWrite ((UINTN)PcdGet64 (PcdSerialRegisterBase), Buffer, NumberOfBytes);
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
  return PL011UartRead ((UINTN)PcdGet64 (PcdSerialRegisterBase), Buffer, NumberOfBytes);
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
  return PL011UartPoll ((UINTN)PcdGet64 (PcdSerialRegisterBase));
}
