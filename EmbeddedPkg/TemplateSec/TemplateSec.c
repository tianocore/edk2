/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/PrePiLib.h>
#include <Library/PcdLib.h>

#include <Ppi/GuidedSectionExtraction.h>

VOID
_ModuleEntryPoint (
  VOID
  )
{
}

VOID
CEntryPoint (
  VOID    *MemoryBase,
  UINTN   MemorySize,
  VOID    *StackBase,
  UINTN   StackSize
  )
{
  EFI_PHYSICAL_ADDRESS  MemoryBegin;
  UINT64                MemoryLength;
  VOID                  *HobBase;

  //
  // Boot strap the C environment so the other library services will work properly.
  //
  MemoryBegin  = (EFI_PHYSICAL_ADDRESS)(UINTN)MemoryBase;
  MemoryLength = (UINT64)MemorySize;
  HobBase      = (VOID *)(UINTN)(FixedPcdGet32(PcdEmbeddedFdBaseAddress) + FixedPcdGet32(PcdEmbeddedFdSize));
  CreateHobList (MemoryBase, MemorySize, HobBase, StackBase);

  MemoryBegin  = (EFI_PHYSICAL_ADDRESS)(UINTN)StackBase;
  MemoryLength = (UINT64)StackSize;
  UpdateStackHob (MemoryBegin, MemoryLength);

  DEBUG ((DEBUG_ERROR, "CEntryPoint (%x,%x,%x,%x)\n", MemoryBase, MemorySize, StackBase, StackSize));

  //
  // Add your C code stuff here....
  //


  //
  // Load the DXE Core and transfer control to it
  //

  // Give the DXE Core access to our DEBUG and ASSERT infrastructure so this will work prior
  // to the DXE version being loaded. Thus we close the debugging gap between phases.
  AddDxeCoreReportStatusCodeCallback ();

  //BuildFvHobs (PcdBfvBase, PcdBfvSize, NULL);

  LoadDxeCoreFromFv (NULL, 0);

  // DXE Core should always load and never return
  ASSERT (FALSE);
}

