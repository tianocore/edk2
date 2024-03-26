/** @file
  LoongArch64 CPU Collect AP resource NULL Library functions.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Register/LoongArch64/Csr.h>
#include "../../../UefiCpuPkg/Library/MpInitLib/LoongArch64/MpLib.h"

VOID
SaveProcessorResourceData (
  IN PROCESSOR_RESOURCE_DATA *
  );

VOID
EFIAPI
SaveProcessorResource (
  PROCESSOR_RESOURCE_DATA  *mProcessorResource
  )
{
  SaveProcessorResourceData (mProcessorResource);
}

VOID
EFIAPI
CollectAllProcessorResource (
  VOID
  )
{
  return;
}
