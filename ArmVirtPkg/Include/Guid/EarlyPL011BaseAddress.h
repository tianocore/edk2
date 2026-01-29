/** @file
  GUID for the HOB that caches the base address(es) of the PL011 serial port(s),
  for when PCD access is not available.

  Copyright (C) 2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EARLY_PL011_BASE_ADDRESS_H__
#define __EARLY_PL011_BASE_ADDRESS_H__

#define EARLY_PL011_BASE_ADDRESS_GUID  {\
          0xB199DEA9, 0xFD5C, 0x4A84, \
          { 0x80, 0x82, 0x2F, 0x41, 0x70, 0x78, 0x03, 0x05 } \
        }

extern EFI_GUID  gEarlyPL011BaseAddressGuid;

typedef struct {
  //
  // for SerialPortLib and console IO
  //
  UINT64    ConsoleAddress;
  //
  // for DebugLib; may equal ConsoleAddress if there's only one PL011 UART
  //
  UINT64    DebugAddress;
} EARLY_PL011_BASE_ADDRESS;

#endif
