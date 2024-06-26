/** @file

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

/**
  Get SmmProfileData.

  @param[in, out]     Size     Return Size of SmmProfileData.

  @return Address of SmmProfileData

**/
EFI_PHYSICAL_ADDRESS
GetSmmProfileData (
  IN OUT  UINT64  *Size
  )
{
  EFI_PEI_HOB_POINTERS  SmmProfileDataHob;

  ASSERT (Size != NULL);

  //
  // Get Smm Profile Base from Memory Allocation HOB
  //
  SmmProfileDataHob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (SmmProfileDataHob.Raw != NULL) {
    //
    // Find gEdkiiSmmProfileDataGuid
    //
    if (CompareGuid (&SmmProfileDataHob.MemoryAllocation->AllocDescriptor.Name, &gEdkiiSmmProfileDataGuid)) {
      break;
    }

    SmmProfileDataHob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (SmmProfileDataHob));
  }

  ASSERT (SmmProfileDataHob.Raw != NULL);

  *Size = SmmProfileDataHob.MemoryAllocation->AllocDescriptor.MemoryLength;

  return SmmProfileDataHob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress;
}
