/** @file
  Static SMBIOS Table for platform


  Copyright (c) 2012, Apple Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SmbiosLib.h>
#include <Library/HobLib.h>

extern SMBIOS_TEMPLATE_ENTRY gSmbiosTemplate[];



SMBIOS_TABLE_TYPE19 gSmbiosType19Template = {
  { EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, sizeof (SMBIOS_TABLE_TYPE19), 0 },
  0xffffffff, // StartingAddress;
  0xffffffff, // EndingAddress;
  0,          // MemoryArrayHandle;
  1,          // PartitionWidth;
  0,          // ExtendedStartingAddress;
  0,          // ExtendedEndingAddress;
};

VOID
CreatePlatformSmbiosMemoryRecords (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS        HobPtr;
  SMBIOS_STRUCTURE_POINTER    Smbios16;
  SMBIOS_STRUCTURE_POINTER    Smbios17;
  EFI_SMBIOS_HANDLE           PhyscialMemoryArrayHandle;
  EFI_SMBIOS_HANDLE           SmbiosHandle;

  Smbios16.Hdr = SmbiosLibGetRecord (EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, 0, &PhyscialMemoryArrayHandle);
  if (Smbios16.Hdr == NULL) {
    // Only make a Type19 entry if a Type16 entry exists.
    return;
  }

  Smbios17.Hdr = SmbiosLibGetRecord (EFI_SMBIOS_TYPE_MEMORY_DEVICE, 0, &SmbiosHandle);
  if (Smbios17.Hdr == NULL) {
    // if type17 exits update with type16 Smbios handle
    Smbios17.Type17->MemoryArrayHandle = PhyscialMemoryArrayHandle;
  }

  // Generate Type16 records
  gSmbiosType19Template.MemoryArrayHandle = PhyscialMemoryArrayHandle;
  HobPtr.Raw = GetHobList ();
  while ((HobPtr.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, HobPtr.Raw)) != NULL) {
    if (HobPtr.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      gSmbiosType19Template.ExtendedStartingAddress = HobPtr.ResourceDescriptor->PhysicalStart;
      gSmbiosType19Template.ExtendedEndingAddress =
        HobPtr.ResourceDescriptor->PhysicalStart +
        HobPtr.ResourceDescriptor->ResourceLength - 1;

      SmbiosLibCreateEntry ((SMBIOS_STRUCTURE *)&gSmbiosType19Template, NULL);
    }
    HobPtr.Raw = GET_NEXT_HOB (HobPtr);
  }
}


/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
PlatformSmbiosDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_SMBIOS_HANDLE           SmbiosHandle;
  SMBIOS_STRUCTURE_POINTER    Smbios;

  // Phase 0 - Patch table to make SMBIOS 2.7 structures smaller to conform
  //           to an early version of the specification.

  // Phase 1 - Initialize SMBIOS tables from template
  Status = SmbiosLibInitializeFromTemplate (gSmbiosTemplate);
  ASSERT_EFI_ERROR (Status);

  // Phase 2 - Patch SMBIOS table entries

  Smbios.Hdr = SmbiosLibGetRecord (EFI_SMBIOS_TYPE_BIOS_INFORMATION, 0, &SmbiosHandle);
  if (Smbios.Type0 != NULL) {
    // 64K * (n+1) bytes
    Smbios.Type0->BiosSize = (UINT8)DivU64x32 (FixedPcdGet64 (PcdEmuFirmwareFdSize), 64*1024) - 1;

    SmbiosLibUpdateUnicodeString (
      SmbiosHandle,
      Smbios.Type0->BiosVersion,
      (CHAR16 *) PcdGetPtr (PcdFirmwareVersionString)
      );
    SmbiosLibUpdateUnicodeString (
      SmbiosHandle,
      Smbios.Type0->BiosReleaseDate,
      (CHAR16 *) PcdGetPtr (PcdFirmwareReleaseDateString)
      );
  }

  // Phase 3 - Create tables from scratch

  // Create Type 13 record from EFI Variables
  // Do we need this record for EFI as the info is available from EFI varaibles
  // Also language types don't always match between EFI and SMBIOS
  // CreateSmbiosLanguageInformation (1, gSmbiosLangToEfiLang);

  CreatePlatformSmbiosMemoryRecords ();

  return EFI_SUCCESS;
}
