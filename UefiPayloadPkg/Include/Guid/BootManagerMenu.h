/** @file
  Define the structure for the Boot Manager Menu File.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU_H_
#define UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack (1)

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER   Header;
  GUID                               FileName;
} UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU;

#pragma pack()

#define UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU_REVISION 1

extern GUID gEdkiiBootManagerMenuFileGuid;
#endif
