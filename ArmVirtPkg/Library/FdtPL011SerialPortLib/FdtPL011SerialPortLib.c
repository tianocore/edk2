/** @file
  Serial I/O Port library functions with base address discovered from FDT

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2012 - 2013, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2014, Red Hat, Inc.<BR>
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/PcdLib.h>
#include <Library/PL011UartLib.h>
#include <Library/SerialPortLib.h>
#include <Pi/PiBootMode.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Guid/EarlyPL011BaseAddress.h>

STATIC UINTN          mSerialBaseAddress;
STATIC RETURN_STATUS  mPermanentStatus = RETURN_SUCCESS;

/**
  Program hardware of Serial port

  @retval RETURN_SUCCESS    If the serial port was initialized successfully by
                            this call, or an earlier call, to
                            SerialPortInitialize().

  @retval RETURN_NOT_FOUND  If no PL011 base address could be found.

  @return                   Error codes forwarded from
                            PL011UartInitializePort().
**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  VOID                            *Hob;
  RETURN_STATUS                   Status;
  CONST EARLY_PL011_BASE_ADDRESS  *UartBase;
  UINTN                           SerialBaseAddress;
  UINT64                          BaudRate;
  UINT32                          ReceiveFifoDepth;
  EFI_PARITY_TYPE                 Parity;
  UINT8                           DataBits;
  EFI_STOP_BITS_TYPE              StopBits;

  if (mSerialBaseAddress != 0) {
    return RETURN_SUCCESS;
  }

  if (RETURN_ERROR (mPermanentStatus)) {
    return mPermanentStatus;
  }

  Hob = GetFirstGuidHob (&gEarlyPL011BaseAddressGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof *UartBase)) {
    Status = RETURN_NOT_FOUND;
    goto Failed;
  }

  UartBase = GET_GUID_HOB_DATA (Hob);

  SerialBaseAddress = (UINTN)UartBase->ConsoleAddress;
  if (SerialBaseAddress == 0) {
    Status = RETURN_NOT_FOUND;
    goto Failed;
  }

  BaudRate         = (UINTN)PcdGet64 (PcdUartDefaultBaudRate);
  ReceiveFifoDepth = 0; // Use the default value for Fifo depth
  Parity           = (EFI_PARITY_TYPE)PcdGet8 (PcdUartDefaultParity);
  DataBits         = PcdGet8 (PcdUartDefaultDataBits);
  StopBits         = (EFI_STOP_BITS_TYPE)PcdGet8 (PcdUartDefaultStopBits);

  Status = PL011UartInitializePort (
             SerialBaseAddress,
             FixedPcdGet32 (PL011UartClkInHz),
             &BaudRate,
             &ReceiveFifoDepth,
             &Parity,
             &DataBits,
             &StopBits
             );
  if (RETURN_ERROR (Status)) {
    goto Failed;
  }

  mSerialBaseAddress = SerialBaseAddress;
  return RETURN_SUCCESS;

Failed:
  mPermanentStatus = Status;
  return Status;
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
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  if (!RETURN_ERROR (SerialPortInitialize ())) {
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
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  )
{
  if (!RETURN_ERROR (SerialPortInitialize ())) {
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
  if (!RETURN_ERROR (SerialPortInitialize ())) {
    return PL011UartPoll (mSerialBaseAddress);
  }

  return FALSE;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data bits, and stop bits on a serial device.

  @param BaudRate           The requested baud rate. A BaudRate value of 0 will use the
                            device's default interface speed.
                            On output, the value actually set.
  @param ReceiveFifoDepth   The requested depth of the FIFO on the receive side of the
                            serial interface. A ReceiveFifoDepth value of 0 will use
                            the device's default FIFO depth.
                            On output, the value actually set.
  @param Timeout            The requested time out for a single character in microseconds.
                            This timeout applies to both the transmit and receive side of the
                            interface. A Timeout value of 0 will use the device's default time
                            out value.
                            On output, the value actually set.
  @param Parity             The type of parity to use on this serial device. A Parity value of
                            DefaultParity will use the device's default parity value.
                            On output, the value actually set.
  @param DataBits           The number of data bits to use on the serial device. A DataBits
                            value of 0 will use the device's default data bit setting.
                            On output, the value actually set.
  @param StopBits           The number of stop bits to use on this serial device. A StopBits
                            value of DefaultStopBits will use the device's default number of
                            stop bits.
                            On output, the value actually set.

  @retval RETURN_SUCCESS            The new attributes were set on the serial device.
  @retval RETURN_UNSUPPORTED        The serial device does not support this operation.
  @retval RETURN_INVALID_PARAMETER  One or more of the attributes has an unsupported value.
  @retval RETURN_DEVICE_ERROR       The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT UINT32              *Timeout,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  RETURN_STATUS  Status;

  if (RETURN_ERROR (SerialPortInitialize ())) {
    Status = RETURN_UNSUPPORTED;
  } else {
    Status = PL011UartInitializePort (
               mSerialBaseAddress,
               FixedPcdGet32 (PL011UartClkInHz),
               BaudRate,
               ReceiveFifoDepth,
               Parity,
               DataBits,
               StopBits
               );
  }

  return Status;
}

/**
  Sets the control bits on a serial device.

  @param Control                Sets the bits of Control that are settable.

  @retval RETURN_SUCCESS        The new control bits were set on the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32  Control
  )
{
  RETURN_STATUS  Status;

  if (RETURN_ERROR (SerialPortInitialize ())) {
    Status = RETURN_UNSUPPORTED;
  } else {
    Status = PL011UartSetControl (mSerialBaseAddress, Control);
  }

  return Status;
}

/**
  Retrieve the status of the control bits on a serial device.

  @param Control                A pointer to return the current control signals from the serial device.

  @retval RETURN_SUCCESS        The control bits were read from the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  RETURN_STATUS  Status;

  if (RETURN_ERROR (SerialPortInitialize ())) {
    Status = RETURN_UNSUPPORTED;
  } else {
    Status = PL011UartGetControl (mSerialBaseAddress, Control);
  }

  return Status;
}
