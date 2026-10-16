/** @file
  This file defines the SPI SMM NOR Flash Protocol.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    This Protocol was introduced in UEFI PI Specification 1.6.

**/

#pragma once

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

#define EFI_SPI_SMM_NOR_FLASH_PROTOCOL_EX_GUID  \
  { 0x67435ded, 0xcaa9, 0x4ba5,                 \
    { 0xb9, 0x3e, 0x9c, 0xe4, 0x28, 0xd4, 0x94, 0xa2 } }

typedef EFI_SPI_NOR_FLASH_PROTOCOL_EX EFI_SPI_SMM_NOR_FLASH_PROTOCOL_EX;

extern EFI_GUID  gEfiSpiSmmNorFlashProtocolGuid;
extern EFI_GUID  gEfiSpiSmmNorFlashProtocolExGuid;
