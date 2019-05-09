/** @file
  Definitions for Authenticated Code Module (ACM) firmware.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FIRMWARE_INTERFACE_TABLE_GUID_H_
#define _FIRMWARE_INTERFACE_TABLE_GUID_H_

#include <Base.h>
#include <IndustryStandard/FirmwareInterfaceTable.h>
#include <Pi/PiHob.h>

#define FIT_STRUCTURE_HOB_GUID  { 0x7336a6b9, 0x1d6e, 0x4059, { 0x80, 0x31, 0x4d, 0x87, 0x4f, 0x8c, 0xdb, 0x82 } }
extern EFI_GUID gFitStructureHobGuid;

#pragma pack(push, 1)
typedef struct {
  EFI_HOB_GUID_TYPE               Header;
  UINT8                           Revision;
  UINT8                           Reserved[3];
  FIRMWARE_INTERFACE_TABLE_ENTRY  *Fit;
} FIT_HOB_STRUCTURE;
#pragma pack(pop)

#endif