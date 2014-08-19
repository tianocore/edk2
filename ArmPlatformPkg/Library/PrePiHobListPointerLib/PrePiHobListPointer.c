/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <PiPei.h>
#include <Library/ArmPlatformGlobalVariableLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

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
  VOID* HobList;

  ArmPlatformGetGlobalVariable (PcdGet32 (PcdHobListPtrGlobalOffset), sizeof(VOID*), &HobList);

  return HobList;
}



/**
  Updates the pointer to the HOB list.

  @param  HobList       Hob list pointer to store

**/
EFI_STATUS
EFIAPI
PrePeiSetHobList (
  IN  VOID      *HobList
  )
{
  ArmPlatformSetGlobalVariable (PcdGet32 (PcdHobListPtrGlobalOffset), sizeof(VOID*), &HobList);

  return EFI_SUCCESS;
}

