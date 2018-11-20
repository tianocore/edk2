/** @file
  This will be invoked only once. It will call FspMemoryInit API,
  register TemporaryRamDonePpi to call TempRamExit API, and register MemoryDiscoveredPpi
  notify to call FspSiliconInit API.

  Copyright (c) 2014 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <Library/FspWrapperHobProcessLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/FspWrapperApiLib.h>

#include <Ppi/FspSiliconInitDone.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/TemporaryRamDone.h>
#include <Ppi/SecPlatformInformation.h>
#include <Library/FspWrapperApiTestLib.h>
#include <FspEas.h>
#include <FspStatusCode.h>

extern EFI_PEI_NOTIFY_DESCRIPTOR mS3EndOfPeiNotifyDesc;
extern EFI_GUID                  gFspHobGuid;

/**
  This function handles S3 resume task at the end of PEI

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
S3EndOfPeiNotify(
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR mS3EndOfPeiNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  S3EndOfPeiNotify
};

/**
  This function handles S3 resume task at the end of PEI

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
S3EndOfPeiNotify(
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  )
{
  NOTIFY_PHASE_PARAMS NotifyPhaseParams;
  EFI_STATUS          Status;

  DEBUG((DEBUG_INFO, "S3EndOfPeiNotify enter\n"));

  NotifyPhaseParams.Phase = EnumInitPhaseAfterPciEnumeration;
  Status = CallFspNotifyPhase (&NotifyPhaseParams);
  DEBUG((DEBUG_INFO, "FSP S3NotifyPhase AfterPciEnumeration status: 0x%x\n", Status));

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG((DEBUG_INFO, "FSP S3NotifyPhase AfterPciEnumeration requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem ((UINT32)Status);
  }

  NotifyPhaseParams.Phase = EnumInitPhaseReadyToBoot;
  Status = CallFspNotifyPhase (&NotifyPhaseParams);
  DEBUG((DEBUG_INFO, "FSP S3NotifyPhase ReadyToBoot status: 0x%x\n", Status));

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG((DEBUG_INFO, "FSP S3NotifyPhase ReadyToBoot requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem ((UINT32)Status);
  }

  NotifyPhaseParams.Phase = EnumInitPhaseEndOfFirmware;
  Status = CallFspNotifyPhase (&NotifyPhaseParams);
  DEBUG((DEBUG_INFO, "FSP S3NotifyPhase EndOfFirmware status: 0x%x\n", Status));

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG((DEBUG_INFO, "FSP S3NotifyPhase EndOfFirmware requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem ((UINT32)Status);
  }

  return EFI_SUCCESS;
}

/**
  Return Hob list produced by FSP.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of this PPI.
  @param[out] FspHobList   The pointer to Hob list produced by FSP.

  @return EFI_SUCCESS FReturn Hob list produced by FSP successfully.
**/
EFI_STATUS
EFIAPI
FspSiliconInitDoneGetFspHobList (
  IN  CONST EFI_PEI_SERVICES         **PeiServices,
  IN  FSP_SILICON_INIT_DONE_PPI      *This,
  OUT VOID                           **FspHobList
  );

FSP_SILICON_INIT_DONE_PPI mFspSiliconInitDonePpi = {
  FspSiliconInitDoneGetFspHobList
};

EFI_PEI_PPI_DESCRIPTOR            mPeiFspSiliconInitDonePpi = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gFspSiliconInitDonePpiGuid,
  &mFspSiliconInitDonePpi
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
FspSiliconInitDoneGetFspHobList (
  IN  CONST EFI_PEI_SERVICES         **PeiServices,
  IN  FSP_SILICON_INIT_DONE_PPI      *This,
  OUT VOID                           **FspHobList
  )
{
  EFI_HOB_GUID_TYPE                  *GuidHob;

  GuidHob = GetFirstGuidHob (&gFspHobGuid);
  if (GuidHob != NULL) {
    *FspHobList = *(VOID **)GET_GUID_HOB_DATA(GuidHob);
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
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
  );

EFI_PEI_NOTIFY_DESCRIPTOR mPeiMemoryDiscoveredNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  PeiMemoryDiscoveredNotify
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
  )
{
  FSP_INFO_HEADER           *FspsHeaderPtr;
  UINT64                    TimeStampCounterStart;
  EFI_STATUS                Status;
  VOID                      *FspHobListPtr;
  EFI_HOB_GUID_TYPE         *GuidHob;
  FSPS_UPD_COMMON           *FspsUpdDataPtr;
  UINTN                     *SourceData;

  DEBUG ((DEBUG_INFO, "PeiMemoryDiscoveredNotify enter\n"));
  FspsUpdDataPtr = NULL;

  FspsHeaderPtr = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspsBaseAddress));
  DEBUG ((DEBUG_INFO, "FspsHeaderPtr - 0x%x\n", FspsHeaderPtr));
  if (FspsHeaderPtr == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (PcdGet32 (PcdFspsUpdDataAddress) == 0 && (FspsHeaderPtr->CfgRegionSize != 0) && (FspsHeaderPtr->CfgRegionOffset != 0)) {
    //
    // Copy default FSP-S UPD data from Flash
    //
    FspsUpdDataPtr = (FSPS_UPD_COMMON *)AllocateZeroPool ((UINTN)FspsHeaderPtr->CfgRegionSize);
    ASSERT (FspsUpdDataPtr != NULL);
    SourceData = (UINTN *)((UINTN)FspsHeaderPtr->ImageBase + (UINTN)FspsHeaderPtr->CfgRegionOffset);
    CopyMem (FspsUpdDataPtr, SourceData, (UINTN)FspsHeaderPtr->CfgRegionSize);
  } else {
    FspsUpdDataPtr = (FSPS_UPD_COMMON *)PcdGet32 (PcdFspsUpdDataAddress);
    ASSERT (FspsUpdDataPtr != NULL);
  }

  UpdateFspsUpdData ((VOID *)FspsUpdDataPtr);

  TimeStampCounterStart = AsmReadTsc ();
  PERF_START_EX(&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  Status = CallFspSiliconInit ((VOID *)FspsUpdDataPtr);
  PERF_END_EX(&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  DEBUG ((DEBUG_INFO, "Total time spent executing FspSiliconInitApi: %d millisecond\n", DivU64x32 (GetTimeInNanoSecond (AsmReadTsc () - TimeStampCounterStart), 1000000)));

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG((DEBUG_INFO, "FspSiliconInitApi requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem ((UINT32)Status);
  }

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - Failed to execute FspSiliconInitApi(), Status = %r\n", Status));
  }
  DEBUG((DEBUG_INFO, "FspSiliconInit status: 0x%x\n", Status));
  ASSERT_EFI_ERROR (Status);

  Status = TestFspSiliconInitApiOutput ((VOID *)NULL);
  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - TestFspSiliconInitApiOutput () fail, Status = %r\n", Status));
  }

  //
  // Now FspHobList complete, process it
  //
  GuidHob = GetFirstGuidHob (&gFspHobGuid);
  ASSERT (GuidHob != NULL);
  FspHobListPtr = *(VOID **)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "FspHobListPtr - 0x%x\n", FspHobListPtr));
  PostFspsHobProcess (FspHobListPtr);

  //
  // Install FspSiliconInitDonePpi so that any other driver can consume this info.
  //
  Status = PeiServicesInstallPpi (&mPeiFspSiliconInitDonePpi);
  ASSERT_EFI_ERROR(Status);

  return Status;
}

/**
  Do FSP initialization.

  @return FSP initialization status.
**/
EFI_STATUS
FspsWrapperInit (
  VOID
  )
{
  EFI_STATUS           Status;
  EFI_BOOT_MODE        BootMode;

  //
  // Register MemoryDiscovered Nofity to run FspSiliconInit
  //
  Status = PeiServicesNotifyPpi (&mPeiMemoryDiscoveredNotifyDesc);
  ASSERT_EFI_ERROR (Status);

  //
  // Register EndOfPei Notify for S3 to run FSP NotifyPhase
  //
  PeiServicesGetBootMode (&BootMode);
  if (BootMode == BOOT_ON_S3_RESUME) {
    Status = PeiServicesNotifyPpi (&mS3EndOfPeiNotifyDesc);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  This is the entrypoint of PEIM

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
FspsWrapperPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  DEBUG ((DEBUG_INFO, "FspsWrapperPeimEntryPoint\n"));

  FspsWrapperInit ();

  return EFI_SUCCESS;
}
