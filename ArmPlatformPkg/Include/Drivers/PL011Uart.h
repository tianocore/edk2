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

#define SERIAL_PORT_MAX_TIMEOUT            100000000    // 100 seconds

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

// data status bits
#define UART_DATA_ERROR_MASK      0x0F00

// status reg bits
#define UART_STATUS_ERROR_MASK    0x0F

// flag reg bits
#define UART_TX_EMPTY_FLAG_MASK   0x80
#define UART_RX_FULL_FLAG_MASK    0x40
#define UART_TX_FULL_FLAG_MASK    0x20
#define UART_RX_EMPTY_FLAG_MASK   0x10
#define UART_BUSY_FLAG_MASK       0x08

// control reg bits
#define UART_CTSEN_CONTROL_MASK   0x8000
#define UART_RTSEN_CONTROL_MASK   0x4000
#define UART_RTS_CONTROL_MASK     0x0800
#define UART_DTR_CONTROL_MASK     0x0400

#endif
