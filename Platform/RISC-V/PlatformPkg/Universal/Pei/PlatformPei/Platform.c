/**@file
  Platform PEI driver

  Copyright (c) 2019-2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/ResourcePublicationLib.h>
#include <Guid/MemoryTypeInformation.h>
#include <Ppi/MasterBootMode.h>
#include <IndustryStandard/Pci22.h>

#include <SiFiveU5MCCoreplex.h>

#include "Platform.h"

EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIMemoryNVS,       0x004 },
  { EfiACPIReclaimMemory,   0x008 },
  { EfiReservedMemoryType,  0x004 },
  { EfiRuntimeServicesData, 0x024 },
  { EfiRuntimeServicesCode, 0x030 },
  { EfiBootServicesCode,    0x180 },
  { EfiBootServicesData,    0xF00 },
  { EfiMaxMemoryType,       0x000 }
};


EFI_PEI_PPI_DESCRIPTOR   mPpiBootMode[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
};

STATIC EFI_BOOT_MODE mBootMode = BOOT_WITH_FULL_CONFIGURATION;

VOID
AddIoMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

VOID
AddReservedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

VOID
AddIoMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  )
{
  AddIoMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}


VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}


VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  )
{
  AddMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}


VOID
AddUntestedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE,
    MemoryBase,
    MemorySize
    );
}

VOID
AddUntestedMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  )
{
  AddUntestedMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}

VOID
AddPciResource (
  VOID
  )
{
  //
  // Platform-specific
  //
}

VOID
MemMapInitialization (
  VOID
  )
{
  //
  // Create Memory Type Information HOB
  //
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof(mDefaultMemoryTypeInformation)
    );

  //
  // Add PCI IO Port space available for PCI resource allocations.
  //
  AddPciResource ();
}

VOID
MiscInitialization (
  VOID
  )
{
  //
  // Build the CPU HOB with guest RAM size dependent address width and 16-bits
  // of IO space. (Side note: unlike other HOBs, the CPU HOB is needed during
  // S3 resume as well, so we build it unconditionally.)
  //
  // TODO: Determine this dynamically from the platform
  // setting or the HART configuration.
  //
  BuildCpuHob (48, 32);
}

/**
  Check if system returns from S3.

  @return BOOLEAN   TRUE, system returned from S3
                    FALSE, system is not returned from S3

**/
BOOLEAN
CheckResumeFromS3 (
  VOID
  )
{
  //
  //Platform implementation-specific
  //
  return FALSE;
}


VOID
BootModeInitialization (
  VOID
  )
{
  EFI_STATUS    Status;

  if (CheckResumeFromS3 () == TRUE) {
    DEBUG ((DEBUG_INFO, "This is wake from S3\n"));
  } else {
    DEBUG ((DEBUG_INFO, "This is normal boot\n"));
  }
  Status = PeiServicesSetBootMode (mBootMode);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (mPpiBootMode);
  ASSERT_EFI_ERROR (Status);
}

/**
  Build processor information for U54 Coreplex processor.

  @return EFI_SUCCESS     Status.

**/
EFI_STATUS
BuildCoreInformationHob (
  VOID
)
{
  EFI_STATUS Status;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA *SmbiosHobPtr;

  // TODO: Create SMBIOS libs for non-U540 platforms
  Status = CreateU5MCCoreplexProcessorSpecificDataHob (0);
  if (EFI_ERROR (Status)) {
    ASSERT(FALSE);
  }
  Status = CreateU5MCProcessorSmbiosDataHob (0, &SmbiosHobPtr);
  if (EFI_ERROR (Status)) {
    ASSERT(FALSE);
  }

  DEBUG ((DEBUG_INFO, "U5 MC Coreplex SMBIOS DATA HOB at address 0x%x\n", SmbiosHobPtr));

  return EFI_SUCCESS;
}

/**
  Perform Platform PEI initialization.

  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
InitializePlatform (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_INFO, "Platform PEIM Loaded\n"));

  BootModeInitialization ();
  DEBUG ((DEBUG_INFO, "Platform BOOT mode initiated.\n"));
  PublishPeiMemory ();
  DEBUG ((DEBUG_INFO, "PEI memory published.\n"));
  InitializeRamRegions ();
  DEBUG ((DEBUG_INFO, "Platform RAM regions initiated.\n"));

  if (mBootMode != BOOT_ON_S3_RESUME) {
    PeiFvInitialization ();
    MemMapInitialization ();
  }

  MiscInitialization ();
  Status = BuildCoreInformationHob ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to build processor informstion HOB.\n"));
    ASSERT(FALSE);
  }
  return EFI_SUCCESS;
}
