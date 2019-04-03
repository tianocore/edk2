/** @file
This protocol indicates that the platform SPI interface is ready for use.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _PLATFORM_SPI_READY_H_
#define _PLATFORM_SPI_READY_H_

// {7A5DBC75-5B2B-4e67-BDE1-D48EEE761562}
#define EFI_SMM_SPI_READY_PROTOCOL_GUID  \
  { 0x7a5dbc75, 0x5b2b, 0x4e67, 0xbd, 0xe1, 0xd4, 0x8e, 0xee, 0x76, 0x15, 0x62 }

//
// Extern the GUID for protocol users.
//
extern EFI_GUID                     gEfiSmmSpiReadyProtocolGuid;

#endif
