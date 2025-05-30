/** @file

  Copyright (c) 2011, ARM Limited. All rights reserved.<BR>
  Copyright (c) 2025 Ventana Micro Systems Inc.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

//
// The protocols, PPI and GUID definitions for this module
//
#include <Pi/PiBootMode.h>
#include <Ppi/MasterBootMode.h>
#include <Ppi/GuidedSectionExtraction.h>
#include <Ppi/SecHobData.h>
#include <RiscVSecHobData.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include "../Library/PlatformSecLib/PlatformSecLib.h"

#define PEI_MEMORY_SIZE  SIZE_64MB

EFI_STATUS
EFIAPI
InitializePlatformPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );

//
// Module globals
//
CONST EFI_PEI_PPI_DESCRIPTOR  mPpiListBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMasterBootModePpiGuid,
  NULL
};

/*
  Install PEI memory.

*/
STATIC
VOID
FindInstallPeiMemory (
  VOID
  )
{
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  VOID                         *HobList;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_PHYSICAL_ADDRESS         PeiMemoryBase;

  // Get the HOB list from PEI services
  HobList = GetHobList ();
  ASSERT (HobList != NULL);

  //
  // Search the top region
  //
  PeiMemoryBase = 0;

  // Iterate through the HOB list
  Hob.Raw = HobList;
  while (!END_OF_HOB_LIST (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      ResourceHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;
      if (  (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY)
         && (ResourceHob->PhysicalStart > PeiMemoryBase)
         && (ResourceHob->ResourceLength >= PEI_MEMORY_SIZE))
      {
        PeiMemoryBase = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - PEI_MEMORY_SIZE;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  ASSERT (PeiMemoryBase != 0);
  ASSERT_EFI_ERROR (PeiServicesInstallPeiMemory (PeiMemoryBase, PEI_MEMORY_SIZE));
}

/*
  Entry point of this module.

  @param  FileHandle            Handle of the file being invoked.
  @param  PeiServices           Pointer to the PEI Services Table.

  @retval EFI_SUCCESS           The entry point executes successfully.
  @retval Others                Some error occurs during the execution of this function.
*/
EFI_STATUS
EFIAPI
InitializePlatformPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;
  VOID                    *Hob;
  RISCV_SEC_HANDOFF_DATA  *SecData;
  EFI_GUID                SecHobDataGuid = RISCV_SEC_HANDOFF_HOB_GUID;

  DEBUG ((DEBUG_LOAD | DEBUG_INFO, "Platform PEIM Loaded\n"));

  Status = PeiServicesSetBootMode (BOOT_WITH_FULL_CONFIGURATION);
  ASSERT_EFI_ERROR (Status);

  Hob = GetFirstGuidHob (&SecHobDataGuid);
  ASSERT (Hob != NULL);
  SecData = GET_GUID_HOB_DATA (Hob);

  //
  // Do all platform initializations
  //
  MemoryInitialization (SecData->FdtPointer);
  CpuInitialization (SecData->FdtPointer);
  PlatformInitialization (SecData->FdtPointer);

  //
  // Install PEI memory
  //
  FindInstallPeiMemory ();

  //
  // Reinstall HOB after memory initialization
  //
  BuildGuidDataHob (
    &SecHobDataGuid,     // GUID for the HOB
    &SecData,            // Pointer to the data
    sizeof (SecData)     // Size of the data
    );

  Status = PeiServicesInstallPpi (&mPpiListBootMode);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
