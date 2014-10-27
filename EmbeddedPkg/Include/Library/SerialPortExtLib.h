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

  Assert or deassert the control signals on a serial port.
  The following control signals are set according their bit settings :
  . Request to Send
  . Data Terminal Ready

  @param[in]  Control  The following bits are taken into account :
                       . EFI_SERIAL_REQUEST_TO_SEND : assert/deassert the
                         "Request To Send" control signal if this bit is
                         equal to one/zero.
                       . EFI_SERIAL_DATA_TERMINAL_READY : assert/deassert
                         the "Data Terminal Ready" control signal if this
                         bit is equal to one/zero.
                       . EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE : enable/disable
                         the hardware loopback if this bit is equal to
                         one/zero.
                       . EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE : not supported.
                       . EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE : enable/
                         disable the hardware flow control based on CTS (Clear
                         To Send) and RTS (Ready To Send) control signals.

  @retval  RETURN_SUCCESS      The new control bits were set on the serial device.
  @retval  RETURN_UNSUPPORTED  The serial device does not support this operation.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32  Control
  );

/**

  Retrieve the status of the control bits on a serial device.

  @param[out]  Control  Status of the control bits on a serial device :

                        . EFI_SERIAL_DATA_CLEAR_TO_SEND, EFI_SERIAL_DATA_SET_READY,
                          EFI_SERIAL_RING_INDICATE, EFI_SERIAL_CARRIER_DETECT,
                          EFI_SERIAL_REQUEST_TO_SEND, EFI_SERIAL_DATA_TERMINAL_READY
                          are all related to the DTE (Data Terminal Equipment) and
                          DCE (Data Communication Equipment) modes of operation of
                          the serial device.
                        . EFI_SERIAL_INPUT_BUFFER_EMPTY : equal to one if the receive
                          buffer is empty, 0 otherwise.
                        . EFI_SERIAL_OUTPUT_BUFFER_EMPTY : equal to one if the transmit
                          buffer is empty, 0 otherwise.
                        . EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE : equal to one if the
                          hardware loopback is enabled (the ouput feeds the receive
                          buffer), 0 otherwise.
                        . EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE : equal to one if a
                          loopback is accomplished by software, 0 otherwise.
                        . EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE : equal to one if the
                          hardware flow control based on CTS (Clear To Send) and RTS
                          (Ready To Send) control signals is enabled, 0 otherwise.

  @retval RETURN_SUCCESS       The control bits were read from the serial device.
  @retval RETURN_DEVICE_ERROR  The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
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

