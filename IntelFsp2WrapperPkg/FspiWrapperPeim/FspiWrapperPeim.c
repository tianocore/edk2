/** @file
  This will be invoked only once. It will call FspSmmInit API,
  to call MmIplPei to load MM Core and dispatch all Standalone
  MM drivers.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <FspEas.h>
#include <FspStatusCode.h>
#include <FspGlobalData.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/FspWrapperHobProcessLib.h>
#include <Library/FspWrapperApiTestLib.h>
#include <Library/FspMeasurementLib.h>
#include <Ppi/Tcg.h>
#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>

/**
  Call FspSmmInit API.

  @return Status returned by FspSmmInit API.
**/
EFI_STATUS
FspiWrapperInitApiMode (
  VOID
  )
{
  FSP_INFO_HEADER    *FspiHeaderPtr;
  EFI_STATUS         Status;
  UINT64             TimeStampCounterStart;
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *FspHobListPtr;
  VOID               *FspiUpdDataPtr;
  UINTN              *SourceData;

  DEBUG ((DEBUG_INFO, "PeiFspSmmInit enter\n"));

  FspHobListPtr  = NULL;
  FspiUpdDataPtr = NULL;

  FspiHeaderPtr = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspiBaseAddress));
  DEBUG ((DEBUG_INFO, "FspiHeaderPtr - 0x%x\n", FspiHeaderPtr));
  if (FspiHeaderPtr == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if ((PcdGet64 (PcdFspiUpdDataAddress) == 0) && (FspiHeaderPtr->CfgRegionSize != 0) && (FspiHeaderPtr->CfgRegionOffset != 0)) {
    //
    // Copy default FSP-I UPD data from Flash
    //
    FspiUpdDataPtr = AllocateZeroPool ((UINTN)FspiHeaderPtr->CfgRegionSize);
    ASSERT (FspiUpdDataPtr != NULL);
    SourceData = (UINTN *)((UINTN)FspiHeaderPtr->ImageBase + (UINTN)FspiHeaderPtr->CfgRegionOffset);
    CopyMem (FspiUpdDataPtr, SourceData, (UINTN)FspiHeaderPtr->CfgRegionSize);
  } else {
    //
    // External UPD is ready, get the buffer from PCD pointer.
    //
    FspiUpdDataPtr = (VOID *)(UINTN)PcdGet64 (PcdFspiUpdDataAddress);
    ASSERT (FspiUpdDataPtr != NULL);
  }

  DEBUG ((DEBUG_INFO, "UpdateFspiUpdData enter\n"));
  UpdateFspiUpdData (FspiUpdDataPtr);
  DEBUG ((DEBUG_INFO, "  BootloaderSmmFvBaseAddress       - 0x%lx\n", ((FSPI_UPD_COMMON *)FspiUpdDataPtr)->FspiArchUpd.BootloaderSmmFvBaseAddress));
  DEBUG ((DEBUG_INFO, "  BootloaderSmmFvLength            - 0x%lx\n", ((FSPI_UPD_COMMON *)FspiUpdDataPtr)->FspiArchUpd.BootloaderSmmFvLength));
  DEBUG ((DEBUG_INFO, "  BootloaderSmmFvContextData       - 0x%lx\n", ((FSPI_UPD_COMMON *)FspiUpdDataPtr)->FspiArchUpd.BootloaderSmmFvContextData));
  DEBUG ((DEBUG_INFO, "  BootloaderSmmFvContextDataLength - 0x%lx\n", ((FSPI_UPD_COMMON *)FspiUpdDataPtr)->FspiArchUpd.BootloaderSmmFvContextDataLength));

  //
  // Get FspHobList
  //
  GuidHob = GetFirstGuidHob (&gFspHobGuid);
  ASSERT (GuidHob != NULL);
  FspHobListPtr = *(VOID **)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "  HobListPtr          - 0x%x\n", &FspHobListPtr));

  TimeStampCounterStart = AsmReadTsc ();
  Status                = CallFspSmmInit (FspiUpdDataPtr);

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FspSmmInitApi requested reset %r\n", Status));
    CallFspWrapperResetSystem (Status);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - Failed to execute FspSmmInitApi(), Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_INFO, "FspSmmInit status: %r\n", Status));
  //
  // Create hobs after FspSmmInit. Hence passing the recorded timestamp here
  //
  PERF_START_EX (&gFspApiPerformanceGuid, "EventRec", NULL, TimeStampCounterStart, FSP_STATUS_CODE_FSPSMM_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  PERF_END_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_FSPSMM_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  DEBUG ((DEBUG_INFO, "Total time spent executing FspSmmInitApi: %d millisecond\n", DivU64x32 (GetTimeInNanoSecond (AsmReadTsc () - TimeStampCounterStart), 1000000)));

  Status = TestFspSmmInitApiOutput (FspiUpdDataPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - TestFspSmmInitApiOutput () fail, Status = %r\n", Status));
  }

  DEBUG ((DEBUG_INFO, "  FspHobListPtr (returned) - 0x%x\n", FspHobListPtr));
  ASSERT (FspHobListPtr != NULL);

  PostFspiHobProcess (FspHobListPtr);

  return Status;
}

/**
  Do FSP SMM initialization in Dispatch mode.

  @retval FSP SMM initialization status.
**/
EFI_STATUS
EFIAPI
FspiWrapperInitDispatchMode (
  VOID
  )
{
  EFI_STATUS                                             Status;
  EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI  *MeasurementExcludedFvPpi;
  EFI_PEI_PPI_DESCRIPTOR                                 *MeasurementExcludedPpiList;

  MeasurementExcludedFvPpi = AllocatePool (sizeof (*MeasurementExcludedFvPpi));
  if (MeasurementExcludedFvPpi != NULL) {
    MeasurementExcludedFvPpi->Count          = 1;
    MeasurementExcludedFvPpi->Fv[0].FvBase   = PcdGet32 (PcdFspiBaseAddress);
    MeasurementExcludedFvPpi->Fv[0].FvLength = ((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFspiBaseAddress))->FvLength;
  } else {
    ASSERT (MeasurementExcludedFvPpi != NULL);
  }

  MeasurementExcludedPpiList = AllocatePool (sizeof (*MeasurementExcludedPpiList));
  if (MeasurementExcludedPpiList != NULL) {
    MeasurementExcludedPpiList->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
    MeasurementExcludedPpiList->Guid  = &gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid;
    MeasurementExcludedPpiList->Ppi   = MeasurementExcludedFvPpi;

    Status = PeiServicesInstallPpi (MeasurementExcludedPpiList);
    ASSERT_EFI_ERROR (Status);
  } else {
    ASSERT (MeasurementExcludedPpiList != NULL);
  }

  //
  // FSP-I Wrapper running in Dispatch mode and reports FSP-I FV to PEI dispatcher.
  //
  PeiServicesInstallFvInfoPpi (
    &((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFspiBaseAddress))->FileSystemGuid,
    (VOID *)(UINTN)PcdGet32 (PcdFspiBaseAddress),
    (UINT32)((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFspiBaseAddress))->FvLength,
    NULL,
    NULL
    );

  return EFI_SUCCESS;
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
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mTcgPpiNotifyDesc = {
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
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  )
{
  UINT32  FspMeasureMask;

  DEBUG ((DEBUG_INFO, "TcgPpiNotify FSPI\n"));

  FspMeasureMask = PcdGet32 (PcdFspMeasurementConfig);

  if ((FspMeasureMask & FSP_MEASURE_FSPI) != 0) {
    MeasureFspFirmwareBlob (
      0,
      "FSPI",
      PcdGet32 (PcdFspiBaseAddress),
      (UINT32)((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFspiBaseAddress))->FvLength
      );
  }

  return EFI_SUCCESS;
}

/**
  This is the entrypoint of PEIM.

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
FspiWrapperPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "FspiWrapperPeimEntryPoint\n"));

  Status = PeiServicesNotifyPpi (&mTcgPpiNotifyDesc);
  ASSERT_EFI_ERROR (Status);

  if (PcdGet8 (PcdFspModeSelection) == 1) {
    Status = FspiWrapperInitApiMode ();
  } else {
    Status = FspiWrapperInitDispatchMode ();
  }

  return Status;
}
