/** @file
  UART Serial Port library functions

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/IoLib.h>
#include <Library/SerialPortLib.h>

// ---------------------------------------------
// UART Register Offsets
// ---------------------------------------------
#define BAUD_LOW_OFFSET    0x00
#define BAUD_HIGH_OFFSET   0x01
#define IER_OFFSET         0x01
#define LCR_SHADOW_OFFSET  0x01
#define FCR_SHADOW_OFFSET  0x02
#define IR_CONTROL_OFFSET  0x02
#define FCR_OFFSET         0x02
#define EIR_OFFSET         0x02
#define BSR_OFFSET         0x03
#define LCR_OFFSET         0x03
#define MCR_OFFSET         0x04
#define LSR_OFFSET         0x05
#define MSR_OFFSET         0x06

// ---------------------------------------------
// UART Register Bit Defines
// ---------------------------------------------
#define LSR_TXRDY  0x20
#define LSR_RXDA   0x01
#define DLAB       0x01
#define MCR_DTRC   0x01
#define MCR_RTS    0x02
#define MSR_CTS    0x10
#define MSR_DSR    0x20
#define MSR_RI     0x40
#define MSR_DCD    0x80

// ---------------------------------------------
// UART Settings
// ---------------------------------------------
UINT16  gUartBase = 0x3F8;
UINTN   gBps      = 115200;
UINT8   gData     = 8;
UINT8   gStop     = 1;
UINT8   gParity   = 0;
UINT8   gBreakSet = 0;

/**
  Initialize the serial device hardware.

  If no initialization is required, then return RETURN_SUCCESS.
  If the serial device was successfully initialized, then return RETURN_SUCCESS.
  If the serial device could not be initialized, then return RETURN_DEVICE_ERROR.

  @retval RETURN_SUCCESS        The serial device was initialized.
  @retval RETURN_DEVICE_ERROR   The serial device could not be initialized.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UINTN  Divisor;
  UINT8  OutputData;
  UINT8  Data;

  //
  // Map 5..8 to 0..3
  //
  Data = (UINT8)(gData - (UINT8)5);

  //
  // Calculate divisor for baud generator
  //
  Divisor = 115200 / gBps;

  //
  // Set communications format
  //
  OutputData = (UINT8)((DLAB << 7) | (gBreakSet << 6) | (gParity << 3) | (gStop << 2) | Data);
  IoWrite8 (gUartBase + LCR_OFFSET, OutputData);

  //
  // Configure baud rate
  //
  IoWrite8 (gUartBase + BAUD_HIGH_OFFSET, (UINT8)(Divisor >> 8));
  IoWrite8 (gUartBase + BAUD_LOW_OFFSET, (UINT8)(Divisor & 0xff));

  //
  // Switch back to bank 0
  //
  OutputData = (UINT8)((gBreakSet << 6) | (gParity << 3) | (gStop << 2) | Data);
  IoWrite8 (gUartBase + LCR_OFFSET, OutputData);

  return RETURN_SUCCESS;
}

/**
  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the serial device.
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.

  If Buffer is NULL, then ASSERT().

  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.
                           If this value is less than NumberOfBytes, then the write operation failed.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  UINTN  Result;
  UINT8  Data;

  if (Buffer == NULL) {
    return 0;
  }

  Result = NumberOfBytes;

  while ((NumberOfBytes--) != 0) {
    //
    // Wait for the serial port to be ready.
    //
    do {
      Data = IoRead8 ((UINT16)gUartBase + LSR_OFFSET);
    } while ((Data & LSR_TXRDY) == 0);

    IoWrite8 ((UINT16)gUartBase, *Buffer++);
  }

  return Result;
}

/**
  Reads data from a serial device into a buffer.

  @param  Buffer           Pointer to the data buffer to store the data read from the serial device.
  @param  NumberOfBytes    Number of bytes to read from the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes read from the serial device.
                           If this value is less than NumberOfBytes, then the read operation failed.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  )
{
  UINTN  Result;
  UINT8  Data;

  if (NULL == Buffer) {
    return 0;
  }

  Result = NumberOfBytes;

  while ((NumberOfBytes--) != 0) {
    //
    // Wait for the serial port to be ready.
    //
    do {
      Data = IoRead8 ((UINT16)gUartBase + LSR_OFFSET);
    } while ((Data & LSR_RXDA) == 0);

    *Buffer++ = IoRead8 ((UINT16)gUartBase);
  }

  return Result;
}

/**
  Polls a serial device to see if there is any data waiting to be read.

  Polls a serial device to see if there is any data waiting to be read.
  If there is data waiting to be read from the serial device, then TRUE is returned.
  If there is no data waiting to be read from the serial device, then FALSE is returned.

  @retval TRUE             Data is waiting to be read from the serial device.
  @retval FALSE            There is no data waiting to be read from the serial device.

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  UINT8  Data;

  //
  // Read the serial port status.
  //
  Data = IoRead8 ((UINT16)gUartBase + LSR_OFFSET);

  return (BOOLEAN)((Data & LSR_RXDA) != 0);
}

/**
  Sets the control bits on a serial device.

  @param Control                Sets the bits of Control that are settable.

  @retval RETURN_SUCCESS        The new control bits were set on the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32  Control
  )
{
  UINT8  Mcr;

  //
  // First determine the parameter is invalid.
  //
  if ((Control & (~(EFI_SERIAL_REQUEST_TO_SEND | EFI_SERIAL_DATA_TERMINAL_READY))) != 0) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Read the Modem Control Register.
  //
  Mcr  = IoRead8 ((UINT16)gUartBase + MCR_OFFSET);
  Mcr &= (~(MCR_DTRC | MCR_RTS));

  if ((Control & EFI_SERIAL_DATA_TERMINAL_READY) == EFI_SERIAL_DATA_TERMINAL_READY) {
    Mcr |= MCR_DTRC;
  }

  if ((Control & EFI_SERIAL_REQUEST_TO_SEND) == EFI_SERIAL_REQUEST_TO_SEND) {
    Mcr |= MCR_RTS;
  }

  //
  // Write the Modem Control Register.
  //
  IoWrite8 ((UINT16)gUartBase + MCR_OFFSET, Mcr);

  return RETURN_SUCCESS;
}

/**
  Retrieve the status of the control bits on a serial device.

  @param Control                A pointer to return the current control signals from the serial device.

  @retval RETURN_SUCCESS        The control bits were read from the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  UINT8  Msr;
  UINT8  Mcr;
  UINT8  Lsr;

  *Control = 0;

  //
  // Read the Modem Status Register.
  //
  Msr = IoRead8 ((UINT16)gUartBase + MSR_OFFSET);

  if ((Msr & MSR_CTS) == MSR_CTS) {
    *Control |= EFI_SERIAL_CLEAR_TO_SEND;
  }

  if ((Msr & MSR_DSR) == MSR_DSR) {
    *Control |= EFI_SERIAL_DATA_SET_READY;
  }

  if ((Msr & MSR_RI) == MSR_RI) {
    *Control |= EFI_SERIAL_RING_INDICATE;
  }

  if ((Msr & MSR_DCD) == MSR_DCD) {
    *Control |= EFI_SERIAL_CARRIER_DETECT;
  }

  //
  // Read the Modem Control Register.
  //
  Mcr = IoRead8 ((UINT16)gUartBase + MCR_OFFSET);

  if ((Mcr & MCR_DTRC) == MCR_DTRC) {
    *Control |= EFI_SERIAL_DATA_TERMINAL_READY;
  }

  if ((Mcr & MCR_RTS) == MCR_RTS) {
    *Control |= EFI_SERIAL_REQUEST_TO_SEND;
  }

  //
  // Read the Line Status Register.
  //
  Lsr = IoRead8 ((UINT16)gUartBase + LSR_OFFSET);

  if ((Lsr & LSR_TXRDY) == LSR_TXRDY) {
    *Control |= EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
  }

  if ((Lsr & LSR_RXDA) == 0) {
    *Control |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }

  return RETURN_SUCCESS;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data bits, and stop bits on a serial device.

  @param BaudRate           The requested baud rate. A BaudRate value of 0 will use the
                            device's default interface speed.
                            On output, the value actually set.
  @param ReceiveFifoDepth   The requested depth of the FIFO on the receive side of the
                            serial interface. A ReceiveFifoDepth value of 0 will use
                            the device's default FIFO depth.
                            On output, the value actually set.
  @param Timeout            The requested time out for a single character in microseconds.
                            This timeout applies to both the transmit and receive side of the
                            interface. A Timeout value of 0 will use the device's default time
                            out value.
                            On output, the value actually set.
  @param Parity             The type of parity to use on this serial device. A Parity value of
                            DefaultParity will use the device's default parity value.
                            On output, the value actually set.
  @param DataBits           The number of data bits to use on the serial device. A DataBits
                            value of 0 will use the device's default data bit setting.
                            On output, the value actually set.
  @param StopBits           The number of stop bits to use on this serial device. A StopBits
                            value of DefaultStopBits will use the device's default number of
                            stop bits.
                            On output, the value actually set.

  @retval RETURN_SUCCESS            The new attributes were set on the serial device.
  @retval RETURN_UNSUPPORTED        The serial device does not support this operation.
  @retval RETURN_INVALID_PARAMETER  One or more of the attributes has an unsupported value.
  @retval RETURN_DEVICE_ERROR       The serial device is not functioning correctly.

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
  UINTN  Divisor;
  UINT8  OutputData;
  UINT8  LcrData;
  UINT8  LcrParity;
  UINT8  LcrStop;

  //
  // Check for default settings and fill in actual values.
  //
  if (*BaudRate == 0) {
    *BaudRate = gBps;
  }

  if (*DataBits == 0) {
    *DataBits = gData;
  }

  if (*Parity == DefaultParity) {
    *Parity = NoParity;
  }

  if (*StopBits == DefaultStopBits) {
    *StopBits = OneStopBit;
  }

  if ((*DataBits < 5) || (*DataBits > 8)) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Map 5..8 to 0..3
  //
  LcrData = (UINT8)(*DataBits - (UINT8)5);

  switch (*Parity) {
    case NoParity:
      LcrParity = 0;
      break;

    case EvenParity:
      LcrParity = 3;
      break;

    case OddParity:
      LcrParity = 1;
      break;

    case SpaceParity:
      LcrParity = 7;
      break;

    case MarkParity:
      LcrParity = 5;
      break;

    default:
      return RETURN_INVALID_PARAMETER;
  }

  switch (*StopBits) {
    case OneStopBit:
      LcrStop = 0;
      break;

    case OneFiveStopBits:
    case TwoStopBits:
      LcrStop = 1;
      break;

    default:
      return RETURN_INVALID_PARAMETER;
  }

  //
  // Calculate divisor for baud generator
  //
  Divisor = 115200 / (UINTN)*BaudRate;

  //
  // Set communications format
  //
  OutputData = (UINT8)((DLAB << 7) | (gBreakSet << 6) | (LcrParity << 3) | (LcrStop << 2) | LcrData);
  IoWrite8 (gUartBase + LCR_OFFSET, OutputData);

  //
  // Configure baud rate
  //
  IoWrite8 (gUartBase + BAUD_HIGH_OFFSET, (UINT8)(Divisor >> 8));
  IoWrite8 (gUartBase + BAUD_LOW_OFFSET, (UINT8)(Divisor & 0xff));

  //
  // Switch back to bank 0
  //
  OutputData = (UINT8)((gBreakSet << 6) | (LcrParity << 3) | (LcrStop << 2) | LcrData);
  IoWrite8 (gUartBase + LCR_OFFSET, OutputData);

  return RETURN_SUCCESS;
}
