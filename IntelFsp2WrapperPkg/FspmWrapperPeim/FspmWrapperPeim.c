/** @file
  This will be invoked only once. It will call FspMemoryInit API,
  register TemporaryRamDonePpi to call TempRamExit API, and register MemoryDiscoveredPpi
  notify to call FspSiliconInit API.

  Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiPei.h>

#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <Library/FspWrapperHobProcessLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/FspMeasurementLib.h>

#include <Ppi/FspSiliconInitDone.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/Tcg.h>
#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>
#include <Library/FspWrapperApiTestLib.h>
#include <FspEas.h>
#include <FspStatusCode.h>

extern EFI_GUID gFspHobGuid;

/**
  Call FspMemoryInit API.

  @return Status returned by FspMemoryInit API.
**/
EFI_STATUS
PeiFspMemoryInit (
  VOID
  )
{
  FSP_INFO_HEADER           *FspmHeaderPtr;
  EFI_STATUS                Status;
  UINT64                    TimeStampCounterStart;
  VOID                      *FspHobListPtr;
  VOID                      *HobData;
  FSPM_UPD_COMMON           *FspmUpdDataPtr;
  UINTN                     *SourceData;

  DEBUG ((DEBUG_INFO, "PeiFspMemoryInit enter\n"));

  FspHobListPtr = NULL;
  FspmUpdDataPtr = NULL;

  FspmHeaderPtr = (FSP_INFO_HEADER *) FspFindFspHeader (PcdGet32 (PcdFspmBaseAddress));
  DEBUG ((DEBUG_INFO, "FspmHeaderPtr - 0x%x\n", FspmHeaderPtr));
  if (FspmHeaderPtr == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (PcdGet32 (PcdFspmUpdDataAddress) == 0 && (FspmHeaderPtr->CfgRegionSize != 0) && (FspmHeaderPtr->CfgRegionOffset != 0)) {
    //
    // Copy default FSP-M UPD data from Flash
    //
    FspmUpdDataPtr = (FSPM_UPD_COMMON *)AllocateZeroPool ((UINTN)FspmHeaderPtr->CfgRegionSize);
    ASSERT (FspmUpdDataPtr != NULL);
    SourceData = (UINTN *)((UINTN)FspmHeaderPtr->ImageBase + (UINTN)FspmHeaderPtr->CfgRegionOffset);
    CopyMem (FspmUpdDataPtr, SourceData, (UINTN)FspmHeaderPtr->CfgRegionSize);
  } else {
    //
    // External UPD is ready, get the buffer from PCD pointer.
    //
    FspmUpdDataPtr = (FSPM_UPD_COMMON *)PcdGet32 (PcdFspmUpdDataAddress);
    ASSERT (FspmUpdDataPtr != NULL);
  }

  DEBUG ((DEBUG_INFO, "UpdateFspmUpdData enter\n"));
  UpdateFspmUpdData ((VOID *)FspmUpdDataPtr);
  DEBUG ((DEBUG_INFO, "  NvsBufferPtr        - 0x%x\n", FspmUpdDataPtr->FspmArchUpd.NvsBufferPtr));
  DEBUG ((DEBUG_INFO, "  StackBase           - 0x%x\n", FspmUpdDataPtr->FspmArchUpd.StackBase));
  DEBUG ((DEBUG_INFO, "  StackSize           - 0x%x\n", FspmUpdDataPtr->FspmArchUpd.StackSize));
  DEBUG ((DEBUG_INFO, "  BootLoaderTolumSize - 0x%x\n", FspmUpdDataPtr->FspmArchUpd.BootLoaderTolumSize));
  DEBUG ((DEBUG_INFO, "  BootMode            - 0x%x\n", FspmUpdDataPtr->FspmArchUpd.BootMode));
  DEBUG ((DEBUG_INFO, "  HobListPtr          - 0x%x\n", &FspHobListPtr));

  TimeStampCounterStart = AsmReadTsc ();
  Status = CallFspMemoryInit (FspmUpdDataPtr, &FspHobListPtr);
  // Create hobs after memory initialization and not in temp RAM. Hence passing the recorded timestamp here
  PERF_START_EX(&gFspApiPerformanceGuid, "EventRec", NULL, TimeStampCounterStart, FSP_STATUS_CODE_MEMORY_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  PERF_END_EX(&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_MEMORY_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  DEBUG ((DEBUG_INFO, "Total time spent executing FspMemoryInitApi: %d millisecond\n", DivU64x32 (GetTimeInNanoSecond (AsmReadTsc () - TimeStampCounterStart), 1000000)));

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG((DEBUG_INFO, "FspMemoryInitApi requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem ((UINT32)Status);
  }

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - Failed to execute FspMemoryInitApi(), Status = %r\n", Status));
  }
  DEBUG((DEBUG_INFO, "FspMemoryInit status: 0x%x\n", Status));
  ASSERT_EFI_ERROR (Status);


  Status = TestFspMemoryInitApiOutput (FspmUpdDataPtr, &FspHobListPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - TestFspMemoryInitApiOutput () fail, Status = %r\n", Status));
  }

  DEBUG ((DEBUG_INFO, "  FspHobListPtr (returned) - 0x%x\n", FspHobListPtr));
  ASSERT (FspHobListPtr != NULL);

  PostFspmHobProcess (FspHobListPtr);

  //
  // FspHobList is not complete at this moment.
  // Save FspHobList pointer to hob, so that it can be got later
  //
  HobData = BuildGuidHob (
             &gFspHobGuid,
             sizeof (VOID *)
             );
  ASSERT (HobData != NULL);
  CopyMem (HobData, &FspHobListPtr, sizeof (FspHobListPtr));

  return Status;
}

/**
  Do FSP initialization.

  @return FSP initialization status.
**/
EFI_STATUS
EFIAPI
FspmWrapperInit (
  VOID
  )
{
  EFI_STATUS                                            Status;
  EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI *MeasurementExcludedFvPpi;
  EFI_PEI_PPI_DESCRIPTOR                                *MeasurementExcludedPpiList;

  MeasurementExcludedFvPpi = AllocatePool (sizeof(*MeasurementExcludedFvPpi));
  ASSERT(MeasurementExcludedFvPpi != NULL);
  MeasurementExcludedFvPpi->Count = 1;
  MeasurementExcludedFvPpi->Fv[0].FvBase = PcdGet32 (PcdFspmBaseAddress);
  MeasurementExcludedFvPpi->Fv[0].FvLength = ((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) PcdGet32 (PcdFspmBaseAddress))->FvLength;

  MeasurementExcludedPpiList = AllocatePool (sizeof(*MeasurementExcludedPpiList));
  ASSERT(MeasurementExcludedPpiList != NULL);
  MeasurementExcludedPpiList->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  MeasurementExcludedPpiList->Guid  = &gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid;
  MeasurementExcludedPpiList->Ppi   = MeasurementExcludedFvPpi;

  Status = EFI_SUCCESS;

  if (PcdGet8 (PcdFspModeSelection) == 1) {
    Status = PeiFspMemoryInit ();
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = PeiServicesInstallPpi (MeasurementExcludedPpiList);
    ASSERT_EFI_ERROR (Status);

    PeiServicesInstallFvInfoPpi (
      NULL,
      (VOID *)(UINTN) PcdGet32 (PcdFspmBaseAddress),
      (UINT32)((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) PcdGet32 (PcdFspmBaseAddress))->FvLength,
      NULL,
      NULL
      );
  }

  return Status;
}

/**
  This function is called after TCG installed PPI.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
TcgPpiNotify (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR mTcgPpiNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiTcgPpiGuid,
  TcgPpiNotify
};

/**
  This function is called after TCG installed PPI.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
TcgPpiNotify (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  )
{
  UINT32                    FspMeasureMask;

  DEBUG ((DEBUG_INFO, "TcgPpiNotify FSPM\n"));

  FspMeasureMask = PcdGet32 (PcdFspMeasurementConfig);

  if ((FspMeasureMask & FSP_MEASURE_FSPT) != 0) {
    MeasureFspFirmwareBlob (0, "FSPT", PcdGet32(PcdFsptBaseAddress),
                            (UINT32)((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) PcdGet32 (PcdFsptBaseAddress))->FvLength);
  }

  if ((FspMeasureMask & FSP_MEASURE_FSPM) != 0) {
    MeasureFspFirmwareBlob (0, "FSPM", PcdGet32(PcdFspmBaseAddress),
                            (UINT32)((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) PcdGet32 (PcdFspmBaseAddress))->FvLength);
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
FspmWrapperPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  DEBUG((DEBUG_INFO, "FspmWrapperPeimEntryPoint\n"));

  Status = PeiServicesNotifyPpi (&mTcgPpiNotifyDesc);
  ASSERT_EFI_ERROR (Status);

  FspmWrapperInit ();

  return EFI_SUCCESS;
}
