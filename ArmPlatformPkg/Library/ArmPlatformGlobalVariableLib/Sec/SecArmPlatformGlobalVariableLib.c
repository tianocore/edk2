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

#include <Uefi.h>
#include <Library/ArmPlatformGlobalVariableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>

VOID
ArmPlatformGetGlobalVariable (
  IN  UINTN     VariableOffset,
  IN  UINTN     VariableSize,
  OUT VOID*     Variable
  )
{
  UINTN  GlobalVariableBase;

  // Ensure the Global Variable Size have been initialized
  ASSERT (VariableOffset < PcdGet32 (PcdSecGlobalVariableSize));

  GlobalVariableBase = PcdGet32 (PcdCPUCoresSecStackBase) + PcdGet32 (PcdCPUCoreSecPrimaryStackSize) - PcdGet32 (PcdSecGlobalVariableSize);
  
  if (VariableSize == 4) {
    *(UINT32*)Variable = ReadUnaligned32 ((CONST UINT32*)(GlobalVariableBase + VariableOffset));
  } else if (VariableSize == 8) {
    *(UINT64*)Variable = ReadUnaligned64 ((CONST UINT64*)(GlobalVariableBase + VariableOffset));
  } else {
    CopyMem (Variable, (VOID*)(GlobalVariableBase + VariableOffset), VariableSize);
  }
}

VOID
ArmPlatformSetGlobalVariable (
  IN  UINTN     VariableOffset,
  IN  UINTN     VariableSize,
  OUT VOID*     Variable
  )
{
  UINTN  GlobalVariableBase;

  // Ensure the Global Variable Size have been initialized
  ASSERT (VariableOffset < PcdGet32 (PcdSecGlobalVariableSize));

  GlobalVariableBase = PcdGet32 (PcdCPUCoresSecStackBase) + PcdGet32 (PcdCPUCoreSecPrimaryStackSize) - PcdGet32 (PcdSecGlobalVariableSize);

  if (VariableSize == 4) {
    WriteUnaligned32 ((UINT32*)(GlobalVariableBase + VariableOffset), *(UINT32*)Variable);
  } else if (VariableSize == 8) {
    WriteUnaligned64 ((UINT64*)(GlobalVariableBase + VariableOffset), *(UINT64*)Variable);
  } else {
    CopyMem ((VOID*)(GlobalVariableBase + VariableOffset), Variable, VariableSize);
  }
}

VOID*
ArmPlatformGetGlobalVariableAddress (
  IN  UINTN     VariableOffset
  )
{
  UINTN  GlobalVariableBase;

  // Ensure the Global Variable Size have been initialized
  ASSERT (VariableOffset < PcdGet32 (PcdSecGlobalVariableSize));

  GlobalVariableBase = PcdGet32 (PcdCPUCoresSecStackBase) + PcdGet32 (PcdCPUCoreSecPrimaryStackSize) - PcdGet32 (PcdSecGlobalVariableSize);

  return (VOID*)(GlobalVariableBase + VariableOffset);
}
