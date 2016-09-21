/** @file
  Recovery module header file.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _RECOVERY_MODULE_LOAD_PEI_H_
#define _RECOVERY_MODULE_LOAD_PEI_H_

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
  UINTN                           NumOfRecovery;
} CONFIG_HEADER;

typedef struct {
  UINTN                           Index;
  EFI_GUID                        FileGuid;
  UINTN                           Length;
  UINTN                           ImageOffset;
} RECOVERY_CONFIG_DATA;

#endif

