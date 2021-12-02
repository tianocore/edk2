/** @file
  Serial I/O Port library functions with no library constructor/destructor

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <Protocol/SerialIo.h>

#include "PL011Uart.h"

#define FRACTION_PART_SIZE_IN_BITS  6
#define FRACTION_PART_MASK          ((1 << FRACTION_PART_SIZE_IN_BITS) - 1)

//
// EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE is the only
// control bit that is not supported.
//
STATIC CONST UINT32  mInvalidControlBits = EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE;

/**

  Initialise the serial port to the specified settings.
  The serial port is re-configured only if the specified settings
  are different from the current settings.
  All unspecified settings will be set to the default values.

  @param  UartBase                The base address of the serial device.
  @param  UartClkInHz             The clock in Hz for the serial device.
                                  Ignored if the PCD PL011UartInteger is not 0
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
PL011UartInitializePort (
  IN     UINTN               UartBase,
  IN     UINT32              UartClkInHz,
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  UINT32  LineControl;
  UINT32  Divisor;
  UINT32  Integer;
  UINT32  Fractional;
  UINT32  HardwareFifoDepth;
  UINT32  UartPid2;

  HardwareFifoDepth = FixedPcdGet16 (PcdUartDefaultReceiveFifoDepth);
  if (HardwareFifoDepth == 0) {
    UartPid2          = MmioRead32 (UartBase + UARTPID2);
    HardwareFifoDepth = (PL011_UARTPID2_VER (UartPid2) > PL011_VER_R1P4) ? 32 : 16;
  }

  // The PL011 supports a buffer of 1, 16 or 32 chars. Therefore we can accept
  // 1 char buffer as the minimum FIFO size. Because everything can be rounded
  // down, there is no maximum FIFO size.
  if ((*ReceiveFifoDepth == 0) || (*ReceiveFifoDepth >= HardwareFifoDepth)) {
    // Enable FIFO
    LineControl       = PL011_UARTLCR_H_FEN;
    *ReceiveFifoDepth = HardwareFifoDepth;
  } else {
    // Disable FIFO
    LineControl = 0;
    // Nothing else to do. 1 byte FIFO is default.
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
      LineControl |= (PL011_UARTLCR_H_PEN \
                      | PL011_UARTLCR_H_SPS \
                      | PL011_UARTLCR_H_EPS);
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
    // Only 1 or 2 stop bits are supported
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

  // If PL011 Integer value has been defined then always ignore the BAUD rate
  if (FixedPcdGet32 (PL011UartInteger) != 0) {
    Integer    = FixedPcdGet32 (PL011UartInteger);
    Fractional = FixedPcdGet32 (PL011UartFractional);
  } else {
    // If BAUD rate is zero then replace it with the system default value
    if (*BaudRate == 0) {
      *BaudRate = FixedPcdGet32 (PcdSerialBaudRate);
      if (*BaudRate == 0) {
        return RETURN_INVALID_PARAMETER;
      }
    }

    if (0 == UartClkInHz) {
      return RETURN_INVALID_PARAMETER;
    }

    Divisor    = (UartClkInHz * 4) / *BaudRate;
    Integer    = Divisor >> FRACTION_PART_SIZE_IN_BITS;
    Fractional = Divisor & FRACTION_PART_MASK;
  }

  //
  // If PL011 is already initialized, check the current settings
  // and re-initialize only if the settings are different.
  //
  if (((MmioRead32 (UartBase + UARTCR) & PL011_UARTCR_UARTEN) != 0) &&
      (MmioRead32 (UartBase + UARTLCR_H) == LineControl) &&
      (MmioRead32 (UartBase + UARTIBRD) == Integer) &&
      (MmioRead32 (UartBase + UARTFBRD) == Fractional))
  {
    // Nothing to do - already initialized with correct attributes
    return RETURN_SUCCESS;
  }

  // Wait for the end of transmission
  while ((MmioRead32 (UartBase + UARTFR) & PL011_UARTFR_TXFE) == 0) {
  }

  // Disable UART: "The UARTLCR_H, UARTIBRD, and UARTFBRD registers must not be changed
  // when the UART is enabled"
  MmioWrite32 (UartBase + UARTCR, 0);

  // Set Baud Rate Registers
  MmioWrite32 (UartBase + UARTIBRD, Integer);
  MmioWrite32 (UartBase + UARTFBRD, Fractional);

  // No parity, 1 stop, no fifo, 8 data bits
  MmioWrite32 (UartBase + UARTLCR_H, LineControl);

  // Clear any pending errors
  MmioWrite32 (UartBase + UARTECR, 0);

  // Enable Tx, Rx, and UART overall
  MmioWrite32 (
    UartBase + UARTCR,
    PL011_UARTCR_RXE | PL011_UARTCR_TXE | PL011_UARTCR_UARTEN
    );

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

  @retval  RETURN_SUCCESS      The new control bits were set on the device.
  @retval  RETURN_UNSUPPORTED  The device does not support this operation.

**/
RETURN_STATUS
EFIAPI
PL011UartSetControl (
  IN UINTN   UartBase,
  IN UINT32  Control
  )
{
  UINT32  Bits;

  if ((Control & mInvalidControlBits) != 0) {
    return RETURN_UNSUPPORTED;
  }

  Bits = MmioRead32 (UartBase + UARTCR);

  if ((Control & EFI_SERIAL_REQUEST_TO_SEND) != 0) {
    Bits |= PL011_UARTCR_RTS;
  } else {
    Bits &= ~PL011_UARTCR_RTS;
  }

  if ((Control & EFI_SERIAL_DATA_TERMINAL_READY) != 0) {
    Bits |= PL011_UARTCR_DTR;
  } else {
    Bits &= ~PL011_UARTCR_DTR;
  }

  if ((Control & EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE) != 0) {
    Bits |= PL011_UARTCR_LBE;
  } else {
    Bits &= ~PL011_UARTCR_LBE;
  }

  if ((Control & EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE) != 0) {
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
                           the hardware loopback is enabled (the output feeds the
                           receive buffer), 0 otherwise.
                         . EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE : equal to one if
                           a loopback is accomplished by software, 0 otherwise.
                         . EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE : equal to
                           one if the hardware flow control based on CTS (Clear
                           To Send) and RTS (Ready To Send) control signals is
                           enabled, 0 otherwise.

  @retval RETURN_SUCCESS  The control bits were read from the serial device.

**/
RETURN_STATUS
EFIAPI
PL011UartGetControl (
  IN UINTN    UartBase,
  OUT UINT32  *Control
  )
{
  UINT32  FlagRegister;
  UINT32  ControlRegister;

  FlagRegister    = MmioRead32 (UartBase + UARTFR);
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
      == (PL011_UARTCR_CTSEN | PL011_UARTCR_RTSEN))
  {
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
  IN  UINTN  UartBase,
  IN UINT8   *Buffer,
  IN UINTN   NumberOfBytes
  )
{
  UINT8 *CONST  Final = &Buffer[NumberOfBytes];

  while (Buffer < Final) {
    // Wait until UART able to accept another char
    while ((MmioRead32 (UartBase + UARTFR) & UART_TX_FULL_FLAG_MASK)) {
    }

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
  IN  UINTN  UartBase,
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  )
{
  UINTN  Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while ((MmioRead32 (UartBase + UARTFR) & UART_RX_EMPTY_FLAG_MASK) != 0) {
    }

    *Buffer = MmioRead8 (UartBase + UARTDR);
  }

  return NumberOfBytes;
}

/**
  Check to see if any data is available to be read from the debug device.

  @retval TRUE       At least one byte of data is available to be read
  @retval FALSE      No data is available to be read

**/
BOOLEAN
EFIAPI
PL011UartPoll (
  IN  UINTN  UartBase
  )
{
  return ((MmioRead32 (UartBase + UARTFR) & UART_RX_EMPTY_FLAG_MASK) == 0);
}
