/** @file
  Serial I/O Port library functions with base address discovered from FDT

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2012 - 2013, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2014, Red Hat, Inc.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>
#include <Pi/PiBootMode.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Guid/EarlyPL011BaseAddress.h>

#include <Drivers/PL011Uart.h>

STATIC UINTN mSerialBaseAddress;

RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  return RETURN_SUCCESS;
}

/**

  Program hardware of Serial port

  @return    RETURN_NOT_FOUND if no PL011 base address could be found
             Otherwise, result of PL011UartInitializePort () is returned

**/
RETURN_STATUS
EFIAPI
FdtPL011SerialPortLibInitialize (
  VOID
  )
{
  VOID                *Hob;
  CONST UINT64        *UartBase;
  UINT64              BaudRate;
  UINT32              ReceiveFifoDepth;
  EFI_PARITY_TYPE     Parity;
  UINT8               DataBits;
  EFI_STOP_BITS_TYPE  StopBits;

  Hob = GetFirstGuidHob (&gEarlyPL011BaseAddressGuid);
  if (Hob == NULL || GET_GUID_HOB_DATA_SIZE (Hob) != sizeof *UartBase) {
    return RETURN_NOT_FOUND;
  }
  UartBase = GET_GUID_HOB_DATA (Hob);

  mSerialBaseAddress = (UINTN)*UartBase;
  if (mSerialBaseAddress == 0) {
    return RETURN_NOT_FOUND;
  }

  BaudRate = (UINTN)PcdGet64 (PcdUartDefaultBaudRate);
  ReceiveFifoDepth = 0; // Use the default value for Fifo depth
  Parity = (EFI_PARITY_TYPE)PcdGet8 (PcdUartDefaultParity);
  DataBits = PcdGet8 (PcdUartDefaultDataBits);
  StopBits = (EFI_STOP_BITS_TYPE) PcdGet8 (PcdUartDefaultStopBits);

  return PL011UartInitializePort (
           mSerialBaseAddress, &BaudRate, &ReceiveFifoDepth,
           &Parity, &DataBits, &StopBits);
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
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  if (mSerialBaseAddress != 0) {
    return PL011UartWrite (mSerialBaseAddress, Buffer, NumberOfBytes);
  }
  return 0;
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
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
)
{
  if (mSerialBaseAddress != 0) {
    return PL011UartRead (mSerialBaseAddress, Buffer, NumberOfBytes);
  }
  return 0;
}

/**
  Check to see if any data is available to be read from the debug device.

  @retval TRUE       At least one byte of data is available to be read
  @retval FALSE      No data is available to be read

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  if (mSerialBaseAddress != 0) {
    return PL011UartPoll (mSerialBaseAddress);
  }
  return FALSE;
}
