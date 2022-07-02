/** @file

  Copyright (c) 2011, ARM Limited. All rights reserved.
  Copyright (c) 2022, Google LLC. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Guid/MemoryTypeInformation.h>

EFI_STATUS
EFIAPI
MemoryPeim (
  IN EFI_PHYSICAL_ADDRESS  UefiMemoryBase,
  IN UINT64                UefiMemorySize
  );

/**
  Build the memory type information HOB that describes how many pages of each
  type to preallocate when initializing the GCD memory map.
**/
VOID
EFIAPI
BuildMemoryTypeInformationHob (
  VOID
  )
{
  EFI_MEMORY_TYPE_INFORMATION  Info[10];

  Info[0].Type          = EfiACPIReclaimMemory;
  Info[0].NumberOfPages = FixedPcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory);
  Info[1].Type          = EfiACPIMemoryNVS;
  Info[1].NumberOfPages = FixedPcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS);
  Info[2].Type          = EfiReservedMemoryType;
  Info[2].NumberOfPages = FixedPcdGet32 (PcdMemoryTypeEfiReservedMemoryType);
  Info[3].Type          = EfiRuntimeServicesData;
  Info[3].NumberOfPages = FixedPcdGet32 (PcdMemoryTypeEfiRuntimeServicesData);
  Info[4].Type          = EfiRuntimeServicesCode;
  Info[4].NumberOfPages = FixedPcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode);
  Info[5].Type          = EfiBootServicesCode;
  Info[5].NumberOfPages = FixedPcdGet32 (PcdMemoryTypeEfiBootServicesCode);
  Info[6].Type          = EfiBootServicesData;
  Info[6].NumberOfPages = FixedPcdGet32 (PcdMemoryTypeEfiBootServicesData);
  Info[7].Type          = EfiLoaderCode;
  Info[7].NumberOfPages = FixedPcdGet32 (PcdMemoryTypeEfiLoaderCode);
  Info[8].Type          = EfiLoaderData;
  Info[8].NumberOfPages = FixedPcdGet32 (PcdMemoryTypeEfiLoaderData);

  // Terminator for the list
  Info[9].Type          = EfiMaxMemoryType;
  Info[9].NumberOfPages = 0;

  BuildGuidDataHob (&gEfiMemoryTypeInformationGuid, &Info, sizeof (Info));
}

/**
  Module entry point.

  @param[in]    FileHandle    Handle of the file being invoked.
  @param[in]    PeiServices   Describes the list of possible PEI Services.

  @return       EFI_SUCCESS unless the operation failed.
**/
EFI_STATUS
EFIAPI
InitializeMemory (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  UINTN       UefiMemoryBase;
  EFI_STATUS  Status;

  ASSERT (FixedPcdGet64 (PcdSystemMemoryBase) < (UINT64)MAX_ALLOC_ADDRESS);

  //
  // Put the permanent PEI memory in the first 128 MiB of DRAM so that
  // it is covered by the statically configured ID map.
  //
  UefiMemoryBase = (UINTN)FixedPcdGet64 (PcdSystemMemoryBase) + SIZE_128MB
                   - FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  Status = PeiServicesInstallPeiMemory (
             UefiMemoryBase,
             FixedPcdGet32 (PcdSystemMemoryUefiRegionSize)
             );
  ASSERT_EFI_ERROR (Status);

  Status = MemoryPeim (
             UefiMemoryBase,
             FixedPcdGet32 (PcdSystemMemoryUefiRegionSize)
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
