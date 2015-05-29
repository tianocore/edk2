/** @file
  Serial I/O Port library functions with base address discovered from FDT

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2012 - 2013, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>

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
#include <Library/SerialPortExtLib.h>
#include <libfdt.h>

#include <Drivers/PL011Uart.h>

RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  //
  // This SerialPortInitialize() function is completely empty, for a number of
  // reasons:
  // - if we are executing from flash, it is hard to keep state (i.e., store the
  //   discovered base address in a global), and the most robust way to deal
  //   with this is to discover the base address at every Write ();
  // - calls to the Write() function in this module may be issued before this
  //   initialization function is called: this is not a problem when the base
  //   address of the UART is hardcoded, and only the baud rate may be wrong,
  //   but if we don't know the base address yet, we may be poking into memory
  //   that does not tolerate being poked into;
  // - SEC and PEI phases produce debug output only, so with debug disabled, no
  //   initialization (or device tree parsing) is performed at all.
  //
  // Note that this means that on *every* Write () call, the device tree will be
  // parsed and the UART re-initialized. However, this is a small price to pay
  // for having serial debug output on a UART with no fixed base address.
  //
  return RETURN_SUCCESS;
}

STATIC
UINT64
SerialPortGetBaseAddress (
  VOID
  )
{
  UINT64              BaudRate;
  UINT32              ReceiveFifoDepth;
  EFI_PARITY_TYPE     Parity;
  UINT8               DataBits;
  EFI_STOP_BITS_TYPE  StopBits;
  VOID                *DeviceTreeBase;
  INT32               Node, Prev;
  INT32               Len;
  CONST CHAR8         *Compatible;
  CONST CHAR8         *CompatibleItem;
  CONST UINT64        *RegProperty;
  UINTN               UartBase;
  RETURN_STATUS       Status;

  DeviceTreeBase = (VOID *)(UINTN)FixedPcdGet64 (PcdDeviceTreeInitialBaseAddress);

  if ((DeviceTreeBase == NULL) || (fdt_check_header (DeviceTreeBase) != 0)) {
    return 0;
  }

  //
  // Enumerate all FDT nodes looking for a PL011 and capture its base address
  //
  for (Prev = 0;; Prev = Node) {
    Node = fdt_next_node (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    Compatible = fdt_getprop (DeviceTreeBase, Node, "compatible", &Len);
    if (Compatible == NULL) {
      continue;
    }

    //
    // Iterate over the NULL-separated items in the compatible string
    //
    for (CompatibleItem = Compatible; CompatibleItem < Compatible + Len;
      CompatibleItem += 1 + AsciiStrLen (CompatibleItem)) {

      if (AsciiStrCmp (CompatibleItem, "arm,pl011") == 0) {
        RegProperty = fdt_getprop (DeviceTreeBase, Node, "reg", &Len);
        if (Len != 16) {
          return 0;
        }
        UartBase = (UINTN)fdt64_to_cpu (ReadUnaligned64 (RegProperty));

        BaudRate = (UINTN)FixedPcdGet64 (PcdUartDefaultBaudRate);
        ReceiveFifoDepth = 0; // Use the default value for Fifo depth
        Parity = (EFI_PARITY_TYPE)FixedPcdGet8 (PcdUartDefaultParity);
        DataBits = FixedPcdGet8 (PcdUartDefaultDataBits);
        StopBits = (EFI_STOP_BITS_TYPE) FixedPcdGet8 (PcdUartDefaultStopBits);

        Status = PL011UartInitializePort (
                   UartBase,
                   &BaudRate, &ReceiveFifoDepth, &Parity, &DataBits, &StopBits);
        if (!EFI_ERROR (Status)) {
          return UartBase;
        }
      }
    }
  }
  return 0;
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
  UINT64 SerialRegisterBase;

  SerialRegisterBase = SerialPortGetBaseAddress ();
  if (SerialRegisterBase != 0) {
    return PL011UartWrite ((UINTN)SerialRegisterBase, Buffer, NumberOfBytes);
  }
  return 0;
}

/**
  Read data from serial device and save the data in buffer.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Size of Buffer[].

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
  return FALSE;
}
