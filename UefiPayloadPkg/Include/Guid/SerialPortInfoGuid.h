/** @file
  This file defines the hob structure for serial port.

  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SERIAL_PORT_INFO_GUID_H_
#define SERIAL_PORT_INFO_GUID_H_

///
/// Serial Port Information GUID
///
extern EFI_GUID  gUefiSerialPortInfoGuid;

#define PLD_SERIAL_TYPE_IO_MAPPED      1
#define PLD_SERIAL_TYPE_MEMORY_MAPPED  2

typedef struct {
  UINT8     Revision;
  UINT8     Reserved0[3];
  UINT32    Type;
  UINT32    BaseAddr;
  UINT32    Baud;
  UINT32    RegWidth;
  UINT32    InputHertz;
  UINT32    UartPciAddr;
} SERIAL_PORT_INFO;

#endif
