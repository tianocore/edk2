/** @file
*
*  Copyright (c) 2011-2016, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __PL011_UART_H__
#define __PL011_UART_H__

#include <Uefi.h>
#include <Protocol/SerialIo.h>

#define PL011_VARIANT_ZTE 1

// PL011 Registers
#if FixedPcdGet8 (PL011UartRegOffsetVariant) == PL011_VARIANT_ZTE
#define UARTDR                    0x004
#define UARTRSR                   0x010
#define UARTECR                   0x010
#define UARTFR                    0x014
#define UARTIBRD                  0x024
#define UARTFBRD                  0x028
#define UARTLCR_H                 0x030
#define UARTCR                    0x034
#define UARTIFLS                  0x038
#define UARTIMSC                  0x040
#define UARTRIS                   0x044
#define UARTMIS                   0x048
#define UARTICR                   0x04c
#define UARTDMACR                 0x050
#else
#define UARTDR                    0x000
#define UARTRSR                   0x004
#define UARTECR                   0x004
#define UARTFR                    0x018
#define UARTILPR                  0x020
#define UARTIBRD                  0x024
#define UARTFBRD                  0x028
#define UARTLCR_H                 0x02C
#define UARTCR                    0x030
#define UARTIFLS                  0x034
#define UARTIMSC                  0x038
#define UARTRIS                   0x03C
#define UARTMIS                   0x040
#define UARTICR                   0x044
#define UARTDMACR                 0x048
#endif

#define UARTPID0                  0xFE0
#define UARTPID1                  0xFE4
#define UARTPID2                  0xFE8
#define UARTPID3                  0xFEC

// Data status bits
#define UART_DATA_ERROR_MASK      0x0F00

// Status reg bits
#define UART_STATUS_ERROR_MASK    0x0F

// Flag reg bits
#if FixedPcdGet8 (PL011UartRegOffsetVariant) == PL011_VARIANT_ZTE
#define PL011_UARTFR_RI           (1 << 0)  // Ring indicator
#define PL011_UARTFR_TXFE         (1 << 7)  // Transmit FIFO empty
#define PL011_UARTFR_RXFF         (1 << 6)  // Receive  FIFO full
#define PL011_UARTFR_TXFF         (1 << 5)  // Transmit FIFO full
#define PL011_UARTFR_RXFE         (1 << 4)  // Receive  FIFO empty
#define PL011_UARTFR_BUSY         (1 << 8)  // UART busy
#define PL011_UARTFR_DCD          (1 << 2)  // Data carrier detect
#define PL011_UARTFR_DSR          (1 << 3)  // Data set ready
#define PL011_UARTFR_CTS          (1 << 1)  // Clear to send
#else
#define PL011_UARTFR_RI           (1 << 8)  // Ring indicator
#define PL011_UARTFR_TXFE         (1 << 7)  // Transmit FIFO empty
#define PL011_UARTFR_RXFF         (1 << 6)  // Receive  FIFO full
#define PL011_UARTFR_TXFF         (1 << 5)  // Transmit FIFO full
#define PL011_UARTFR_RXFE         (1 << 4)  // Receive  FIFO empty
#define PL011_UARTFR_BUSY         (1 << 3)  // UART busy
#define PL011_UARTFR_DCD          (1 << 2)  // Data carrier detect
#define PL011_UARTFR_DSR          (1 << 1)  // Data set ready
#define PL011_UARTFR_CTS          (1 << 0)  // Clear to send
#endif

// Flag reg bits - alternative names
#define UART_TX_EMPTY_FLAG_MASK   PL011_UARTFR_TXFE
#define UART_RX_FULL_FLAG_MASK    PL011_UARTFR_RXFF
#define UART_TX_FULL_FLAG_MASK    PL011_UARTFR_TXFF
#define UART_RX_EMPTY_FLAG_MASK   PL011_UARTFR_RXFE
#define UART_BUSY_FLAG_MASK       PL011_UARTFR_BUSY

// Control reg bits
#define PL011_UARTCR_CTSEN        (1 << 15) // CTS hardware flow control enable
#define PL011_UARTCR_RTSEN        (1 << 14) // RTS hardware flow control enable
#define PL011_UARTCR_RTS          (1 << 11) // Request to send
#define PL011_UARTCR_DTR          (1 << 10) // Data transmit ready.
#define PL011_UARTCR_RXE          (1 << 9)  // Receive enable
#define PL011_UARTCR_TXE          (1 << 8)  // Transmit enable
#define PL011_UARTCR_LBE          (1 << 7)  // Loopback enable
#define PL011_UARTCR_UARTEN       (1 << 0)  // UART Enable

// Line Control Register Bits
#define PL011_UARTLCR_H_SPS       (1 << 7)  // Stick parity select
#define PL011_UARTLCR_H_WLEN_8    (3 << 5)
#define PL011_UARTLCR_H_WLEN_7    (2 << 5)
#define PL011_UARTLCR_H_WLEN_6    (1 << 5)
#define PL011_UARTLCR_H_WLEN_5    (0 << 5)
#define PL011_UARTLCR_H_FEN       (1 << 4)  // FIFOs Enable
#define PL011_UARTLCR_H_STP2      (1 << 3)  // Two stop bits select
#define PL011_UARTLCR_H_EPS       (1 << 2)  // Even parity select
#define PL011_UARTLCR_H_PEN       (1 << 1)  // Parity Enable
#define PL011_UARTLCR_H_BRK       (1 << 0)  // Send break

#define PL011_UARTPID2_VER(X)     (((X) >> 4) & 0xF)
#define PL011_VER_R1P4            0x2

/**

  Initialise the serial port to the specified settings.
  All unspecified settings will be set to the default values.

  @param[in]  UartBase            The base address of the serial device.
  @param[in]  UartClkInHz         The clock in Hz for the serial device.
                                  Ignored if the PCD PL011UartInteger is not 0
  @param[in out] BaudRate         The baud rate of the serial device. If the
                                  baud rate is not supported, the speed will be
                                  reduced to the nearest supported one and the
                                  variable's value will be updated accordingly.
  @param[in out] ReceiveFifoDepth The number of characters the device will
                                  buffer on input.  Value of 0 will use the
                                  device's default FIFO depth.
  @param[in out]  Parity          If applicable, this is the EFI_PARITY_TYPE
                                  that is computed or checked as each character
                                  is transmitted or received. If the device
                                  does not support parity, the value is the
                                  default parity value.
  @param[in out]  DataBits        The number of data bits in each character.
  @param[in out]  StopBits        If applicable, the EFI_STOP_BITS_TYPE number
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
  );

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
  );

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
                           the hardware loopback is enabled (the ouput feeds the
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
  IN UINTN     UartBase,
  OUT UINT32  *Control
  );

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
  IN  UINTN       UartBase,
  IN  UINT8       *Buffer,
  IN  UINTN       NumberOfBytes
  );

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
  IN  UINTN       UartBase,
  OUT UINT8       *Buffer,
  IN  UINTN       NumberOfBytes
  );

/**
  Check to see if any data is available to be read from the debug device.

  @retval TRUE       At least one byte of data is available to be read
  @retval FALSE      No data is available to be read

**/
BOOLEAN
EFIAPI
PL011UartPoll (
  IN  UINTN       UartBase
  );

#endif
