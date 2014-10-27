/** @file
  Extended Serial I/O Port library functions

  Copyright (c) 2012, ARM Ltd. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/SerialPortLib.h>
#include <Library/SerialPortExtLib.h>

/**
  Set the serial device control bits.

  @return    Always return RETURN_UNSUPPORTED.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
    IN UINT32                   Control
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Get the serial device control bits.

  @param  Control  Control signals read from the serial device.

  @retval EFI_SUCCESS             The control bits were read from the serial device.
  @retval EFI_DEVICE_ERROR        The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  if (SerialPortPoll ()) {
    // If a character is pending don't set EFI_SERIAL_INPUT_BUFFER_EMPTY
    *Control = EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
  } else {
    *Control = EFI_SERIAL_INPUT_BUFFER_EMPTY | EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
  }
  return RETURN_SUCCESS;
}

/**
  Set the serial device attributes.

  @return    Always return RETURN_UNSUPPORTED.

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
  return RETURN_UNSUPPORTED;
}

