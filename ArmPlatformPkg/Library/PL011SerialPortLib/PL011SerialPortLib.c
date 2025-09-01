/** @file
  Serial I/O Port library functions with no library constructor/destructor

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2012 - 2016, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PL011UartClockLib.h>
#include <Library/PL011UartLib.h>
#include <Library/SerialPortLib.h>

/** Initialise the serial device hardware with default settings.

  @retval RETURN_SUCCESS            The serial device was initialised.
  @retval RETURN_INVALID_PARAMETER  One or more of the default settings
                                    has an unsupported value.
 **/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UINT64              BaudRate;
  UINT32              ReceiveFifoDepth;
  EFI_PARITY_TYPE     Parity;
  UINT8               DataBits;
  EFI_STOP_BITS_TYPE  StopBits;
  EFI_STATUS          Status;
  UINT8               Scratch;

  BaudRate         = PcdGet64 (PcdUartDefaultBaudRate);
  ReceiveFifoDepth = 0;         // Use default FIFO depth
  Parity           = (EFI_PARITY_TYPE)PcdGet8 (PcdUartDefaultParity);
  DataBits         = PcdGet8 (PcdUartDefaultDataBits);
  StopBits         = (EFI_STOP_BITS_TYPE)PcdGet8 (PcdUartDefaultStopBits);

  Status = PL011UartInitializePort (
             (UINTN)PcdGet64 (PcdSerialRegisterBase),
             PL011UartClockGetFreq (),
             &BaudRate,
             &ReceiveFifoDepth,
             &Parity,
             &DataBits,
             &StopBits
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (SerialPortPoll ()) {
    SerialPortRead (&Scratch, sizeof (Scratch));
  }

  return EFI_SUCCESS;
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
  return PL011UartWrite ((UINTN)PcdGet64 (PcdSerialRegisterBase), Buffer, NumberOfBytes);
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
  return PL011UartRead ((UINTN)PcdGet64 (PcdSerialRegisterBase), Buffer, NumberOfBytes);
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
  return PL011UartPoll ((UINTN)PcdGet64 (PcdSerialRegisterBase));
}

/**
  Set new attributes to PL011.

  @param  BaudRate                The baud rate of the serial device. If the
                                  baud rate is not supported, the speed will
                                  be reduced down to the nearest supported one
                                  and the variable's value will be updated
                                  accordingly.
  @param  ReceiveFifoDepth        The number of characters the device will
                                  buffer on input. If the specified value is
                                  not supported, the variable's value will
                                  be reduced down to the nearest supported one.
  @param  Timeout                 If applicable, the number of microseconds the
                                  device will wait before timing out a Read or
                                  a Write operation.
  @param  Parity                  If applicable, this is the EFI_PARITY_TYPE
                                  that is computed or checked as each character
                                  is transmitted or received. If the device
                                  does not support parity, the value is the
                                  default parity value.
  @param  DataBits                The number of data bits in each character
  @param  StopBits                If applicable, the EFI_STOP_BITS_TYPE number
                                  of stop bits per character. If the device
                                  does not support stop bits, the value is the
                                  default stop bit value.

  @retval EFI_SUCCESS             All attributes were set correctly.
  @retval EFI_INVALID_PARAMETERS  One or more attributes has an unsupported
                                  value.

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
           PL011UartClockGetFreq (),
           BaudRate,
           ReceiveFifoDepth,
           Parity,
           DataBits,
           StopBits
           );
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

  @retval  RETURN_SUCCESS      The new control bits were set on the device.
  @retval  RETURN_UNSUPPORTED  The device does not support this operation.

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

                        . EFI_SERIAL_DATA_CLEAR_TO_SEND,
                          EFI_SERIAL_DATA_SET_READY,
                          EFI_SERIAL_RING_INDICATE,
                          EFI_SERIAL_CARRIER_DETECT,
                          EFI_SERIAL_REQUEST_TO_SEND,
                          EFI_SERIAL_DATA_TERMINAL_READY
                          are all related to the DTE (Data Terminal Equipment)
                          and DCE (Data Communication Equipment) modes of
                          operation of the serial device.
                        . EFI_SERIAL_INPUT_BUFFER_EMPTY : equal to one if the
                          receive buffer is empty, 0 otherwise.
                        . EFI_SERIAL_OUTPUT_BUFFER_EMPTY : equal to one if the
                          transmit buffer is empty, 0 otherwise.
                        . EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE : equal to one if
                          the hardware loopback is enabled (the output feeds
                          the receive buffer), 0 otherwise.
                        . EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE : equal to one
                          if a loopback is accomplished by software, else 0.
                        . EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE : equal to
                          one if the hardware flow control based on CTS (Clear
                          To Send) and RTS (Ready To Send) control signals is
                          enabled, 0 otherwise.

  @retval RETURN_SUCCESS  The control bits were read from the device.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  return PL011UartGetControl ((UINTN)PcdGet64 (PcdSerialRegisterBase), Control);
}
