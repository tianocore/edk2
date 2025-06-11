/** @file
  MM IPL that load the MM Core into MMRAM at PEI stage

  Copyright (c) 2024 - 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmIplPei.h"
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Ppi/MmControl.h>
#include <Ppi/MmCoreFvLocationPpi.h>

EFI_PEI_MM_COMMUNICATION_PPI   mMmCommunicationPpi  = { Communicate };
EFI_PEI_MM_COMMUNICATION3_PPI  mMmCommunication3Ppi = { Communicate3 };

EFI_PEI_PPI_DESCRIPTOR  mPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiPeiMmCommunicationPpiGuid,
    &mMmCommunicationPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMmCommunication3PpiGuid,
    &mMmCommunication3Ppi
  }
};

/**
  This is the callback function on end of PEI.

  This callback is used for call MmEndOfPeiHandler in standalone MM core.

  @param   PeiServices       General purpose services available to every PEIM.
  @param   NotifyDescriptor  The notification structure this PEIM registered on install.
  @param   Ppi               Pointer to the PPI data associated with this function.

  @retval  EFI_SUCCESS       Exit boot services successfully.
  @retval  Other             Exit boot services failed.
**/
STATIC
EFI_STATUS
EFIAPI
EndOfPeiCallback (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN  VOID                       *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyList = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiEndOfPeiSignalPpiGuid,
  EndOfPeiCallback
};

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_MM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into MMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_NOT_STARTED        The service is NOT started.
**/
EFI_STATUS
EFIAPI
Communicate (
  IN CONST EFI_PEI_MM_COMMUNICATION_PPI  *This,
  IN OUT VOID                            *CommBuffer,
  IN OUT UINTN                           *CommSize
  )
{
  EFI_STATUS              Status;
  EFI_PEI_MM_CONTROL_PPI  *MmControl;
  UINT8                   SmiCommand;
  UINTN                   Size;
  UINTN                   TempCommSize;
  EFI_HOB_GUID_TYPE       *GuidHob;
  MM_COMM_BUFFER          *MmCommBuffer;
  MM_COMM_BUFFER_STATUS   *MmCommBufferStatus;

  DEBUG ((DEBUG_INFO, "StandaloneMmIpl Communicate Enter\n"));

  GuidHob = GetFirstGuidHob (&gMmCommBufferHobGuid);
  if (GuidHob != NULL) {
    MmCommBuffer       = GET_GUID_HOB_DATA (GuidHob);
    MmCommBufferStatus = (MM_COMM_BUFFER_STATUS *)(UINTN)MmCommBuffer->Status;
  } else {
    DEBUG ((DEBUG_ERROR, "MmCommBuffer is not existed !!!\n"));
    ASSERT (GuidHob != NULL);
    return EFI_NOT_FOUND;
  }

  SmiCommand = 0;
  Size       = sizeof (SmiCommand);

  //
  // Check parameters
  //
  if ((CommBuffer == NULL) || (CommSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  } else {
    TempCommSize = *CommSize;
    //
    // CommSize must hold HeaderGuid and MessageLength
    //
    if (TempCommSize < OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (TempCommSize > EFI_PAGES_TO_SIZE (MmCommBuffer->NumberOfPages)) {
    DEBUG ((DEBUG_ERROR, "Communicate buffer size (%d) is over MAX (%d) size!", TempCommSize, EFI_PAGES_TO_SIZE (MmCommBuffer->NumberOfPages)));
    return EFI_INVALID_PARAMETER;
  }

  CopyMem ((VOID *)(UINTN)MmCommBuffer->PhysicalStart, CommBuffer, TempCommSize);
  MmCommBufferStatus->IsCommBufferValid = TRUE;

  //
  // Generate Software SMI
  //
  Status = PeiServicesLocatePpi (&gEfiPeiMmControlPpiGuid, 0, NULL, (VOID **)&MmControl);
  ASSERT_EFI_ERROR (Status);

  Status = MmControl->Trigger (
                        (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                        MmControl,
                        (INT8 *)&SmiCommand,
                        &Size,
                        FALSE,
                        0
                        );
  ASSERT_EFI_ERROR (Status);

  //
  // Return status from software SMI
  //
  *CommSize = (UINTN)MmCommBufferStatus->ReturnBufferSize;

  //
  // Copy the returned data to the non-mmram buffer (CommBuffer)
  //
  CopyMem (CommBuffer, (VOID *)(MmCommBuffer->PhysicalStart), *CommSize);

  Status = (EFI_STATUS)MmCommBufferStatus->ReturnStatus;
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "StandaloneMmIpl Communicate failed (%r)\n", Status));
  } else {
    MmCommBufferStatus->IsCommBufferValid = FALSE;
  }

  return Status;
}

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_MM_COMMUNICATE3 instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into MMRAM.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
**/
EFI_STATUS
EFIAPI
Communicate3 (
  IN CONST EFI_PEI_MM_COMMUNICATION3_PPI  *This,
  IN OUT VOID                             *CommBuffer
  )
{
  EFI_STATUS                    Status;
  EFI_PEI_MM_CONTROL_PPI        *MmControl;
  UINT8                         SmiCommand;
  UINTN                         Size;
  UINTN                         TempCommSize;
  EFI_HOB_GUID_TYPE             *GuidHob;
  MM_COMM_BUFFER                *MmCommBuffer;
  MM_COMM_BUFFER_STATUS         *MmCommBufferStatus;
  EFI_MM_COMMUNICATE_HEADER_V3  *CommunicateHeader;

  DEBUG ((DEBUG_INFO, "StandaloneMmIpl Communicate Enter\n"));

  GuidHob = GetFirstGuidHob (&gMmCommBufferHobGuid);
  if (GuidHob != NULL) {
    MmCommBuffer       = GET_GUID_HOB_DATA (GuidHob);
    MmCommBufferStatus = (MM_COMM_BUFFER_STATUS *)(UINTN)MmCommBuffer->Status;
  } else {
    DEBUG ((DEBUG_ERROR, "MmCommBuffer is not existed !!!\n"));
    ASSERT (GuidHob != NULL);
    return EFI_NOT_FOUND;
  }

  SmiCommand = 0;
  Size       = sizeof (SmiCommand);

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  } else {
    CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER_V3 *)CommBuffer;
    //
    // Check if the HeaderGuid is valid
    //
    if (CompareGuid (&CommunicateHeader->HeaderGuid, &gEfiMmCommunicateHeaderV3Guid)) {
      DEBUG ((DEBUG_ERROR, "HeaderGuid is not valid!\n"));
      return EFI_INVALID_PARAMETER;
    }

    TempCommSize = CommunicateHeader->BufferSize;
    //
    // CommSize must hold HeaderGuid and MessageLength
    //
    if (TempCommSize < sizeof (EFI_MM_COMMUNICATE_HEADER_V3)) {
      DEBUG ((DEBUG_ERROR, "Communicate buffer size (%d) is less than minimum size (%d)!", TempCommSize, sizeof (EFI_MM_COMMUNICATE_HEADER_V3)));
      return EFI_INVALID_PARAMETER;
    }
  }

  if (TempCommSize > EFI_PAGES_TO_SIZE (MmCommBuffer->NumberOfPages)) {
    DEBUG ((DEBUG_ERROR, "Communicate buffer size (%d) is over MAX (%d) size!", TempCommSize, EFI_PAGES_TO_SIZE (MmCommBuffer->NumberOfPages)));
    return EFI_INVALID_PARAMETER;
  }

  CopyMem ((VOID *)(UINTN)MmCommBuffer->PhysicalStart, CommBuffer, TempCommSize);
  MmCommBufferStatus->IsCommBufferValid = TRUE;

  //
  // Generate Software SMI
  //
  Status = PeiServicesLocatePpi (&gEfiPeiMmControlPpiGuid, 0, NULL, (VOID **)&MmControl);
  ASSERT_EFI_ERROR (Status);

  Status = MmControl->Trigger (
                        (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                        MmControl,
                        (INT8 *)&SmiCommand,
                        &Size,
                        FALSE,
                        0
                        );
  ASSERT_EFI_ERROR (Status);

  //
  // Return status from software SMI
  //
  TempCommSize = (UINTN)MmCommBufferStatus->ReturnBufferSize;

  //
  // Copy the returned data to the non-mmram buffer (CommBuffer)
  //
  CopyMem (CommBuffer, (VOID *)(MmCommBuffer->PhysicalStart), TempCommSize);

  CommunicateHeader->BufferSize = TempCommSize;

  Status = (EFI_STATUS)MmCommBufferStatus->ReturnStatus;
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "StandaloneMmIpl Communicate failed (%r)\n", Status));
  } else {
    MmCommBufferStatus->IsCommBufferValid = FALSE;
  }

  return Status;
}

/**
  Search all the available firmware volumes for MM Core driver.

  @param  MmFvBase             Base address of FV which included MM Core driver.
  @param  MmFvSize             Size of FV which included MM Core driver.
  @param  MmCoreFileName       GUID of MM Core.
  @param  MmCoreImageAddress   MM Core image address.

  @retval EFI_SUCCESS          The specified FFS section was returned.
  @retval EFI_NOT_FOUND        The specified FFS section could not be found.

**/
EFI_STATUS
LocateMmCoreFv (
  OUT EFI_PHYSICAL_ADDRESS  *MmFvBase,
  OUT UINTN                 *MmFvSize,
  OUT EFI_GUID              *MmCoreFileName,
  OUT VOID                  **MmCoreImageAddress
  )
{
  EFI_STATUS               Status;
  UINTN                    FvIndex;
  EFI_PEI_FV_HANDLE        VolumeHandle;
  EFI_PEI_FILE_HANDLE      FileHandle;
  EFI_PE32_SECTION         *SectionData;
  EFI_FV_INFO              VolumeInfo;
  MM_CORE_FV_LOCATION_PPI  *MmCoreFvLocation;

  //
  // The producer of the MmCoreFvLocation PPI is responsible for ensuring
  // that it reports the correct Firmware Volume (FV) containing the MmCore.
  // If the gMmCoreFvLocationPpiGuid is not found, the system will search
  // all Firmware Volumes (FVs) to locate the FV that contains the MM Core.
  //
  Status = PeiServicesLocatePpi (&gMmCoreFvLocationPpiGuid, 0, NULL, (VOID **)&MmCoreFvLocation);
  if (Status == EFI_SUCCESS) {
    *MmFvBase  = MmCoreFvLocation->Address;
    *MmFvSize  = MmCoreFvLocation->Size;
    FileHandle = NULL;
    Status     = PeiServicesFfsFindNextFile (EFI_FV_FILETYPE_MM_CORE_STANDALONE, (VOID *)(UINTN)MmCoreFvLocation->Address, &FileHandle);
    ASSERT_EFI_ERROR (Status);
    if (Status == EFI_SUCCESS) {
      ASSERT (FileHandle != NULL);
      if (FileHandle != NULL) {
        CopyGuid (MmCoreFileName, &((EFI_FFS_FILE_HEADER *)FileHandle)->Name);
        //
        // Search Section
        //
        Status = PeiServicesFfsFindSectionData (EFI_SECTION_PE32, FileHandle, MmCoreImageAddress);
        ASSERT_EFI_ERROR (Status);

        //
        // Get MM Core section data.
        //
        SectionData = (EFI_PE32_SECTION *)((UINT8 *)*MmCoreImageAddress - sizeof (EFI_PE32_SECTION));
        ASSERT (SectionData->Type == EFI_SECTION_PE32);
      }
    }

    return EFI_SUCCESS;
  }

  //
  // Search all FV
  //
  VolumeHandle = NULL;
  for (FvIndex = 0; ; FvIndex++) {
    Status = PeiServicesFfsFindNextVolume (FvIndex, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Search MM Core FFS
    //
    FileHandle = NULL;
    Status     = PeiServicesFfsFindNextFile (EFI_FV_FILETYPE_MM_CORE_STANDALONE, VolumeHandle, &FileHandle);
    if (EFI_ERROR (Status)) {
      continue;
    }

    ASSERT (FileHandle != NULL);
    if (FileHandle != NULL) {
      CopyGuid (MmCoreFileName, &((EFI_FFS_FILE_HEADER *)FileHandle)->Name);
    }

    //
    // Search Section
    //
    Status = PeiServicesFfsFindSectionData (EFI_SECTION_PE32, FileHandle, MmCoreImageAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Get MM Core section data.
    //
    SectionData = (EFI_PE32_SECTION *)((UINT8 *)*MmCoreImageAddress - sizeof (EFI_PE32_SECTION));
    ASSERT (SectionData->Type == EFI_SECTION_PE32);

    //
    // This is the FV that contains MM Core.
    //
    Status = PeiServicesFfsGetVolumeInfo (VolumeHandle, &VolumeInfo);
    if (!EFI_ERROR (Status)) {
      *MmFvBase = (EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart;
      *MmFvSize = VolumeInfo.FvSize;
      return EFI_SUCCESS;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This is the callback function on end of PEI.

  This callback is used for call MmEndOfPeiHandler in standalone MM core.

  @param   PeiServices       General purpose services available to every PEIM.
  @param   NotifyDescriptor  The notification structure this PEIM registered on install.
  @param   Ppi               Pointer to the PPI data associated with this function.

  @retval  EFI_SUCCESS       Exit boot services successfully.
  @retval  Other             Exit boot services failed.
**/
STATIC
EFI_STATUS
EFIAPI
EndOfPeiCallback (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN  VOID                       *Ppi
  )
{
  EFI_MM_COMMUNICATE_HEADER  CommunicateHeader;
  UINTN                      Size;
  EFI_STATUS                 Status;

  //
  // Use Guid to initialize EFI_MM_COMMUNICATE_HEADER structure
  //
  CopyGuid (&CommunicateHeader.HeaderGuid, &gEfiMmEndOfPeiProtocol);
  CommunicateHeader.MessageLength = 1;
  CommunicateHeader.Data[0]       = 0;

  //
  // Generate the Software SMI and return the result
  //
  Size   = sizeof (CommunicateHeader);
  Status = Communicate (NULL, &CommunicateHeader, &Size);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Build communication buffer HOB.

  @return  MM_COMM_BUFFER     Pointer of MM communication buffer

**/
MM_COMM_BUFFER *
MmIplBuildCommBufferHob (
  VOID
  )
{
  EFI_STATUS      Status;
  MM_COMM_BUFFER  *MmCommBuffer;
  UINT64          MmCommBufferPages;

  MmCommBufferPages = PcdGet32 (PcdMmCommBufferPages);

  MmCommBuffer = BuildGuidHob (&gMmCommBufferHobGuid, sizeof (MM_COMM_BUFFER));
  ASSERT (MmCommBuffer != NULL);

  //
  // Set MM communicate buffer size
  //
  MmCommBuffer->NumberOfPages = MmCommBufferPages;

  //
  // Allocate runtime memory for MM communicate buffer
  //
  MmCommBuffer->PhysicalStart = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (MmCommBufferPages);
  if (MmCommBuffer->PhysicalStart == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate MM communication buffer\n"));
    ASSERT (MmCommBuffer->PhysicalStart != 0);
  }

  //
  // Build MM unblock memory region HOB for MM communication buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->PhysicalStart, MmCommBufferPages);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate runtime memory for MM communication status parameters :
  // ReturnStatus, ReturnBufferSize, IsCommBufferValid
  //
  MmCommBuffer->Status = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (EFI_SIZE_TO_PAGES (sizeof (MM_COMM_BUFFER_STATUS)));
  if (MmCommBuffer->Status == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory for MM communication status\n"));
    ASSERT (MmCommBuffer->Status != 0);
  }

  //
  // Build MM unblock memory region HOB for MM communication status
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->Status, EFI_SIZE_TO_PAGES (sizeof (MM_COMM_BUFFER_STATUS)));
  ASSERT_EFI_ERROR (Status);

  return MmCommBuffer;
}

/**
  The Entry Point for MM IPL at PEI stage.

  Load MM Core into MMRAM.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
StandaloneMmIplPeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS      Status;
  MM_COMM_BUFFER  *MmCommBuffer;

  //
  // Build communication buffer HOB.
  //
  MmCommBuffer = MmIplBuildCommBufferHob ();
  ASSERT (MmCommBuffer != NULL);

  //
  // Locate and execute Mm Core to dispatch MM drivers.
  //
  Status = ExecuteMmCoreFromMmram (MmCommBuffer);
  ASSERT_EFI_ERROR (Status);

  //
  // Install MmCommunicationPpi
  //
  Status = PeiServicesInstallPpi (mPpiList);
  ASSERT_EFI_ERROR (Status);

  //
  // Create end of pei callback to call MmEndOfPeiHandler
  //
  Status = PeiServicesNotifyPpi (&mNotifyList);
  ASSERT_EFI_ERROR (Status);

  //
  // Dispatch StandaloneMm drivers in MM
  //
  Status = MmIplDispatchMmDrivers ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
