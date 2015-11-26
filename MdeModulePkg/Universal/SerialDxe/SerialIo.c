/** @file
  Serial driver that layers on top of a Serial Port Library instance.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/UefiBootServicesTableLib.h>
#include <Library/SerialPortLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Protocol/SerialIo.h>
#include <Protocol/DevicePath.h>

typedef struct {
  VENDOR_DEVICE_PATH        Guid;
  UART_DEVICE_PATH          Uart;
  EFI_DEVICE_PATH_PROTOCOL  End;
} SERIAL_DEVICE_PATH;

/**
  Reset the serial device.

  @param  This              Protocol instance pointer.

  @retval EFI_SUCCESS       The device was reset.
  @retval EFI_DEVICE_ERROR  The serial device could not be reset.

**/
EFI_STATUS
EFIAPI
SerialReset (
  IN EFI_SERIAL_IO_PROTOCOL *This
  );

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data bits, and stop bits on a serial device.

  @param  This             Protocol instance pointer.
  @param  BaudRate         The requested baud rate. A BaudRate value of 0 will use the the
                           device's default interface speed.
  @param  ReceiveFifoDepth The requested depth of the FIFO on the receive side of the
                           serial interface. A ReceiveFifoDepth value of 0 will use
                           the device's default FIFO depth.
  @param  Timeout          The requested time out for a single character in microseconds.
                           This timeout applies to both the transmit and receive side of the
                           interface. A Timeout value of 0 will use the device's default time
                           out value.
  @param  Parity           The type of parity to use on this serial device. A Parity value of
                           DefaultParity will use the device's default parity value.
  @param  DataBits         The number of data bits to use on the serial device. A DataBits
                           value of 0 will use the device's default data bit setting.
  @param  StopBits         The number of stop bits to use on this serial device. A StopBits
                           value of DefaultStopBits will use the device's default number of
                           stop bits.

  @retval EFI_SUCCESS      The device was reset.
  @retval EFI_DEVICE_ERROR The serial device could not be reset.

**/
EFI_STATUS
EFIAPI
SerialSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN UINT64                 BaudRate,
  IN UINT32                 ReceiveFifoDepth,
  IN UINT32                 Timeout,
  IN EFI_PARITY_TYPE        Parity,
  IN UINT8                  DataBits,
  IN EFI_STOP_BITS_TYPE     StopBits
  );

/**
  Set the control bits on a serial device

  @param  This             Protocol instance pointer.
  @param  Control          Set the bits of Control that are settable.

  @retval EFI_SUCCESS      The new control bits were set on the serial device.
  @retval EFI_UNSUPPORTED  The serial device does not support this operation.
  @retval EFI_DEVICE_ERROR The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SerialSetControl (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN UINT32                 Control
  );

/**
  Retrieves the status of the control bits on a serial device

  @param  This              Protocol instance pointer.
  @param  Control           A pointer to return the current Control signals from the serial device.

  @retval EFI_SUCCESS       The control bits were read from the serial device.
  @retval EFI_DEVICE_ERROR  The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SerialGetControl (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  OUT UINT32                *Control
  );

/**
  Writes data to a serial device.

  @param  This              Protocol instance pointer.
  @param  BufferSize        On input, the size of the Buffer. On output, the amount of
                            data actually written.
  @param  Buffer            The buffer of data to write

  @retval EFI_SUCCESS       The data was written.
  @retval EFI_DEVICE_ERROR  The device reported an error.
  @retval EFI_TIMEOUT       The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
SerialWrite (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN OUT UINTN              *BufferSize,
  IN VOID                   *Buffer
  );

/**
  Reads data from a serial device.

  @param  This              Protocol instance pointer.
  @param  BufferSize        On input, the size of the Buffer. On output, the amount of
                            data returned in Buffer.
  @param  Buffer            The buffer to return the data into.

  @retval EFI_SUCCESS       The data was read.
  @retval EFI_DEVICE_ERROR  The device reported an error.
  @retval EFI_TIMEOUT       The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
SerialRead (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  );

EFI_HANDLE mSerialHandle = NULL;

SERIAL_DEVICE_PATH mSerialDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, { sizeof (VENDOR_DEVICE_PATH), 0} },
    EFI_CALLER_ID_GUID  // Use the driver's GUID
  },
  {
    { MESSAGING_DEVICE_PATH, MSG_UART_DP, { sizeof (UART_DEVICE_PATH), 0} },
    0,                  // Reserved
    0,                  // BaudRate
    0,                  // DataBits
    0,                  // Parity
    0                   // StopBits
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 } }
};

//
// Template used to initialize the Serial IO protocols.
//
EFI_SERIAL_IO_MODE mSerialIoMode = {
  0, // ControlMask
  0, // Timeout
  0, // BaudRate
  1, // ReceiveFifoDepth
  0, // DataBits
  0, // Parity
  0  // StopBits
};

EFI_SERIAL_IO_PROTOCOL mSerialIoTemplate = {
  SERIAL_IO_INTERFACE_REVISION,
  SerialReset,
  SerialSetAttributes,
  SerialSetControl,
  SerialGetControl,
  SerialWrite,
  SerialRead,
  &mSerialIoMode
};

/**
  Reset the serial device.

  @param  This              Protocol instance pointer.

  @retval EFI_SUCCESS       The device was reset.
  @retval EFI_DEVICE_ERROR  The serial device could not be reset.

**/
EFI_STATUS
EFIAPI
SerialReset (
  IN EFI_SERIAL_IO_PROTOCOL *This
  )
{
  EFI_STATUS    Status;
  EFI_TPL       Tpl;

  Status = SerialPortInitialize ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set the Serial I/O mode and update the device path
  //

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Set the Serial I/O mode
  //
  This->Mode->ReceiveFifoDepth  = 1;
  This->Mode->Timeout           = 0;
  This->Mode->BaudRate          = PcdGet64 (PcdUartDefaultBaudRate);
  This->Mode->DataBits          = (UINT32) PcdGet8 (PcdUartDefaultDataBits);
  This->Mode->Parity            = (UINT32) PcdGet8 (PcdUartDefaultParity);
  This->Mode->StopBits          = (UINT32) PcdGet8 (PcdUartDefaultStopBits);

  //
  // Check if the device path has actually changed
  //
  if (mSerialDevicePath.Uart.BaudRate == This->Mode->BaudRate &&
      mSerialDevicePath.Uart.DataBits == (UINT8) This->Mode->DataBits &&
      mSerialDevicePath.Uart.Parity   == (UINT8) This->Mode->Parity &&
      mSerialDevicePath.Uart.StopBits == (UINT8) This->Mode->StopBits
     ) {
    gBS->RestoreTPL (Tpl);
    return EFI_SUCCESS;
  }

  //
  // Update the device path
  //
  mSerialDevicePath.Uart.BaudRate = This->Mode->BaudRate;
  mSerialDevicePath.Uart.DataBits = (UINT8) This->Mode->DataBits;
  mSerialDevicePath.Uart.Parity   = (UINT8) This->Mode->Parity;
  mSerialDevicePath.Uart.StopBits = (UINT8) This->Mode->StopBits;

  Status = gBS->ReinstallProtocolInterface (
                  mSerialHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mSerialDevicePath,
                  &mSerialDevicePath
                  );

  gBS->RestoreTPL (Tpl);

  return Status;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data bits, and stop bits on a serial device.

  @param  This             Protocol instance pointer.
  @param  BaudRate         The requested baud rate. A BaudRate value of 0 will use the the
                           device's default interface speed.
  @param  ReceiveFifoDepth The requested depth of the FIFO on the receive side of the
                           serial interface. A ReceiveFifoDepth value of 0 will use
                           the device's default FIFO depth.
  @param  Timeout          The requested time out for a single character in microseconds.
                           This timeout applies to both the transmit and receive side of the
                           interface. A Timeout value of 0 will use the device's default time
                           out value.
  @param  Parity           The type of parity to use on this serial device. A Parity value of
                           DefaultParity will use the device's default parity value.
  @param  DataBits         The number of data bits to use on the serial device. A DataBits
                           value of 0 will use the device's default data bit setting.
  @param  StopBits         The number of stop bits to use on this serial device. A StopBits
                           value of DefaultStopBits will use the device's default number of
                           stop bits.

  @retval EFI_SUCCESS      The device was reset.
  @retval EFI_DEVICE_ERROR The serial device could not be reset.

**/
EFI_STATUS
EFIAPI
SerialSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN UINT64                 BaudRate,
  IN UINT32                 ReceiveFifoDepth,
  IN UINT32                 Timeout,
  IN EFI_PARITY_TYPE        Parity,
  IN UINT8                  DataBits,
  IN EFI_STOP_BITS_TYPE     StopBits
  )
{
  EFI_STATUS    Status;
  EFI_TPL       Tpl;

  Status = SerialPortSetAttributes (&BaudRate, &ReceiveFifoDepth, &Timeout, &Parity, &DataBits, &StopBits);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set the Serial I/O mode and update the device path
  //

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Set the Serial I/O mode
  //
  This->Mode->ReceiveFifoDepth  = ReceiveFifoDepth;
  This->Mode->Timeout           = Timeout;
  This->Mode->BaudRate          = BaudRate;
  This->Mode->DataBits          = (UINT32) DataBits;
  This->Mode->Parity            = (UINT32) Parity;
  This->Mode->StopBits          = (UINT32) StopBits;

  //
  // Check if the device path has actually changed
  //
  if (mSerialDevicePath.Uart.BaudRate == BaudRate &&
      mSerialDevicePath.Uart.DataBits == DataBits &&
      mSerialDevicePath.Uart.Parity   == (UINT8) Parity &&
      mSerialDevicePath.Uart.StopBits == (UINT8) StopBits
     ) {
    gBS->RestoreTPL (Tpl);
    return EFI_SUCCESS;
  }

  //
  // Update the device path
  //
  mSerialDevicePath.Uart.BaudRate = BaudRate;
  mSerialDevicePath.Uart.DataBits = DataBits;
  mSerialDevicePath.Uart.Parity   = (UINT8) Parity;
  mSerialDevicePath.Uart.StopBits = (UINT8) StopBits;

  Status = gBS->ReinstallProtocolInterface (
                  mSerialHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mSerialDevicePath,
                  &mSerialDevicePath
                  );

  gBS->RestoreTPL (Tpl);

  return Status;
}

/**
  Set the control bits on a serial device

  @param  This             Protocol instance pointer.
  @param  Control          Set the bits of Control that are settable.

  @retval EFI_SUCCESS      The new control bits were set on the serial device.
  @retval EFI_UNSUPPORTED  The serial device does not support this operation.
  @retval EFI_DEVICE_ERROR The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SerialSetControl (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN UINT32                 Control
  )
{
  return SerialPortSetControl (Control);
}

/**
  Retrieves the status of the control bits on a serial device

  @param  This              Protocol instance pointer.
  @param  Control           A pointer to return the current Control signals from the serial device.

  @retval EFI_SUCCESS       The control bits were read from the serial device.
  @retval EFI_DEVICE_ERROR  The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SerialGetControl (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  OUT UINT32                *Control
  )
{
  return SerialPortGetControl (Control);
}

/**
  Writes data to a serial device.

  @param  This              Protocol instance pointer.
  @param  BufferSize        On input, the size of the Buffer. On output, the amount of
                            data actually written.
  @param  Buffer            The buffer of data to write

  @retval EFI_SUCCESS       The data was written.
  @retval EFI_DEVICE_ERROR  The device reported an error.
  @retval EFI_TIMEOUT       The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
SerialWrite (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN OUT UINTN              *BufferSize,
  IN VOID                   *Buffer
  )
{
  UINTN Count;

  Count = SerialPortWrite (Buffer, *BufferSize);

  if (Count != *BufferSize) {
    *BufferSize = Count;
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Reads data from a serial device.

  @param  This              Protocol instance pointer.
  @param  BufferSize        On input, the size of the Buffer. On output, the amount of
                            data returned in Buffer.
  @param  Buffer            The buffer to return the data into.

  @retval EFI_SUCCESS       The data was read.
  @retval EFI_DEVICE_ERROR  The device reported an error.
  @retval EFI_TIMEOUT       The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
SerialRead (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  )
{
  UINTN Count;

  Count = 0;

  if (SerialPortPoll ()) {
    Count = SerialPortRead (Buffer, *BufferSize);
  }

  if (Count != *BufferSize) {
    *BufferSize = Count;
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Initialization for the Serial Io Protocol.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SerialDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS            Status;

  Status = SerialPortInitialize ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mSerialIoMode.BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  mSerialIoMode.DataBits = (UINT32) PcdGet8 (PcdUartDefaultDataBits);
  mSerialIoMode.Parity   = (UINT32) PcdGet8 (PcdUartDefaultParity);
  mSerialIoMode.StopBits = (UINT32) PcdGet8 (PcdUartDefaultStopBits);
  mSerialDevicePath.Uart.BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  mSerialDevicePath.Uart.DataBits = PcdGet8 (PcdUartDefaultDataBits);
  mSerialDevicePath.Uart.Parity   = PcdGet8 (PcdUartDefaultParity);
  mSerialDevicePath.Uart.StopBits = PcdGet8 (PcdUartDefaultStopBits);

  //
  // Make a new handle with Serial IO protocol and its device path on it.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSerialHandle,
                  &gEfiSerialIoProtocolGuid,   &mSerialIoTemplate,
                  &gEfiDevicePathProtocolGuid, &mSerialDevicePath,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

