/** @file
  This will be invoked only once. It will call FspSmmInit API,
  to call MmIplPei to load MM Core and dispatch all Standalone
  MM drivers.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>

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
  ASSERT (MeasurementExcludedFvPpi != NULL);
  if (MeasurementExcludedFvPpi == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MeasurementExcludedFvPpi->Count          = 1;
  MeasurementExcludedFvPpi->Fv[0].FvBase   = PcdGet32 (PcdFspiBaseAddress);
  MeasurementExcludedFvPpi->Fv[0].FvLength = ((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFspiBaseAddress))->FvLength;

  MeasurementExcludedPpiList = AllocatePool (sizeof (*MeasurementExcludedPpiList));
  ASSERT (MeasurementExcludedPpiList != NULL);
  if (MeasurementExcludedPpiList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MeasurementExcludedPpiList->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  MeasurementExcludedPpiList->Guid  = &gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid;
  MeasurementExcludedPpiList->Ppi   = MeasurementExcludedFvPpi;

  Status = PeiServicesInstallPpi (MeasurementExcludedPpiList);
  ASSERT_EFI_ERROR (Status);

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

  return Status;
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

  Status = FspiWrapperInitDispatchMode ();

  return Status;
}
