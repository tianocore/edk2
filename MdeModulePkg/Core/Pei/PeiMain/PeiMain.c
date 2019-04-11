/** @file
  Pei Core Main Entry Point

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"

EFI_PEI_PPI_DESCRIPTOR mMemoryDiscoveredPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  NULL
};

///
/// Pei service instance
///
EFI_PEI_SERVICES  gPs = {
  {
    PEI_SERVICES_SIGNATURE,
    PEI_SERVICES_REVISION,
    sizeof (EFI_PEI_SERVICES),
    0,
    0
  },
  PeiInstallPpi,
  PeiReInstallPpi,
  PeiLocatePpi,
  PeiNotifyPpi,

  PeiGetBootMode,
  PeiSetBootMode,

  PeiGetHobList,
  PeiCreateHob,

  PeiFfsFindNextVolume,
  PeiFfsFindNextFile,
  PeiFfsFindSectionData,

  PeiInstallPeiMemory,
  PeiAllocatePages,
  PeiAllocatePool,
  (EFI_PEI_COPY_MEM)CopyMem,
  (EFI_PEI_SET_MEM)SetMem,

  PeiReportStatusCode,
  PeiResetSystem,

  &gPeiDefaultCpuIoPpi,
  &gPeiDefaultPciCfg2Ppi,

  PeiFfsFindFileByName,
  PeiFfsGetFileInfo,
  PeiFfsGetVolumeInfo,
  PeiRegisterForShadow,
  PeiFfsFindSectionData3,
  PeiFfsGetFileInfo2,
  PeiResetSystem2,
  PeiFreePages,
};

/**
  Shadow PeiCore module from flash to installed memory.

  @param PrivateData    PeiCore's private data structure

  @return PeiCore function address after shadowing.
**/
PEICORE_FUNCTION_POINTER
ShadowPeiCore (
  IN PEI_CORE_INSTANCE  *PrivateData
  )
{
  EFI_PEI_FILE_HANDLE          PeiCoreFileHandle;
  EFI_PHYSICAL_ADDRESS         EntryPoint;
  EFI_STATUS                   Status;
  UINT32                       AuthenticationState;
  UINTN                        Index;
  EFI_PEI_CORE_FV_LOCATION_PPI *PeiCoreFvLocationPpi;
  UINTN                        PeiCoreFvIndex;

  PeiCoreFileHandle = NULL;
  //
  // Default PeiCore is in BFV
  //
  PeiCoreFvIndex = 0;
  //
  // Find the PEI Core either from EFI_PEI_CORE_FV_LOCATION_PPI indicated FV or BFV
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiCoreFvLocationPpiGuid,
             0,
             NULL,
             (VOID **) &PeiCoreFvLocationPpi
             );
  if (!EFI_ERROR (Status) && (PeiCoreFvLocationPpi->PeiCoreFvLocation != NULL)) {
    //
    // If PeiCoreFvLocation present, the PEI Core should be found from indicated FV
    //
    for (Index = 0; Index < PrivateData->FvCount; Index ++) {
      if (PrivateData->Fv[Index].FvHandle == PeiCoreFvLocationPpi->PeiCoreFvLocation) {
        PeiCoreFvIndex = Index;
        break;
      }
    }
    ASSERT (Index < PrivateData->FvCount);
  }
  //
  // Find PEI Core from the given FV index
  //
  Status = PrivateData->Fv[PeiCoreFvIndex].FvPpi->FindFileByType (
                                                    PrivateData->Fv[PeiCoreFvIndex].FvPpi,
                                                    EFI_FV_FILETYPE_PEI_CORE,
                                                    PrivateData->Fv[PeiCoreFvIndex].FvHandle,
                                                    &PeiCoreFileHandle
                                                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Shadow PEI Core into memory so it will run faster
  //
  Status = PeiLoadImage (
              GetPeiServicesTablePointer (),
              *((EFI_PEI_FILE_HANDLE*)&PeiCoreFileHandle),
              PEIM_STATE_REGISTER_FOR_SHADOW,
              &EntryPoint,
              &AuthenticationState
              );
  ASSERT_EFI_ERROR (Status);

  //
  // Compute the PeiCore's function address after shadowed PeiCore.
  // _ModuleEntryPoint is PeiCore main function entry
  //
  return (PEICORE_FUNCTION_POINTER)((UINTN) EntryPoint + (UINTN) PeiCore - (UINTN) _ModuleEntryPoint);
}

/**
  This routine is invoked by main entry of PeiMain module during transition
  from SEC to PEI. After switching stack in the PEI core, it will restart
  with the old core data.

  @param SecCoreDataPtr  Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.
  @param PpiList         Points to a list of one or more PPI descriptors to be installed initially by the PEI core.
                         An empty PPI list consists of a single descriptor with the end-tag
                         EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST. As part of its initialization
                         phase, the PEI Foundation will add these SEC-hosted PPIs to its PPI database such
                         that both the PEI Foundation and any modules can leverage the associated service
                         calls and/or code in these early PPIs
  @param Data            Pointer to old core data that is used to initialize the
                         core's data areas.
                         If NULL, it is first PeiCore entering.

**/
VOID
EFIAPI
PeiCore (
  IN CONST EFI_SEC_PEI_HAND_OFF        *SecCoreDataPtr,
  IN CONST EFI_PEI_PPI_DESCRIPTOR      *PpiList,
  IN VOID                              *Data
  )
{
  PEI_CORE_INSTANCE           PrivateData;
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  EFI_SEC_PEI_HAND_OFF        NewSecCoreData;
  EFI_STATUS                  Status;
  PEI_CORE_TEMP_POINTERS      TempPtr;
  PEI_CORE_INSTANCE           *OldCoreData;
  EFI_PEI_CPU_IO_PPI          *CpuIo;
  EFI_PEI_PCI_CFG2_PPI        *PciCfg;
  EFI_HOB_HANDOFF_INFO_TABLE  *HandoffInformationTable;
  EFI_PEI_TEMPORARY_RAM_DONE_PPI *TemporaryRamDonePpi;
  UINTN                       Index;

  //
  // Retrieve context passed into PEI Core
  //
  OldCoreData = (PEI_CORE_INSTANCE *) Data;
  SecCoreData = (EFI_SEC_PEI_HAND_OFF *) SecCoreDataPtr;

  //
  // Perform PEI Core phase specific actions.
  //
  if (OldCoreData == NULL) {
    //
    // If OldCoreData is NULL, means current is the first entry into the PEI Core before memory is available.
    //
    ZeroMem (&PrivateData, sizeof (PEI_CORE_INSTANCE));
    PrivateData.Signature = PEI_CORE_HANDLE_SIGNATURE;
    CopyMem (&PrivateData.ServiceTableShadow, &gPs, sizeof (gPs));
  } else {
    //
    // Memory is available to the PEI Core.  See if the PEI Core has been shadowed to memory yet.
    //
    if (OldCoreData->ShadowedPeiCore == NULL) {
      //
      // Fixup the PeiCore's private data
      //
      OldCoreData->Ps    = &OldCoreData->ServiceTableShadow;
      OldCoreData->CpuIo = &OldCoreData->ServiceTableShadow.CpuIo;
      if (OldCoreData->HeapOffsetPositive) {
        OldCoreData->HobList.Raw = (VOID *)(OldCoreData->HobList.Raw + OldCoreData->HeapOffset);
        if (OldCoreData->UnknownFvInfo != NULL) {
          OldCoreData->UnknownFvInfo      = (PEI_CORE_UNKNOW_FORMAT_FV_INFO *) ((UINT8 *) OldCoreData->UnknownFvInfo + OldCoreData->HeapOffset);
        }
        if (OldCoreData->CurrentFvFileHandles != NULL) {
          OldCoreData->CurrentFvFileHandles = (EFI_PEI_FILE_HANDLE *) ((UINT8 *) OldCoreData->CurrentFvFileHandles + OldCoreData->HeapOffset);
        }
        if (OldCoreData->PpiData.PpiList.PpiPtrs != NULL) {
          OldCoreData->PpiData.PpiList.PpiPtrs = (PEI_PPI_LIST_POINTERS *) ((UINT8 *) OldCoreData->PpiData.PpiList.PpiPtrs + OldCoreData->HeapOffset);
        }
        if (OldCoreData->PpiData.CallbackNotifyList.NotifyPtrs != NULL) {
          OldCoreData->PpiData.CallbackNotifyList.NotifyPtrs = (PEI_PPI_LIST_POINTERS *) ((UINT8 *) OldCoreData->PpiData.CallbackNotifyList.NotifyPtrs + OldCoreData->HeapOffset);
        }
        if (OldCoreData->PpiData.DispatchNotifyList.NotifyPtrs != NULL) {
          OldCoreData->PpiData.DispatchNotifyList.NotifyPtrs = (PEI_PPI_LIST_POINTERS *) ((UINT8 *) OldCoreData->PpiData.DispatchNotifyList.NotifyPtrs + OldCoreData->HeapOffset);
        }
        OldCoreData->Fv                   = (PEI_CORE_FV_HANDLE *) ((UINT8 *) OldCoreData->Fv + OldCoreData->HeapOffset);
        for (Index = 0; Index < OldCoreData->FvCount; Index ++) {
          if (OldCoreData->Fv[Index].PeimState != NULL) {
            OldCoreData->Fv[Index].PeimState     = (UINT8 *) OldCoreData->Fv[Index].PeimState + OldCoreData->HeapOffset;
          }
          if (OldCoreData->Fv[Index].FvFileHandles != NULL) {
            OldCoreData->Fv[Index].FvFileHandles = (EFI_PEI_FILE_HANDLE *) ((UINT8 *) OldCoreData->Fv[Index].FvFileHandles + OldCoreData->HeapOffset);
          }
        }
        OldCoreData->TempFileGuid         = (EFI_GUID *) ((UINT8 *) OldCoreData->TempFileGuid + OldCoreData->HeapOffset);
        OldCoreData->TempFileHandles      = (EFI_PEI_FILE_HANDLE *) ((UINT8 *) OldCoreData->TempFileHandles + OldCoreData->HeapOffset);
      } else {
        OldCoreData->HobList.Raw = (VOID *)(OldCoreData->HobList.Raw - OldCoreData->HeapOffset);
        if (OldCoreData->UnknownFvInfo != NULL) {
          OldCoreData->UnknownFvInfo      = (PEI_CORE_UNKNOW_FORMAT_FV_INFO *) ((UINT8 *) OldCoreData->UnknownFvInfo - OldCoreData->HeapOffset);
        }
        if (OldCoreData->CurrentFvFileHandles != NULL) {
          OldCoreData->CurrentFvFileHandles = (EFI_PEI_FILE_HANDLE *) ((UINT8 *) OldCoreData->CurrentFvFileHandles - OldCoreData->HeapOffset);
        }
        if (OldCoreData->PpiData.PpiList.PpiPtrs != NULL) {
          OldCoreData->PpiData.PpiList.PpiPtrs = (PEI_PPI_LIST_POINTERS *) ((UINT8 *) OldCoreData->PpiData.PpiList.PpiPtrs - OldCoreData->HeapOffset);
        }
        if (OldCoreData->PpiData.CallbackNotifyList.NotifyPtrs != NULL) {
          OldCoreData->PpiData.CallbackNotifyList.NotifyPtrs = (PEI_PPI_LIST_POINTERS *) ((UINT8 *) OldCoreData->PpiData.CallbackNotifyList.NotifyPtrs - OldCoreData->HeapOffset);
        }
        if (OldCoreData->PpiData.DispatchNotifyList.NotifyPtrs != NULL) {
          OldCoreData->PpiData.DispatchNotifyList.NotifyPtrs = (PEI_PPI_LIST_POINTERS *) ((UINT8 *) OldCoreData->PpiData.DispatchNotifyList.NotifyPtrs - OldCoreData->HeapOffset);
        }
        OldCoreData->Fv                   = (PEI_CORE_FV_HANDLE *) ((UINT8 *) OldCoreData->Fv - OldCoreData->HeapOffset);
        for (Index = 0; Index < OldCoreData->FvCount; Index ++) {
          if (OldCoreData->Fv[Index].PeimState != NULL) {
            OldCoreData->Fv[Index].PeimState     = (UINT8 *) OldCoreData->Fv[Index].PeimState - OldCoreData->HeapOffset;
          }
          if (OldCoreData->Fv[Index].FvFileHandles != NULL) {
            OldCoreData->Fv[Index].FvFileHandles = (EFI_PEI_FILE_HANDLE *) ((UINT8 *) OldCoreData->Fv[Index].FvFileHandles - OldCoreData->HeapOffset);
          }
        }
        OldCoreData->TempFileGuid         = (EFI_GUID *) ((UINT8 *) OldCoreData->TempFileGuid - OldCoreData->HeapOffset);
        OldCoreData->TempFileHandles      = (EFI_PEI_FILE_HANDLE *) ((UINT8 *) OldCoreData->TempFileHandles - OldCoreData->HeapOffset);
      }

      //
      // Fixup for PeiService's address
      //
      SetPeiServicesTablePointer ((CONST EFI_PEI_SERVICES **)&OldCoreData->Ps);

      //
      // Initialize libraries that the PEI Core is linked against
      //
      ProcessLibraryConstructorList (NULL, (CONST EFI_PEI_SERVICES **)&OldCoreData->Ps);

      //
      // Update HandOffHob for new installed permanent memory
      //
      HandoffInformationTable = OldCoreData->HobList.HandoffInformationTable;
      if (OldCoreData->HeapOffsetPositive) {
        HandoffInformationTable->EfiEndOfHobList   = HandoffInformationTable->EfiEndOfHobList + OldCoreData->HeapOffset;
      } else {
        HandoffInformationTable->EfiEndOfHobList   = HandoffInformationTable->EfiEndOfHobList - OldCoreData->HeapOffset;
      }
      HandoffInformationTable->EfiMemoryTop        = OldCoreData->PhysicalMemoryBegin + OldCoreData->PhysicalMemoryLength;
      HandoffInformationTable->EfiMemoryBottom     = OldCoreData->PhysicalMemoryBegin;
      HandoffInformationTable->EfiFreeMemoryTop    = OldCoreData->FreePhysicalMemoryTop;
      HandoffInformationTable->EfiFreeMemoryBottom = HandoffInformationTable->EfiEndOfHobList + sizeof (EFI_HOB_GENERIC_HEADER);

      //
      // We need convert MemoryBaseAddress in memory allocation HOBs
      //
      ConvertMemoryAllocationHobs (OldCoreData);

      //
      // We need convert the PPI descriptor's pointer
      //
      ConvertPpiPointers (SecCoreData, OldCoreData);

      //
      // After the whole temporary memory is migrated, then we can allocate page in
      // permanent memory.
      //
      OldCoreData->PeiMemoryInstalled = TRUE;

      //
      // Indicate that PeiCore reenter
      //
      OldCoreData->PeimDispatcherReenter = TRUE;

      if (PcdGet64(PcdLoadModuleAtFixAddressEnable) != 0 && (OldCoreData->HobList.HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME)) {
        //
        // if Loading Module at Fixed Address is enabled, allocate the PEI code memory range usage bit map array.
        // Every bit in the array indicate the status of the corresponding memory page available or not
        //
        OldCoreData->PeiCodeMemoryRangeUsageBitMap = AllocateZeroPool (((PcdGet32(PcdLoadFixAddressPeiCodePageNumber)>>6) + 1)*sizeof(UINT64));
      }

      //
      // Shadow PEI Core. When permanent memory is available, shadow
      // PEI Core and PEIMs to get high performance.
      //
      OldCoreData->ShadowedPeiCore = (PEICORE_FUNCTION_POINTER) (UINTN) PeiCore;
      if (PcdGetBool (PcdMigrateTemporaryRamFirmwareVolumes) ||
          (HandoffInformationTable->BootMode == BOOT_ON_S3_RESUME && PcdGetBool (PcdShadowPeimOnS3Boot)) ||
          (HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME && PcdGetBool (PcdShadowPeimOnBoot))) {
        OldCoreData->ShadowedPeiCore = ShadowPeiCore (OldCoreData);
      }

      //
      // PEI Core has now been shadowed to memory.  Restart PEI Core in memory.
      //
      OldCoreData->ShadowedPeiCore (SecCoreData, PpiList, OldCoreData);

      //
      // Should never reach here.
      //
      ASSERT (FALSE);
      CpuDeadLoop();

      UNREACHABLE ();
    }

    //
    // Memory is available to the PEI Core and the PEI Core has been shadowed to memory.
    //
    CopyMem (&NewSecCoreData, SecCoreDataPtr, sizeof (NewSecCoreData));
    SecCoreData = &NewSecCoreData;

    CopyMem (&PrivateData, OldCoreData, sizeof (PrivateData));

    CpuIo = (VOID*)PrivateData.ServiceTableShadow.CpuIo;
    PciCfg = (VOID*)PrivateData.ServiceTableShadow.PciCfg;

    CopyMem (&PrivateData.ServiceTableShadow, &gPs, sizeof (gPs));

    PrivateData.ServiceTableShadow.CpuIo  = CpuIo;
    PrivateData.ServiceTableShadow.PciCfg = PciCfg;
  }

  //
  // Cache a pointer to the PEI Services Table that is either in temporary memory or permanent memory
  //
  PrivateData.Ps = &PrivateData.ServiceTableShadow;

  //
  // Save PeiServicePointer so that it can be retrieved anywhere.
  //
  SetPeiServicesTablePointer ((CONST EFI_PEI_SERVICES **)&PrivateData.Ps);

  //
  // Initialize libraries that the PEI Core is linked against
  //
  ProcessLibraryConstructorList (NULL, (CONST EFI_PEI_SERVICES **)&PrivateData.Ps);

  //
  // Initialize PEI Core Services
  //
  InitializeMemoryServices   (&PrivateData, SecCoreData, OldCoreData);

  //
  // Update performance measurements
  //
  if (OldCoreData == NULL) {
    PERF_EVENT ("SEC"); // Means the end of SEC phase.

    //
    // If first pass, start performance measurement.
    //
    PERF_CROSSMODULE_BEGIN ("PEI");
    PERF_INMODULE_BEGIN ("PreMem");

  } else {
    PERF_INMODULE_END ("PreMem");
    PERF_INMODULE_BEGIN ("PostMem");
  }

  //
  // Complete PEI Core Service initialization
  //
  InitializeSecurityServices (&PrivateData.Ps, OldCoreData);
  InitializeDispatcherData   (&PrivateData,    OldCoreData, SecCoreData);
  InitializeImageServices    (&PrivateData,    OldCoreData);

  //
  // Perform PEI Core Phase specific actions
  //
  if (OldCoreData == NULL) {
    //
    // Report Status Code EFI_SW_PC_INIT
    //
    REPORT_STATUS_CODE (
      EFI_PROGRESS_CODE,
      (EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT)
      );

    //
    // If SEC provided the PpiList, process it.
    //
    if (PpiList != NULL) {
      ProcessPpiListFromSec ((CONST EFI_PEI_SERVICES **) &PrivateData.Ps, PpiList);
    }
  } else {
    if (PcdGetBool (PcdMigrateTemporaryRamFirmwareVolumes)) {
      //
      // When PcdMigrateTemporaryRamFirmwareVolumes is TRUE, alway shadow all
      // PEIMs no matter the condition of PcdShadowPeimOnBoot and PcdShadowPeimOnS3Boot
      //
      DEBUG ((DEBUG_VERBOSE, "PPI lists before temporary RAM evacuation:\n"));
      DumpPpiList (&PrivateData);

      //
      // Migrate installed content from Temporary RAM to Permanent RAM
      //
      EvacuateTempRam (&PrivateData, SecCoreData);

      DEBUG ((DEBUG_VERBOSE, "PPI lists after temporary RAM evacuation:\n"));
      DumpPpiList (&PrivateData);
    }

    //
    // Try to locate Temporary RAM Done Ppi.
    //
    Status = PeiServicesLocatePpi (
               &gEfiTemporaryRamDonePpiGuid,
               0,
               NULL,
               (VOID**)&TemporaryRamDonePpi
               );
    if (!EFI_ERROR (Status)) {
      //
      // Disable the use of Temporary RAM after the transition from Temporary RAM to Permanent RAM is complete.
      //
      TemporaryRamDonePpi->TemporaryRamDone ();
    }

    //
    // Alert any listeners that there is permanent memory available
    //
    PERF_INMODULE_BEGIN ("DisMem");
    Status = PeiServicesInstallPpi (&mMemoryDiscoveredPpi);

    //
    // Process the Notify list and dispatch any notifies for the Memory Discovered PPI
    //
    ProcessDispatchNotifyList (&PrivateData);

    PERF_INMODULE_END ("DisMem");
  }

  //
  // Call PEIM dispatcher
  //
  PeiDispatcher (SecCoreData, &PrivateData);

  if (PrivateData.HobList.HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME) {
    //
    // Check if InstallPeiMemory service was called on non-S3 resume boot path.
    //
    ASSERT(PrivateData.PeiMemoryInstalled == TRUE);
  }

  //
  // Measure PEI Core execution time.
  //
  PERF_INMODULE_END ("PostMem");

  //
  // Lookup DXE IPL PPI
  //
  Status = PeiServicesLocatePpi (
             &gEfiDxeIplPpiGuid,
             0,
             NULL,
             (VOID **)&TempPtr.DxeIpl
             );
  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    //
    // Report status code to indicate DXE IPL PPI could not be found.
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MAJOR,
      (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_EC_DXEIPL_NOT_FOUND)
      );
    CpuDeadLoop ();
  }

  //
  // Enter DxeIpl to load Dxe core.
  //
  DEBUG ((EFI_D_INFO, "DXE IPL Entry\n"));
  Status = TempPtr.DxeIpl->Entry (
                             TempPtr.DxeIpl,
                             &PrivateData.Ps,
                             PrivateData.HobList
                             );
  //
  // Should never reach here.
  //
  ASSERT_EFI_ERROR (Status);
  CpuDeadLoop();

  UNREACHABLE ();
}
