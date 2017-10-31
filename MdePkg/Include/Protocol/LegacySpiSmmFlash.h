/** @file
  This file defines the Legacy SPI SMM Flash Protocol.

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

#ifndef __LEGACY_SPI_SMM_FLASH_PROTOCOL_H__
#define __LEGACY_SPI_SMM_FLASH_PROTOCOL_H__

#include <Protocol/LegacySpiFlash.h>

///
/// Global ID for the Legacy SPI SMM Flash Protocol
///
#define EFI_LEGACY_SPI_SMM_FLASH_PROTOCOL_GUID  \
  { 0x5e3848d4, 0x0db5, 0x4fc0,                 \
    { 0x97, 0x29, 0x3f, 0x35, 0x3d, 0x4f, 0x87, 0x9f }}

typedef
struct _EFI_LEGACY_SPI_FLASH_PROTOCOL
EFI_LEGACY_SPI_SMM_FLASH_PROTOCOL;

extern EFI_GUID gEfiLegacySpiSmmFlashProtocolGuid;

#endif // __SPI_SMM_FLASH_PROTOCOL_H__
