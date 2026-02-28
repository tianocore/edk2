/** @file
  Recovery module header file.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/EdkiiSystemCapsuleLib.h>

//
// Update data
//

typedef struct {
  UINTN    NumOfRecovery;
} CONFIG_HEADER;

typedef struct {
  UINTN       Index;
  EFI_GUID    FileGuid;
  UINTN       Length;
  UINTN       ImageOffset;
} RECOVERY_CONFIG_DATA;
