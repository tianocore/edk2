/** @file
  Pei Core Main Entry Point
  
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PeiMain.h>

STATIC EFI_PEI_PPI_DESCRIPTOR mMemoryDiscoveredPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  NULL
};

///
/// Pei Core Module Variables
///
///
STATIC EFI_PEI_SERVICES  gPs = {
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
  PeiResetSystem,

  NULL,
  NULL,

  PeiFfsFindFileByName,
  PeiFfsGetFileInfo,
  PeiFfsGetVolumeInfo,
  PeiRegisterForShadow
};

/**

  The entry routine to Pei Core, invoked by PeiMain during transition
  from SEC to PEI. After switching stack in the PEI core, it will restart
  with the old core data.


  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
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

  @retval EFI_NOT_FOUND  Never reach

**/
EFI_STATUS
EFIAPI
PeiCore (
  IN CONST EFI_SEC_PEI_HAND_OFF        *SecCoreData,
  IN CONST EFI_PEI_PPI_DESCRIPTOR      *PpiList,
  IN VOID                              *Data
  )
{
  PEI_CORE_INSTANCE                                     PrivateData;
  EFI_STATUS                                            Status;
  PEI_CORE_TEMP_POINTERS                                TempPtr;
  UINT64                                                mTick;
  PEI_CORE_INSTANCE                                     *OldCoreData;
  EFI_PEI_CPU_IO_PPI                                    *CpuIo;
  EFI_PEI_PCI_CFG2_PPI                                  *PciCfg;
  PEICORE_FUNCTION_POINTER                              ShadowedPeiCore;

  mTick = 0;
  OldCoreData = (PEI_CORE_INSTANCE *) Data;

  //
  // Record the system tick for first entering PeiCore.
  // This tick is duration of executing platform seccore module.
  // 
  if (PerformanceMeasurementEnabled()) {
    if (OldCoreData == NULL) {
      mTick = GetPerformanceCounter ();
    }
  }

  //
  // PeiCore has been shadowed to memory for first entering, so
  // just jump to PeiCore in memory here.
  //
  if (OldCoreData != NULL) {
    ShadowedPeiCore = (PEICORE_FUNCTION_POINTER) (UINTN) OldCoreData->ShadowedPeiCore;
    if (ShadowedPeiCore != NULL) {
      OldCoreData->ShadowedPeiCore = NULL;
      ShadowedPeiCore (
        SecCoreData,
        PpiList,
        OldCoreData
        );
    }

    CopyMem (&PrivateData, OldCoreData, sizeof (PEI_CORE_INSTANCE));
    
    CpuIo = (VOID*)PrivateData.ServiceTableShadow.CpuIo;
    PciCfg = (VOID*)PrivateData.ServiceTableShadow.PciCfg;
    
    CopyMem (&PrivateData.ServiceTableShadow, &gPs, sizeof (gPs));
    
    PrivateData.ServiceTableShadow.CpuIo  = CpuIo;
    PrivateData.ServiceTableShadow.PciCfg = PciCfg;
  } else {
    ZeroMem (&PrivateData, sizeof (PEI_CORE_INSTANCE));
    PrivateData.Signature = PEI_CORE_HANDLE_SIGNATURE;
    CopyMem (&PrivateData.ServiceTableShadow, &gPs, sizeof (gPs));
  }

  PrivateData.PS = &PrivateData.ServiceTableShadow;

  //
  // Initialize libraries that the PeiCore is linked against
  //
  ProcessLibraryConstructorList (NULL, &PrivateData.PS);

  InitializeMemoryServices (&PrivateData, SecCoreData, OldCoreData);

  InitializePpiServices (&PrivateData, OldCoreData);

  //
  // Save PeiServicePointer so that it can be retrieved anywhere.
  //
  SetPeiServicesTablePointer(&PrivateData.PS);
  
  if (OldCoreData != NULL) {

    PERF_END (NULL,"PreMem", NULL, 0);
    PERF_START (NULL,"PostMem", NULL, 0);

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
      FixedPcdGet32 (PcdStatusCodeValuePeiCoreEntry)
      );

    PERF_START (NULL,"PEI", NULL, mTick);
    //
    // If first pass, start performance measurement.
    //
    PERF_START (NULL,"PreMem", NULL, mTick);

    //
    // If SEC provided any PPI services to PEI, install them.
    //
    if (PpiList != NULL) {
      Status = PeiServicesInstallPpi (PpiList);
      ASSERT_EFI_ERROR (Status);
    }
  }

  InitializeSecurityServices (&PrivateData.PS, OldCoreData);

  InitializeDispatcherData (&PrivateData, OldCoreData, SecCoreData);

  //
  // Install Pei Load File PPI. 
  //
  InitializeImageServices (&PrivateData, OldCoreData);

  //
  // Call PEIM dispatcher
  //
  PeiDispatcher (SecCoreData, &PrivateData);

  //
  // Check if InstallPeiMemory service was called.
  //
  ASSERT(PrivateData.PeiMemoryInstalled == TRUE);

  //
  // Till now, PEI phase will be finished, get performace count
  // for computing duration of PEI phase
  //
  PERF_END (NULL, "PostMem", NULL, 0);

  Status = PeiServicesLocatePpi (
             &gEfiDxeIplPpiGuid,
             0,
             NULL,
             (VOID **)&TempPtr.DxeIpl
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Enter DxeIpl to load Dxe core.
  //
  DEBUG ((EFI_D_INFO, "DXE IPL Entry\n"));
  Status = TempPtr.DxeIpl->Entry (
                             TempPtr.DxeIpl,
                             &PrivateData.PS,
                             PrivateData.HobList
                             );

  ASSERT_EFI_ERROR (Status);

  return EFI_NOT_FOUND;
}


