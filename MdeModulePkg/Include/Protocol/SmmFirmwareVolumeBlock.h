/** @file
  SMM Firmware Volume Block protocol is related to EDK II-specific implementation of
  FVB driver, provides control over block-oriented firmware devices and is intended
  to use in the EFI SMM environment.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_FIRMWARE_VOLUME_BLOCK_H__
#define __SMM_FIRMWARE_VOLUME_BLOCK_H__

#include <Protocol/FirmwareVolumeBlock.h>

#define EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID \
  { \
    0xd326d041, 0xbd31, 0x4c01, { 0xb5, 0xa8, 0x62, 0x8b, 0xe8, 0x7f, 0x6, 0x53 } \
  }

//
// SMM Firmware Volume Block protocol structure is the same as Firmware Volume Block
// protocol. The SMM one is intend to run in SMM environment, which means it can be
// used by SMM drivers after ExitPmAuth.
//
typedef EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL;

extern EFI_GUID gEfiSmmFirmwareVolumeBlockProtocolGuid;

#endif
