/** @file
  GUID for the HOB that caches the base address of the 16550 serial port, for
  when PCD access is not available.

  Copyright (c) 2020, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EARLY_16550_UART_BASE_ADDRESS_H__
#define EARLY_16550_UART_BASE_ADDRESS_H__

#define EARLY_16550_UART_BASE_ADDRESS_GUID  {      \
  0xea67ca3e, 0x1f54, 0x436b, {                    \
    0x97, 0x88, 0xd4, 0xeb, 0x29, 0xc3, 0x42, 0x67 \
    }                                              \
  }

extern EFI_GUID  gEarly16550UartBaseAddressGuid;

#endif // EARLY_16550_UART_BASE_ADDRESS_H__
