/** @file
  Serial I/O Port library functions with no library constructor/destructor

  Copyright (c) 2012-2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/SerialPortExtLib.h>

#include <Drivers/PL011Uart.h>

/**
  Set new attributes to PL011.

  @param  BaudRate                The baud rate of the serial device. If the baud rate is not supported,
                                  the speed will be reduced down to the nearest supported one and the
                                  variable's value will be updated accordingly.
  @param  ReceiveFifoDepth        The number of characters the device will buffer on input. If the specified
                                  value is not supported, the variable's value will be reduced down to the
                                  nearest supported one.
  @param  Timeout                 If applicable, the number of microseconds the device will wait
                                  before timing out a Read or a Write operation.
  @param  Parity                  If applicable, this is the EFI_PARITY_TYPE that is computed or checked
                                  as each character is transmitted or received. If the device does not
                                  support parity, the value is the default parity value.
  @param  DataBits                The number of data bits in each character
  @param  StopBits                If applicable, the EFI_STOP_BITS_TYPE number of stop bits per character.
                                  If the device does not support stop bits, the value is the default stop
                                  bit value.

  @retval EFI_SUCCESS             All attributes were set correctly on the serial device.
  @retval EFI_INVALID_PARAMETERS  One or more of the attributes has an unsupported value.

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
  return PL011UartInitializePort (
        (UINTN)PcdGet64 (PcdSerialRegisterBase),
        BaudRate,
        ReceiveFifoDepth,
        Parity,
        DataBits,
        StopBits);
}

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
  )
{
  return PL011UartSetControl ((UINTN)PcdGet64 (PcdSerialRegisterBase), Control);
}

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

  @retval RETURN_SUCCESS  The control bits were read from the serial device.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  return PL011UartGetControl ((UINTN)PcdGet64 (PcdSerialRegisterBase), Control);
}
