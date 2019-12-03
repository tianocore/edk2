/** @file
  Platform Hook Library instance for UART device.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Library/PciLib.h>
#include <Library/PlatformHookLib.h>
#include <Library/BlParseLib.h>
#include <Library/PcdLib.h>

typedef struct {
  UINT16  VendorId;          ///< Vendor ID to match the PCI device.  The value 0xFFFF terminates the list of entries.
  UINT16  DeviceId;          ///< Device ID to match the PCI device
  UINT32  ClockRate;         ///< UART clock rate.  Set to 0 for default clock rate of 1843200 Hz
  UINT64  Offset;            ///< The byte offset into to the BAR
  UINT8   BarIndex;          ///< Which BAR to get the UART base address
  UINT8   RegisterStride;    ///< UART register stride in bytes.  Set to 0 for default register stride of 1 byte.
  UINT16  ReceiveFifoDepth;  ///< UART receive FIFO depth in bytes. Set to 0 for a default FIFO depth of 16 bytes.
  UINT16  TransmitFifoDepth; ///< UART transmit FIFO depth in bytes. Set to 0 for a default FIFO depth of 16 bytes.
  UINT8   Reserved[2];
} PCI_SERIAL_PARAMETER;

/**
  Performs platform specific initialization required for the CPU to access
  the hardware associated with a SerialPortLib instance.  This function does
  not initialize the serial port hardware itself.  Instead, it initializes
  hardware devices that are required for the CPU to access the serial port
  hardware.  This function may be called more than once.

  @retval RETURN_SUCCESS       The platform specific initialization succeeded.
  @retval RETURN_DEVICE_ERROR  The platform specific initialization could not be completed.

**/
RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  )
{
  RETURN_STATUS         Status;
  UINT32                DeviceVendor;
  PCI_SERIAL_PARAMETER  *SerialParam;
  SERIAL_PORT_INFO      SerialPortInfo;

  Status = ParseSerialInfo (&SerialPortInfo);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if (SerialPortInfo.Type == PLD_SERIAL_TYPE_MEMORY_MAPPED) {
    Status = PcdSetBoolS (PcdSerialUseMmio, TRUE);
  } else { //IO
    Status = PcdSetBoolS (PcdSerialUseMmio, FALSE);
  }
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  Status = PcdSet64S (PcdSerialRegisterBase, SerialPortInfo.BaseAddr);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdSerialRegisterStride, SerialPortInfo.RegWidth);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdSerialBaudRate, SerialPortInfo.Baud);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet64S (PcdUartDefaultBaudRate, SerialPortInfo.Baud);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdSerialClockRate, SerialPortInfo.InputHertz);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if (SerialPortInfo.UartPciAddr >= 0x80000000) {
    DeviceVendor = PciRead32 (SerialPortInfo.UartPciAddr & 0x0ffff000);
    SerialParam  = PcdGetPtr(PcdPciSerialParameters);
    SerialParam->VendorId  = (UINT16)DeviceVendor;
    SerialParam->DeviceId  = DeviceVendor >> 16;
    SerialParam->ClockRate = SerialPortInfo.InputHertz;
    SerialParam->RegisterStride = (UINT8)SerialPortInfo.RegWidth;
  }

  return RETURN_SUCCESS;
}
