/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Chipset/ARM1176JZ-S.h>

#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

VOID
EFIAPI
ArmWriteVBar (
  IN  UINTN   VectorBase
  )
{
  ASSERT(FeaturePcdGet (PcdRelocateVectorTable) == TRUE);

  if (VectorBase == 0x0) {
    ArmSetLowVectors ();
  } else if (VectorBase == 0xFFFF0000) {
    ArmSetHighVectors ();
  } else {
    // Feature not supported by ARM11. The Vector Table is either at 0x0 or 0xFFFF0000
    ASSERT(0);
  }
}

UINTN
EFIAPI
ArmReadVBar (
  VOID
  )
{
  ASSERT((FeaturePcdGet (PcdRelocateVectorTable) == TRUE) && ((PcdGet32 (PcdCpuVectorBaseAddress) == 0x0) || (PcdGet32 (PcdCpuVectorBaseAddress) == 0xFFFF0000)));
  return PcdGet32 (PcdCpuVectorBaseAddress);
}

