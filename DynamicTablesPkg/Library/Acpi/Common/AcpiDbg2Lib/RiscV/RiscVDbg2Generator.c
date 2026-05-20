/** @file
  RISC-V DBG2 Table Generator

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Microsoft Debug Port Table 2 (DBG2) Specification
**/

#include <ConfigurationManagerObject.h>
#include <Protocol/SerialIo.h>
#include "Dbg2Generator.h"

/**
  Initialise the serial port to the specified settings.
  The serial port is re-configured only if the specified settings
  are different from the current settings.
  All unspecified settings will be set to the default values.

  @param  SerialPortInfo          CM_ARCH_COMMON_SERIAL_PORT_INFO object describing
                                  the serial port.
  @param  BaudRate                The baud rate of the serial device. If the
                                  baud rate is not supported, the speed will be
                                  reduced to the nearest supported one and the
                                  variable's value will be updated accordingly.
  @param  ReceiveFifoDepth        The number of characters the device will
                                  buffer on input.  Value of 0 will use the
                                  device's default FIFO depth.
  @param  Parity                  If applicable, this is the EFI_PARITY_TYPE
                                  that is computed or checked as each character
                                  is transmitted or received. If the device
                                  does not support parity, the value is the
                                  default parity value.
  @param  DataBits                The number of data bits in each character.
  @param  StopBits                If applicable, the EFI_STOP_BITS_TYPE number
                                  of stop bits per character.
                                  If the device does not support stop bits, the
                                  value is the default stop bit value.

  @retval RETURN_SUCCESS            All attributes were set correctly on the
                                    serial device.
  @retval RETURN_INVALID_PARAMETER  One or more of the attributes has an
                                    unsupported value.
**/
RETURN_STATUS
EFIAPI
Dbg2InitializePort (
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo,
  IN OUT UINT64                              *BaudRate,
  IN OUT UINT32                              *ReceiveFifoDepth,
  IN OUT EFI_PARITY_TYPE                     *Parity,
  IN OUT UINT8                               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE                  *StopBits
  )
{
  // Nothing to do.
  return EFI_SUCCESS;
}
