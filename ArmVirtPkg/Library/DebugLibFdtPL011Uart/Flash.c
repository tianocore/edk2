/** @file
  Define DebugLibFdtPL011UartWrite() for modules that may run from flash or RAM.

  Copyright (C) Red Hat

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/FdtSerialPortAddressLib.h>
#include <Library/PL011UartLib.h>
#include <Library/PcdLib.h>

#include "Write.h"

/**
  (Copied from SerialPortWrite() in "MdePkg/Include/Library/SerialPortLib.h" at
  commit c4547aefb3d0, with the Buffer non-nullity assertion removed:)

  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the serial device.
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.
                           If this value is less than NumberOfBytes, then the write operation failed.
**/
UINTN
DebugLibFdtPL011UartWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  CONST VOID          *DeviceTree;
  RETURN_STATUS       Status;
  FDT_SERIAL_PORTS    Ports;
  UINT64              DebugAddress;
  UINT64              BaudRate;
  UINT32              ReceiveFifoDepth;
  EFI_PARITY_TYPE     Parity;
  UINT8               DataBits;
  EFI_STOP_BITS_TYPE  StopBits;

  DeviceTree = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  if (DeviceTree == NULL) {
    return 0;
  }

  Status = FdtSerialGetPorts (DeviceTree, "arm,pl011", &Ports);
  if (RETURN_ERROR (Status)) {
    return 0;
  }

  if (Ports.NumberOfPorts == 1) {
    //
    // Just one UART; direct DebugLib to it.
    //
    DebugAddress = Ports.BaseAddress[0];
  } else {
    UINT64  ConsoleAddress;

    Status = FdtSerialGetConsolePort (DeviceTree, &ConsoleAddress);
    if (EFI_ERROR (Status)) {
      //
      // At least two UARTs; but failed to get the console preference. Use the
      // second UART for DebugLib.
      //
      DebugAddress = Ports.BaseAddress[1];
    } else {
      //
      // At least two UARTs; and console preference available. Use the first
      // such UART for DebugLib that *differs* from ConsoleAddress.
      //
      if (ConsoleAddress == Ports.BaseAddress[0]) {
        DebugAddress = Ports.BaseAddress[1];
      } else {
        DebugAddress = Ports.BaseAddress[0];
      }
    }
  }

  BaudRate         = (UINTN)FixedPcdGet64 (PcdUartDefaultBaudRate);
  ReceiveFifoDepth = 0; // Use the default value for Fifo depth
  Parity           = (EFI_PARITY_TYPE)FixedPcdGet8 (PcdUartDefaultParity);
  DataBits         = FixedPcdGet8 (PcdUartDefaultDataBits);
  StopBits         = (EFI_STOP_BITS_TYPE)FixedPcdGet8 (PcdUartDefaultStopBits);

  Status = PL011UartInitializePort (
             (UINTN)DebugAddress,
             FixedPcdGet32 (PL011UartClkInHz),
             &BaudRate,
             &ReceiveFifoDepth,
             &Parity,
             &DataBits,
             &StopBits
             );
  if (RETURN_ERROR (Status)) {
    return 0;
  }

  return PL011UartWrite ((UINTN)DebugAddress, Buffer, NumberOfBytes);
}
