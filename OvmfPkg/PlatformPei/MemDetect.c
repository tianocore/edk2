/**@file
  Memory Detection for Virtual Machines.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
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
#include <Library/MtrrLib.h>

#include "Platform.h"
#include "Cmos.h"

STATIC
UINTN
GetSystemMemorySizeBelow4gb (
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

  return (((UINTN)((Cmos0x35 << 8) + Cmos0x34) << 16) + SIZE_16MB);
}


STATIC
UINT64
GetSystemMemorySizeAbove4gb (
  )
{
  UINT32 Size;
  UINTN  CmosIndex;

  //
  // CMOS 0x5b-0x5d specifies the system memory above 4GB MB.
  // * CMOS(0x5d) is the most significant size byte
  // * CMOS(0x5c) is the middle size byte
  // * CMOS(0x5b) is the least significant size byte
  // * The size is specified in 64kb chunks
  //

  Size = 0;
  for (CmosIndex = 0x5d; CmosIndex >= 0x5b; CmosIndex--) {
    Size = (UINT32) (Size << 8) + (UINT32) CmosRead8 (CmosIndex);
  }

  return LShiftU64 (Size, 16);
}


/**
  Peform Memory Detection

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_PHYSICAL_ADDRESS
MemDetect (
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        MemoryBase;
  UINT64                      MemorySize;
  UINT64                      LowerMemorySize;
  UINT64                      UpperMemorySize;

  DEBUG ((EFI_D_ERROR, "MemDetect called\n"));

  //
  // Determine total memory size available
  //
  LowerMemorySize = GetSystemMemorySizeBelow4gb ();
  UpperMemorySize = GetSystemMemorySizeAbove4gb ();

  //
  // Determine the range of memory to use during PEI
  //
  MemoryBase = PcdGet32 (PcdOvmfMemFvBase) + PcdGet32 (PcdOvmfMemFvSize);
  MemorySize = LowerMemorySize - MemoryBase;
  if (MemorySize > SIZE_64MB) {
    MemoryBase = LowerMemorySize - SIZE_64MB;
    MemorySize = SIZE_64MB;
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

  MtrrSetMemoryAttribute (BASE_1MB, MemoryBase + MemorySize - BASE_1MB, CacheWriteBack);

  MtrrSetMemoryAttribute (0, BASE_512KB + BASE_128KB, CacheWriteBack);

  if (UpperMemorySize != 0) {
    AddUntestedMemoryBaseSizeHob (BASE_4GB, UpperMemorySize);

    MtrrSetMemoryAttribute (BASE_4GB, UpperMemorySize, CacheWriteBack);
  }

  return MemoryBase + MemorySize;
}

