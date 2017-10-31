/** @file
  This file defines the Legacy SPI SMM Controler Protocol.

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

#ifndef __LEGACY_SPI_SMM_CONTROLLER_PROTOCOL_H__
#define __LEGACY_SPI_SMM_CONTROLLER_PROTOCOL_H__

#include <Protocol/LegacySpiController.h>

///
/// Global ID for the Legacy SPI SMM Controller Protocol
///
#define EFI_LEGACY_SPI_SMM_CONTROLLER_PROTOCOL_GUID  \
  { 0x62331b78, 0xd8d0, 0x4c8c,                 \
    { 0x8c, 0xcb, 0xd2, 0x7d, 0xfe, 0x32, 0xdb, 0x9b }}

typedef
struct _EFI_LEGACY_SPI_CONTROLLER_PROTOCOL
EFI_LEGACY_SPI_SMM_CONTROLLER_PROTOCOL;

extern EFI_GUID gEfiLegacySpiSmmControllerProtocolGuid;

#endif // __LEGACY_SPI_SMM_CONTROLLER_PROTOCOL_H__
