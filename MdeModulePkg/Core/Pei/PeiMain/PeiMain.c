/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PeiMain.c

Abstract:

  Pei Core Main Entry Point

Revision History

--*/

#include <PeiMain.h>

//
//CAR is filled with this initial value during SEC phase
//
#define INIT_CAR_VALUE 0x5AA55AA5

static EFI_PEI_PPI_DESCRIPTOR mMemoryDiscoveredPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  NULL
};

//
// Pei Core Module Variables
//
//
static EFI_PEI_SERVICES  mPS = {
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

  PeiFvFindNextVolume,
  PeiFfsFindNextFile,
  PeiFfsFindSectionData,

  PeiInstallPeiMemory,
  PeiAllocatePages,
  PeiAllocatePool,
  (EFI_PEI_COPY_MEM)CopyMem,
  (EFI_PEI_SET_MEM)SetMem,

  PeiReportStatusCode,

  PeiResetSystem
};

EFI_STATUS
EFIAPI
PeiCore (
  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,
  IN VOID                        *Data
  )
/*++

Routine Description:

  The entry routine to Pei Core, invoked by PeiMain during transition
  from SEC to PEI. After switching stack in the PEI core, it will restart
  with the old core data.

Arguments:

  PeiStartupDescriptor - Information and services provided by SEC phase.
  OldCoreData          - Pointer to old core data that is used to initialize the
                         core's data areas.

Returns:

  This function never returns
  EFI_NOT_FOUND        - Never reach

--*/
{
  PEI_CORE_INSTANCE                                     PrivateData;
  EFI_STATUS                                            Status;
  PEI_CORE_TEMP_POINTERS                                TempPtr;
  PEI_CORE_DISPATCH_DATA                                *DispatchData;
  UINT64                                                mTick;
  PEI_CORE_INSTANCE                                     *OldCoreData;

  mTick = 0;
  OldCoreData = (PEI_CORE_INSTANCE *) Data;

  if (PerformanceMeasurementEnabled()) {
    if (OldCoreData == NULL) {
      mTick = GetPerformanceCounter ();
    }
  }

  //
  // For IPF in CAR mode the real memory access is uncached,in InstallPeiMemory()
  //  the 63-bit of address is set to 1.
  //
  SWITCH_TO_CACHE_MODE (OldCoreData);

  if (OldCoreData != NULL) {
    CopyMem (&PrivateData, OldCoreData, sizeof (PEI_CORE_INSTANCE));
  } else {
    ZeroMem (&PrivateData, sizeof (PEI_CORE_INSTANCE));
  }

  PrivateData.Signature = PEI_CORE_HANDLE_SIGNATURE;
  PrivateData.PS = &mPS;

  //
  // Initialize libraries that the PeiCore is linked against
  // BUGBUG: The FfsHeader is passed in as NULL.  Do we look it up or remove it from the lib init?
  //
  ProcessLibraryConstructorList (NULL, &PrivateData.PS);

  InitializeMemoryServices (&PrivateData.PS, PeiStartupDescriptor, OldCoreData);

  InitializePpiServices (&PrivateData.PS, OldCoreData);

  InitializeSecurityServices (&PrivateData.PS, OldCoreData);

  InitializeDispatcherData (&PrivateData.PS, OldCoreData, PeiStartupDescriptor);

  if (OldCoreData != NULL) {

    PERF_END (NULL,"PreMem", NULL, 0);
    PERF_START (NULL,"PostMem", NULL, 0);

    //
    // The following code dumps out interesting cache as RAM usage information
    // so we can keep tabs on how the cache as RAM is being utilized.  The
    // DEBUG_CODE_BEGIN macro is used to prevent this code from being compiled
    // on a debug build.
    //
    DEBUG_CODE_BEGIN ();
      UINTN  *StackPointer;
      UINTN  StackValue;

      StackValue = INIT_CAR_VALUE;
      for (StackPointer = (UINTN *) OldCoreData->MaxTopOfCarHeap;
           ((UINTN) StackPointer < ((UINTN) OldCoreData->BottomOfCarHeap + OldCoreData->SizeOfCacheAsRam))
           && StackValue == INIT_CAR_VALUE;
           StackPointer++) {
        StackValue = *StackPointer;
      }

      DEBUG ((EFI_D_INFO, "Total Cache as RAM:    %d bytes.\n", OldCoreData->SizeOfCacheAsRam));
      DEBUG ((EFI_D_INFO, "  CAR stack ever used: %d bytes.\n",
        ((UINTN) OldCoreData->TopOfCarHeap - (UINTN) StackPointer)
        ));
      DEBUG ((EFI_D_INFO, "  CAR heap used:       %d bytes.\n",
        ((UINTN) OldCoreData->HobList.HandoffInformationTable->EfiFreeMemoryBottom -
        (UINTN) OldCoreData->HobList.Raw)
        ));
    DEBUG_CODE_END ();

    //
    // Alert any listeners that there is permanent memory available
    //
    
    PERF_START (NULL,"DisMem", NULL, 0);
    Status = PeiServicesInstallPpi (&mMemoryDiscoveredPpi);
    PERF_END (NULL,"DisMem", NULL, 0);

  } else {

    //
    // Report Status Code EFI_SW_PC_INIT
    //
    REPORT_STATUS_CODE (
      EFI_PROGRESS_CODE,
      EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT
      );

    PERF_START (NULL,"PEI", NULL, mTick);
    //
    // If first pass, start performance measurement.
    //
    PERF_START (NULL,"PreMem", NULL, mTick);

    //
    // If SEC provided any PPI services to PEI, install them.
    //
    if (PeiStartupDescriptor->DispatchTable != NULL) {
      Status = PeiServicesInstallPpi (PeiStartupDescriptor->DispatchTable);
      ASSERT_EFI_ERROR (Status);
    }
  }

  DispatchData = &PrivateData.DispatchData;

  //
  // Call PEIM dispatcher
  //
  PeiDispatcher (PeiStartupDescriptor, &PrivateData, DispatchData);

  //
  // Check if InstallPeiMemory service was called.
  //
  ASSERT(PrivateData.PeiMemoryInstalled == TRUE);

  PERF_END (NULL, "PostMem", NULL, 0);

  Status = PeiServicesLocatePpi (
             &gEfiDxeIplPpiGuid,
             0,
             NULL,
             (VOID **)&TempPtr.DxeIpl
             );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_INFO, "DXE IPL Entry\n"));
  Status = TempPtr.DxeIpl->Entry (
                             TempPtr.DxeIpl,
                             &PrivateData.PS,
                             PrivateData.HobList
                             );

  ASSERT_EFI_ERROR (Status);

  return EFI_NOT_FOUND;
}

