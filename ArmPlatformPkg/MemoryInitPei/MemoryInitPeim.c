/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiPei.h>

//
// The protocols, PPI and GUID definitions for this module
//
#include <Ppi/MasterBootMode.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Guid/MemoryTypeInformation.h>
//
// The Library classes this module consumes
//
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>

EFI_STATUS
EFIAPI
MemoryPeim (
  IN EFI_PHYSICAL_ADDRESS               UefiMemoryBase,
  IN UINT64                             UefiMemorySize
  );

// May want to put this into a library so you only need the PCD settings if you are using the feature?
VOID
BuildMemoryTypeInformationHob (
  VOID
  )
{
  EFI_MEMORY_TYPE_INFORMATION   Info[10];

  Info[0].Type          = EfiACPIReclaimMemory;
  Info[0].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory);
  Info[1].Type          = EfiACPIMemoryNVS;
  Info[1].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS);
  Info[2].Type          = EfiReservedMemoryType;
  Info[2].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiReservedMemoryType);
  Info[3].Type          = EfiRuntimeServicesData;
  Info[3].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesData);
  Info[4].Type          = EfiRuntimeServicesCode;
  Info[4].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode);
  Info[5].Type          = EfiBootServicesCode;
  Info[5].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesCode);
  Info[6].Type          = EfiBootServicesData;
  Info[6].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesData);
  Info[7].Type          = EfiLoaderCode;
  Info[7].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiLoaderCode);
  Info[8].Type          = EfiLoaderData;
  Info[8].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiLoaderData);

  // Terminator for the list
  Info[9].Type          = EfiMaxMemoryType;
  Info[9].NumberOfPages = 0;

  BuildGuidDataHob (&gEfiMemoryTypeInformationGuid, &Info, sizeof (Info));
}

/*++

Routine Description:



Arguments:

  FileHandle  - Handle of the file being invoked.
  PeiServices - Describes the list of possible PEI Services.

Returns:

  Status -  EFI_SUCCESS if the boot mode could be set

--*/
EFI_STATUS
EFIAPI
InitializeMemory (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                            Status;
  UINTN                                 SystemMemoryBase;
  UINT64                                SystemMemoryTop;
  UINTN                                 FdBase;
  UINTN                                 FdTop;
  UINTN                                 UefiMemoryBase;

  DEBUG ((EFI_D_LOAD | EFI_D_INFO, "Memory Init PEIM Loaded\n"));

  // Ensure PcdSystemMemorySize has been set
  ASSERT (PcdGet64 (PcdSystemMemorySize) != 0);
  ASSERT (PcdGet64 (PcdSystemMemoryBase) < (UINT64)MAX_ALLOC_ADDRESS);

  SystemMemoryBase = (UINTN)PcdGet64 (PcdSystemMemoryBase);
  SystemMemoryTop = SystemMemoryBase + PcdGet64 (PcdSystemMemorySize);
  if (SystemMemoryTop - 1 > MAX_ALLOC_ADDRESS) {
    SystemMemoryTop = (UINT64)MAX_ALLOC_ADDRESS + 1;
  }
  FdBase = (UINTN)PcdGet64 (PcdFdBaseAddress);
  FdTop = FdBase + (UINTN)PcdGet32 (PcdFdSize);

  //
  // Declare the UEFI memory to PEI
  //

  // In case the firmware has been shadowed in the System Memory
  if ((FdBase >= SystemMemoryBase) && (FdTop <= SystemMemoryTop)) {
    // Check if there is enough space between the top of the system memory and the top of the
    // firmware to place the UEFI memory (for PEI & DXE phases)
    if (SystemMemoryTop - FdTop >= FixedPcdGet32 (PcdSystemMemoryUefiRegionSize)) {
      UefiMemoryBase = SystemMemoryTop - FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
    } else {
      // Check there is enough space for the UEFI memory
      ASSERT (SystemMemoryBase + FixedPcdGet32 (PcdSystemMemoryUefiRegionSize) <= FdBase);

      UefiMemoryBase = FdBase - FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
    }
  } else {
    // Check the Firmware does not overlapped with the system memory
    ASSERT ((FdBase < SystemMemoryBase) || (FdBase >= SystemMemoryTop));
    ASSERT ((FdTop <= SystemMemoryBase) || (FdTop > SystemMemoryTop));

    UefiMemoryBase = SystemMemoryTop - FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  }

  Status = PeiServicesInstallPeiMemory (UefiMemoryBase, FixedPcdGet32 (PcdSystemMemoryUefiRegionSize));
  ASSERT_EFI_ERROR (Status);

  // Initialize MMU and Memory HOBs (Resource Descriptor HOBs)
  Status = MemoryPeim (UefiMemoryBase, FixedPcdGet32 (PcdSystemMemoryUefiRegionSize));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
