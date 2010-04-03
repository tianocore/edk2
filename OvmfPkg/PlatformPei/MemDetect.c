/**@file
  Memory Detection for Virtual Machines.

  Copyright (c) 2006 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>

#include "Platform.h"
#include "Cmos.h"

STATIC
UINTN
GetSystemMemorySize (
  )
{
  UINT8 Cmos0x34;
  UINT8 Cmos0x35;

  //
  // CMOS 0x34/0x35 specifies the system memory above 16 MB.
  // * CMOS(0x35) is the high byte
  // * CMOS(0x34) is the low byte
  // * The size is specified in 64kb chunks
  // * Since this is memory above 16MB, the 16MB must be added
  //   into the calculation to get the total memory size.
  //

  Cmos0x34 = (UINT8) CmosRead8 (0x34);
  Cmos0x35 = (UINT8) CmosRead8 (0x35);

  return ((((Cmos0x35 << 8) + Cmos0x34) << 16) + SIZE_16MB);
}


/**
  Peform Memory Detection

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
MemDetect (
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        MemoryBase;
  UINT64                      MemorySize;
  UINT64                      TotalMemorySize;

  DEBUG ((EFI_D_ERROR, "MemDetect called\n"));

  //
  // Determine total memory size available
  //
  TotalMemorySize = (UINT64)GetSystemMemorySize ();

  //
  // Determine the range of memory to use during PEI
  //
  MemoryBase = PcdGet32 (PcdOvmfMemFvBase) + PcdGet32 (PcdOvmfMemFvSize);
  MemorySize = TotalMemorySize - MemoryBase;
  if (MemorySize > SIZE_16MB) {
    MemoryBase = TotalMemorySize - SIZE_16MB;
    MemorySize = SIZE_16MB;
  }

  //
  // Publish this memory to the PEI Core
  //
  Status = PublishSystemMemory(MemoryBase, MemorySize);
  ASSERT_EFI_ERROR (Status);

  //
  // Create memory HOBs
  //
  AddMemoryBaseSizeHob (MemoryBase, MemorySize);
  AddMemoryRangeHob (BASE_1MB, MemoryBase);
  AddMemoryRangeHob (0, BASE_512KB + BASE_128KB);

  return EFI_SUCCESS;
}

