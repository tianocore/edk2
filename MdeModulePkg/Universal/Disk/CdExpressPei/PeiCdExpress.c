/** @file
  Source file for CD recovery PEIM

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiCdExpress.h"

PEI_CD_EXPRESS_PRIVATE_DATA  *mPrivateData = NULL;
CHAR8                        *mRecoveryFileName;
UINTN                        mRecoveryFileNameSize;

/**
  Installs the Device Recovery Module PPI, Initialize BlockIo Ppi
  installation notification

  @param  FileHandle            The file handle of the image.
  @param  PeiServices           General purpose services available to every PEIM.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_OUT_OF_RESOURCES  There is not enough system memory.

**/
EFI_STATUS
EFIAPI
CdExpressPeimEntry (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                   Status;
  PEI_CD_EXPRESS_PRIVATE_DATA  *PrivateData;

  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  PrivateData = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (*PrivateData)));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mRecoveryFileNameSize = PcdGetSize (PcdRecoveryFileName) / sizeof (CHAR16);
  mRecoveryFileName     = AllocatePool (mRecoveryFileNameSize);
  if (mRecoveryFileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UnicodeStrToAsciiStrS (PcdGetPtr (PcdRecoveryFileName), mRecoveryFileName, mRecoveryFileNameSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize Private Data (to zero, as is required by subsequent operations)
  //
  ZeroMem (PrivateData, sizeof (*PrivateData));
  PrivateData->Signature = PEI_CD_EXPRESS_PRIVATE_DATA_SIGNATURE;

  PrivateData->BlockBuffer = AllocatePages (EFI_SIZE_TO_PAGES (PEI_CD_BLOCK_SIZE));
  if (PrivateData->BlockBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->CapsuleCount = 0;
  Status                    = UpdateBlocksAndVolumes (PrivateData, TRUE);
  Status                    = UpdateBlocksAndVolumes (PrivateData, FALSE);

  //
  // Installs Ppi
  //
  PrivateData->DeviceRecoveryPpi.GetNumberRecoveryCapsules = GetNumberRecoveryCapsules;
  PrivateData->DeviceRecoveryPpi.GetRecoveryCapsuleInfo    = GetRecoveryCapsuleInfo;
  PrivateData->DeviceRecoveryPpi.LoadRecoveryCapsule       = LoadRecoveryCapsule;

  PrivateData->PpiDescriptor.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  PrivateData->PpiDescriptor.Guid  = &gEfiPeiDeviceRecoveryModulePpiGuid;
  PrivateData->PpiDescriptor.Ppi   = &PrivateData->DeviceRecoveryPpi;

  Status = PeiServicesInstallPpi (&PrivateData->PpiDescriptor);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // PrivateData is allocated now, set it to the module variable
  //
  mPrivateData = PrivateData;

  //
  // Installs Block Io Ppi notification function
  //
  PrivateData->NotifyDescriptor.Flags =
    (
     EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK
    );
  PrivateData->NotifyDescriptor.Guid   = &gEfiPeiVirtualBlockIoPpiGuid;
  PrivateData->NotifyDescriptor.Notify = BlockIoNotifyEntry;

  PrivateData->NotifyDescriptor2.Flags =
    (
     EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK |
     EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST
    );
  PrivateData->NotifyDescriptor2.Guid   = &gEfiPeiVirtualBlockIo2PpiGuid;
  PrivateData->NotifyDescriptor2.Notify = BlockIoNotifyEntry;

  return PeiServicesNotifyPpi (&PrivateData->NotifyDescriptor);
}

/**
  BlockIo installation notification function.

  This function finds out all the current Block IO PPIs in the system and add them
  into private data.

  @param  PeiServices            Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor       Address of the notification descriptor data structure.
  @param  Ppi                    Address of the PPI that was installed.

  @retval EFI_SUCCESS            The function completes successfully.

**/
EFI_STATUS
EFIAPI
BlockIoNotifyEntry (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  if (CompareGuid (NotifyDescriptor->Guid, &gEfiPeiVirtualBlockIo2PpiGuid)) {
    UpdateBlocksAndVolumes (mPrivateData, TRUE);
  } else {
    UpdateBlocksAndVolumes (mPrivateData, FALSE);
  }

  return EFI_SUCCESS;
}

/**
  Finds out all the current Block IO PPIs in the system and add them into private data.

  @param PrivateData                    The private data structure that contains recovery module information.
  @param BlockIo2                       Boolean to show whether using BlockIo2 or BlockIo.

  @retval EFI_SUCCESS                   The blocks and volumes are updated successfully.

**/
EFI_STATUS
UpdateBlocksAndVolumes (
  IN OUT PEI_CD_EXPRESS_PRIVATE_DATA  *PrivateData,
  IN     BOOLEAN                      BlockIo2
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_PPI_DESCRIPTOR          *TempPpiDescriptor;
  UINTN                           BlockIoPpiInstance;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI   *BlockIoPpi;
  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *BlockIo2Ppi;
  UINTN                           NumberBlockDevices;
  UINTN                           IndexBlockDevice;
  EFI_PEI_BLOCK_IO_MEDIA          Media;
  EFI_PEI_BLOCK_IO2_MEDIA         Media2;
  EFI_PEI_SERVICES                **PeiServices;

  IndexBlockDevice = 0;
  BlockIo2Ppi      = NULL;
  BlockIoPpi       = NULL;
  //
  // Find out all Block Io Ppi instances within the system
  // Assuming all device Block Io Peims are dispatched already
  //
  for (BlockIoPpiInstance = 0; BlockIoPpiInstance < PEI_CD_EXPRESS_MAX_BLOCK_IO_PPI; BlockIoPpiInstance++) {
    if (BlockIo2) {
      Status = PeiServicesLocatePpi (
                 &gEfiPeiVirtualBlockIo2PpiGuid,
                 BlockIoPpiInstance,
                 &TempPpiDescriptor,
                 (VOID **)&BlockIo2Ppi
                 );
    } else {
      Status = PeiServicesLocatePpi (
                 &gEfiPeiVirtualBlockIoPpiGuid,
                 BlockIoPpiInstance,
                 &TempPpiDescriptor,
                 (VOID **)&BlockIoPpi
                 );
    }

    if (EFI_ERROR (Status)) {
      //
      // Done with all Block Io Ppis
      //
      break;
    }

    PeiServices = (EFI_PEI_SERVICES  **)GetPeiServicesTablePointer ();
    if (BlockIo2) {
      Status = BlockIo2Ppi->GetNumberOfBlockDevices (
                              PeiServices,
                              BlockIo2Ppi,
                              &NumberBlockDevices
                              );
    } else {
      Status = BlockIoPpi->GetNumberOfBlockDevices (
                             PeiServices,
                             BlockIoPpi,
                             &NumberBlockDevices
                             );
    }

    if (EFI_ERROR (Status) || (NumberBlockDevices == 0)) {
      continue;
    }

    //
    // Just retrieve the first block, should emulate all blocks.
    //
    for (IndexBlockDevice = 1; IndexBlockDevice <= NumberBlockDevices && PrivateData->CapsuleCount < PEI_CD_EXPRESS_MAX_CAPSULE_NUMBER; IndexBlockDevice++) {
      if (BlockIo2) {
        Status = BlockIo2Ppi->GetBlockDeviceMediaInfo (
                                PeiServices,
                                BlockIo2Ppi,
                                IndexBlockDevice,
                                &Media2
                                );
        if (EFI_ERROR (Status) ||
            !Media2.MediaPresent ||
            ((Media2.InterfaceType != MSG_ATAPI_DP) && (Media2.InterfaceType != MSG_USB_DP)) ||
            (Media2.BlockSize != PEI_CD_BLOCK_SIZE)
            )
        {
          continue;
        }

        DEBUG ((DEBUG_INFO, "PeiCdExpress InterfaceType is %d\n", Media2.InterfaceType));
        DEBUG ((DEBUG_INFO, "PeiCdExpress MediaPresent is %d\n", Media2.MediaPresent));
        DEBUG ((DEBUG_INFO, "PeiCdExpress BlockSize is  0x%x\n", Media2.BlockSize));
      } else {
        Status = BlockIoPpi->GetBlockDeviceMediaInfo (
                               PeiServices,
                               BlockIoPpi,
                               IndexBlockDevice,
                               &Media
                               );
        if (EFI_ERROR (Status) ||
            !Media.MediaPresent ||
            ((Media.DeviceType != IdeCDROM) && (Media.DeviceType != UsbMassStorage)) ||
            (Media.BlockSize != PEI_CD_BLOCK_SIZE)
            )
        {
          continue;
        }

        DEBUG ((DEBUG_INFO, "PeiCdExpress DeviceType is %d\n", Media.DeviceType));
        DEBUG ((DEBUG_INFO, "PeiCdExpress MediaPresent is %d\n", Media.MediaPresent));
        DEBUG ((DEBUG_INFO, "PeiCdExpress BlockSize is  0x%x\n", Media.BlockSize));
      }

      DEBUG ((DEBUG_INFO, "PeiCdExpress Status is %d\n", Status));

      DEBUG ((DEBUG_INFO, "IndexBlockDevice is %d\n", IndexBlockDevice));
      PrivateData->CapsuleData[PrivateData->CapsuleCount].IndexBlock = IndexBlockDevice;
      if (BlockIo2) {
        PrivateData->CapsuleData[PrivateData->CapsuleCount].BlockIo2 = BlockIo2Ppi;
      } else {
        PrivateData->CapsuleData[PrivateData->CapsuleCount].BlockIo = BlockIoPpi;
      }

      Status = FindRecoveryCapsules (PrivateData);
      DEBUG ((DEBUG_INFO, "Status is %d\n", Status));

      if (EFI_ERROR (Status)) {
        continue;
      }

      PrivateData->CapsuleCount++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Finds out the recovery capsule in the current volume.

  @param PrivateData                    The private data structure that contains recovery module information.

  @retval EFI_SUCCESS                   The recovery capsule is successfully found in the volume.
  @retval EFI_NOT_FOUND                 The recovery capsule is not found in the volume.

**/
EFI_STATUS
EFIAPI
FindRecoveryCapsules (
  IN OUT PEI_CD_EXPRESS_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS                      Status;
  UINTN                           Lba;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI   *BlockIoPpi;
  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *BlockIo2Ppi;
  UINTN                           BufferSize;
  UINT8                           *Buffer;
  UINT8                           Type;
  UINT8                           *StandardID;
  UINT32                          RootDirLBA;
  PEI_CD_EXPRESS_DIR_FILE_RECORD  *RoorDirRecord;
  UINTN                           VolumeSpaceSize;
  BOOLEAN                         StartOfVolume;
  UINTN                           OriginalLBA;
  UINTN                           IndexBlockDevice;

  Buffer     = PrivateData->BlockBuffer;
  BufferSize = PEI_CD_BLOCK_SIZE;

  Lba = 16;
  //
  // The volume descriptor starts on Lba 16
  //
  IndexBlockDevice = PrivateData->CapsuleData[PrivateData->CapsuleCount].IndexBlock;
  BlockIoPpi       = PrivateData->CapsuleData[PrivateData->CapsuleCount].BlockIo;
  BlockIo2Ppi      = PrivateData->CapsuleData[PrivateData->CapsuleCount].BlockIo2;

  VolumeSpaceSize = 0;
  StartOfVolume   = TRUE;
  OriginalLBA     = 16;

  while (TRUE) {
    SetMem (Buffer, BufferSize, 0);
    if (BlockIo2Ppi != NULL) {
      Status = BlockIo2Ppi->ReadBlocks (
                              (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                              BlockIo2Ppi,
                              IndexBlockDevice,
                              Lba,
                              BufferSize,
                              Buffer
                              );
    } else {
      Status = BlockIoPpi->ReadBlocks (
                             (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                             BlockIoPpi,
                             IndexBlockDevice,
                             Lba,
                             BufferSize,
                             Buffer
                             );
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    StandardID = (UINT8 *)(Buffer + PEI_CD_EXPRESS_STANDARD_ID_OFFSET);
    if (!StringCmp (StandardID, (UINT8 *)PEI_CD_STANDARD_ID, PEI_CD_EXPRESS_STANDARD_ID_SIZE, TRUE)) {
      break;
    }

    if (StartOfVolume) {
      OriginalLBA   = Lba;
      StartOfVolume = FALSE;
    }

    Type = *(UINT8 *)(Buffer + PEI_CD_EXPRESS_VOLUME_TYPE_OFFSET);
    if (Type == PEI_CD_EXPRESS_VOLUME_TYPE_TERMINATOR) {
      if (VolumeSpaceSize == 0) {
        break;
      } else {
        Lba             = (OriginalLBA + VolumeSpaceSize);
        VolumeSpaceSize = 0;
        StartOfVolume   = TRUE;
        continue;
      }
    }

    if (Type != PEI_CD_EXPRESS_VOLUME_TYPE_PRIMARY) {
      Lba++;
      continue;
    }

    VolumeSpaceSize = *(UINT32 *)(Buffer + PEI_CD_EXPRESS_VOLUME_SPACE_OFFSET);

    RoorDirRecord = (PEI_CD_EXPRESS_DIR_FILE_RECORD *)(Buffer + PEI_CD_EXPRESS_ROOT_DIR_RECORD_OFFSET);
    RootDirLBA    = RoorDirRecord->LocationOfExtent[0];

    Status = RetrieveCapsuleFileFromRoot (PrivateData, BlockIoPpi, BlockIo2Ppi, IndexBlockDevice, RootDirLBA);
    if (!EFI_ERROR (Status)) {
      //
      // Just look for the first primary descriptor
      //
      return EFI_SUCCESS;
    }

    Lba++;
  }

  return EFI_NOT_FOUND;
}

/**
  Retrieves the recovery capsule in root directory of the current volume.

  @param PrivateData                    The private data structure that contains recovery module information.
  @param BlockIoPpi                     The Block IO PPI used to access the volume.
  @param BlockIo2Ppi                    The Block IO 2 PPI used to access the volume.
  @param IndexBlockDevice               The index of current block device.
  @param Lba                            The starting logic block address to retrieve capsule.

  @retval EFI_SUCCESS                   The recovery capsule is successfully found in the volume.
  @retval EFI_NOT_FOUND                 The recovery capsule is not found in the volume.
  @retval Others

**/
EFI_STATUS
EFIAPI
RetrieveCapsuleFileFromRoot (
  IN OUT PEI_CD_EXPRESS_PRIVATE_DATA  *PrivateData,
  IN EFI_PEI_RECOVERY_BLOCK_IO_PPI    *BlockIoPpi,
  IN EFI_PEI_RECOVERY_BLOCK_IO2_PPI   *BlockIo2Ppi,
  IN UINTN                            IndexBlockDevice,
  IN UINT32                           Lba
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  UINT8                           *Buffer;
  PEI_CD_EXPRESS_DIR_FILE_RECORD  *FileRecord;
  UINTN                           Index;

  Buffer     = PrivateData->BlockBuffer;
  BufferSize = PEI_CD_BLOCK_SIZE;

  SetMem (Buffer, BufferSize, 0);

  if (BlockIo2Ppi != NULL) {
    Status = BlockIo2Ppi->ReadBlocks (
                            (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                            BlockIo2Ppi,
                            IndexBlockDevice,
                            Lba,
                            BufferSize,
                            Buffer
                            );
  } else {
    Status = BlockIoPpi->ReadBlocks (
                           (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                           BlockIoPpi,
                           IndexBlockDevice,
                           Lba,
                           BufferSize,
                           Buffer
                           );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (1) {
    FileRecord = (PEI_CD_EXPRESS_DIR_FILE_RECORD *)Buffer;

    if (FileRecord->Length == 0) {
      break;
    }

    //
    // Not intend to check other flag now
    //
    if ((FileRecord->Flag & PEI_CD_EXPRESS_DIR_FILE_REC_FLAG_ISDIR) != 0) {
      Buffer += FileRecord->Length;
      continue;
    }

    for (Index = 0; Index < FileRecord->FileIDLength; Index++) {
      if (FileRecord->FileID[Index] == ';') {
        break;
      }
    }

    if (Index != mRecoveryFileNameSize - 1) {
      Buffer += FileRecord->Length;
      continue;
    }

    if (!StringCmp (FileRecord->FileID, (UINT8 *)mRecoveryFileName, mRecoveryFileNameSize - 1, FALSE)) {
      Buffer += FileRecord->Length;
      continue;
    }

    PrivateData->CapsuleData[PrivateData->CapsuleCount].CapsuleStartLBA         = FileRecord->LocationOfExtent[0];
    PrivateData->CapsuleData[PrivateData->CapsuleCount].CapsuleBlockAlignedSize =
      (
       FileRecord->DataLength[0] /
       PEI_CD_BLOCK_SIZE +
       1
      ) *
      PEI_CD_BLOCK_SIZE;
    PrivateData->CapsuleData[PrivateData->CapsuleCount].CapsuleSize = FileRecord->DataLength[0];

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Returns the number of DXE capsules residing on the device.

  This function searches for DXE capsules from the associated device and returns
  the number and maximum size in bytes of the capsules discovered. Entry 1 is
  assumed to be the highest load priority and entry N is assumed to be the lowest
  priority.

  @param[in]  PeiServices              General-purpose services that are available
                                       to every PEIM
  @param[in]  This                     Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI
                                       instance.
  @param[out] NumberRecoveryCapsules   Pointer to a caller-allocated UINTN. On
                                       output, *NumberRecoveryCapsules contains
                                       the number of recovery capsule images
                                       available for retrieval from this PEIM
                                       instance.

  @retval EFI_SUCCESS        One or more capsules were discovered.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
GetNumberRecoveryCapsules (
  IN EFI_PEI_SERVICES                    **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI  *This,
  OUT UINTN                              *NumberRecoveryCapsules
  )
{
  PEI_CD_EXPRESS_PRIVATE_DATA  *PrivateData;

  PrivateData = PEI_CD_EXPRESS_PRIVATE_DATA_FROM_THIS (This);
  UpdateBlocksAndVolumes (PrivateData, TRUE);
  UpdateBlocksAndVolumes (PrivateData, FALSE);
  *NumberRecoveryCapsules = PrivateData->CapsuleCount;

  if (*NumberRecoveryCapsules == 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Returns the size and type of the requested recovery capsule.

  This function gets the size and type of the capsule specified by CapsuleInstance.

  @param[in]  PeiServices       General-purpose services that are available to every PEIM
  @param[in]  This              Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI
                                instance.
  @param[in]  CapsuleInstance   Specifies for which capsule instance to retrieve
                                the information.  This parameter must be between
                                one and the value returned by GetNumberRecoveryCapsules()
                                in NumberRecoveryCapsules.
  @param[out] Size              A pointer to a caller-allocated UINTN in which
                                the size of the requested recovery module is
                                returned.
  @param[out] CapsuleType       A pointer to a caller-allocated EFI_GUID in which
                                the type of the requested recovery capsule is
                                returned.  The semantic meaning of the value
                                returned is defined by the implementation.

  @retval EFI_SUCCESS        One or more capsules were discovered.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
GetRecoveryCapsuleInfo (
  IN  EFI_PEI_SERVICES                    **PeiServices,
  IN  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI  *This,
  IN  UINTN                               CapsuleInstance,
  OUT UINTN                               *Size,
  OUT EFI_GUID                            *CapsuleType
  )
{
  PEI_CD_EXPRESS_PRIVATE_DATA  *PrivateData;
  UINTN                        NumberRecoveryCapsules;
  EFI_STATUS                   Status;

  Status = GetNumberRecoveryCapsules (PeiServices, This, &NumberRecoveryCapsules);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CapsuleInstance == 0) || (CapsuleInstance > NumberRecoveryCapsules)) {
    return EFI_NOT_FOUND;
  }

  PrivateData = PEI_CD_EXPRESS_PRIVATE_DATA_FROM_THIS (This);

  *Size = PrivateData->CapsuleData[CapsuleInstance - 1].CapsuleSize;
  CopyMem (
    CapsuleType,
    &gRecoveryOnDataCdGuid,
    sizeof (EFI_GUID)
    );

  return EFI_SUCCESS;
}

/**
  Loads a DXE capsule from some media into memory.

  This function, by whatever mechanism, retrieves a DXE capsule from some device
  and loads it into memory. Note that the published interface is device neutral.

  @param[in]     PeiServices       General-purpose services that are available
                                   to every PEIM
  @param[in]     This              Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI
                                   instance.
  @param[in]     CapsuleInstance   Specifies which capsule instance to retrieve.
  @param[out]    Buffer            Specifies a caller-allocated buffer in which
                                   the requested recovery capsule will be returned.

  @retval EFI_SUCCESS        The capsule was loaded correctly.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A requested recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
LoadRecoveryCapsule (
  IN EFI_PEI_SERVICES                    **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI  *This,
  IN UINTN                               CapsuleInstance,
  OUT VOID                               *Buffer
  )
{
  EFI_STATUS                      Status;
  PEI_CD_EXPRESS_PRIVATE_DATA     *PrivateData;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI   *BlockIoPpi;
  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *BlockIo2Ppi;
  UINTN                           NumberRecoveryCapsules;

  Status = GetNumberRecoveryCapsules (PeiServices, This, &NumberRecoveryCapsules);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CapsuleInstance == 0) || (CapsuleInstance > NumberRecoveryCapsules)) {
    return EFI_NOT_FOUND;
  }

  PrivateData = PEI_CD_EXPRESS_PRIVATE_DATA_FROM_THIS (This);
  BlockIoPpi  = PrivateData->CapsuleData[CapsuleInstance - 1].BlockIo;
  BlockIo2Ppi = PrivateData->CapsuleData[CapsuleInstance - 1].BlockIo2;

  if (BlockIo2Ppi != NULL) {
    Status = BlockIo2Ppi->ReadBlocks (
                            PeiServices,
                            BlockIo2Ppi,
                            PrivateData->CapsuleData[CapsuleInstance - 1].IndexBlock,
                            PrivateData->CapsuleData[CapsuleInstance - 1].CapsuleStartLBA,
                            PrivateData->CapsuleData[CapsuleInstance - 1].CapsuleBlockAlignedSize,
                            Buffer
                            );
  } else {
    Status = BlockIoPpi->ReadBlocks (
                           PeiServices,
                           BlockIoPpi,
                           PrivateData->CapsuleData[CapsuleInstance - 1].IndexBlock,
                           PrivateData->CapsuleData[CapsuleInstance - 1].CapsuleStartLBA,
                           PrivateData->CapsuleData[CapsuleInstance - 1].CapsuleBlockAlignedSize,
                           Buffer
                           );
  }

  return Status;
}

/**
  This function compares two ASCII strings in case sensitive/insensitive way.

  @param  Source1           The first string.
  @param  Source2           The second string.
  @param  Size              The maximum comparison length.
  @param  CaseSensitive     Flag to indicate whether the comparison is case sensitive.

  @retval TRUE              The two strings are the same.
  @retval FALSE             The two string are not the same.

**/
BOOLEAN
StringCmp (
  IN UINT8    *Source1,
  IN UINT8    *Source2,
  IN UINTN    Size,
  IN BOOLEAN  CaseSensitive
  )
{
  UINTN  Index;
  UINT8  Dif;

  for (Index = 0; Index < Size; Index++) {
    if (Source1[Index] == Source2[Index]) {
      continue;
    }

    if (!CaseSensitive) {
      Dif = (UINT8)((Source1[Index] > Source2[Index]) ? (Source1[Index] - Source2[Index]) : (Source2[Index] - Source1[Index]));
      if (Dif == ('a' - 'A')) {
        continue;
      }
    }

    return FALSE;
  }

  return TRUE;
}
