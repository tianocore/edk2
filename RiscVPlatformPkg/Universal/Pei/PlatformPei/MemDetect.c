/** @file
  Memory Detection for Virtual Machines.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MemDetect.c

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>

#include "Platform.h"


/**
  Publish PEI core memory.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
PublishPeiMemory (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        MemoryBase;
  UINT64                      MemorySize;

  //
  // TODO: This value should come from platform
  // configuration or the memory sizing code.
  //
  MemoryBase = 0x80000000UL + 0x1000000UL;
  MemorySize = 0x40000000UL - 0x1000000UL; //1GB - 16MB

  DEBUG((DEBUG_INFO, "%a: MemoryBase:0x%x MemorySize:%x\n", __FUNCTION__, MemoryBase, MemorySize));

  //
  // Publish this memory to the PEI Core
  //
  Status = PublishSystemMemory(MemoryBase, MemorySize);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Publish system RAM and reserve memory regions.

**/
VOID
InitializeRamRegions (
  VOID
  )
{
  //
  // TODO: This value should come from platform
  // configuration or the memory sizing code.
  //
  AddMemoryRangeHob(0x81000000UL, 0x81000000UL + 0x3F000000UL);
}
