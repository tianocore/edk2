/** @file
 Define the structure for the Boot Manager Menu File.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOOT_MANAGER_MENU_FILE_H_
#define _BOOT_MANAGER_MENU_FILE_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack (1)

typedef struct {
  PLD_GENERIC_HEADER   Header;
  GUID                 PassedGUID;
} BOOT_MANAGER_MENU_FILE;

#pragma pack()

#define BOOT_MANAGER_MENU_FILE_REVISION 1

extern GUID gUniversalPayloadBootManagerMenuFileGuid;
#endif
