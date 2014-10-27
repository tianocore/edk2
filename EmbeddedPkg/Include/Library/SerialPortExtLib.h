/** @file

  Serial I/O port control interface extension.

  This library provides an extension to the library providing common
  serial I/O port functions that is defined in MdePkg. The aim is to
  provide more control over the functionalities of a serial port. The
  extension covers all the needs of the UEFI Serial I/O Protocol.
  Though, its use is not restricted to the UEFI Serial I/O Protocol.
  It could for example be used in the PEI phase of the boot sequence
  as well.

  Copyright (c) 2012 - 2014, ARM Ltd. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SERIAL_PORT_EXT_LIB_H__
#define __SERIAL_PORT_EXT_LIB_H__

#include <Uefi/UefiBaseType.h>
#include <Protocol/SerialIo.h>

/**
  Set the serial device control bits.

  @return    Always return EFI_UNSUPPORTED.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32                   Control
  );

/**
  Get the serial device control bits.

  @param  Control                 Control signals read from the serial device.

  @retval EFI_SUCCESS             The control bits were read from the serial device.
  @retval EFI_DEVICE_ERROR        The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32                  *Control
  );

/**
  Set the serial device attributes.

  @return    Always return EFI_UNSUPPORTED.

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
  );

#endif

