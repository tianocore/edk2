/** @file

  Copyright (c) 2011-2016, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PL011_UART_H__
#define __PL011_UART_H__

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

#endif
