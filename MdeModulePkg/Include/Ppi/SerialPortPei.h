/** @file
  PPI that is installed after the initialization of a serial stream device
  is complete.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PEI_SERIAL_PORT_PPI_H__
#define __PEI_SERIAL_PORT_PPI_H__

#define PEI_SERIAL_PORT_PPI \
  { \
    0x490e9d85, 0x8aef, 0x4193, { 0x8e, 0x56, 0xf7, 0x34, 0xa9, 0xff, 0xac, 0x8b } \
  }

extern EFI_GUID  gPeiSerialPortPpiGuid;

#endif
