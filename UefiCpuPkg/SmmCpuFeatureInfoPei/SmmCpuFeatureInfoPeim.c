/** @file
  This PEI Module creates SMM_CPU_FEATURE_INFO_HOB.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmmCpuFeatureInfoPeim.h"

//
// Configure the SMM_PROFILE DTS region size
//
#define SMM_PROFILE_DTS_SIZE  (4 * 1024 * 1024)      // 4M
#define CPUID1_EDX_BTS_AVAILABLE  0x200000

/**
  Build Memory Allocation HOB for SmmProfileData

**/
VOID
BuildSmmProfileDataMemAllocHob (
  VOID
  )
{
  BOOLEAN                        BtsSupported;
  UINT32                         RegEdx;
  MSR_IA32_MISC_ENABLE_REGISTER  MiscEnableMsr;
  UINTN                          TotalSize;
  VOID                           *Alloc;
  EFI_PEI_HOB_POINTERS           Hob;

  //
  // Check if processor support BTS
  //
  BtsSupported = TRUE;
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx);
  if ((RegEdx & CPUID1_EDX_BTS_AVAILABLE) != 0) {
    //
    // Per IA32 manuals:
    // When CPUID.1:EDX[21] is set, the following BTS facilities are available:
    // 1. The BTS_UNAVAILABLE flag in the IA32_MISC_ENABLE MSR indicates the
    //    availability of the BTS facilities, including the ability to set the BTS and
    //    BTINT bits in the MSR_DEBUGCTLA MSR.
    // 2. The IA32_DS_AREA MSR can be programmed to point to the DS save area.
    //
    MiscEnableMsr.Uint64 = AsmReadMsr64 (MSR_IA32_MISC_ENABLE);
    if (MiscEnableMsr.Bits.BTS == 1) {
      //
      // BTS facilities is not supported if MSR_IA32_MISC_ENABLE.BTS bit is set.
      //
      BtsSupported = FALSE;
    }
  }

  if (BtsSupported) {
    TotalSize =  PcdGet32 (PcdCpuSmmProfileSize) + SMM_PROFILE_DTS_SIZE;
  } else {
    TotalSize =  PcdGet32 (PcdCpuSmmProfileSize);
  }

  Alloc = AllocatePages (EFI_SIZE_TO_PAGES (TotalSize));
  if (Alloc == NULL) {
    return;
  }

  ZeroMem ((VOID *)(UINTN)Alloc, TotalSize);

  //
  // Find the HOB just created:
  // 1. change the name to gEdkiiSmmProfileDataGuid
  // 2. change the MemoryType to EfiReservedMemoryType
  //
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (Hob.Raw != NULL) {
    if (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress == (UINTN)Alloc) {
      CopyGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gEdkiiSmmProfileDataGuid);
      Hob.MemoryAllocation->AllocDescriptor.MemoryType = EfiReservedMemoryType;
      return;
    }

    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (Hob));
  }

  ASSERT (FALSE);

  FreePages (Alloc, EFI_SIZE_TO_PAGES (TotalSize));
  return;
}

/**
  The Entry point of SmmCpuFeatureInfoPei.

  @param  FileHandle    Handle of the file being invoked.
  @param  PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
SmmCpuFeatureInfoPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  SMM_CPU_FEATURE_INFO_HOB  *SmmSyncModeInfoHob;

  SmmSyncModeInfoHob = BuildGuidHob (&gEdkiiSmmCpuFeatureInfoHobGuid, sizeof (SMM_CPU_FEATURE_INFO_HOB));
  ASSERT (SmmSyncModeInfoHob != NULL);

  SmmSyncModeInfoHob->RelaxedCpuSyncMode  = (BOOLEAN)PcdGet8 (PcdCpuSmmSyncMode);
  SmmSyncModeInfoHob->AcpiS3Enable        = (BOOLEAN)PcdGetBool (PcdAcpiS3Enable);
  SmmSyncModeInfoHob->CpuSmmApSyncTimeout = PcdGet64 (PcdCpuSmmApSyncTimeout);

  //
  // Build Memory Allocation HOB for SmmProfileData
  //
  BuildSmmProfileDataMemAllocHob ();

  return EFI_SUCCESS;
}
