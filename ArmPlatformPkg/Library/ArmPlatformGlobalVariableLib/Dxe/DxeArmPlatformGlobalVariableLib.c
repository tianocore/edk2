/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
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

#include <PiDxe.h>
#include <Library/ArmPlatformGlobalVariableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

#include <Guid/ArmGlobalVariableHob.h>

UINTN  mGlobalVariableBase = 0;

RETURN_STATUS
EFIAPI
ArmPlatformGlobalVariableConstructor (
  VOID
  )
{
  ARM_HOB_GLOBAL_VARIABLE  *Hob;

  Hob = GetFirstGuidHob (&gArmGlobalVariableGuid);
  ASSERT (Hob != NULL);

  mGlobalVariableBase = Hob->GlobalVariableBase;

  return EFI_SUCCESS;
}

VOID
ArmPlatformGetGlobalVariable (
  IN  UINTN     VariableOffset,
  IN  UINTN     VariableSize,
  OUT VOID*     Variable
  )
{
  if (mGlobalVariableBase == 0) {
    ArmPlatformGlobalVariableConstructor ();
  }

  CopyMem (Variable, (VOID*)(mGlobalVariableBase + VariableOffset), VariableSize);
}

VOID
ArmPlatformSetGlobalVariable (
  IN  UINTN     VariableOffset,
  IN  UINTN     VariableSize,
  OUT VOID*     Variable
  )
{
  if (mGlobalVariableBase == 0) {
    ArmPlatformGlobalVariableConstructor ();
  }

  CopyMem ((VOID*)(mGlobalVariableBase + VariableOffset), Variable, VariableSize);
}

VOID*
ArmPlatformGetGlobalVariableAddress (
  IN  UINTN     VariableOffset
  )
{
  return (VOID*)(mGlobalVariableBase + VariableOffset);
}
