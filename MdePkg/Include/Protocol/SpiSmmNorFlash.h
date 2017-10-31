/** @file
  This file defines the SPI SMM NOR Flash Protocol.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD
  License which accompanies this distribution. The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
    This Protocol was introduced in UEFI PI Specification 1.6.

**/

#ifndef __SPI_SMM_NOR_FLASH_PROTOCOL_H__
#define __SPI_SMM_NOR_FLASH_PROTOCOL_H__

#include <Protocol/SpiNorFlash.h>

///
/// Global ID for the SPI SMM NOR Flash Protocol
///
#define EFI_SPI_SMM_NOR_FLASH_PROTOCOL_GUID  \
  { 0xaab18f19, 0xfe14, 0x4666,              \
    { 0x86, 0x04, 0x87, 0xff, 0x6d, 0x66, 0x2c, 0x9a } }

typedef
struct _EFI_SPI_NOR_FLASH_PROTOCOL
EFI_SPI_SMM_NOR_FLASH_PROTOCOL;

extern EFI_GUID gEfiSpiSmmNorFlashProtocolGuid;

#endif // __SPI_SMM_NOR_FLASH_PROTOCOL_H__
