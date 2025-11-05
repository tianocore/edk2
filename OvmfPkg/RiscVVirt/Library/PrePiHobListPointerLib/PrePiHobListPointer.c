/** @file
*
*  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiPei.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>

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
  return (VOID *)RiscVGetSupervisorScratch ();
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
  RiscVSetSupervisorScratch ((UINTN)HobList);
  return EFI_SUCCESS;
}
