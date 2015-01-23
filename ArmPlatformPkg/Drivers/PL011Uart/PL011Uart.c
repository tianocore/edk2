/** @file
  Serial I/O Port library functions with no library constructor/destructor

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <Drivers/PL011Uart.h>

//
// EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE is the only
// control bit that is not supported.
//
STATIC CONST UINT32 mInvalidControlBits = EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE;

/*

  Initialise the serial port to the specified settings.
  All unspecified settings will be set to the default values.

  @return    Always return EFI_SUCCESS or EFI_INVALID_PARAMETER.

**/
RETURN_STATUS
EFIAPI
PL011UartInitializePort (
  IN OUT UINTN               UartBase,
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  UINT32      LineControl;
  UINT32      Divisor;

  LineControl = 0;

  // The PL011 supports a buffer of 1, 16 or 32 chars. Therefore we can accept
  // 1 char buffer as the minimum fifo size. Because everything can be rounded down,
  // there is no maximum fifo size.
  if ((*ReceiveFifoDepth == 0) || (*ReceiveFifoDepth >= 32)) {
    LineControl |= PL011_UARTLCR_H_FEN;
    if (PL011_UARTPID2_VER (MmioRead32 (UartBase + UARTPID2)) > PL011_VER_R1P4)
      *ReceiveFifoDepth = 32;
    else
      *ReceiveFifoDepth = 16;
  } else {
    ASSERT (*ReceiveFifoDepth < 32);
    // Nothing else to do. 1 byte fifo is default.
    *ReceiveFifoDepth = 1;
  }

  //
  // Parity
  //
  switch (*Parity) {
  case DefaultParity:
    *Parity = NoParity;
  case NoParity:
    // Nothing to do. Parity is disabled by default.
    break;
  case EvenParity:
    LineControl |= (PL011_UARTLCR_H_PEN | PL011_UARTLCR_H_EPS);
    break;
  case OddParity:
    LineControl |= PL011_UARTLCR_H_PEN;
    break;
  case MarkParity:
    LineControl |= (PL011_UARTLCR_H_PEN | PL011_UARTLCR_H_SPS | PL011_UARTLCR_H_EPS);
    break;
  case SpaceParity:
    LineControl |= (PL011_UARTLCR_H_PEN | PL011_UARTLCR_H_SPS);
    break;
  default:
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Data Bits
  //
  switch (*DataBits) {
  case 0:
    *DataBits = 8;
  case 8:
    LineControl |= PL011_UARTLCR_H_WLEN_8;
    break;
  case 7:
    LineControl |= PL011_UARTLCR_H_WLEN_7;
    break;
  case 6:
    LineControl |= PL011_UARTLCR_H_WLEN_6;
    break;
  case 5:
    LineControl |= PL011_UARTLCR_H_WLEN_5;
    break;
  default:
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Stop Bits
  //
  switch (*StopBits) {
  case DefaultStopBits:
    *StopBits = OneStopBit;
  case OneStopBit:
    // Nothing to do. One stop bit is enabled by default.
    break;
  case TwoStopBits:
    LineControl |= PL011_UARTLCR_H_STP2;
    break;
  case OneFiveStopBits:
    // Only 1 or 2 stops bits are supported
  default:
    return RETURN_INVALID_PARAMETER;
  }

  // Don't send the LineControl value to the PL011 yet,
  // wait until after the Baud Rate setting.
  // This ensures we do not mess up the UART settings halfway through
  // in the rare case when there is an error with the Baud Rate.

  //
  // Baud Rate
  //

  // If PL011 Integral value has been defined then always ignore the BAUD rate
  if (PcdGet32 (PL011UartInteger) != 0) {
      MmioWrite32 (UartBase + UARTIBRD, PcdGet32 (PL011UartInteger));
      MmioWrite32 (UartBase + UARTFBRD, PcdGet32 (PL011UartFractional));
  } else {
    // If BAUD rate is zero then replace it with the system default value
    if (*BaudRate == 0) {
      *BaudRate = PcdGet32 (PcdSerialBaudRate);
      ASSERT (*BaudRate != 0);
    }

    Divisor = (PcdGet32 (PL011UartClkInHz) * 4) / *BaudRate;
    MmioWrite32 (UartBase + UARTIBRD, Divisor >> 6);
    MmioWrite32 (UartBase + UARTFBRD, Divisor & 0x3F);
  }

  // No parity, 1 stop, no fifo, 8 data bits
  MmioWrite32 (UartBase + UARTLCR_H, LineControl);

  // Clear any pending errors
  MmioWrite32 (UartBase + UARTECR, 0);

  // Enable tx, rx, and uart overall
  MmioWrite32 (UartBase + UARTCR, PL011_UARTCR_RXE | PL011_UARTCR_TXE | PL011_UARTCR_UARTEN);

  return RETURN_SUCCESS;
}

/**

  Assert or deassert the control signals on a serial port.
  The following control signals are set according their bit settings :
  . Request to Send
  . Data Terminal Ready

  @param[in]  UartBase  UART registers base address
  @param[in]  Control   The following bits are taken into account :
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
PL011UartSetControl (
    IN UINTN   UartBase,
    IN UINT32  Control
  )
{
  UINT32  Bits;

  if (Control & (mInvalidControlBits)) {
    return RETURN_UNSUPPORTED;
  }

  Bits = MmioRead32 (UartBase + UARTCR);

  if (Control & EFI_SERIAL_REQUEST_TO_SEND) {
    Bits |= PL011_UARTCR_RTS;
  } else {
    Bits &= ~PL011_UARTCR_RTS;
  }

  if (Control & EFI_SERIAL_DATA_TERMINAL_READY) {
    Bits |= PL011_UARTCR_DTR;
  } else {
    Bits &= ~PL011_UARTCR_DTR;
  }

  if (Control & EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE) {
    Bits |= PL011_UARTCR_LBE;
  } else {
    Bits &= ~PL011_UARTCR_LBE;
  }

  if (Control & EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE) {
    Bits |= (PL011_UARTCR_CTSEN | PL011_UARTCR_RTSEN);
  } else {
    Bits &= ~(PL011_UARTCR_CTSEN | PL011_UARTCR_RTSEN);
  }

  MmioWrite32 (UartBase + UARTCR, Bits);

  return RETURN_SUCCESS;
}

/**

  Retrieve the status of the control bits on a serial device.

  @param[in]   UartBase  UART registers base address
  @param[out]  Control   Status of the control bits on a serial device :

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
PL011UartGetControl (
    IN UINTN     UartBase,
    OUT UINT32  *Control
  )
{
  UINT32      FlagRegister;
  UINT32      ControlRegister;


  FlagRegister = MmioRead32 (UartBase + UARTFR);
  ControlRegister = MmioRead32 (UartBase + UARTCR);

  *Control = 0;

  if ((FlagRegister & PL011_UARTFR_CTS) == PL011_UARTFR_CTS) {
    *Control |= EFI_SERIAL_CLEAR_TO_SEND;
  }

  if ((FlagRegister & PL011_UARTFR_DSR) == PL011_UARTFR_DSR) {
    *Control |= EFI_SERIAL_DATA_SET_READY;
  }

  if ((FlagRegister & PL011_UARTFR_RI) == PL011_UARTFR_RI) {
    *Control |= EFI_SERIAL_RING_INDICATE;
  }

  if ((FlagRegister & PL011_UARTFR_DCD) == PL011_UARTFR_DCD) {
    *Control |= EFI_SERIAL_CARRIER_DETECT;
  }

  if ((ControlRegister & PL011_UARTCR_RTS) == PL011_UARTCR_RTS) {
    *Control |= EFI_SERIAL_REQUEST_TO_SEND;
  }

  if ((ControlRegister & PL011_UARTCR_DTR) == PL011_UARTCR_DTR) {
    *Control |= EFI_SERIAL_DATA_TERMINAL_READY;
  }

  if ((FlagRegister & PL011_UARTFR_RXFE) == PL011_UARTFR_RXFE) {
    *Control |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }

  if ((FlagRegister & PL011_UARTFR_TXFE) == PL011_UARTFR_TXFE) {
    *Control |= EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
  }

  if ((ControlRegister & (PL011_UARTCR_CTSEN | PL011_UARTCR_RTSEN))
       == (PL011_UARTCR_CTSEN | PL011_UARTCR_RTSEN)) {
    *Control |= EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
  }

  if ((ControlRegister & PL011_UARTCR_LBE) == PL011_UARTCR_LBE) {
    *Control |= EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE;
  }

  return RETURN_SUCCESS;
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
PL011UartWrite (
  IN  UINTN    UartBase,
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  UINT8* CONST Final = &Buffer[NumberOfBytes];

  while (Buffer < Final) {
    // Wait until UART able to accept another char
    while ((MmioRead32 (UartBase + UARTFR) & UART_TX_FULL_FLAG_MASK));

    MmioWrite8 (UartBase + UARTDR, *Buffer++);
  }

  return NumberOfBytes;
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
PL011UartRead (
  IN  UINTN     UartBase,
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
  )
{
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while ((MmioRead32 (UartBase + UARTFR) & UART_RX_EMPTY_FLAG_MASK) != 0);
    *Buffer = MmioRead8 (UartBase + UARTDR);
  }

  return NumberOfBytes;
}

/**
  Check to see if any data is available to be read from the debug device.

  @retval EFI_SUCCESS       At least one byte of data is available to be read
  @retval EFI_NOT_READY     No data is available to be read
  @retval EFI_DEVICE_ERROR  The serial device is not functioning properly

**/
BOOLEAN
EFIAPI
PL011UartPoll (
  IN  UINTN     UartBase
  )
{
  return ((MmioRead32 (UartBase + UARTFR) & UART_RX_EMPTY_FLAG_MASK) == 0);
}
