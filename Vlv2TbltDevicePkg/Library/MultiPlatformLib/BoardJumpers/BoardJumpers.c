/** @file
  Jumper setting for multiplatform.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

**/

#include <BoardJumpers.h>

BOOLEAN
IsRecoveryJumper (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
)
{
  return FALSE;
}

BOOLEAN
IsManufacturingMode(
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
)
{
  return FALSE;
}
