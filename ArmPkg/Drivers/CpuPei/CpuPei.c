/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2011 Hewlett Packard Corporation. All rights reserved.<BR>
Copyright (c) 2011-2013, ARM Limited. All rights reserved.<BR>
Copyright (c) 2023, Google, LLC. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MemoryInit.c

Abstract:

  PEIM to provide fake memory init

**/

//
// The package level header files this module uses
//
#include <PiPei.h>
//
// The protocols, PPI and GUID definitions for this module
//
#include <Ppi/ArmMpCoreInfo.h>
#include <Ppi/MemoryAttribute.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>

/**
  Set the requested memory permission attributes on a region of memory.

  BaseAddress and Length must be aligned to EFI_PAGE_SIZE.

  Attributes must contain a combination of EFI_MEMORY_RP, EFI_MEMORY_RO and
  EFI_MEMORY_XP, and specifies the attributes that must be set for the
  region in question. Attributes that are omitted will be cleared from the
  region only if they are set in AttributeMask.

  AttributeMask must contain a combination of EFI_MEMORY_RP, EFI_MEMORY_RO and
  EFI_MEMORY_XP, and specifies the attributes that the call will operate on.
  AttributeMask must not be 0x0, and must contain at least the bits set in
  Attributes.

  @param[in]  This              The protocol instance pointer.
  @param[in]  BaseAddress       The physical address that is the start address
                                of a memory region.
  @param[in]  Length            The size in bytes of the memory region.
  @param[in]  Attributes        Memory attributes to set or clear.
  @param[in]  AttributeMask     Mask of memory attributes to operate on.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                AttributeMask is zero.
                                AttributeMask lacks bits set in Attributes.
                                BaseAddress or Length is not suitably aligned.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.
                                The bit mask of attributes is not supported for
                                the memory resource range specified by
                                BaseAddress and Length.
  @retval EFI_OUT_OF_RESOURCES  Requested attributes cannot be applied due to
                                lack of system resources.

**/
STATIC
EFI_STATUS
EFIAPI
SetMemoryPermissions (
  IN  EDKII_MEMORY_ATTRIBUTE_PPI  *This,
  IN  EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN  UINT64                      Length,
  IN  UINT64                      Attributes,
  IN  UINT64                      AttributeMask
  )
{
  if ((Length == 0) ||
      (AttributeMask == 0) ||
      ((AttributeMask & (EFI_MEMORY_RP | EFI_MEMORY_RO | EFI_MEMORY_XP)) == 0) ||
      ((Attributes & ~AttributeMask) != 0) ||
      (((BaseAddress | Length) & EFI_PAGE_MASK) != 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  return ArmSetMemoryAttributes (BaseAddress, Length, Attributes, AttributeMask);
}

STATIC CONST EDKII_MEMORY_ATTRIBUTE_PPI  mMemoryAttributePpi = {
  SetMemoryPermissions
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mMemoryAttributePpiDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiMemoryAttributePpiGuid,
  (VOID *)&mMemoryAttributePpi
};

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
InitializeCpuPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS            Status;
  ARM_MP_CORE_INFO_PPI  *ArmMpCoreInfoPpi;
  UINTN                 ArmCoreCount;
  ARM_CORE_INFO         *ArmCoreInfoTable;

  // Enable program flow prediction, if supported.
  ArmEnableBranchPrediction ();

  // Publish the CPU memory and io spaces sizes
  BuildCpuHob (ArmGetPhysicalAddressBits (), PcdGet8 (PcdPrePiCpuIoSize));

  // Only MP Core platform need to produce gArmMpCoreInfoPpiGuid
  Status = PeiServicesLocatePpi (&gArmMpCoreInfoPpiGuid, 0, NULL, (VOID **)&ArmMpCoreInfoPpi);
  if (!EFI_ERROR (Status)) {
    // Build the MP Core Info Table
    ArmCoreCount = 0;
    Status       = ArmMpCoreInfoPpi->GetMpCoreInfo (&ArmCoreCount, &ArmCoreInfoTable);
    if (!EFI_ERROR (Status) && (ArmCoreCount > 0)) {
      // Build MPCore Info HOB
      BuildGuidDataHob (&gArmMpCoreInfoGuid, ArmCoreInfoTable, sizeof (ARM_CORE_INFO) * ArmCoreCount);
    }
  }

  Status = PeiServicesInstallPpi (&mMemoryAttributePpiDesc);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
