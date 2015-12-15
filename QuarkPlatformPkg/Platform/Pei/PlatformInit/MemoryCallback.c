/** @file
This file includes a memory call back function notified when MRC is done,
following action is performed in this file,
  1. ICH initialization after MRC.
  2. SIO initialization.
  3. Install ResetSystem and FinvFv PPI.
  4. Set MTRR for PEI
  5. Create FV HOB and Flash HOB

Copyright (c) 2013 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "CommonHeader.h"

#include "PlatformEarlyInit.h"

extern EFI_PEI_PPI_DESCRIPTOR mPpiStall[];

EFI_PEI_RESET_PPI mResetPpi = { ResetSystem };

EFI_PEI_PPI_DESCRIPTOR mPpiList[1] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiResetPpiGuid,
    &mResetPpi
  }
};

/**
  This function reset the entire platform, including all processor and devices, and
  reboots the system.

  @param  PeiServices General purpose services available to every PEIM.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
ResetSystem (
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  ResetCold();
  return EFI_SUCCESS;
}

/**
  This function provides a blocking stall for reset at least the given number of microseconds
  stipulated in the final argument.

  @param  PeiServices General purpose services available to every PEIM.

  @param  this Pointer to the local data for the interface.

  @param  Microseconds number of microseconds for which to stall.

  @retval EFI_SUCCESS the function provided at least the required stall.
**/
EFI_STATUS
EFIAPI
Stall (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN CONST EFI_PEI_STALL_PPI  *This,
  IN UINTN                    Microseconds
  )
{
  MicroSecondDelay (Microseconds);
  return EFI_SUCCESS;
}


/**
  This function will be called when MRC is done.

  @param  PeiServices General purpose services available to every PEIM.

  @param  NotifyDescriptor Information about the notify event..

  @param  Ppi The notify context.

  @retval EFI_SUCCESS If the function completed successfully.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                            Status;
  EFI_BOOT_MODE                         BootMode;
  UINT64                                MemoryLength;
  EFI_SMRAM_DESCRIPTOR                  *SmramDescriptor;
  UINTN                                 NumSmramRegions;
  UINT32                                RmuMainBaseAddress;
  UINT32                                RegData32;
  UINT8                                 CpuAddressWidth;
  UINT32                                RegEax;
  MTRR_SETTINGS                         MtrrSettings;

  DEBUG ((EFI_D_INFO, "Platform PEIM Memory Callback\n"));

  NumSmramRegions = 0;
  SmramDescriptor = NULL;
  RmuMainBaseAddress = 0;

  PERF_START (NULL, "SetCache", NULL, 0);

  InfoPostInstallMemory (&RmuMainBaseAddress, &SmramDescriptor, &NumSmramRegions);
  ASSERT (SmramDescriptor != NULL);
  ASSERT (RmuMainBaseAddress != 0);

  MemoryLength = ((UINT64) RmuMainBaseAddress) + 0x10000;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // Get current MTRR settings
  //
  MtrrGetAllMtrrs (&MtrrSettings);

  //
  // Set all DRAM cachability to CacheWriteBack
  //
  Status = MtrrSetMemoryAttributeInMtrrSettings (&MtrrSettings, 0, MemoryLength, CacheWriteBack);
  ASSERT_EFI_ERROR (Status);

  //
  // RTC:28208 - System hang/crash when entering probe mode(ITP) when relocating SMBASE
  //             Workaround to make default SMRAM UnCachable
  //
  Status = MtrrSetMemoryAttributeInMtrrSettings (&MtrrSettings, 0x30000, SIZE_64KB, CacheUncacheable);
  ASSERT_EFI_ERROR (Status);

  //
  // Set new MTRR settings
  //
  MtrrSetAllMtrrs (&MtrrSettings);

  PERF_END (NULL, "SetCache", NULL, 0);

  //
  // Install PeiReset for PeiResetSystem service
  //
  Status = PeiServicesInstallPpi (&mPpiList[0]);
  ASSERT_EFI_ERROR (Status);

  //
  // Do QNC initialization after MRC
  //
  PeiQNCPostMemInit ();

  Status = PeiServicesInstallPpi (&mPpiStall[0]);
  ASSERT_EFI_ERROR (Status);

  //
  // Set E000/F000 Routing
  //
  RegData32 = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  RegData32 |= (BIT2|BIT1);
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, RegData32);

  if (BootMode == BOOT_IN_RECOVERY_MODE) {
    Status = PeimInitializeRecovery (PeiServices);
    ASSERT_EFI_ERROR (Status);
  } else if (BootMode == BOOT_ON_S3_RESUME) {
    return EFI_SUCCESS;
  } else {
    PeiServicesInstallFvInfoPpi (
      NULL,
      (VOID *) (UINTN) PcdGet32 (PcdFlashFvMainBase),
      PcdGet32 (PcdFlashFvMainSize),
      NULL,
      NULL
      );

    //
    // Publish the FVMAIN FV so the DXE Phase can dispatch drivers from this FV
    // and produce Load File Protocols for UEFI Applications in this FV.
    //
    BuildFvHob (
      PcdGet32 (PcdFlashFvMainBase),
      PcdGet32 (PcdFlashFvMainSize)
      );

    //
    // Publish the Payload FV so the DXE Phase can dispatch drivers from this FV
    // and produce Load File Protocols for UEFI Applications in this FV.
    //
    BuildFvHob (
      PcdGet32 (PcdFlashFvPayloadBase),
      PcdGet32 (PcdFlashFvPayloadSize)
      );
  }

  //
  // Build flash HOB, it's going to be used by GCD and E820 building
  // Map full SPI flash decode range (regardless of smaller SPI flash parts installed)
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_FIRMWARE_DEVICE,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT    |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    (SIZE_4GB - SIZE_8MB),
    SIZE_8MB
    );

  //
  // Create a CPU hand-off information
  //
  CpuAddressWidth = 32;
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
  if (RegEax >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &RegEax, NULL, NULL, NULL);
    CpuAddressWidth = (UINT8) (RegEax & 0xFF);
  }
  DEBUG ((EFI_D_INFO, "CpuAddressWidth: %d\n", CpuAddressWidth));

  BuildCpuHob (CpuAddressWidth, 16);

  ASSERT_EFI_ERROR (Status);

  return Status;
}
