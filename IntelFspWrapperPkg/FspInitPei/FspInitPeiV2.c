/** @file
  In FSP API V2 mode, it will be invoked only once. It will call FspMemoryInit API,
  register TemporaryRamDonePpi to call TempRamExit API, and register MemoryDiscoveredPpi
  notify to call FspSiliconInit API.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "FspInitPei.h"

/**
  Return Hob list produced by FSP.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of this PPI.
  @param[out] FspHobList   The pointer to Hob list produced by FSP.

  @return EFI_SUCCESS FReturn Hob list produced by FSP successfully.
**/
EFI_STATUS
EFIAPI
FspInitDoneGetFspHobListV2 (
  IN  CONST EFI_PEI_SERVICES         **PeiServices,
  IN  FSP_INIT_DONE_PPI              *This,
  OUT VOID                           **FspHobList
  );

FSP_INIT_DONE_PPI mFspInitDonePpiV2 = {
  FspInitDoneGetFspHobListV2
};

EFI_PEI_PPI_DESCRIPTOR            mPeiFspInitDonePpiV2 = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gFspInitDonePpiGuid,
  &mFspInitDonePpiV2
};

/**
  This function is called after PEI core discover memory and finish migration.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
PeiMemoryDiscoveredNotify (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR mPeiMemoryDiscoveredNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  PeiMemoryDiscoveredNotify
};

/**
  TemporaryRamDone() disables the use of Temporary RAM. If present, this service is invoked
  by the PEI Foundation after the EFI_PEI_PERMANANT_MEMORY_INSTALLED_PPI is installed.

  @retval EFI_SUCCESS           Use of Temporary RAM was disabled.
  @retval EFI_INVALID_PARAMETER Temporary RAM could not be disabled.

**/
EFI_STATUS
EFIAPI
PeiTemporaryRamDone (
  VOID
  );

EFI_PEI_TEMPORARY_RAM_DONE_PPI mPeiTemporaryRamDonePpi = {
  PeiTemporaryRamDone
};

EFI_PEI_PPI_DESCRIPTOR         mPeiTemporaryRamDoneDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiTemporaryRamDonePpiGuid,
  &mPeiTemporaryRamDonePpi
};

/**
  Return Hob list produced by FSP.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of this PPI.
  @param[out] FspHobList   The pointer to Hob list produced by FSP.

  @return EFI_SUCCESS FReturn Hob list produced by FSP successfully.
**/
EFI_STATUS
EFIAPI
FspInitDoneGetFspHobListV2 (
  IN  CONST EFI_PEI_SERVICES         **PeiServices,
  IN  FSP_INIT_DONE_PPI              *This,
  OUT VOID                           **FspHobList
  )
{
  EFI_HOB_GUID_TYPE                  *GuidHob;

  GuidHob = GetFirstGuidHob (&gFspInitDonePpiGuid);
  if (GuidHob != NULL) {
    *FspHobList = *(VOID **)GET_GUID_HOB_DATA (GuidHob);
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  Call FspMemoryInit API.

  @param[in]  FspHeader   FSP header pointer.

  @return Status returned by FspMemoryInit API.
**/
EFI_STATUS
PeiFspMemoryInit (
  IN FSP_INFO_HEADER *FspHeader
  )
{
  FSP_MEMORY_INIT_PARAMS    FspMemoryInitParams;
  FSP_INIT_RT_COMMON_BUFFER FspRtBuffer;
  UINT8                     FspUpdRgn[FixedPcdGet32 (PcdMaxUpdRegionSize)];
  UINT32                    UpdRegionSize;
  EFI_BOOT_MODE             BootMode;
  UINT64                    StackSize;
  EFI_PHYSICAL_ADDRESS      StackBase;
  EFI_STATUS                Status;
  VOID                      *FspHobList;
  VOID                      *HobData;

  DEBUG ((DEBUG_INFO, "PeiFspMemoryInit enter\n"));

  PeiServicesGetBootMode (&BootMode);
  DEBUG ((DEBUG_INFO, "BootMode - 0x%x\n", BootMode));

  GetStackInfo (BootMode, FALSE, &StackBase, &StackSize);
  DEBUG ((DEBUG_INFO, "StackBase - 0x%x\n", StackBase));
  DEBUG ((DEBUG_INFO, "StackSize - 0x%x\n", StackSize));

  ZeroMem (&FspRtBuffer, sizeof(FspRtBuffer));
  FspRtBuffer.StackTop = (UINT32 *)(UINTN)(StackBase + StackSize);

  FspRtBuffer.BootMode = BootMode;

  /* Platform override any UPD configs */
  UpdRegionSize = GetUpdRegionSize();
  DEBUG ((DEBUG_INFO, "UpdRegionSize - 0x%x\n", UpdRegionSize));
  DEBUG ((DEBUG_INFO, "sizeof(FspUpdRgn) - 0x%x\n", sizeof(FspUpdRgn)));
  ASSERT(sizeof(FspUpdRgn) >= UpdRegionSize);
  ZeroMem (FspUpdRgn, UpdRegionSize);
  FspRtBuffer.UpdDataRgnPtr = UpdateFspUpdConfigs (FspUpdRgn);
  FspRtBuffer.BootLoaderTolumSize = GetBootLoaderTolumSize ();

  ZeroMem (&FspMemoryInitParams, sizeof(FspMemoryInitParams));
  FspMemoryInitParams.NvsBufferPtr = GetNvsBuffer ();
  DEBUG ((DEBUG_INFO, "NvsBufferPtr - 0x%x\n", FspMemoryInitParams.NvsBufferPtr));
  FspMemoryInitParams.RtBufferPtr  = (VOID *)&FspRtBuffer;
  FspHobList = NULL;
  FspMemoryInitParams.HobListPtr   = &FspHobList;

  DEBUG ((DEBUG_INFO, "FspMemoryInitParams - 0x%x\n", &FspMemoryInitParams));
  DEBUG ((DEBUG_INFO, "  NvsBufferPtr      - 0x%x\n", FspMemoryInitParams.NvsBufferPtr));
  DEBUG ((DEBUG_INFO, "  RtBufferPtr       - 0x%x\n", FspMemoryInitParams.RtBufferPtr));
  DEBUG ((DEBUG_INFO, "    StackTop        - 0x%x\n", FspRtBuffer.StackTop));
  DEBUG ((DEBUG_INFO, "    BootMode        - 0x%x\n", FspRtBuffer.BootMode));
  DEBUG ((DEBUG_INFO, "    UpdDataRgnPtr   - 0x%x\n", FspRtBuffer.UpdDataRgnPtr));
  DEBUG ((DEBUG_INFO, "  HobListPtr        - 0x%x\n", FspMemoryInitParams.HobListPtr));

  Status = CallFspMemoryInit (FspHeader, &FspMemoryInitParams);
  DEBUG((DEBUG_INFO, "FspMemoryInit status: 0x%x\n", Status));
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "  HobListPtr (returned) - 0x%x\n", FspHobList));
  ASSERT (FspHobList != NULL);

  FspHobProcessForMemoryResource (FspHobList);

  //
  // FspHobList is not complete at this moment.
  // Save FspHobList pointer to hob, so that it can be got later
  //
  HobData = BuildGuidHob (
             &gFspInitDonePpiGuid,
             sizeof (VOID *)
             );
  ASSERT (HobData != NULL);
  CopyMem (HobData, &FspHobList, sizeof (FspHobList));

  return Status;
}

/**
  TemporaryRamDone() disables the use of Temporary RAM. If present, this service is invoked
  by the PEI Foundation after the EFI_PEI_PERMANANT_MEMORY_INSTALLED_PPI is installed.

  @retval EFI_SUCCESS           Use of Temporary RAM was disabled.
  @retval EFI_INVALID_PARAMETER Temporary RAM could not be disabled.

**/
EFI_STATUS
EFIAPI
PeiTemporaryRamDone (
  VOID
  )
{
  EFI_STATUS                Status;
  VOID                      *TempRamExitParam;
  FSP_INFO_HEADER           *FspHeader;

  FspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvFspBase));
  if (FspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }
  
  DEBUG ((DEBUG_INFO, "PeiTemporaryRamDone enter\n"));
  
  TempRamExitParam = GetTempRamExitParam ();
  Status = CallTempRamExit (FspHeader, TempRamExitParam);
  DEBUG((DEBUG_INFO, "TempRamExit status: 0x%x\n", Status));
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  This function is called after PEI core discover memory and finish migration.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
PeiMemoryDiscoveredNotify (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  )
{
  EFI_STATUS                Status;
  VOID                      *FspSiliconInitParam;
  FSP_INFO_HEADER           *FspHeader;
  VOID                      *FspHobList;
  EFI_HOB_GUID_TYPE         *GuidHob;

  if (PcdGet32 (PcdFlashFvSecondFspBase) == 0) {
    FspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvFspBase));
  } else {
    FspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvSecondFspBase));
  }
  if (FspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }
  
  DEBUG ((DEBUG_INFO, "PeiMemoryDiscoveredNotify enter\n"));
  
  FspSiliconInitParam = GetFspSiliconInitParam ();
  Status = CallFspSiliconInit (FspHeader, FspSiliconInitParam);
  DEBUG((DEBUG_ERROR, "FspSiliconInit status: 0x%x\n", Status));
  ASSERT_EFI_ERROR (Status);
  
  //
  // Now FspHobList complete, process it
  //
  GuidHob = GetFirstGuidHob (&gFspInitDonePpiGuid);
  ASSERT (GuidHob != NULL);
  FspHobList = *(VOID **)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "FspHobList - 0x%x\n", FspHobList));
  FspHobProcessForOtherData (FspHobList);

  //
  // Install FspInitDonePpi so that any other driver can consume this info.
  //
  Status = PeiServicesInstallPpi (&mPeiFspInitDonePpiV2);
  ASSERT_EFI_ERROR(Status);

  return EFI_SUCCESS;
}

/**
  Do FSP initialization based on FspApi version 2.

  @param[in] FspHeader FSP header pointer.

  @return FSP initialization status.
**/
EFI_STATUS
PeiFspInitV2 (
  IN FSP_INFO_HEADER *FspHeader
  )
{
  EFI_STATUS           Status;
  EFI_BOOT_MODE        BootMode;

  Status = PeiFspMemoryInit (FspHeader);
  ASSERT_EFI_ERROR (Status);

  //
  // Install TempramDonePpi to run TempRamExit
  //
  Status = PeiServicesInstallPpi (&mPeiTemporaryRamDoneDesc);
  ASSERT_EFI_ERROR(Status);

  //
  // Register MemoryDiscovered Nofity to run FspSiliconInit
  //
  Status = PeiServicesNotifyPpi (&mPeiMemoryDiscoveredNotifyDesc);
  ASSERT_EFI_ERROR (Status);
      
  //
  // Register EndOfPei Notify for S3 to run FspNotifyPhase
  //
  PeiServicesGetBootMode (&BootMode);
  if (BootMode == BOOT_ON_S3_RESUME) {
    Status = PeiServicesNotifyPpi (&mS3EndOfPeiNotifyDesc);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}