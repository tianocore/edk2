/** @file
  Arm IORT parser common functionality.

  Copyright (c) 2025, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include "Arm/Iort/ArmIortParser.h"

STATIC UINT16  IortIdentifier = 0;

UINT16
EFIAPI
GetNextIortIdentifier (
  )
{
  return IortIdentifier++;
}
