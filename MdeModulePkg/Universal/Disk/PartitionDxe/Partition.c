/** @file
  Partition driver that produces logical BlockIo devices from a physical
  BlockIo device. The logical BlockIo devices are based on the format
  of the raw block devices media. Currently "El Torito CD-ROM", Legacy
  MBR, and GPT partition schemes are supported.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Partition.h"

//
// Partition Driver Global Variables.
//
EFI_DRIVER_BINDING_PROTOCOL gPartitionDriverBinding = {
  PartitionDriverBindingSupported,
  PartitionDriverBindingStart,
  PartitionDriverBindingStop,
  //
  // Grub4Dos copies the BPB of the first partition to the MBR. If the 
  // DriverBindingStart() of the Fat driver gets run before that of Partition 
  // driver only the first partition can be recognized.
  // Let the driver binding version of Partition driver be higher than that of
  // Fat driver to make sure the DriverBindingStart() of the Partition driver
  // gets run before that of Fat driver so that all the partitions can be recognized.
  //
  0xb,
  NULL,
  NULL
};

//
// Prioritized function list to detect partition table. 
//
PARTITION_DETECT_ROUTINE mPartitionDetectRoutineTable[] = {
  PartitionInstallGptChildHandles,
  PartitionInstallElToritoChildHandles,
  PartitionInstallMbrChildHandles,
  NULL
};

/**
  Test to see if this driver supports ControllerHandle. Any ControllerHandle
  than contains a BlockIo and DiskIo protocol or a BlockIo2 protocol can be
  supported.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of device to test.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific child
                                  device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
PartitionDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_DISK_IO_PROTOCOL      *DiskIo;
  EFI_DEV_PATH              *Node;

  //
  // Check RemainingDevicePath validation
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node, 
    // if yes, go on checking other conditions
    //
    if (!IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath isn't the End of Device Path Node,
      // check its validation
      //
      Node = (EFI_DEV_PATH *) RemainingDevicePath;
      if (Node->DevPath.Type != MEDIA_DEVICE_PATH ||
        Node->DevPath.SubType != MEDIA_HARDDRIVE_DP ||
        DevicePathNodeLength (&Node->DevPath) != sizeof (HARDDRIVE_DEVICE_PATH)) {
        return EFI_UNSUPPORTED;
      }
    }
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **) &DiskIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDiskIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close protocol, don't use device path protocol in the Support() function
  //
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;  
}

/**
  Start this driver on ControllerHandle by opening a Block IO or a Block IO2
  or both, and Disk IO protocol, reading Device Path, and creating a child
  handle with a Disk IO and device path protocol.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to bind driver to
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific child
                                   device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
PartitionDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_STATUS                OpenStatus;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL    *BlockIo2;
  EFI_DISK_IO_PROTOCOL      *DiskIo;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  PARTITION_DETECT_ROUTINE  *Routine;
  BOOLEAN                   MediaPresent;
  EFI_TPL                   OldTpl;

  BlockIo2 = NULL;
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK); 
  //
  // Check RemainingDevicePath validation
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node, 
    // if yes, return EFI_SUCCESS
    //
    if (IsDevicePathEnd (RemainingDevicePath)) {
      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

  //
  // Try to open BlockIO and BlockIO2. If BlockIO would be opened, continue,
  // otherwise, return error.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlockIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIo2ProtocolGuid,
                  (VOID **) &BlockIo2,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  //
  // Get the Device Path Protocol on ControllerHandle's handle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **) &DiskIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    goto Exit;
  }

  OpenStatus = Status;

  //
  // Try to read blocks when there's media or it is removable physical partition.
  //
  Status       = EFI_UNSUPPORTED;
  MediaPresent = BlockIo->Media->MediaPresent;
  if (BlockIo->Media->MediaPresent ||
      (BlockIo->Media->RemovableMedia && !BlockIo->Media->LogicalPartition)) {
    //
    // Try for GPT, then El Torito, and then legacy MBR partition types. If the
    // media supports a given partition type install child handles to represent
    // the partitions described by the media.
    //
    Routine = &mPartitionDetectRoutineTable[0];
    while (*Routine != NULL) {
      Status = (*Routine) (
                   This,
                   ControllerHandle,
                   DiskIo,
                   BlockIo,
                   BlockIo2,
                   ParentDevicePath
                   );
      if (!EFI_ERROR (Status) || Status == EFI_MEDIA_CHANGED || Status == EFI_NO_MEDIA) {
        break;
      }
      Routine++;
    }
  }
  //
  // In the case that the driver is already started (OpenStatus == EFI_ALREADY_STARTED),
  // the DevicePathProtocol and the DiskIoProtocol are not actually opened by the
  // driver. So don't try to close them. Otherwise, we will break the dependency
  // between the controller and the driver set up before.
  //
  // In the case that when the media changes on a device it will Reinstall the 
  // BlockIo interaface. This will cause a call to our Stop(), and a subsequent
  // reentrant call to our Start() successfully. We should leave the device open
  // when this happen. The "media change" case includes either the status is
  // EFI_MEDIA_CHANGED or it is a "media" to "no media" change. 
  //  
  if (EFI_ERROR (Status)          &&
      !EFI_ERROR (OpenStatus)     &&
      Status != EFI_MEDIA_CHANGED &&
      !(MediaPresent && Status == EFI_NO_MEDIA)) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDiskIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    //
    // Close Parent BlockIO2 if has.
    //    
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiBlockIo2ProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
  }

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
PartitionDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL  *BlockIo2;
  BOOLEAN                 AllChildrenStopped;
  PARTITION_PRIVATE_DATA  *Private;
  EFI_DISK_IO_PROTOCOL    *DiskIo;

  BlockIo  = NULL;
  BlockIo2 = NULL;
  Private = NULL;

  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDiskIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    //
    // Close Parent BlockIO2 if has.
    //    
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiBlockIo2ProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;
  for (Index = 0; Index < NumberOfChildren; Index++) {
    gBS->OpenProtocol (
           ChildHandleBuffer[Index],
           &gEfiBlockIoProtocolGuid,
           (VOID **) &BlockIo,
           This->DriverBindingHandle,
           ControllerHandle,
           EFI_OPEN_PROTOCOL_GET_PROTOCOL
           );
    //
    // Try to locate BlockIo2.
    //
    gBS->OpenProtocol (
           ChildHandleBuffer[Index],
           &gEfiBlockIo2ProtocolGuid,
           (VOID **) &BlockIo2,
           This->DriverBindingHandle,
           ControllerHandle,
           EFI_OPEN_PROTOCOL_GET_PROTOCOL
           ); 


    Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (BlockIo);

    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiDiskIoProtocolGuid,
                    This->DriverBindingHandle,
                    ChildHandleBuffer[Index]
                    );
    //
    // All Software protocols have be freed from the handle so remove it.
    // Remove the BlockIo Protocol if has.
    // Remove the BlockIo2 Protocol if has.
    //
    if (BlockIo2 != NULL) {
      BlockIo->FlushBlocks (BlockIo);
      BlockIo2->FlushBlocksEx (BlockIo2, NULL);
      Status = gBS->UninstallMultipleProtocolInterfaces (
                       ChildHandleBuffer[Index],
                       &gEfiDevicePathProtocolGuid,
                       Private->DevicePath,
                       &gEfiBlockIoProtocolGuid,
                       &Private->BlockIo,
                       &gEfiBlockIo2ProtocolGuid,
                       &Private->BlockIo2,
                       Private->EspGuid,
                       NULL,
                       NULL
                       );
    } else {
      BlockIo->FlushBlocks (BlockIo);
      Status = gBS->UninstallMultipleProtocolInterfaces (
                       ChildHandleBuffer[Index],
                       &gEfiDevicePathProtocolGuid,
                       Private->DevicePath,
                       &gEfiBlockIoProtocolGuid,
                       &Private->BlockIo,
                       Private->EspGuid,
                       NULL,
                       NULL
                       );
    }

    if (EFI_ERROR (Status)) {
      gBS->OpenProtocol (
             ControllerHandle,
             &gEfiDiskIoProtocolGuid,
             (VOID **) &DiskIo,
             This->DriverBindingHandle,
             ChildHandleBuffer[Index],
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
    } else {
      FreePool (Private->DevicePath);
      FreePool (Private);
    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Reset the Block Device.

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
PartitionReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  PARTITION_PRIVATE_DATA  *Private;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (This);

  return Private->ParentBlockIo->Reset (
                                  Private->ParentBlockIo,
                                  ExtendedVerification
                                  );
}

/**
  Probe the media status and return EFI_NO_MEDIA or EFI_MEDIA_CHANGED
  for no media or media change case. Otherwise DefaultStatus is returned.

  @param DiskIo             Pointer to the DiskIo instance.
  @param MediaId            Id of the media, changes every time the media is replaced.
  @param DefaultStatus      The default status to return when it's not the no media
                            or media change case.

  @retval EFI_NO_MEDIA      There is no media.
  @retval EFI_MEDIA_CHANGED The media was changed.
  @retval others            The default status to return.
**/
EFI_STATUS
ProbeMediaStatus (
  IN EFI_DISK_IO_PROTOCOL    *DiskIo,
  IN UINT32                  MediaId,
  IN EFI_STATUS              DefaultStatus
  )
{
  EFI_STATUS                 Status;

  //
  // Read 1 byte from offset 0 but passing NULL as buffer pointer
  //
  Status = DiskIo->ReadDisk (DiskIo, MediaId, 0, 1, NULL);
  if ((Status == EFI_NO_MEDIA) || (Status == EFI_MEDIA_CHANGED)) {
    return Status;
  }
  return DefaultStatus;
}

/**
  Read by using the Disk IO protocol on the parent device. Lba addresses
  must be converted to byte offsets.

  @param  This       Protocol instance pointer.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     Buffer containing read data

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
PartitionReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  )
{
  PARTITION_PRIVATE_DATA  *Private;
  UINT64                  Offset;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (This);

  if (BufferSize % Private->BlockSize != 0) {
    return ProbeMediaStatus (Private->DiskIo, MediaId, EFI_BAD_BUFFER_SIZE);
  }

  Offset = MultU64x32 (Lba, Private->BlockSize) + Private->Start;
  if (Offset + BufferSize > Private->End) {
    return ProbeMediaStatus (Private->DiskIo, MediaId, EFI_INVALID_PARAMETER);
  }
  //
  // Because some kinds of partition have different block size from their parent
  // device, we call the Disk IO protocol on the parent device, not the Block IO
  // protocol
  //
  return Private->DiskIo->ReadDisk (Private->DiskIo, MediaId, Offset, BufferSize, Buffer);
}

/**
  Write by using the Disk IO protocol on the parent device. Lba addresses
  must be converted to byte offsets.

  @param[in]  This       Protocol instance pointer.
  @param[in]  MediaId    Id of the media, changes every time the media is replaced.
  @param[in]  Lba        The starting Logical Block Address to read from
  @param[in]  BufferSize Size of Buffer, must be a multiple of device block size.
  @param[in]  Buffer     Buffer containing data to be written to device.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains a LBA that is not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
PartitionWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                  *Buffer
  )
{
  PARTITION_PRIVATE_DATA  *Private;
  UINT64                  Offset;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (This);

  if (BufferSize % Private->BlockSize != 0) {
    return ProbeMediaStatus (Private->DiskIo, MediaId, EFI_BAD_BUFFER_SIZE);
  }

  Offset = MultU64x32 (Lba, Private->BlockSize) + Private->Start;
  if (Offset + BufferSize > Private->End) {
    return ProbeMediaStatus (Private->DiskIo, MediaId, EFI_INVALID_PARAMETER);
  }
  //
  // Because some kinds of partition have different block size from their parent
  // device, we call the Disk IO protocol on the parent device, not the Block IO
  // protocol
  //
  return Private->DiskIo->WriteDisk (Private->DiskIo, MediaId, Offset, BufferSize, Buffer);
}


/**
  Flush the parent Block Device.

  @param  This              Protocol instance pointer.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writting back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
PartitionFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  PARTITION_PRIVATE_DATA  *Private;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO_THIS (This);

  return Private->ParentBlockIo->FlushBlocks (Private->ParentBlockIo);
}

/**
  Probe the media status and return EFI_NO_MEDIA or EFI_MEDIA_CHANGED
  for no media or media change case. Otherwise DefaultStatus is returned.

  @param BlockIo2           Pointer to the BlockIo2 instance.
  @param MediaId            Id of the media, changes every time the media is replaced.
  @param DefaultStatus      The default status to return when it's not the no media
                            or media change case.

  @retval EFI_NO_MEDIA      There is no media.
  @retval EFI_MEDIA_CHANGED The media was changed.
  @retval others            The default status to return.
**/
EFI_STATUS
ProbeMediaStatusEx (
  IN EFI_BLOCK_IO2_PROTOCOL  *BlockIo2,
  IN UINT32                  MediaId,
  IN EFI_STATUS              DefaultStatus
  )
{
  EFI_STATUS                 Status;

  //
  // Read from LBA 0 but passing NULL as buffer pointer to detect the media status.
  //
  Status = BlockIo2->ReadBlocksEx (
                       BlockIo2,
                       MediaId,
                       0,
                       NULL,
                       0,
                       NULL
                       );
  if ((Status == EFI_NO_MEDIA) || (Status == EFI_MEDIA_CHANGED)) {
    return Status;
  }
  return DefaultStatus;
}

/**
  Reset the Block Device throught Block I/O2 protocol.

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
PartitionResetEx (
  IN EFI_BLOCK_IO2_PROTOCOL *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  PARTITION_PRIVATE_DATA  *Private;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO2_THIS (This);

  return Private->ParentBlockIo2->Reset (
                                    Private->ParentBlockIo2,
                                    ExtendedVerification
                                    );
}

/**
  Read BufferSize bytes from Lba into Buffer.
  
  This function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned.
  If EFI_DEVICE_ERROR, EFI_NO_MEDIA,_or EFI_MEDIA_CHANGED is returned and
  non-blocking I/O is being used, the Event associated with this request will
  not be signaled.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    Id of the media, changes every time the media is 
                              replaced.
  @param[in]       Lba        The starting Logical Block Address to read from.
  @param[in, out]  Token	    A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.  
  @param[out]      Buffer     A pointer to the destination buffer for the data. The 
                              caller is responsible for either having implicit or 
                              explicit ownership of the buffer.

  @retval EFI_SUCCESS           The read request was queued if Token->Event is
                                not NULL.The data was read correctly from the
                                device if the Token->Event is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing
                                the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of the
                                intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.
**/
EFI_STATUS
EFIAPI
PartitionReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  OUT    VOID                   *Buffer
  )
{
  PARTITION_PRIVATE_DATA  *Private;
  UINT64                  Offset;
  UINT32                  UnderRun;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO2_THIS (This);

  if (Token == NULL) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_INVALID_PARAMETER);
  }

  if (BufferSize % Private->BlockSize != 0) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_BAD_BUFFER_SIZE);
  }

  Offset = MultU64x32 (Lba, Private->BlockSize) + Private->Start;
  if (Offset + BufferSize > Private->End) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_INVALID_PARAMETER);
  }

  //
  // Since the BlockIO2 call Parent BlockIO2 directly, so here the offset must
  // be multiple of BlockSize. If the Spec will be updated the DiskIO to support
  // BlockIO2, this limitation will be removed and call DiskIO here.
  //
  Lba = DivU64x32Remainder (Offset, Private->BlockSize, &UnderRun);
  if (UnderRun != 0) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_UNSUPPORTED);
  }

  //
  // Because some partitions have different block size from their parent
  // device, in that case the Block I/O2 couldn't be called.
  //
  if (Private->BlockSize != Private->ParentBlockIo->Media->BlockSize) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_UNSUPPORTED);
  }

  return Private->ParentBlockIo2->ReadBlocksEx (Private->ParentBlockIo2, MediaId, Lba, Token, BufferSize, Buffer);
}

/**
  Write BufferSize bytes from Lba into Buffer.

  This function writes the requested number of blocks to the device. All blocks
  are written, or an error is returned.If EFI_DEVICE_ERROR, EFI_NO_MEDIA,
  EFI_WRITE_PROTECTED or EFI_MEDIA_CHANGED is returned and non-blocking I/O is
  being used, the Event associated with this request will not be signaled.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    The media ID that the write request is for.
  @param[in]       Lba        The starting logical block address to be written. The
                              caller is responsible for writing to only legitimate
                              locations.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[in]       Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The write request was queued if Event is not NULL.
                                The data was written correctly to the device if
                                the Event is NULL.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.

**/
EFI_STATUS
EFIAPI
PartitionWriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  IN     VOID                   *Buffer
  )
{
  PARTITION_PRIVATE_DATA  *Private;
  UINT64                  Offset;
  UINT32                  UnderRun;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO2_THIS (This);

  if (Token == NULL) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_INVALID_PARAMETER);
  }

  if (BufferSize % Private->BlockSize != 0) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_BAD_BUFFER_SIZE);
  }

  Offset = MultU64x32 (Lba, Private->BlockSize) + Private->Start;
  if (Offset + BufferSize > Private->End) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_INVALID_PARAMETER);
  }

  //
  // Since the BlockIO2 call Parent BlockIO2 directly, so here the offset must
  // be multiple of BlockSize. If the Spec will be updated the DiskIO to support
  // BlockIO2, this limitation will be removed and call DiskIO here.
  //
  Lba = DivU64x32Remainder (Offset, Private->BlockSize, &UnderRun);
  if (UnderRun != 0) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_UNSUPPORTED);
  }

  //
  // Because some kinds of partition have different block size from their parent,
  // in that case it couldn't call parent Block I/O2. 
  //
  if (Private->BlockSize != Private->ParentBlockIo->Media->BlockSize) {
    return ProbeMediaStatusEx (Private->ParentBlockIo2, MediaId, EFI_UNSUPPORTED);
  }

  return Private->ParentBlockIo2->WriteBlocksEx (Private->ParentBlockIo2, MediaId, Lba, Token, BufferSize, Buffer);
}

/**
  Flush the Block Device.
 
  If EFI_DEVICE_ERROR, EFI_NO_MEDIA,_EFI_WRITE_PROTECTED or EFI_MEDIA_CHANGED
  is returned and non-blocking I/O is being used, the Event associated with
  this request will not be signaled.  

  @param[in]      This     Indicates a pointer to the calling context.
  @param[in, out] Token    A pointer to the token associated with the transaction

  @retval EFI_SUCCESS          The flush request was queued if Event is not NULL.
                               All outstanding data was written correctly to the
                               device if the Event is NULL.
  @retval EFI_DEVICE_ERROR     The device reported an error while writting back
                               the data.
  @retval EFI_WRITE_PROTECTED  The device cannot be written to.
  @retval EFI_NO_MEDIA         There is no media in the device.
  @retval EFI_MEDIA_CHANGED    The MediaId is not for the current media.
  @retval EFI_OUT_OF_RESOURCES The request could not be completed due to a lack
                               of resources.

**/
EFI_STATUS
EFIAPI
PartitionFlushBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token
  )
{
  PARTITION_PRIVATE_DATA  *Private;

  Private = PARTITION_DEVICE_FROM_BLOCK_IO2_THIS (This);

  //
  // Because some kinds of partition have different block size from their parent,
  // in that case it couldn't call parent Block I/O2. 
  //
  if (Private->BlockSize != Private->ParentBlockIo->Media->BlockSize) {
    return EFI_UNSUPPORTED;
  }

  return Private->ParentBlockIo2->FlushBlocksEx (Private->ParentBlockIo2, Token);
}


/**
  Create a child handle for a logical block device that represents the
  bytes Start to End of the Parent Block IO device.

  @param[in]  This              Protocol instance pointer.
  @param[in]  ParentHandle      Parent Handle for new child.
  @param[in]  ParentDiskIo      Parent DiskIo interface.
  @param[in]  ParentBlockIo     Parent BlockIo interface.
  @param[in]  ParentBlockIo2    Parent BlockIo2 interface.
  @param[in]  ParentDevicePath  Parent Device Path.
  @param[in]  DevicePathNode    Child Device Path node.
  @param[in]  Start             Start Block.
  @param[in]  End               End Block.
  @param[in]  BlockSize         Child block size.
  @param[in]  InstallEspGuid    Flag to install EFI System Partition GUID on handle.

  @retval EFI_SUCCESS       A child handle was added.
  @retval other             A child handle was not added.

**/
EFI_STATUS
PartitionInstallChildHandle (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ParentHandle,
  IN  EFI_DISK_IO_PROTOCOL         *ParentDiskIo,
  IN  EFI_BLOCK_IO_PROTOCOL        *ParentBlockIo,
  IN  EFI_BLOCK_IO2_PROTOCOL       *ParentBlockIo2,
  IN  EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePathNode,
  IN  EFI_LBA                      Start,
  IN  EFI_LBA                      End,
  IN  UINT32                       BlockSize,
  IN  BOOLEAN                      InstallEspGuid
  )
{
  EFI_STATUS              Status;
  PARTITION_PRIVATE_DATA  *Private;

  Status  = EFI_SUCCESS;
  Private = AllocateZeroPool (sizeof (PARTITION_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature        = PARTITION_PRIVATE_DATA_SIGNATURE;

  Private->Start            = MultU64x32 (Start, ParentBlockIo->Media->BlockSize);
  Private->End              = MultU64x32 (End + 1, ParentBlockIo->Media->BlockSize);

  Private->BlockSize        = BlockSize;
  Private->ParentBlockIo    = ParentBlockIo;
  Private->ParentBlockIo2   = ParentBlockIo2;
  Private->DiskIo           = ParentDiskIo;

  //
  // Set the BlockIO into Private Data.
  //
  Private->BlockIo.Revision = ParentBlockIo->Revision;
  
  Private->BlockIo.Media    = &Private->Media;
  CopyMem (Private->BlockIo.Media, ParentBlockIo->Media, sizeof (EFI_BLOCK_IO_MEDIA));

  Private->BlockIo.Reset        = PartitionReset;
  Private->BlockIo.ReadBlocks   = PartitionReadBlocks;
  Private->BlockIo.WriteBlocks  = PartitionWriteBlocks;
  Private->BlockIo.FlushBlocks  = PartitionFlushBlocks;

  //
  // Set the BlockIO2 into Private Data.
  //
  if (Private->ParentBlockIo2 != NULL) {
    Private->BlockIo2.Media    = &Private->Media2;
    CopyMem (Private->BlockIo2.Media, ParentBlockIo2->Media, sizeof (EFI_BLOCK_IO_MEDIA));

    Private->BlockIo2.Reset          = PartitionResetEx;
    Private->BlockIo2.ReadBlocksEx   = PartitionReadBlocksEx;
    Private->BlockIo2.WriteBlocksEx  = PartitionWriteBlocksEx;
    Private->BlockIo2.FlushBlocksEx  = PartitionFlushBlocksEx; 
  }

  Private->Media.IoAlign   = 0;
  Private->Media.LogicalPartition = TRUE;
  Private->Media.LastBlock = DivU64x32 (
                               MultU64x32 (
                                 End - Start + 1,
                                 ParentBlockIo->Media->BlockSize
                                 ),
                                BlockSize
                               ) - 1;

  Private->Media.BlockSize = (UINT32) BlockSize;

  //
  // For BlockIO2, it should keep the same alignment with the parent BlockIO2's.
  //
  Private->Media2.LogicalPartition = TRUE;
  Private->Media2.LastBlock = Private->Media.LastBlock;
  Private->Media2.BlockSize = (UINT32) BlockSize;

  //
  // Per UEFI Spec, LowestAlignedLba, LogicalBlocksPerPhysicalBlock and OptimalTransferLengthGranularity must be 0
  //  for logical partitions.
  //
  if (Private->BlockIo.Revision >= EFI_BLOCK_IO_PROTOCOL_REVISION2) {
    Private->Media.LowestAlignedLba               = 0;
    Private->Media.LogicalBlocksPerPhysicalBlock  = 0;
    Private->Media2.LowestAlignedLba              = 0;
    Private->Media2.LogicalBlocksPerPhysicalBlock = 0;
    if (Private->BlockIo.Revision >= EFI_BLOCK_IO_PROTOCOL_REVISION3) {
      Private->Media.OptimalTransferLengthGranularity  = 0;
      Private->Media2.OptimalTransferLengthGranularity = 0;
    }
  }

  Private->DevicePath     = AppendDevicePathNode (ParentDevicePath, DevicePathNode);

  if (Private->DevicePath == NULL) {
    FreePool (Private);
    return EFI_OUT_OF_RESOURCES;
  }

  if (InstallEspGuid) {
    Private->EspGuid = &gEfiPartTypeSystemPartGuid;
  } else {
    //
    // If NULL InstallMultipleProtocolInterfaces will ignore it.
    //
    Private->EspGuid = NULL;
  }

  //
  // Create the new handle. 
  // BlockIO2 will be installed on the condition that the blocksize of parent BlockIO 
  // is same with the child BlockIO's. Instead of calling the DiskIO, the child BlockIO2 
  // directly call the parent BlockIO and doesn't handle the different block size issue.
  // If SPEC will update the DiskIO to support the Non-Blocking model, the BlockIO2 will call
  // DiskIO to handle the blocksize unequal issue and the limitation will be remove from
  // here.
  //
  Private->Handle = NULL;
  if ((Private->ParentBlockIo2 != NULL) &&
      (Private->ParentBlockIo2->Media->BlockSize == BlockSize)
     ) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Private->Handle,
                    &gEfiDevicePathProtocolGuid,
                    Private->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Private->BlockIo,
                    &gEfiBlockIo2ProtocolGuid,
                    &Private->BlockIo2,
                    Private->EspGuid,
                    NULL,
                    NULL
                    );
  } else {    
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Private->Handle,
                    &gEfiDevicePathProtocolGuid,
                    Private->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Private->BlockIo,
                    Private->EspGuid,
                    NULL,
                    NULL
                    );
  }

  if (!EFI_ERROR (Status)) {
    //
    // Open the Parent Handle for the child
    //
    Status = gBS->OpenProtocol (
                    ParentHandle,
                    &gEfiDiskIoProtocolGuid,
                    (VOID **) &ParentDiskIo,
                    This->DriverBindingHandle,
                    Private->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
  } else {
    FreePool (Private->DevicePath);
    FreePool (Private);
  }

  return Status;
}


/**
  The user Entry Point for module Partition. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializePartition (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gPartitionDriverBinding,
             ImageHandle,
             &gPartitionComponentName,
             &gPartitionComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

