/** @file
  MM IPL that loads the MM Core into MMRAM during DXE.

  Copyright (c) 2025, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmIplPei.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/MmAccess.h>
#include <Protocol/MmCommunication.h>
#include <Protocol/MpService.h>
#include <Guid/MpInformation2.h>

STATIC VOID  *MmCoreBufferAddress = NULL;

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
  EFI_MM_COMMUNICATION_PROTOCOL  *MmCommunication;
  EFI_STATUS                     Status;

  Status = gBS->LocateProtocol (&gEfiMmCommunicationProtocolGuid, NULL, (VOID **)&MmCommunication);
  ASSERT_EFI_ERROR (Status);

  return MmCommunication->Communicate (MmCommunication, CommBuffer, CommSize);
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
  EFI_FV_FILETYPE                      FileType;
  EFI_STATUS                           Status;
  EFI_HANDLE                           *HandleBuffer;
  UINTN                                HandleCount;
  UINTN                                IndexFv;
  EFI_FIRMWARE_VOLUME2_PROTOCOL        *Fv;
  UINTN                                Key;
  EFI_GUID                             NameGuid;
  EFI_FV_FILE_ATTRIBUTES               Attributes;
  UINTN                                Size;
  UINT32                               AuthenticationStatus;
  EFI_HANDLE                           FvHandle;
  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER           *FwVolHeader;

  *MmCoreImageAddress = NULL;

  //
  // First, find the FV with MM core.
  //
  FileType = EFI_FV_FILETYPE_MM_CORE_STANDALONE;

  //
  // Locate all available FVs.
  //
  HandleBuffer = NULL;
  Status       = gBS->LocateHandleBuffer (
                        ByProtocol,
                        &gEfiFirmwareVolume2ProtocolGuid,
                        NULL,
                        &HandleCount,
                        &HandleBuffer
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Go through FVs one by one to find the required section data.
  //
  for (IndexFv = 0; IndexFv < HandleCount; IndexFv++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[IndexFv],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **)&Fv
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Use Firmware Volume 2 Protocol to search for a file of type FileType in all FVs.
    //
    Key    = 0;
    Status = Fv->GetNextFile (Fv, &Key, &FileType, &NameGuid, &Attributes, &Size);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Fv File with the required FV file type is found.
    // Search the section file in the found FV file.
    //
    Size   = 0;
    Status = Fv->ReadSection (
                   Fv,
                   &NameGuid,
                   EFI_SECTION_PE32,
                   0,
                   MmCoreImageAddress,
                   &Size,
                   &AuthenticationStatus
                   );
    if (!EFI_ERROR (Status)) {
      goto Found;
    }
  }

  //
  // The required FFS section file is not found.
  //
  if (IndexFv == HandleCount) {
    Status = EFI_NOT_FOUND;
  }

Found:
  if (HandleBuffer != NULL) {
    FvHandle = HandleBuffer[IndexFv];
    FreePool (HandleBuffer);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Stash this to free when done.
  //
  MmCoreBufferAddress = *MmCoreImageAddress;

  //
  // Now, retrieve the other requested parameters.
  //
  CopyMem (MmCoreFileName, &NameGuid, sizeof (EFI_GUID));

  Status = gBS->HandleProtocol (
                  FvHandle,
                  &gEfiFirmwareVolumeBlock2ProtocolGuid,
                  (VOID **)&Fvb
                  );
  ASSERT_EFI_ERROR (Status);

  Status = Fvb->GetPhysicalAddress (
                  Fvb,
                  MmFvBase
                  );
  ASSERT_EFI_ERROR (Status);

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)*MmFvBase;
  *MmFvSize   = FwVolHeader->FvLength;

  return EFI_SUCCESS;
}

/**
  Open all MMRAM ranges if platform provides an MmAccess implementation.
  If it does, it's likely required to be able to use MMRAM.

  @retval EFI_SUCCESS       MMRAM is opened.
  @retval EFI_DEVICE_ERROR  MMRAM could not be opened.

**/
EFI_STATUS
MmAccessOpen (
  VOID
  )
{
  EFI_MM_ACCESS_PROTOCOL  *MmAccess;
  EFI_STATUS              Status;

  //
  // Open all MMRAM ranges, if MmAccess is available.
  //
  Status = gBS->LocateProtocol (&gEfiMmAccessProtocolGuid, NULL, (VOID **)&MmAccess);
  if (!EFI_ERROR (Status)) {
    Status = MmAccess->Open (MmAccess);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MM IPL failed to open MMRAM window - %r\n", Status));
      ASSERT_EFI_ERROR (Status);
      return EFI_DEVICE_ERROR;
    }

    //
    // Print debug message that the MMRAM window is now opened.
    //
    DEBUG ((DEBUG_INFO, "MM IPL opened MMRAM window\n"));
  }

  return EFI_SUCCESS;
}

/**
  Close and lock all MMRAM ranges if platform provides an MmAccess implementation.
  If it does, it's likely required for security reasons.

**/
EFI_STATUS
MmAccessClose (
  VOID
  )
{
  EFI_MM_ACCESS_PROTOCOL  *MmAccess;
  EFI_STATUS              Status;

  //
  // Close all MMRAM ranges, if MmAccess is available.
  //
  Status = gBS->LocateProtocol (&gEfiMmAccessProtocolGuid, NULL, (VOID **)&MmAccess);
  if (!EFI_ERROR (Status)) {
    Status = MmAccess->Close (MmAccess);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MM IPL failed to close MMRAM window - %r\n", Status));
      ASSERT_EFI_ERROR (Status);
      return EFI_DEVICE_ERROR;
    }

    //
    // Print debug message that the MMRAM window is now closed.
    //
    DEBUG ((DEBUG_INFO, "MM IPL closed MMRAM window\n"));

    //
    // Lock the MMRAM (Note: Locking MMRAM may not be supported on all platforms)
    //
    Status = MmAccess->Lock (MmAccess);
    if (EFI_ERROR (Status)) {
      //
      // Print error message that the MMRAM failed to lock...
      //
      DEBUG ((DEBUG_ERROR, "MM IPL could not lock MMRAM window after executing MM Core - %r\n", Status));
      ASSERT_EFI_ERROR (Status);
      return EFI_DEVICE_ERROR;
    }

    //
    // Print debug message that the MMRAM window is now locked.
    //
    DEBUG ((DEBUG_INFO, "MM IPL locked MMRAM window\n"));
  }

  return EFI_SUCCESS;
}

/**
  This is the callback function on end of PEI.

  This callback is used for call MmEndOfPeiHandler in standalone MM core.

  @retval  EFI_SUCCESS       Exit boot services successfully.
  @retval  Other             Exit boot services failed.
**/
EFI_STATUS
EFIAPI
SignalEndOfPei (
  VOID
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
  The Entry Point for MmCommunicateDxe driver.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
StandaloneMmIplDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  MM_COMM_BUFFER     *MmCommBuffer;
  EFI_STATUS         Status;

  //
  // Retrieve communication buffer HOB.
  //
  GuidHob = GetFirstGuidHob (&gMmCommBufferHobGuid);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "StandaloneMmIplDxe used without platform support!\n"));
    return EFI_UNSUPPORTED;
  }

  MmCommBuffer = GET_GUID_HOB_DATA (GuidHob);

  //
  // Locate and execute Mm Core to dispatch MM drivers.
  //
  Status = ExecuteMmCoreFromMmram (MmCommBuffer);
  ASSERT_EFI_ERROR (Status);

  FreePool (MmCoreBufferAddress);

  //
  // Dispatch StandaloneMm drivers in MM
  //
  Status = MmIplDispatchMmDrivers ();
  ASSERT_EFI_ERROR (Status);

  //
  // Call MmEndOfPeiHandler in MM core
  //
  SignalEndOfPei ();

  return EFI_SUCCESS;
}
