/** @file

  Copyright (c) 2011, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/ArmLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/DebugLib.h>

/**
  Returns the pointer to the HOB list.

  This function returns the pointer to first HOB in the list.

  @return The pointer to the HOB list.

**/
VOID *
EFIAPI
PrePeiGetHobList (
  VOID
  )
{
  return (VOID *)ArmReadTpidrurw ();
}

/**
  Updates the pointer to the HOB list.

  @param  HobList       Hob list pointer to store

**/
EFI_STATUS
EFIAPI
PrePeiSetHobList (
  IN  VOID  *HobList
  )
{
  ArmWriteTpidrurw ((UINTN)HobList);

  return EFI_SUCCESS;
}
