/** @file
Agent Module to load other modules to deploy SMM Entry Vector for X86 CPU.

Copyright (c) 2009 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>
Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

/**
  Extract NumberOfCpus, MaxNumberOfCpus and EFI_PROCESSOR_INFORMATION for all CPU from gEfiMpServiceProtocolGuid.

  @param[out] NumberOfCpus           Pointer to NumberOfCpus.
  @param[out] MaxNumberOfCpus        Pointer to MaxNumberOfCpus.

  @retval ProcessorInfo              Pointer to EFI_PROCESSOR_INFORMATION buffer.
**/
EFI_PROCESSOR_INFORMATION *
GetMpInformationFromMpServices (
  OUT UINTN  *NumberOfCpus,
  OUT UINTN  *MaxNumberOfCpus
  )
{
  EFI_STATUS                 Status;
  UINTN                      Index;
  UINTN                      NumberOfEnabledProcessors;
  UINTN                      NumberOfProcessors;
  EFI_MP_SERVICES_PROTOCOL   *MpService;
  EFI_PROCESSOR_INFORMATION  *ProcessorInfo;

  if ((NumberOfCpus == NULL) || (MaxNumberOfCpus == NULL)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return NULL;
  }

  ProcessorInfo    = NULL;
  *NumberOfCpus    = 0;
  *MaxNumberOfCpus = 0;

  /// Get the MP Services Protocol
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpService);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return NULL;
  }

  /// Get the number of processors
  Status = MpService->GetNumberOfProcessors (MpService, &NumberOfProcessors, &NumberOfEnabledProcessors);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return NULL;
  }

  ASSERT (NumberOfProcessors <= PcdGet32 (PcdCpuMaxLogicalProcessorNumber));

  /// Allocate buffer for processor information
  ProcessorInfo = AllocateZeroPool (sizeof (EFI_PROCESSOR_INFORMATION) * NumberOfProcessors);
  if (ProcessorInfo == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return NULL;
  }

  /// Get processor information
  for (Index = 0; Index < NumberOfProcessors; Index++) {
    Status = MpService->GetProcessorInfo (MpService, Index | CPU_V2_EXTENDED_TOPOLOGY, &ProcessorInfo[Index]);
    if (EFI_ERROR (Status)) {
      FreePool (ProcessorInfo);
      DEBUG ((DEBUG_ERROR, "%a: Failed to get processor information for processor %d\n", __func__, Index));
      ASSERT_EFI_ERROR (Status);
      return NULL;
    }
  }

  *NumberOfCpus = NumberOfEnabledProcessors;

  ASSERT (*NumberOfCpus <= PcdGet32 (PcdCpuMaxLogicalProcessorNumber));
  //
  // If support CPU hot plug, we need to allocate resources for possibly hot-added processors
  //
  if (FeaturePcdGet (PcdCpuHotPlugSupport)) {
    *MaxNumberOfCpus = PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
  } else {
    *MaxNumberOfCpus = *NumberOfCpus;
  }

  return ProcessorInfo;
}

/**
  The module Entry Point of the CPU SMM driver.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
PiCpuSmmEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Save the PcdPteMemoryEncryptionAddressOrMask value into a global variable.
  // Make sure AddressEncMask is contained to smallest supported address field.
  //
  mAddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;
  DEBUG ((DEBUG_INFO, "mAddressEncMask = 0x%lx\n", mAddressEncMask));

  Status =  PiSmmCpuEntryCommon ();

  ASSERT_EFI_ERROR (Status);

  //
  // Install the SMM Configuration Protocol onto a new handle on the handle database.
  // The entire SMM Configuration Protocol is allocated from SMRAM, so only a pointer
  // to an SMRAM address will be present in the handle database
  //
  Status = SystemTable->BootServices->InstallMultipleProtocolInterfaces (
                                        &gSmmCpuPrivate->SmmCpuHandle,
                                        &gEfiSmmConfigurationProtocolGuid,
                                        &gSmmCpuPrivate->SmmConfiguration,
                                        NULL
                                        );
  ASSERT_EFI_ERROR (Status);

  //
  // Expose address of CPU Hot Plug Data structure if CPU hot plug is supported.
  //
  if (FeaturePcdGet (PcdCpuHotPlugSupport)) {
    Status = PcdSet64S (PcdCpuHotPlugDataAddress, (UINT64)(UINTN)&mCpuHotPlugData);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
