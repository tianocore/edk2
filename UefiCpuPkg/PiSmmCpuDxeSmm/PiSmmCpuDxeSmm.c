/** @file
Agent Module to load other modules to deploy SMM Entry Vector for X86 CPU.

Copyright (c) 2009 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>
Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"
#include <Library/SmmMemLib.h>

//
// SMM ready to lock flag
//
BOOLEAN  mSmmReadyToLock = FALSE;

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

    SmmGetSystemConfigurationTable (&gEdkiiPiSmmMemoryAttributesTableGuid, (VOID **)&MemoryAttributesTable);
    if (MemoryAttributesTable == NULL) {
      return;
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
    // Start SMM Profile feature
    //
    if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
      SmmProfileStart ();
    }

    //
    // Update Page Table
    //
    if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
      SmmProfileUpdateMemoryAttributes ();
    } else {
      UpdateUefiMemMapAttributes ();
    }

    //
    // Set critical region attribute in page table according to the MemoryAttributesTable
    //
    SetMemMapAttributes (MemoryAttributesTable);

    if (IsRestrictedMemoryAccess ()) {
      //
      // Set page table itself to be read-only
      //
      SetPageTableAttributes ();
    }

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
  Perform the remaining tasks for SMM Initialization.

  @param[in] CpuIndex        The index of the CPU.

**/
VOID
PerformRemainingTasksForSmiInit (
  IN UINTN  CpuIndex
  )
{
  return;
}

/**
  SMM Ready To Lock event notification handler.

  The CPU S3 data is copied to SMRAM for security and mSmmReadyToLock is set to
  perform additional lock actions that must be performed from SMM on the next SMI.

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
  if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
    //
    // Get system port address of the SMI Command Port
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
  // TODO: check with security team for this PCD usage & support on Intel side.
  //
  mAddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;
  DEBUG ((DEBUG_INFO, "mAddressEncMask = 0x%lx\n", mAddressEncMask));

  Status =  PiSmmCpuEntryCommon ();

  ASSERT_EFI_ERROR (Status);

  //
  // Install the SMM Configuration Protocol onto a new handle on the handle database.
  // The entire SMM Configuration Protocol is allocated from SMRAM, so only a pointer
  // to an SMRAM address will be present in the handle database
  // TODO: Consider to make DXE_SMM_DRIVER & MM_STANDALONE aligned
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
  // TODO: Consider how to support CpuHotPlug in Standalone MM
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
