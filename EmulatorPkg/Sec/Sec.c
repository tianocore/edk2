/*++ @file
  Stub SEC that is called from the OS application that is the root of the emulator.

  The OS application will call the SEC with the PEI Entry Point API.

Copyright (c) 2011, Apple Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Sec.h"



EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI mSecTemporaryRamSupportPpi = {
  SecTemporaryRamSupport
};


EFI_PEI_PPI_DESCRIPTOR  gPrivateDispatchTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiTemporaryRamSupportPpiGuid,
    &mSecTemporaryRamSupportPpi
  }
};



/**
  The entry point of PE/COFF Image for the PEI Core, that has been hijacked by this
  SEC that sits on top of an OS application. So the entry and exit of this module
  has the same API.

  This function is the entry point for the PEI Foundation, which allows the SEC phase
  to pass information about the stack, temporary RAM and the Boot Firmware Volume.
  In addition, it also allows the SEC phase to pass services and data forward for use
  during the PEI phase in the form of one or more PPIs.
  There is no limit to the number of additional PPIs that can be passed from SEC into
  the PEI Foundation. As part of its initialization phase, the PEI Foundation will add
  these SEC-hosted PPIs to its PPI database such that both the PEI Foundation and any
  modules can leverage the associated service calls and/or code in these early PPIs.
  This function is required to call ProcessModuleEntryPointList() with the Context
  parameter set to NULL.  ProcessModuleEntryPoint() is never expected to return.
  The PEI Core is responsible for calling ProcessLibraryConstructorList() as soon as
  the PEI Services Table and the file handle for the PEI Core itself have been established.
  If ProcessModuleEntryPointList() returns, then ASSERT() and halt the system.

  @param SecCoreData  Points to a data structure containing information about the PEI
                      core's operating environment, such as the size and location of
                      temporary RAM, the stack location and the BFV location.

  @param PpiList      Points to a list of one or more PPI descriptors to be installed
                      initially by the PEI core. An empty PPI list consists of a single
                      descriptor with the end-tag EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST.
                      As part of its initialization phase, the PEI Foundation will add
                      these SEC-hosted PPIs to its PPI database such that both the PEI
                      Foundation and any modules can leverage the associated service calls
                      and/or code in these early PPIs.

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN EFI_SEC_PEI_HAND_OFF   *SecCoreData,
  IN EFI_PEI_PPI_DESCRIPTOR *PpiList
  )
{
  EFI_STATUS                Status;
  EFI_PEI_FV_HANDLE         VolumeHandle;
  EFI_PEI_FILE_HANDLE       FileHandle;
  VOID                      *PeCoffImage;
  EFI_PEI_CORE_ENTRY_POINT  EntryPoint;
  EFI_PEI_PPI_DESCRIPTOR    *Ppi;
  EFI_PEI_PPI_DESCRIPTOR    *SecPpiList;
  UINTN                     SecReseveredMemorySize;
  UINTN                     Index;
  EFI_PEI_PPI_DESCRIPTOR    PpiArray[10];

  EMU_MAGIC_PAGE()->PpiList = PpiList;
  ProcessLibraryConstructorList ();

  DEBUG ((EFI_D_ERROR, "SEC Has Started\n"));

  //
  // Add Our PPIs to the list
  //
  SecReseveredMemorySize = sizeof (gPrivateDispatchTable);
  for (Ppi = PpiList, Index = 1; ; Ppi++, Index++) {
    SecReseveredMemorySize += sizeof (EFI_PEI_PPI_DESCRIPTOR);

    if ((Ppi->Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) == EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) {
      // Since we are appending, need to clear out previous list terminator.
      Ppi->Flags &= ~EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
      break;
    }
  }

  // Keep everything on a good alignment
  SecReseveredMemorySize = ALIGN_VALUE (SecReseveredMemorySize, CPU_STACK_ALIGNMENT);

#if 0
  // Tell the PEI Core to not use our buffer in temp RAM
  SecPpiList = (EFI_PEI_PPI_DESCRIPTOR *)SecCoreData->PeiTemporaryRamBase;
  SecCoreData->PeiTemporaryRamBase = (VOID *)((UINTN)SecCoreData->PeiTemporaryRamBase + SecReseveredMemorySize);
  SecCoreData->PeiTemporaryRamSize -= SecReseveredMemorySize;
#else
  //
  // When I subtrack from SecCoreData->PeiTemporaryRamBase PEI Core crashes? Either there is a bug
  // or I don't understand temp RAM correctly?
  //

  SecPpiList = &PpiArray[0];
  ASSERT (sizeof (PpiArray) >= SecReseveredMemorySize);
#endif
  // Copy existing list, and append our entries.
  CopyMem (SecPpiList, PpiList, sizeof (EFI_PEI_PPI_DESCRIPTOR) * Index);
  CopyMem (&SecPpiList[Index], gPrivateDispatchTable, sizeof (gPrivateDispatchTable));

  // Find PEI Core and transfer control
  VolumeHandle = (EFI_PEI_FV_HANDLE)(UINTN)SecCoreData->BootFirmwareVolumeBase;
  FileHandle   = NULL;
  Status = PeiServicesFfsFindNextFile (EFI_FV_FILETYPE_PEI_CORE, VolumeHandle, &FileHandle);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesFfsFindSectionData (EFI_SECTION_PE32, FileHandle, &PeCoffImage);
  ASSERT_EFI_ERROR (Status);

  Status = PeCoffLoaderGetEntryPoint (PeCoffImage, (VOID **)&EntryPoint);
  ASSERT_EFI_ERROR (Status);

  // Transfer control to PEI Core
  EntryPoint (SecCoreData, SecPpiList);

  // PEI Core never returns
  ASSERT (FALSE);
  return;
}


