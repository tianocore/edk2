/** @file
  FAT recovery PEIM entry point, Ppi Functions and FAT Api functions.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FatLitePeim.h"

PEI_FAT_PRIVATE_DATA  *mPrivateData = NULL;

/**
  BlockIo installation nofication function. Find out all the current BlockIO
  PPIs in the system and add them into private data. Assume there is

  @param  PeiServices             General purpose services available to every
                                  PEIM.
  @param  NotifyDescriptor        The typedef structure of the notification
                                  descriptor. Not used in this function.
  @param  Ppi                     The typedef structure of the PPI descriptor.
                                  Not used in this function.

  @retval EFI_SUCCESS             The function completed successfully.

**/
EFI_STATUS
EFIAPI
BlockIoNotifyEntry (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );


/**
  Discover all the block I/O devices to find the FAT volume.

  @param  PrivateData             Global memory map for accessing global
                                  variables.
  @param  BlockIo2                Boolean to show whether using BlockIo2 or BlockIo

  @retval EFI_SUCCESS             The function completed successfully.

**/
EFI_STATUS
UpdateBlocksAndVolumes (
  IN OUT PEI_FAT_PRIVATE_DATA            *PrivateData,
  IN     BOOLEAN                         BlockIo2
  )
{
  EFI_STATUS                     Status;
  EFI_PEI_PPI_DESCRIPTOR         *TempPpiDescriptor;
  UINTN                          BlockIoPpiInstance;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *BlockIoPpi;
  EFI_PEI_RECOVERY_BLOCK_IO2_PPI *BlockIo2Ppi;
  UINTN                          NumberBlockDevices;
  UINTN                          Index;
  EFI_PEI_BLOCK_IO_MEDIA         Media;
  EFI_PEI_BLOCK_IO2_MEDIA        Media2;
  PEI_FAT_VOLUME                 Volume;
  EFI_PEI_SERVICES               **PeiServices;

  PeiServices = (EFI_PEI_SERVICES **) GetPeiServicesTablePointer ();
  BlockIo2Ppi = NULL;
  BlockIoPpi  = NULL;
  //
  // Clean up caches
  //
  for (Index = 0; Index < PEI_FAT_CACHE_SIZE; Index++) {
    PrivateData->CacheBuffer[Index].Valid = FALSE;
  }

  PrivateData->BlockDeviceCount = 0;

  //
  // Find out all Block Io Ppi instances within the system
  // Assuming all device Block Io Peims are dispatched already
  //
  for (BlockIoPpiInstance = 0; BlockIoPpiInstance < PEI_FAT_MAX_BLOCK_IO_PPI; BlockIoPpiInstance++) {
    if (BlockIo2) {
      Status = PeiServicesLocatePpi (
                &gEfiPeiVirtualBlockIo2PpiGuid,
                BlockIoPpiInstance,
                &TempPpiDescriptor,
                (VOID **) &BlockIo2Ppi
                );
    } else {
      Status = PeiServicesLocatePpi (
                &gEfiPeiVirtualBlockIoPpiGuid,
                BlockIoPpiInstance,
                &TempPpiDescriptor,
                (VOID **) &BlockIoPpi
                );
    }
    if (EFI_ERROR (Status)) {
      //
      // Done with all Block Io Ppis
      //
      break;
    }

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
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (Index = 1; Index <= NumberBlockDevices && PrivateData->BlockDeviceCount < PEI_FAT_MAX_BLOCK_DEVICE; Index++) {

      if (BlockIo2) {
        Status = BlockIo2Ppi->GetBlockDeviceMediaInfo (
                                PeiServices,
                                BlockIo2Ppi,
                                Index,
                                &Media2
                                );
        if (EFI_ERROR (Status) || !Media2.MediaPresent) {
          continue;
        }
        PrivateData->BlockDevice[PrivateData->BlockDeviceCount].BlockIo2        = BlockIo2Ppi;
        PrivateData->BlockDevice[PrivateData->BlockDeviceCount].InterfaceType   = Media2.InterfaceType;
        PrivateData->BlockDevice[PrivateData->BlockDeviceCount].LastBlock       = Media2.LastBlock;
        PrivateData->BlockDevice[PrivateData->BlockDeviceCount].BlockSize       = Media2.BlockSize;
      } else {
        Status = BlockIoPpi->GetBlockDeviceMediaInfo (
                               PeiServices,
                               BlockIoPpi,
                               Index,
                               &Media
                               );
        if (EFI_ERROR (Status) || !Media.MediaPresent) {
          continue;
        }
        PrivateData->BlockDevice[PrivateData->BlockDeviceCount].BlockIo    = BlockIoPpi;
        PrivateData->BlockDevice[PrivateData->BlockDeviceCount].DevType    = Media.DeviceType;
        PrivateData->BlockDevice[PrivateData->BlockDeviceCount].LastBlock  = Media.LastBlock;
        PrivateData->BlockDevice[PrivateData->BlockDeviceCount].BlockSize  = (UINT32) Media.BlockSize;
      }

      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].IoAlign = 0;
      //
      // Not used here
      //
      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].Logical           = FALSE;
      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].PartitionChecked  = FALSE;

      PrivateData->BlockDevice[PrivateData->BlockDeviceCount].PhysicalDevNo     = (UINT8) Index;
      PrivateData->BlockDeviceCount++;
    }
  }
  //
  // Find out all logical devices
  //
  FatFindPartitions (PrivateData);

  //
  // Build up file system volume array
  //
  PrivateData->VolumeCount = 0;
  for (Index = 0; Index < PrivateData->BlockDeviceCount; Index++) {
    Volume.BlockDeviceNo  = Index;
    Status                = FatGetBpbInfo (PrivateData, &Volume);
    if (Status == EFI_SUCCESS) {
      //
      // Add the detected volume to the volume array
      //
      CopyMem (
        (UINT8 *) &(PrivateData->Volume[PrivateData->VolumeCount]),
        (UINT8 *) &Volume,
        sizeof (PEI_FAT_VOLUME)
        );
      PrivateData->VolumeCount += 1;
      if (PrivateData->VolumeCount >= PEI_FAT_MAX_VOLUME) {
        break;
      }
    }
  }

  return EFI_SUCCESS;
}


/**
  BlockIo installation notification function. Find out all the current BlockIO
  PPIs in the system and add them into private data. Assume there is

  @param  PeiServices             General purpose services available to every
                                  PEIM.
  @param  NotifyDescriptor        The typedef structure of the notification
                                  descriptor. Not used in this function.
  @param  Ppi                     The typedef structure of the PPI descriptor.
                                  Not used in this function.

  @retval EFI_SUCCESS             The function completed successfully.

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
  Installs the Device Recovery Module PPI, Initialize BlockIo Ppi
  installation notification

  @param  FileHandle              Handle of the file being invoked. Type
                                  EFI_PEI_FILE_HANDLE is defined in
                                  FfsFindNextFile().
  @param  PeiServices             Describes the list of possible PEI Services.

  @retval EFI_SUCCESS             The entry point was executed successfully.
  @retval EFI_OUT_OF_RESOURCES    There is no enough memory to complete the
                                  operations.

**/
EFI_STATUS
EFIAPI
FatPeimEntry (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;
  PEI_FAT_PRIVATE_DATA  *PrivateData;

  Status = PeiServicesRegisterForShadow (FileHandle);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  Status = PeiServicesAllocatePages (
            EfiBootServicesCode,
            (sizeof (PEI_FAT_PRIVATE_DATA) - 1) / PEI_FAT_MEMMORY_PAGE_SIZE + 1,
            &Address
            );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData = (PEI_FAT_PRIVATE_DATA *) (UINTN) Address;

  //
  // Initialize Private Data (to zero, as is required by subsequent operations)
  //
  ZeroMem ((UINT8 *) PrivateData, sizeof (PEI_FAT_PRIVATE_DATA));

  PrivateData->Signature = PEI_FAT_PRIVATE_DATA_SIGNATURE;

  //
  // Installs Ppi
  //
  PrivateData->DeviceRecoveryPpi.GetNumberRecoveryCapsules  = GetNumberRecoveryCapsules;
  PrivateData->DeviceRecoveryPpi.GetRecoveryCapsuleInfo     = GetRecoveryCapsuleInfo;
  PrivateData->DeviceRecoveryPpi.LoadRecoveryCapsule        = LoadRecoveryCapsule;

  PrivateData->PpiDescriptor.Flags                          = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  PrivateData->PpiDescriptor.Guid = &gEfiPeiDeviceRecoveryModulePpiGuid;
  PrivateData->PpiDescriptor.Ppi  = &PrivateData->DeviceRecoveryPpi;

  Status = PeiServicesInstallPpi (&PrivateData->PpiDescriptor);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Other initializations
  //
  PrivateData->BlockDeviceCount = 0;

  UpdateBlocksAndVolumes (PrivateData, TRUE);
  UpdateBlocksAndVolumes (PrivateData, FALSE);

  //
  // PrivateData is allocated now, set it to the module variable
  //
  mPrivateData = PrivateData;

  //
  // Installs Block Io Ppi notification function
  //
  PrivateData->NotifyDescriptor[0].Flags =
    (
      EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK
    );
  PrivateData->NotifyDescriptor[0].Guid    = &gEfiPeiVirtualBlockIoPpiGuid;
  PrivateData->NotifyDescriptor[0].Notify  = BlockIoNotifyEntry;
  PrivateData->NotifyDescriptor[1].Flags  =
    (
      EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK |
      EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST
    );
  PrivateData->NotifyDescriptor[1].Guid    = &gEfiPeiVirtualBlockIo2PpiGuid;
  PrivateData->NotifyDescriptor[1].Notify  = BlockIoNotifyEntry;
  return PeiServicesNotifyPpi (&PrivateData->NotifyDescriptor[0]);
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
  IN EFI_PEI_SERVICES                               **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI             *This,
  OUT UINTN                                         *NumberRecoveryCapsules
  )
{
  EFI_STATUS            Status;
  PEI_FAT_PRIVATE_DATA  *PrivateData;
  UINTN                 Index;
  UINTN                 RecoveryCapsuleCount;
  PEI_FILE_HANDLE       Handle;

  PrivateData = PEI_FAT_PRIVATE_DATA_FROM_THIS (This);

  //
  // Search each volume in the root directory for the Recovery capsule
  //
  RecoveryCapsuleCount = 0;
  for (Index = 0; Index < PrivateData->VolumeCount; Index++) {
    Status = FindRecoveryFile (PrivateData, Index, (CHAR16 *)PcdGetPtr(PcdRecoveryFileName), &Handle);
    if (EFI_ERROR (Status)) {
      continue;
    }

    RecoveryCapsuleCount++;
  }

  *NumberRecoveryCapsules = RecoveryCapsuleCount;

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
  IN  EFI_PEI_SERVICES                              **PeiServices,
  IN  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI            *This,
  IN  UINTN                                         CapsuleInstance,
  OUT UINTN                                         *Size,
  OUT EFI_GUID                                      *CapsuleType
  )
{
  EFI_STATUS            Status;
  PEI_FAT_PRIVATE_DATA  *PrivateData;
  UINTN                 Index;
  UINTN                 BlockDeviceNo;
  UINTN                 RecoveryCapsuleCount;
  PEI_FILE_HANDLE       Handle;
  UINTN                 NumberRecoveryCapsules;

  Status = GetNumberRecoveryCapsules (PeiServices, This, &NumberRecoveryCapsules);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CapsuleInstance == 0) || (CapsuleInstance > NumberRecoveryCapsules)) {
    return EFI_NOT_FOUND;
  }

  PrivateData = PEI_FAT_PRIVATE_DATA_FROM_THIS (This);

  //
  // Search each volume in the root directory for the Recovery capsule
  //
  RecoveryCapsuleCount = 0;
  for (Index = 0; Index < PrivateData->VolumeCount; Index++) {
    Status = FindRecoveryFile (PrivateData, Index, (CHAR16 *)PcdGetPtr(PcdRecoveryFileName), &Handle);

    if (EFI_ERROR (Status)) {
      continue;
    }

    if (CapsuleInstance - 1 == RecoveryCapsuleCount) {
      //
      // Get file size
      //
      *Size = (UINTN) (((PEI_FAT_FILE *) Handle)->FileSize);

      //
      // Find corresponding physical block device
      //
      BlockDeviceNo = PrivateData->Volume[Index].BlockDeviceNo;
      while (PrivateData->BlockDevice[BlockDeviceNo].Logical && BlockDeviceNo < PrivateData->BlockDeviceCount) {
        BlockDeviceNo = PrivateData->BlockDevice[BlockDeviceNo].ParentDevNo;
      }
      //
      // Fill in the Capsule Type GUID according to the block device type
      //
      if (BlockDeviceNo < PrivateData->BlockDeviceCount) {
        if (PrivateData->BlockDevice[BlockDeviceNo].BlockIo2 != NULL) {
          switch (PrivateData->BlockDevice[BlockDeviceNo].InterfaceType) {
          case MSG_ATAPI_DP:
            CopyGuid (CapsuleType, &gRecoveryOnFatIdeDiskGuid);
            break;

          case MSG_USB_DP:
            CopyGuid (CapsuleType, &gRecoveryOnFatUsbDiskGuid);
            break;

          case MSG_NVME_NAMESPACE_DP:
            CopyGuid (CapsuleType, &gRecoveryOnFatNvmeDiskGuid);
            break;

          default:
            break;
          }
        }
        if (PrivateData->BlockDevice[BlockDeviceNo].BlockIo != NULL) {
          switch (PrivateData->BlockDevice[BlockDeviceNo].DevType) {
          case LegacyFloppy:
            CopyGuid (CapsuleType, &gRecoveryOnFatFloppyDiskGuid);
            break;

          case IdeCDROM:
          case IdeLS120:
            CopyGuid (CapsuleType, &gRecoveryOnFatIdeDiskGuid);
            break;

          case UsbMassStorage:
            CopyGuid (CapsuleType, &gRecoveryOnFatUsbDiskGuid);
            break;

          default:
            break;
          }
        }
      }

      return EFI_SUCCESS;
    }

    RecoveryCapsuleCount++;
  }

  return EFI_NOT_FOUND;
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
  IN EFI_PEI_SERVICES                             **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI           *This,
  IN UINTN                                        CapsuleInstance,
  OUT VOID                                        *Buffer
  )
{
  EFI_STATUS            Status;
  PEI_FAT_PRIVATE_DATA  *PrivateData;
  UINTN                 Index;
  UINTN                 RecoveryCapsuleCount;
  PEI_FILE_HANDLE       Handle;
  UINTN                 NumberRecoveryCapsules;

  Status = GetNumberRecoveryCapsules (PeiServices, This, &NumberRecoveryCapsules);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CapsuleInstance == 0) || (CapsuleInstance > NumberRecoveryCapsules)) {
    return EFI_NOT_FOUND;
  }

  PrivateData = PEI_FAT_PRIVATE_DATA_FROM_THIS (This);

  //
  // Search each volume in the root directory for the Recovery capsule
  //
  RecoveryCapsuleCount = 0;
  for (Index = 0; Index < PrivateData->VolumeCount; Index++) {
    Status = FindRecoveryFile (PrivateData, Index, (CHAR16 *)PcdGetPtr(PcdRecoveryFileName), &Handle);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (CapsuleInstance - 1 == RecoveryCapsuleCount) {

      Status = FatReadFile (
                PrivateData,
                Handle,
                (UINTN) (((PEI_FAT_FILE *) Handle)->FileSize),
                Buffer
                );
      return Status;
    }

    RecoveryCapsuleCount++;
  }

  return EFI_NOT_FOUND;
}


/**
  Finds the recovery file on a FAT volume.
  This function finds the the recovery file named FileName on a specified FAT volume and returns
  its FileHandle pointer.

  @param  PrivateData             Global memory map for accessing global
                                  variables.
  @param  VolumeIndex             The index of the volume.
  @param  FileName                The recovery file name to find.
  @param  Handle                  The output file handle.

  @retval EFI_DEVICE_ERROR        Some error occured when operating the FAT
                                  volume.
  @retval EFI_NOT_FOUND           The recovery file was not found.
  @retval EFI_SUCCESS             The recovery file was successfully found on the
                                  FAT volume.

**/
EFI_STATUS
FindRecoveryFile (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  UINTN                 VolumeIndex,
  IN  CHAR16                *FileName,
  OUT PEI_FILE_HANDLE       *Handle
  )
{
  EFI_STATUS    Status;
  PEI_FAT_FILE  Parent;
  PEI_FAT_FILE  *File;

  File = &PrivateData->File;

  //
  // VolumeIndex must be less than PEI_FAT_MAX_VOLUME because PrivateData->VolumeCount
  // cannot be larger than PEI_FAT_MAX_VOLUME when detecting recovery volume.
  //
  ASSERT (VolumeIndex < PEI_FAT_MAX_VOLUME);

  //
  // Construct root directory file
  //
  ZeroMem (&Parent, sizeof (PEI_FAT_FILE));
  Parent.IsFixedRootDir   = (BOOLEAN) ((PrivateData->Volume[VolumeIndex].FatType == Fat32) ? FALSE : TRUE);
  Parent.Attributes       = FAT_ATTR_DIRECTORY;
  Parent.CurrentPos       = 0;
  Parent.CurrentCluster   = Parent.IsFixedRootDir ? 0 : PrivateData->Volume[VolumeIndex].RootDirCluster;
  Parent.StartingCluster  = Parent.CurrentCluster;
  Parent.Volume           = &PrivateData->Volume[VolumeIndex];

  Status                  = FatSetFilePos (PrivateData, &Parent, 0);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Search for recovery capsule in root directory
  //
  Status = FatReadNextDirectoryEntry (PrivateData, &Parent, File);
  while (Status == EFI_SUCCESS) {
    //
    // Compare whether the file name is recovery file name.
    //
    if (EngStriColl (PrivateData, FileName, File->FileName)) {
      break;
    }

    Status = FatReadNextDirectoryEntry (PrivateData, &Parent, File);
  }

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the recovery file, set its file position to 0.
  //
  if (File->StartingCluster != 0) {
    Status = FatSetFilePos (PrivateData, File, 0);
  }

  *Handle = File;

  return EFI_SUCCESS;

}
