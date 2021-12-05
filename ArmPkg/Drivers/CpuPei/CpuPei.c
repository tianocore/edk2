/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2011 Hewlett Packard Corporation. All rights reserved.<BR>
Copyright (c) 2011-2013, ARM Limited. All rights reserved.<BR>

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

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/ArmLib.h>

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

  return EFI_SUCCESS;
}
