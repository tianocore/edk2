/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

// PL011 Registers
#define UARTDR          0x000
#define UARTRSR         0x004
#define UARTECR         0x004
#define UARTFR          0x018
#define UARTILPR        0x020
#define UARTIBRD        0x024
#define UARTFBRD        0x028
#define UARTLCR_H       0x02C
#define UARTCR          0x030
#define UARTIFLS        0x034
#define UARTIMSC        0x038
#define UARTRIS         0x03C
#define UARTMIS         0x040
#define UARTICR         0x044
#define UARTDMACR       0x048

#define UART_115200_IDIV      13      // Integer Part
#define UART_115200_FDIV      1       // Fractional Part
#define UART_38400_IDIV       39
#define UART_38400_FDIV       5
#define UART_19200_IDIV       12
#define UART_19200_FDIV       37

// Data status bits
#define UART_DATA_ERROR_MASK      0x0F00

// Status reg bits
#define UART_STATUS_ERROR_MASK    0x0F

// Flag reg bits
#define UART_TX_EMPTY_FLAG_MASK   0x80
#define UART_RX_FULL_FLAG_MASK    0x40
#define UART_TX_FULL_FLAG_MASK    0x20
#define UART_RX_EMPTY_FLAG_MASK   0x10
#define UART_BUSY_FLAG_MASK       0x08

// Control reg bits
#define PL011_UARTCR_CTSEN        (1 << 15) // CTS hardware flow control enable
#define PL011_UARTCR_RTSEN        (1 << 14) // RTS hardware flow control enable
#define PL011_UARTCR_RTS          (1 << 11) // Request to send
#define PL011_UARTCR_DTR          (1 << 10) // Data transmit ready.
#define PL011_UARTCR_RXE          (1 << 9)  // Receive enable
#define PL011_UARTCR_TXE          (1 << 8)  // Transmit enable
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

/*

  Programmed hardware of Serial port.

  @return    Always return EFI_UNSUPPORTED.

**/
RETURN_STATUS
EFIAPI
PL011UartInitialize (
  IN  UINTN       UartBase,
  IN  UINTN       BaudRate,
  IN  UINTN       LineControl
  );

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes writed to serial device.

**/
UINTN
EFIAPI
PL011UartWrite (
  IN  UINTN       UartBase,
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  );

/**
  Read data from serial device and save the datas in buffer.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Aactual number of bytes read from serial device.

**/
UINTN
EFIAPI
PL011UartRead (
  IN  UINTN       UartBase,
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
  );

/**
  Check to see if any data is avaiable to be read from the debug device.

  @retval EFI_SUCCESS       At least one byte of data is avaiable to be read
  @retval EFI_NOT_READY     No data is avaiable to be read
  @retval EFI_DEVICE_ERROR  The serial device is not functioning properly

**/
BOOLEAN
EFIAPI
PL011UartPoll (
  IN  UINTN     UartBase
  );

#endif
