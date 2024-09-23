/** @file
Agent Module to load other modules to deploy SMM Entry Vector for X86 CPU.

Copyright (c) 2009 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>
Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"
#include <Library/UefiBootServicesTableLib.h>

//
// TRUE to indicate it's the MM_STANDALONE MM CPU driver.
// FALSE to indicate it's the DXE_SMM_DRIVER SMM CPU driver.
//
const BOOLEAN  mIsStandaloneMm = FALSE;

//
// SMM ready to lock flag
//
BOOLEAN  mSmmReadyToLock = FALSE;

/**
  Check SmmProfile is enabled or not.

  @return TRUE     SmmProfile is enabled.
          FALSE    SmmProfile is not enabled.

**/
BOOLEAN
IsSmmProfileEnabled (
  VOID
  )
{
  return FeaturePcdGet (PcdCpuSmmProfileEnable);
}

/**
  Perform the remaining tasks.

**/
VOID
PerformRemainingTasks (
  VOID
  )
{
  EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE  *MemoryAttributesTable;

  if (mSmmReadyToLock) {
    PERF_FUNCTION_BEGIN ();

    //
    // Start SMM Profile feature
    //
    if (mSmmProfileEnabled) {
      SmmProfileStart ();
    }

    //
    // Check if all Aps enter SMM. In Relaxed-AP Sync Mode, BSP will not wait for
    // all Aps arrive. However,PerformRemainingTasks() needs to wait all Aps arrive before calling
    // SetMemMapAttributes() and ConfigSmmCodeAccessCheck() when mSmmReadyToLock
    // is true. In SetMemMapAttributes(), SmmSetMemoryAttributesEx() will call
    // FlushTlbForAll() that need to start up the aps. So it need to let all
    // aps arrive. Same as SetMemMapAttributes(), ConfigSmmCodeAccessCheck()
    // also will start up the aps.
    //
    if (EFI_ERROR (SmmCpuRendezvous (NULL, TRUE))) {
      DEBUG ((DEBUG_ERROR, "PerformRemainingTasks: fail to wait for all AP check in SMM!\n"));
    }

    //
    // Update Page Table for outside SMRAM.
    //
    if (mSmmProfileEnabled) {
      SmmProfileUpdateMemoryAttributes ();
    } else {
      UpdateUefiMemMapAttributes ();
    }

    //
    // gEdkiiPiSmmMemoryAttributesTableGuid should have been published at EndOfDxe by SmmCore
    // Note: gEdkiiPiSmmMemoryAttributesTableGuid is not always installed since it depends on
    //       the memory protection attribute setting in MM Core.
    //
    SmmGetSystemConfigurationTable (&gEdkiiPiSmmMemoryAttributesTableGuid, (VOID **)&MemoryAttributesTable);

    //
    // Set critical region attribute in page table according to the MemoryAttributesTable
    //
    if (MemoryAttributesTable != NULL) {
      SetMemMapAttributes (MemoryAttributesTable);
    }

    //
    // Set page table itself to be read-only
    //
    SetPageTableAttributes ();

    //
    // Configure SMM Code Access Check feature if available.
    //
    ConfigSmmCodeAccessCheck ();

    //
    // Measure performance of SmmCpuFeaturesCompleteSmmReadyToLock() from caller side
    // as the implementation is provided by platform.
    //
    PERF_START (NULL, "SmmCompleteReadyToLock", NULL, 0);
    SmmCpuFeaturesCompleteSmmReadyToLock ();
    PERF_END (NULL, "SmmCompleteReadyToLock", NULL, 0);

    //
    // Clean SMM ready to lock flag
    //
    mSmmReadyToLock = FALSE;

    PERF_FUNCTION_END ();
  }
}

/**
  To get system port address of the SMI Command Port in FADT table.

**/
VOID
GetSmiCommandPort (
  VOID
  )
{
  EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE  *Fadt;

  Fadt = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)EfiLocateFirstAcpiTable (
                                                        EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE
                                                        );
  ASSERT (Fadt != NULL);

  mSmiCommandPort = Fadt->SmiCmd;
  DEBUG ((DEBUG_INFO, "mSmiCommandPort = %x\n", mSmiCommandPort));
}

/**
  SMM Ready To Lock event notification handler.

  mSmmReadyToLock is set to perform additional lock actions that must be
  performed from SMM on the next SMI.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification handler runs successfully.
 **/
EFI_STATUS
EFIAPI
SmmReadyToLockEventNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  //
  // Cache a copy of UEFI memory map before we start profiling feature.
  //
  GetUefiMemoryMap ();

  //
  // Skip SMM profile initialization if feature is disabled
  //
  if (mSmmProfileEnabled) {
    //
    // Get Software SMI from FADT
    //
    GetSmiCommandPort ();

    //
    // Initialize protected memory range for patching page table later.
    //
    InitProtectedMemRange ();
  }

  //
  // Set SMM ready to lock flag and return
  //
  mSmmReadyToLock = TRUE;
  return EFI_SUCCESS;
}

/**
  Get SmmCpuSyncConfig data: RelaxedMode, SyncTimeout, SyncTimeout2.

  @param[in,out] RelaxedMode   It indicates if Relaxed CPU synchronization method or
                               traditional CPU synchronization method is used when processing an SMI.
  @param[in,out] SyncTimeout   It indicates the 1st BSP/AP synchronization timeout value in SMM.
  @param[in,out] SyncTimeout2  It indicates the 2nd BSP/AP synchronization timeout value in SMM.

 **/
VOID
GetSmmCpuSyncConfigData (
  IN OUT BOOLEAN *RelaxedMode, OPTIONAL
  IN OUT UINT64  *SyncTimeout, OPTIONAL
  IN OUT UINT64  *SyncTimeout2 OPTIONAL
  )
{
  if (RelaxedMode != NULL) {
    *RelaxedMode = (BOOLEAN)(PcdGet8 (PcdCpuSmmSyncMode) == MmCpuSyncModeRelaxedAp);
  }

  if (SyncTimeout != NULL) {
    *SyncTimeout = PcdGet64 (PcdCpuSmmApSyncTimeout);
  }

  if (SyncTimeout2 != NULL) {
    *SyncTimeout2 = PcdGet64 (PcdCpuSmmApSyncTimeout2);
  }
}

/**
  Get ACPI S3 enable flag.

**/
VOID
GetAcpiS3EnableFlag (
  VOID
  )
{
  mAcpiS3Enable = PcdGetBool (PcdAcpiS3Enable);
}

/**
  Get the maximum number of logical processors supported by the system.

  @retval The maximum number of logical processors supported by the system
          is indicated by the return value.
**/
UINTN
GetSupportedMaxLogicalProcessorNumber (
  VOID
  )
{
  return PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
}

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

  ASSERT (NumberOfProcessors <= GetSupportedMaxLogicalProcessorNumber ());

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

  ASSERT (*NumberOfCpus <= GetSupportedMaxLogicalProcessorNumber ());
  //
  // If support CPU hot plug, we need to allocate resources for possibly hot-added processors
  //
  if (FeaturePcdGet (PcdCpuHotPlugSupport)) {
    *MaxNumberOfCpus = GetSupportedMaxLogicalProcessorNumber ();
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
  VOID        *Registration;

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

  //
  // Register SMM Ready To Lock Protocol notification
  //
  Status = gMmst->MmRegisterProtocolNotify (
                    &gEfiSmmReadyToLockProtocolGuid,
                    SmmReadyToLockEventNotify,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
