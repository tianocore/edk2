/** @file
  SCSI disk driver that layers on every SCSI IO protocol in the system.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "ScsiDisk.h"


EFI_DRIVER_BINDING_PROTOCOL gScsiDiskDriverBinding = {
  ScsiDiskDriverBindingSupported,
  ScsiDiskDriverBindingStart,
  ScsiDiskDriverBindingStop,
  0xa,
  NULL,
  NULL
};


/**
  The user Entry Point for module ScsiDisk.

  The user code starts with this function.

  @param  ImageHandle    The firmware allocated handle for the EFI image.  
  @param  SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeScsiDisk(
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
             &gScsiDiskDriverBinding,
             ImageHandle,
             &gScsiDiskComponentName,
             &gScsiDiskComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

/**
  Test to see if this driver supports ControllerHandle.

  This service is called by the EFI boot service ConnectController(). In order
  to make drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these calling restrictions.
  If any other agent wishes to call Supported() it must also follow these
  calling restrictions.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
ScsiDiskDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath   OPTIONAL
  )
{
  EFI_STATUS            Status;
  EFI_SCSI_IO_PROTOCOL  *ScsiIo;
  UINT8                 DeviceType;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiScsiIoProtocolGuid,
                  (VOID **) &ScsiIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ScsiIo->GetDeviceType (ScsiIo, &DeviceType);
  if (!EFI_ERROR (Status)) {
    if ((DeviceType == EFI_SCSI_TYPE_DISK) || (DeviceType == EFI_SCSI_TYPE_CDROM)) {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_UNSUPPORTED;
    }
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiScsiIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;
}


/**
  Start this driver on ControllerHandle.

  This service is called by the EFI boot service ConnectController(). In order
  to make drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these calling restrictions. If
  any other agent wishes to call Start() it must also follow these calling
  restrictions.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
ScsiDiskDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath   OPTIONAL
  )
{
  EFI_STATUS            Status;
  EFI_SCSI_IO_PROTOCOL  *ScsiIo;
  SCSI_DISK_DEV         *ScsiDiskDevice;
  BOOLEAN               Temp;
  UINT8                 Index;
  UINT8                 MaxRetry;
  BOOLEAN               NeedRetry;

  ScsiDiskDevice = (SCSI_DISK_DEV *) AllocateZeroPool (sizeof (SCSI_DISK_DEV));
  if (ScsiDiskDevice == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiScsiIoProtocolGuid,
                  (VOID **) &ScsiIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    FreePool (ScsiDiskDevice);
    return Status;
  }

  ScsiDiskDevice->Signature         = SCSI_DISK_DEV_SIGNATURE;
  ScsiDiskDevice->ScsiIo            = ScsiIo;
  ScsiDiskDevice->BlkIo.Media       = &ScsiDiskDevice->BlkIoMedia;
  ScsiDiskDevice->BlkIo.Reset       = ScsiDiskReset;
  ScsiDiskDevice->BlkIo.ReadBlocks  = ScsiDiskReadBlocks;
  ScsiDiskDevice->BlkIo.WriteBlocks = ScsiDiskWriteBlocks;
  ScsiDiskDevice->BlkIo.FlushBlocks = ScsiDiskFlushBlocks;
  ScsiDiskDevice->Handle            = Controller;

  ScsiIo->GetDeviceType (ScsiIo, &(ScsiDiskDevice->DeviceType));
  switch (ScsiDiskDevice->DeviceType) {
  case EFI_SCSI_TYPE_DISK:
    ScsiDiskDevice->BlkIo.Media->BlockSize = 0x200;
    break;

  case EFI_SCSI_TYPE_CDROM:
    ScsiDiskDevice->BlkIo.Media->BlockSize = 0x800;
    break;
  }
  //
  // The Sense Data Array's initial size is 6
  //
  ScsiDiskDevice->SenseDataNumber = 6;
  ScsiDiskDevice->SenseData = (EFI_SCSI_SENSE_DATA *) AllocateZeroPool (
                                 sizeof (EFI_SCSI_SENSE_DATA) * ScsiDiskDevice->SenseDataNumber
                                 );
  if (ScsiDiskDevice->SenseData == NULL) {
    gBS->CloseProtocol (
          Controller,
          &gEfiScsiIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    FreePool (ScsiDiskDevice);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Retrieve device information
  //
  MaxRetry = 2;
  for (Index = 0; Index < MaxRetry; Index++) {
    Status = ScsiDiskInquiryDevice (ScsiDiskDevice, &NeedRetry);
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (!NeedRetry) {
      FreePool (ScsiDiskDevice->SenseData);
      gBS->CloseProtocol (
             Controller,
             &gEfiScsiIoProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
      FreePool (ScsiDiskDevice);
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // The second parameter "TRUE" means must
  // retrieve media capacity
  //
  Status = ScsiDiskDetectMedia (ScsiDiskDevice, TRUE, &Temp);
  if (!EFI_ERROR (Status)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Controller,
                    &gEfiBlockIoProtocolGuid,
                    &ScsiDiskDevice->BlkIo,
                    NULL
                    );
  }

  if (EFI_ERROR (Status)) {
    FreePool (ScsiDiskDevice->SenseData);
    gBS->CloseProtocol (
           Controller,
           &gEfiScsiIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    FreePool (ScsiDiskDevice);
    return Status;
  }

  ScsiDiskDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gScsiDiskComponentName.SupportedLanguages,
    &ScsiDiskDevice->ControllerNameTable,
    L"SCSI Disk Device",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gScsiDiskComponentName2.SupportedLanguages,
    &ScsiDiskDevice->ControllerNameTable,
    L"SCSI Disk Device",
    FALSE
    );


  return EFI_SUCCESS;

}


/**
  Stop this driver on ControllerHandle.

  This service is called by the EFI boot service DisconnectController().
  In order to make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController() must follow these
  calling restrictions. If any other agent wishes to call Stop() it must
  also follow these calling restrictions.
  
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
ScsiDiskDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer   OPTIONAL
  )
{
  EFI_BLOCK_IO_PROTOCOL *BlkIo;
  SCSI_DISK_DEV         *ScsiDiskDevice;
  EFI_STATUS            Status;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlkIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ScsiDiskDevice = SCSI_DISK_DEV_FROM_THIS (BlkIo);
  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  &ScsiDiskDevice->BlkIo
                  );
  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiScsiIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    ReleaseScsiDiskDeviceResources (ScsiDiskDevice);

    return EFI_SUCCESS;
  }
  //
  // errors met
  //
  return Status;
}

/**
  Reset SCSI Disk.


  @param  This                 The pointer of EFI_BLOCK_IO_PROTOCOL
  @param  ExtendedVerification The flag about if extend verificate

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.
  @return EFI_STATUS is retured from EFI_SCSI_IO_PROTOCOL.ResetDevice().

**/
EFI_STATUS
EFIAPI
ScsiDiskReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  EFI_TPL       OldTpl;
  SCSI_DISK_DEV *ScsiDiskDevice;
  EFI_STATUS    Status;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  ScsiDiskDevice  = SCSI_DISK_DEV_FROM_THIS (This);

  Status          = ScsiDiskDevice->ScsiIo->ResetDevice (ScsiDiskDevice->ScsiIo);

  if (!ExtendedVerification) {
    goto Done;
  }

  Status = ScsiDiskDevice->ScsiIo->ResetBus (ScsiDiskDevice->ScsiIo);

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  The function is to Read Block from SCSI Disk.

  @param  This       The pointer of EFI_BLOCK_IO_PROTOCOL.
  @param  MediaId    The Id of Media detected
  @param  Lba        The logic block address
  @param  BufferSize The size of Buffer
  @param  Buffer     The buffer to fill the read out data

  @retval EFI_SUCCESS           Successfully to read out block.
  @retval EFI_DEVICE_ERROR      Fail to detect media.
  @retval EFI_NO_MEDIA          Media is not present.
  @retval EFI_MEDIA_CHANGED     Media has changed.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER Invalid parameter passed in.

**/
EFI_STATUS
EFIAPI
ScsiDiskReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  MediaChange = FALSE;
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  ScsiDiskDevice = SCSI_DISK_DEV_FROM_THIS (This);

  if (!IS_DEVICE_FIXED(ScsiDiskDevice)) {

    Status = ScsiDiskDetectMedia (ScsiDiskDevice, FALSE, &MediaChange);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    if (MediaChange) {
      gBS->ReinstallProtocolInterface (
            ScsiDiskDevice->Handle,
            &gEfiBlockIoProtocolGuid,
            &ScsiDiskDevice->BlkIo,
            &ScsiDiskDevice->BlkIo
            );
    }
  }
  //
  // Get the intrinsic block size
  //
  Media           = ScsiDiskDevice->BlkIo.Media;
  BlockSize       = Media->BlockSize;

  NumberOfBlocks  = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (BufferSize % BlockSize != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  if (Lba > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  //
  // If all the parameters are valid, then perform read sectors command
  // to transfer data from device to host.
  //
  Status = ScsiDiskReadSectors (ScsiDiskDevice, Buffer, Lba, NumberOfBlocks);

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  The function is to Write Block to SCSI Disk.

  @param  This       The pointer of EFI_BLOCK_IO_PROTOCOL
  @param  MediaId    The Id of Media detected
  @param  Lba        The logic block address
  @param  BufferSize The size of Buffer
  @param  Buffer     The buffer to fill the read out data

  @retval EFI_SUCCESS           Successfully to read out block.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      Fail to detect media.
  @retval EFI_NO_MEDIA          Media is not present.
  @retval EFI_MEDIA_CHNAGED     Media has changed.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER Invalid parameter passed in.

**/
EFI_STATUS
EFIAPI
ScsiDiskWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  MediaChange = FALSE;
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  ScsiDiskDevice = SCSI_DISK_DEV_FROM_THIS (This);

  if (!IS_DEVICE_FIXED(ScsiDiskDevice)) {

    Status = ScsiDiskDetectMedia (ScsiDiskDevice, FALSE, &MediaChange);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    if (MediaChange) {
      gBS->ReinstallProtocolInterface (
            ScsiDiskDevice->Handle,
            &gEfiBlockIoProtocolGuid,
            &ScsiDiskDevice->BlkIo,
            &ScsiDiskDevice->BlkIo
            );
    }
  }
  //
  // Get the intrinsic block size
  //
  Media           = ScsiDiskDevice->BlkIo.Media;
  BlockSize       = Media->BlockSize;

  NumberOfBlocks  = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (BufferSize % BlockSize != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  if (Lba > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }
  //
  // if all the parameters are valid, then perform read sectors command
  // to transfer data from device to host.
  //
  Status = ScsiDiskWriteSectors (ScsiDiskDevice, Buffer, Lba, NumberOfBlocks);

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Flush Block to Disk.

  EFI_SUCCESS is returned directly.

  @param  This              The pointer of EFI_BLOCK_IO_PROTOCOL

  @retval EFI_SUCCESS       All outstanding data was written to the device

**/
EFI_STATUS
EFIAPI
ScsiDiskFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
{
  //
  // return directly
  //
  return EFI_SUCCESS;
}


/**
  Dectect Device and read out capacity ,if error occurs, parse the sense key.

  @param  ScsiDiskDevice    The pointer of SCSI_DISK_DEV
  @param  MustReadCapacity  The flag about reading device capacity
  @param  MediaChange       The pointer of flag indicates if media has changed 

  @retval EFI_DEVICE_ERROR  Indicates that error occurs
  @retval EFI_SUCCESS       Successfully to detect media

**/
EFI_STATUS
ScsiDiskDetectMedia (
  IN   SCSI_DISK_DEV   *ScsiDiskDevice,
  IN   BOOLEAN         MustReadCapacity,
  OUT  BOOLEAN         *MediaChange
  )
{
  EFI_STATUS          Status;
  EFI_STATUS          ReadCapacityStatus;
  EFI_SCSI_SENSE_DATA *SenseData;
  UINTN               NumberOfSenseKeys;
  BOOLEAN             NeedRetry;
  BOOLEAN             NeedReadCapacity;
  UINT8               Index;
  UINT8               MaxRetry;
  EFI_BLOCK_IO_MEDIA  OldMedia;
  UINTN               Action;

  Status              = EFI_SUCCESS;
  ReadCapacityStatus  = EFI_SUCCESS;
  SenseData           = NULL;
  NumberOfSenseKeys   = 0;
  NeedReadCapacity    = FALSE;
  CopyMem (&OldMedia, ScsiDiskDevice->BlkIo.Media, sizeof (OldMedia));
  *MediaChange        = FALSE;
  MaxRetry            = 3;

  for (Index = 0; Index < MaxRetry; Index++) {
    Status = ScsiDiskTestUnitReady (
              ScsiDiskDevice,
              &NeedRetry,
              &SenseData,
              &NumberOfSenseKeys
              );
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (!NeedRetry) {
      return Status;
    }
  }

  if ((Index == MaxRetry) && EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = DetectMediaParsingSenseKeys (
            ScsiDiskDevice,
            SenseData,
            NumberOfSenseKeys,
            &Action
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // ACTION_NO_ACTION: need not read capacity
  // other action code: need read capacity
  //
  if (Action == ACTION_NO_ACTION) {
    NeedReadCapacity = FALSE;
  } else {
    NeedReadCapacity = TRUE;
  }

  //
  // either NeedReadCapacity is TRUE, or MustReadCapacity is TRUE,
  // retrieve capacity via Read Capacity command
  //
  if (NeedReadCapacity || MustReadCapacity) {
    //
    // retrieve media information
    //
    MaxRetry = 3;
    for (Index = 0; Index < MaxRetry; Index++) {

      ReadCapacityStatus = ScsiDiskReadCapacity (
                            ScsiDiskDevice,
                            &NeedRetry,
                            &SenseData,
                            &NumberOfSenseKeys
                            );
      if (EFI_ERROR (ReadCapacityStatus) && !NeedRetry) {
        return EFI_DEVICE_ERROR;
      }
      //
      // analyze sense key to action
      //
      Status = DetectMediaParsingSenseKeys (
                ScsiDiskDevice,
                SenseData,
                NumberOfSenseKeys,
                &Action
                );
      //
      // if Status is error, it may indicate crisis error,
      // so return without retry.
      //
      if (EFI_ERROR (Status)) {
        return Status;
      }

      switch (Action) {
      case ACTION_NO_ACTION:
        //
        // no retry
        //
        Index = MaxRetry;
        break;

      case ACTION_RETRY_COMMAND_LATER:
        //
        // retry the ReadCapacity later and continuously, until the condition
        // no longer emerges.
        // stall time is 100000us, or say 0.1 second.
        //
        gBS->Stall (100000);
        Index = 0;
        break;

      default:
        //
        // other cases, just retry the command
        //
        break;
      }
    }

    if ((Index == MaxRetry) && EFI_ERROR (ReadCapacityStatus)) {
      return EFI_DEVICE_ERROR;
    }
  }

  if (ScsiDiskDevice->BlkIo.Media->MediaId != OldMedia.MediaId) {
    //
    // Media change information got from the device
    //
    *MediaChange = TRUE;
  }

  if (ScsiDiskDevice->BlkIo.Media->ReadOnly != OldMedia.ReadOnly) {
    *MediaChange = TRUE;
    ScsiDiskDevice->BlkIo.Media->MediaId += 1;
  }

  if (ScsiDiskDevice->BlkIo.Media->BlockSize != OldMedia.BlockSize) {
    *MediaChange = TRUE;
    ScsiDiskDevice->BlkIo.Media->MediaId += 1;
  }

  if (ScsiDiskDevice->BlkIo.Media->LastBlock != OldMedia.LastBlock) {
    *MediaChange = TRUE;
    ScsiDiskDevice->BlkIo.Media->MediaId += 1;
  }

  if (ScsiDiskDevice->BlkIo.Media->MediaPresent != OldMedia.MediaPresent) {
    if (ScsiDiskDevice->BlkIo.Media->MediaPresent) {
      //
      // when change from no media to media present, reset the MediaId to 1.
      //
      ScsiDiskDevice->BlkIo.Media->MediaId = 1;
    } else {
      //
      // when no media, reset the MediaId to zero.
      //
      ScsiDiskDevice->BlkIo.Media->MediaId = 0;
    }

    *MediaChange = TRUE;
  }

  return EFI_SUCCESS;
}


/**
  Send out Inquiry command to Device.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV
  @param  NeedRetry       Indicates if needs try again when error happens

  @retval  EFI_DEVICE_ERROR  Indicates that error occurs
  @retval  EFI_SUCCESS       Successfully to detect media

**/
EFI_STATUS
ScsiDiskInquiryDevice (
  IN OUT  SCSI_DISK_DEV   *ScsiDiskDevice,
     OUT  BOOLEAN         *NeedRetry
  )
{
  UINT32              InquiryDataLength;
  UINT8               SenseDataLength;
  UINT8               HostAdapterStatus;
  UINT8               TargetStatus;
  EFI_SCSI_SENSE_DATA *SenseDataArray;
  UINTN               NumberOfSenseKeys;
  EFI_STATUS          Status;
  UINT8               MaxRetry;
  UINT8               Index;

  InquiryDataLength = sizeof (EFI_SCSI_INQUIRY_DATA);
  SenseDataLength   = 0;

  Status = ScsiInquiryCommand (
            ScsiDiskDevice->ScsiIo,
            EFI_TIMER_PERIOD_SECONDS (1),
            NULL,
            &SenseDataLength,
            &HostAdapterStatus,
            &TargetStatus,
            (VOID *) &(ScsiDiskDevice->InquiryData),
            &InquiryDataLength,
            FALSE
            );
    //
    // no need to check HostAdapterStatus and TargetStatus
    //
  if ((Status == EFI_SUCCESS) || (Status == EFI_WARN_BUFFER_TOO_SMALL)) {
     ParseInquiryData (ScsiDiskDevice);
     return EFI_SUCCESS;
 
   } else if (Status == EFI_NOT_READY) {
     *NeedRetry = TRUE;
     return EFI_DEVICE_ERROR;
 
   } else if ((Status == EFI_INVALID_PARAMETER) || (Status == EFI_UNSUPPORTED)) {
     *NeedRetry = FALSE;
     return EFI_DEVICE_ERROR;
   }
   //
   // go ahead to check HostAdapterStatus and TargetStatus
   // (EFI_TIMEOUT, EFI_DEVICE_ERROR)
   //
 
   Status = CheckHostAdapterStatus (HostAdapterStatus);
   if ((Status == EFI_TIMEOUT) || (Status == EFI_NOT_READY)) {
     *NeedRetry = TRUE;
     return EFI_DEVICE_ERROR;
   } else if (Status == EFI_DEVICE_ERROR) {
      //
      // reset the scsi channel
      //
    ScsiDiskDevice->ScsiIo->ResetBus (ScsiDiskDevice->ScsiIo);
    *NeedRetry = FALSE;
    return EFI_DEVICE_ERROR;
  }

  Status = CheckTargetStatus (TargetStatus);
  if (Status == EFI_NOT_READY) {
    //
    // reset the scsi device
    //
    ScsiDiskDevice->ScsiIo->ResetDevice (ScsiDiskDevice->ScsiIo);
    *NeedRetry = TRUE;
    return EFI_DEVICE_ERROR;

  } else if (Status == EFI_DEVICE_ERROR) {
    *NeedRetry = FALSE;
    return EFI_DEVICE_ERROR;
  }
  
  //
  // if goes here, meant SubmitInquiryCommand() failed.
  // if ScsiDiskRequestSenseKeys() succeeds at last,
  // better retry SubmitInquiryCommand(). (by setting *NeedRetry = TRUE)
  //
  MaxRetry = 3;
  for (Index = 0; Index < MaxRetry; Index++) {
    Status = ScsiDiskRequestSenseKeys (
              ScsiDiskDevice,
              NeedRetry,
              &SenseDataArray,
              &NumberOfSenseKeys,
              TRUE
              );
    if (!EFI_ERROR (Status)) {
      *NeedRetry = TRUE;
      return EFI_DEVICE_ERROR;
    }

    if (!*NeedRetry) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // ScsiDiskRequestSenseKeys() failed after several rounds of retry.
  // set *NeedRetry = FALSE to avoid the outside caller try again.
  //
  *NeedRetry = FALSE;
  return EFI_DEVICE_ERROR;
}

/**
  To test deivice.

  When Test Unit Ready command succeeds, retrieve Sense Keys via Request Sense;
  When Test Unit Ready command encounters any error caused by host adapter or
  target, return error without retrieving Sense Keys.

  @param  ScsiDiskDevice     The pointer of SCSI_DISK_DEV
  @param  NeedRetry          The pointer of flag indicates try again
  @param  SenseDataArray     The pointer of an array of sense data
  @param  NumberOfSenseKeys  The pointer of the number of sense data array

  @retval EFI_DEVICE_ERROR   Indicates that error occurs
  @retval EFI_SUCCESS        Successfully to test unit

**/
EFI_STATUS
ScsiDiskTestUnitReady (
  IN  SCSI_DISK_DEV         *ScsiDiskDevice,
  OUT BOOLEAN               *NeedRetry,
  OUT EFI_SCSI_SENSE_DATA   **SenseDataArray,
  OUT UINTN                 *NumberOfSenseKeys
  )
{
  EFI_STATUS  Status;
  UINT8       SenseDataLength;
  UINT8       HostAdapterStatus;
  UINT8       TargetStatus;
  UINT8       Index;
  UINT8       MaxRetry;

  SenseDataLength     = 0;
  *NumberOfSenseKeys  = 0;

  //
  // Parameter 3 and 4: do not require sense data, retrieve it when needed.
  //
  Status = ScsiTestUnitReadyCommand (
            ScsiDiskDevice->ScsiIo,
            EFI_TIMER_PERIOD_SECONDS (1),
            NULL,
            &SenseDataLength,
            &HostAdapterStatus,
            &TargetStatus
            );
  //
  // no need to check HostAdapterStatus and TargetStatus
  //
  if (Status == EFI_NOT_READY) {
    *NeedRetry = TRUE;
    return EFI_DEVICE_ERROR;

  } else if ((Status == EFI_INVALID_PARAMETER) || (Status == EFI_UNSUPPORTED)) {
    *NeedRetry = FALSE;
    return EFI_DEVICE_ERROR;
  }
  //
  // go ahead to check HostAdapterStatus and TargetStatus(in case of EFI_DEVICE_ERROR)
  //

  Status = CheckHostAdapterStatus (HostAdapterStatus);
  if ((Status == EFI_TIMEOUT) || (Status == EFI_NOT_READY)) {
    *NeedRetry = TRUE;
    return EFI_DEVICE_ERROR;

  } else if (Status == EFI_DEVICE_ERROR) {
    //
    // reset the scsi channel
    //
    ScsiDiskDevice->ScsiIo->ResetBus (ScsiDiskDevice->ScsiIo);
    *NeedRetry = FALSE;
    return EFI_DEVICE_ERROR;
  }

  Status = CheckTargetStatus (TargetStatus);
  if (Status == EFI_NOT_READY) {
    //
    // reset the scsi device
    //
    ScsiDiskDevice->ScsiIo->ResetDevice (ScsiDiskDevice->ScsiIo);
    *NeedRetry = TRUE;
    return EFI_DEVICE_ERROR;

  } else if (Status == EFI_DEVICE_ERROR) {
    *NeedRetry = FALSE;
    return EFI_DEVICE_ERROR;
  }

  MaxRetry = 3;
  for (Index = 0; Index < MaxRetry; Index++) {
    Status = ScsiDiskRequestSenseKeys (
              ScsiDiskDevice,
              NeedRetry,
              SenseDataArray,
              NumberOfSenseKeys,
              FALSE
              );
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }

    if (!*NeedRetry) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // ScsiDiskRequestSenseKeys() failed after several rounds of retry.
  // set *NeedRetry = FALSE to avoid the outside caller try again.
  //
  *NeedRetry = FALSE;
  return EFI_DEVICE_ERROR;
}

/**
  Parsing Sense Keys which got from request sense command.

  @param  ScsiDiskDevice     The pointer of SCSI_DISK_DEV
  @param  SenseData          The pointer of EFI_SCSI_SENSE_DATA
  @param  NumberOfSenseKeys  The number of sense key  
  @param  Action             The pointer of action which indicates what is need to do next

  @retval EFI_DEVICE_ERROR   Indicates that error occurs
  @retval EFI_SUCCESS        Successfully to complete the parsing

**/
EFI_STATUS
DetectMediaParsingSenseKeys (
  OUT  SCSI_DISK_DEV           *ScsiDiskDevice,
  IN   EFI_SCSI_SENSE_DATA     *SenseData,
  IN   UINTN                   NumberOfSenseKeys,
  OUT  UINTN                   *Action
  )
{
  BOOLEAN RetryLater;

  //
  // Default is to read capacity, unless..
  //
  *Action = ACTION_READ_CAPACITY;

  if (NumberOfSenseKeys == 0) {
    *Action = ACTION_NO_ACTION;
    return EFI_SUCCESS;
  }

  if (!ScsiDiskHaveSenseKey (SenseData, NumberOfSenseKeys)) {
    //
    // No Sense Key returned from last submitted command
    //
    *Action = ACTION_NO_ACTION;
    return EFI_SUCCESS;
  }

  if (ScsiDiskIsNoMedia (SenseData, NumberOfSenseKeys)) {
    ScsiDiskDevice->BlkIo.Media->MediaPresent = FALSE;
    ScsiDiskDevice->BlkIo.Media->LastBlock    = 0;
    *Action = ACTION_NO_ACTION;
    return EFI_SUCCESS;
  }

  if (ScsiDiskIsMediaChange (SenseData, NumberOfSenseKeys)) {
    ScsiDiskDevice->BlkIo.Media->MediaId++;
    return EFI_SUCCESS;
  }

  if (ScsiDiskIsMediaError (SenseData, NumberOfSenseKeys)) {
    ScsiDiskDevice->BlkIo.Media->MediaPresent = FALSE;
    ScsiDiskDevice->BlkIo.Media->LastBlock    = 0;
    return EFI_DEVICE_ERROR;
  }

  if (ScsiDiskIsHardwareError (SenseData, NumberOfSenseKeys)) {
    return EFI_DEVICE_ERROR;
  }

  if (!ScsiDiskIsDriveReady (SenseData, NumberOfSenseKeys, &RetryLater)) {
    if (RetryLater) {
      *Action = ACTION_RETRY_COMMAND_LATER;
      return EFI_SUCCESS;
    }

    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Send read capacity command to device and get the device parameter.

  @param  ScsiDiskDevice     The pointer of SCSI_DISK_DEV
  @param  NeedRetry          The pointer of flag indicates if need a retry
  @param  SenseDataArray     The pointer of an array of sense data
  @param  NumberOfSenseKeys  The number of sense key

  @retval EFI_DEVICE_ERROR   Indicates that error occurs
  @retval EFI_SUCCESS        Successfully to read capacity

**/
EFI_STATUS
ScsiDiskReadCapacity (
  IN  OUT  SCSI_DISK_DEV           *ScsiDiskDevice,
      OUT  BOOLEAN                 *NeedRetry,
      OUT  EFI_SCSI_SENSE_DATA     **SenseDataArray,
      OUT  UINTN                   *NumberOfSenseKeys
  )
{
  EFI_SCSI_DISK_CAPACITY_DATA CapacityData;
  UINT32                      DataLength;
  UINT8                       HostAdapterStatus;
  UINT8                       TargetStatus;
  EFI_STATUS                  CommandStatus;
  EFI_STATUS                  Status;
  UINT8                       Index;
  UINT8                       MaxRetry;
  UINT8                       SenseDataLength;

  SenseDataLength = 0;
  ZeroMem (&CapacityData, sizeof (EFI_SCSI_DISK_CAPACITY_DATA));
  DataLength          = sizeof (EFI_SCSI_DISK_CAPACITY_DATA);

  *NumberOfSenseKeys  = 0;
  *NeedRetry          = FALSE;
  //
  // submit Read Capacity Command. in this call,not request sense data
  //
  CommandStatus = ScsiReadCapacityCommand (
                    ScsiDiskDevice->ScsiIo,
                    EFI_TIMER_PERIOD_SECONDS (1),
                    NULL,
                    &SenseDataLength,
                    &HostAdapterStatus,
                    &TargetStatus,
                    (VOID *) &CapacityData,
                    &DataLength,
                    FALSE
                    );
  //
    // no need to check HostAdapterStatus and TargetStatus
    //
   if (CommandStatus == EFI_SUCCESS) {
     GetMediaInfo (ScsiDiskDevice, &CapacityData);
     return EFI_SUCCESS;
 
   } else if (CommandStatus == EFI_NOT_READY) {
     *NeedRetry = TRUE;
     return EFI_DEVICE_ERROR;
 
   } else if ((CommandStatus == EFI_INVALID_PARAMETER) || (CommandStatus == EFI_UNSUPPORTED)) {
     *NeedRetry = FALSE;
     return EFI_DEVICE_ERROR;
   }
   //
   // go ahead to check HostAdapterStatus and TargetStatus
   // (EFI_TIMEOUT, EFI_DEVICE_ERROR, EFI_WARN_BUFFER_TOO_SMALL)
   //
 
   Status = CheckHostAdapterStatus (HostAdapterStatus);
   if ((Status == EFI_TIMEOUT) || (Status == EFI_NOT_READY)) {
     *NeedRetry = TRUE;
     return EFI_DEVICE_ERROR;
 
   } else if (Status == EFI_DEVICE_ERROR) {
    //
    // reset the scsi channel
    //
    ScsiDiskDevice->ScsiIo->ResetBus (ScsiDiskDevice->ScsiIo);
    *NeedRetry = FALSE;
    return EFI_DEVICE_ERROR;
  }

  Status = CheckTargetStatus (TargetStatus);
  if (Status == EFI_NOT_READY) {
    //
    // reset the scsi device
    //
    ScsiDiskDevice->ScsiIo->ResetDevice (ScsiDiskDevice->ScsiIo);
    *NeedRetry = TRUE;
    return EFI_DEVICE_ERROR;

  } else if (Status == EFI_DEVICE_ERROR) {
    *NeedRetry = FALSE;
    return EFI_DEVICE_ERROR;
  }
  
  //
  // if goes here, meant SubmitReadCapacityCommand() failed.
  // if ScsiDiskRequestSenseKeys() succeeds at last,
  // better retry SubmitReadCapacityCommand(). (by setting *NeedRetry = TRUE)
  //
  MaxRetry = 3;
  for (Index = 0; Index < MaxRetry; Index++) {

    Status = ScsiDiskRequestSenseKeys (
              ScsiDiskDevice,
              NeedRetry,
              SenseDataArray,
              NumberOfSenseKeys,
              TRUE
              );
    if (!EFI_ERROR (Status)) {
      *NeedRetry = TRUE;
      return EFI_DEVICE_ERROR;
    }

    if (!*NeedRetry) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // ScsiDiskRequestSenseKeys() failed after several rounds of retry.
  // set *NeedRetry = FALSE to avoid the outside caller try again.
  //
  *NeedRetry = FALSE;
  return EFI_DEVICE_ERROR;
}

/**
  Check the HostAdapter status and re-interpret it in EFI_STATUS.

  @param  HostAdapterStatus  Host Adapter status

  @retval  EFI_SUCCESS       Host adapter is OK.
  @retval  EFI_TIMEOUT       Timeout.
  @retval  EFI_NOT_READY     Adapter NOT ready.
  @retval  EFI_DEVICE_ERROR  Adapter device error.

**/
EFI_STATUS
CheckHostAdapterStatus (
  IN UINT8   HostAdapterStatus
  )
{
  switch (HostAdapterStatus) {
  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OK:
    return EFI_SUCCESS;

  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_SELECTION_TIMEOUT:
  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_TIMEOUT:
  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_TIMEOUT_COMMAND:
    return EFI_TIMEOUT;

  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_MESSAGE_REJECT:
  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_PARITY_ERROR:
  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_REQUEST_SENSE_FAILED:
  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN:
  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_BUS_RESET:
    return EFI_NOT_READY;

  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_BUS_FREE:
  case EFI_EXT_SCSI_STATUS_HOST_ADAPTER_PHASE_ERROR:
    return EFI_DEVICE_ERROR;

  default:
    return EFI_SUCCESS;
  }
}


/**
  Check the target status and re-interpret it in EFI_STATUS.

  @param  TargetStatus  Target status

  @retval EFI_NOT_READY       Device is NOT ready.
  @retval EFI_DEVICE_ERROR 
  @retval EFI_SUCCESS

**/
EFI_STATUS
CheckTargetStatus (
  IN  UINT8   TargetStatus
  )
{
  switch (TargetStatus) {
  case EFI_EXT_SCSI_STATUS_TARGET_GOOD:
  case EFI_EXT_SCSI_STATUS_TARGET_CHECK_CONDITION:
  case EFI_EXT_SCSI_STATUS_TARGET_CONDITION_MET:
    return EFI_SUCCESS;

  case EFI_EXT_SCSI_STATUS_TARGET_INTERMEDIATE:
  case EFI_EXT_SCSI_STATUS_TARGET_INTERMEDIATE_CONDITION_MET:
  case EFI_EXT_SCSI_STATUS_TARGET_BUSY:
  case EFI_EXT_SCSI_STATUS_TARGET_TASK_SET_FULL:
    return EFI_NOT_READY;

  case EFI_EXT_SCSI_STATUS_TARGET_RESERVATION_CONFLICT:
    return EFI_DEVICE_ERROR;
    break;

  default:
    return EFI_SUCCESS;
  }
}


/**
  Retrieve all sense keys from the device.

  When encountering error during the process, if retrieve sense keys before
  error encounterred, it returns the sense keys with return status set to EFI_SUCCESS,
  and NeedRetry set to FALSE; otherwize, return the proper return status.

  @param  ScsiDiskDevice     The pointer of SCSI_DISK_DEV
  @param  NeedRetry          The pointer of flag indicates if need a retry
  @param  SenseDataArray     The pointer of an array of sense data
  @param  NumberOfSenseKeys  The number of sense key
  @param  AskResetIfError    The flag indicates if need reset when error occurs

  @retval EFI_DEVICE_ERROR   Indicates that error occurs
  @retval EFI_SUCCESS        Successfully to request sense key

**/
EFI_STATUS
ScsiDiskRequestSenseKeys (
  IN  OUT  SCSI_DISK_DEV           *ScsiDiskDevice,
      OUT  BOOLEAN                 *NeedRetry,
      OUT  EFI_SCSI_SENSE_DATA     **SenseDataArray,
      OUT  UINTN                   *NumberOfSenseKeys,
  IN       BOOLEAN                 AskResetIfError
  )
{
  EFI_SCSI_SENSE_DATA *PtrSenseData;
  UINT8               SenseDataLength;
  BOOLEAN             SenseReq;
  EFI_STATUS          Status;
  EFI_STATUS          FallStatus;
  UINT8               HostAdapterStatus;
  UINT8               TargetStatus;

  FallStatus      = EFI_SUCCESS;
  SenseDataLength = sizeof (EFI_SCSI_SENSE_DATA);

  ZeroMem (
    ScsiDiskDevice->SenseData,
    sizeof (EFI_SCSI_SENSE_DATA) * (ScsiDiskDevice->SenseDataNumber)
    );

  *NumberOfSenseKeys  = 0;
  *SenseDataArray     = ScsiDiskDevice->SenseData;
  PtrSenseData        = ScsiDiskDevice->SenseData;

  for (SenseReq = TRUE; SenseReq;) {
    Status = ScsiRequestSenseCommand (
              ScsiDiskDevice->ScsiIo,
              EFI_TIMER_PERIOD_SECONDS (2),
              PtrSenseData,
              &SenseDataLength,
              &HostAdapterStatus,
              &TargetStatus
              );
     if ((Status == EFI_SUCCESS) || (Status == EFI_WARN_BUFFER_TOO_SMALL)) {
        FallStatus = EFI_SUCCESS;
  
     } else if ((Status == EFI_TIMEOUT) || (Status == EFI_NOT_READY)) {
       *NeedRetry  = TRUE;
       FallStatus  = EFI_DEVICE_ERROR;
 
     } else if ((Status == EFI_INVALID_PARAMETER) || (Status == EFI_UNSUPPORTED)) {
       *NeedRetry  = FALSE;
       FallStatus  = EFI_DEVICE_ERROR;
 
     } else if (Status == EFI_DEVICE_ERROR) {
        if (AskResetIfError) {
          ScsiDiskDevice->ScsiIo->ResetDevice (ScsiDiskDevice->ScsiIo);
        }
  
        FallStatus = EFI_DEVICE_ERROR;
    }

    if (EFI_ERROR (FallStatus)) {
      if (*NumberOfSenseKeys != 0) {
        *NeedRetry = FALSE;
        return EFI_SUCCESS;
      } else {
        return EFI_DEVICE_ERROR;
      }
    }

    (*NumberOfSenseKeys) += 1;

    //
    // no more sense key or number of sense keys exceeds predefined,
    // skip the loop.
    //
    if ((PtrSenseData->Sense_Key == EFI_SCSI_SK_NO_SENSE) || 
        (*NumberOfSenseKeys == ScsiDiskDevice->SenseDataNumber)) {
      SenseReq = FALSE;
    }
    PtrSenseData += 1;
  }
  return EFI_SUCCESS;
}


/**
  Get information from media read capacity command.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV
  @param  Capacity        The pointer of EFI_SCSI_DISK_CAPACITY_DATA

**/
VOID
GetMediaInfo (
  IN  OUT  SCSI_DISK_DEV                 *ScsiDiskDevice,
  IN       EFI_SCSI_DISK_CAPACITY_DATA   *Capacity
  )
{
  ScsiDiskDevice->BlkIo.Media->LastBlock =  (Capacity->LastLba3 << 24) |
                                            (Capacity->LastLba2 << 16) |
                                            (Capacity->LastLba1 << 8)  |
                                             Capacity->LastLba0;

  ScsiDiskDevice->BlkIo.Media->MediaPresent = TRUE;
  ScsiDiskDevice->BlkIo.Media->BlockSize = (Capacity->BlockSize3 << 24) |
                                           (Capacity->BlockSize2 << 16) | 
                                           (Capacity->BlockSize1 << 8)  |
                                            Capacity->BlockSize0;
  if (ScsiDiskDevice->DeviceType == EFI_SCSI_TYPE_DISK) {
    ScsiDiskDevice->BlkIo.Media->BlockSize = 0x200;
  }

  if (ScsiDiskDevice->DeviceType == EFI_SCSI_TYPE_CDROM) {
    ScsiDiskDevice->BlkIo.Media->BlockSize = 0x800;
  }
}

/**
  Parse Inquiry data.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV

**/
VOID
ParseInquiryData (
  IN OUT SCSI_DISK_DEV   *ScsiDiskDevice
  )
{
  ScsiDiskDevice->FixedDevice               = (BOOLEAN) (ScsiDiskDevice->InquiryData.RMB ? 0 : 1);
  ScsiDiskDevice->BlkIoMedia.RemovableMedia = (BOOLEAN) (!ScsiDiskDevice->FixedDevice);
}

/**
  Read sector from SCSI Disk.

  @param  ScsiDiskDevice  The poiniter of SCSI_DISK_DEV
  @param  Buffer          The buffer to fill in the read out data
  @param  Lba             Logic block address
  @param  NumberOfBlocks  The number of blocks to read

  @retval EFI_DEVICE_ERROR  Indicates a device error.
  @retval EFI_SUCCESS       Operation is successful.

**/
EFI_STATUS
ScsiDiskReadSectors (
  IN   SCSI_DISK_DEV     *ScsiDiskDevice,
  OUT  VOID              *Buffer,
  IN   EFI_LBA           Lba,
  IN   UINTN             NumberOfBlocks
  )
{
  UINTN               BlocksRemaining;
  UINT32              Lba32;
  UINT8               *PtrBuffer;
  UINT32              BlockSize;
  UINT32              ByteCount;
  UINT32              MaxBlock;
  UINT32              SectorCount;
  UINT64              Timeout;
  EFI_STATUS          Status;
  UINT8               Index;
  UINT8               MaxRetry;
  BOOLEAN             NeedRetry;
  EFI_SCSI_SENSE_DATA *SenseData;
  UINTN               NumberOfSenseKeys;

  SenseData         = NULL;
  NumberOfSenseKeys = 0;

  Status            = EFI_SUCCESS;

  BlocksRemaining   = NumberOfBlocks;
  BlockSize         = ScsiDiskDevice->BlkIo.Media->BlockSize;
  //
  // limit the data bytes that can be transferred by one Read(10) Command
  //
  MaxBlock  = 65536;

  PtrBuffer = Buffer;
  Lba32     = (UINT32) Lba;

  while (BlocksRemaining > 0) {

    if (BlocksRemaining <= MaxBlock) {

      SectorCount = (UINT16) BlocksRemaining;
    } else {

      SectorCount = MaxBlock;
    }

    ByteCount = SectorCount * BlockSize;
    Timeout   = EFI_TIMER_PERIOD_SECONDS (2);

    MaxRetry  = 2;
    for (Index = 0; Index < MaxRetry; Index++) {

      Status = ScsiDiskRead10 (
                ScsiDiskDevice,
                &NeedRetry,
                &SenseData,
                &NumberOfSenseKeys,
                Timeout,
                PtrBuffer,
                &ByteCount,
                Lba32,
                SectorCount
                );
      if (!EFI_ERROR (Status)) {
        break;
      }

      if (!NeedRetry) {
        return EFI_DEVICE_ERROR;
      }

    }

    if ((Index == MaxRetry) && (Status != EFI_SUCCESS)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // actual transferred sectors
    //
    SectorCount = ByteCount / BlockSize;

    Lba32 += SectorCount;
    PtrBuffer = PtrBuffer + SectorCount * BlockSize;
    BlocksRemaining -= SectorCount;
  }

  return EFI_SUCCESS;
}

/**
  Write sector to SCSI Disk.

  @param  ScsiDiskDevice  The poiniter of SCSI_DISK_DEV
  @param  Buffer          The buffer of data to be written into SCSI Disk
  @param  Lba             Logic block address
  @param  NumberOfBlocks  The number of blocks to read

  @retval EFI_DEVICE_ERROR  Indicates a device error.
  @retval EFI_SUCCESS       Operation is successful.

**/
EFI_STATUS
ScsiDiskWriteSectors (
  IN  SCSI_DISK_DEV     *ScsiDiskDevice,
  IN  VOID              *Buffer,
  IN  EFI_LBA           Lba,
  IN  UINTN             NumberOfBlocks
  )
{
  UINTN               BlocksRemaining;
  UINT32              Lba32;
  UINT8               *PtrBuffer;
  UINT32              BlockSize;
  UINT32              ByteCount;
  UINT32              MaxBlock;
  UINT32              SectorCount;
  UINT64              Timeout;
  EFI_STATUS          Status;
  UINT8               Index;
  UINT8               MaxRetry;
  BOOLEAN             NeedRetry;
  EFI_SCSI_SENSE_DATA *SenseData;
  UINTN               NumberOfSenseKeys;

  SenseData         = NULL;
  NumberOfSenseKeys = 0;

  Status            = EFI_SUCCESS;

  BlocksRemaining   = NumberOfBlocks;
  BlockSize         = ScsiDiskDevice->BlkIo.Media->BlockSize;
  //
  // limit the data bytes that can be transferred by one Write(10) Command
  //
  MaxBlock  = 65536;

  PtrBuffer = Buffer;
  Lba32     = (UINT32) Lba;

  while (BlocksRemaining > 0) {

    if (BlocksRemaining <= MaxBlock) {

      SectorCount = (UINT16) BlocksRemaining;
    } else {

      SectorCount = MaxBlock;
    }

    ByteCount = SectorCount * BlockSize;
    Timeout   = EFI_TIMER_PERIOD_SECONDS (2);
    MaxRetry  = 2;
    for (Index = 0; Index < MaxRetry; Index++) {
      Status = ScsiDiskWrite10 (
                ScsiDiskDevice,
                &NeedRetry,
                &SenseData,
                &NumberOfSenseKeys,
                Timeout,
                PtrBuffer,
                &ByteCount,
                Lba32,
                SectorCount
                );
      if (!EFI_ERROR (Status)) {
        break;
      }

      if (!NeedRetry) {
        return EFI_DEVICE_ERROR;
      }
    }

    if ((Index == MaxRetry) && (Status != EFI_SUCCESS)) {
      return EFI_DEVICE_ERROR;
    }
    //
    // actual transferred sectors
    //
    SectorCount = ByteCount / BlockSize;

    Lba32 += SectorCount;
    PtrBuffer = PtrBuffer + SectorCount * BlockSize;
    BlocksRemaining -= SectorCount;
  }

  return EFI_SUCCESS;
}


/**
  Sumbmit Read command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  SenseDataArray     NOT used yet in this function
  @param  NumberOfSenseKeys  The number of sense key
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorSize         The size of sector

  @return  EFI_STATUS is returned by calling ScsiRead10Command().
**/
EFI_STATUS
ScsiDiskRead10 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
     OUT BOOLEAN               *NeedRetry,
     OUT EFI_SCSI_SENSE_DATA   **SenseDataArray,   OPTIONAL
     OUT UINTN                 *NumberOfSenseKeys,
  IN     UINT64                Timeout,
     OUT UINT8                 *DataBuffer,
  IN OUT UINT32                *DataLength,
  IN     UINT32                StartLba,
  IN     UINT32                SectorSize
  )
{
  UINT8       SenseDataLength;
  EFI_STATUS  Status;
  UINT8       HostAdapterStatus;
  UINT8       TargetStatus;

  *NeedRetry          = FALSE;
  *NumberOfSenseKeys  = 0;
  SenseDataLength     = 0;
  Status = ScsiRead10Command (
            ScsiDiskDevice->ScsiIo,
            Timeout,
            NULL,
            &SenseDataLength,
            &HostAdapterStatus,
            &TargetStatus,
            DataBuffer,
            DataLength,
            StartLba,
            SectorSize
            );
  return Status;
}


/**
  Submit Write Command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  SenseDataArray     NOT used yet in this function
  @param  NumberOfSenseKeys  The number of sense key
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorSize         The size of sector

  @return  EFI_STATUS is returned by calling ScsiWrite10Command().

**/
EFI_STATUS
ScsiDiskWrite10 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
     OUT BOOLEAN               *NeedRetry,
     OUT EFI_SCSI_SENSE_DATA   **SenseDataArray,   OPTIONAL
     OUT UINTN                 *NumberOfSenseKeys,
  IN     UINT64                Timeout,
  IN     UINT8                 *DataBuffer,
  IN OUT UINT32                *DataLength,
  IN     UINT32                StartLba,
  IN     UINT32                SectorSize
  )
{
  EFI_STATUS  Status;
  UINT8       SenseDataLength;
  UINT8       HostAdapterStatus;
  UINT8       TargetStatus;

  *NeedRetry          = FALSE;
  *NumberOfSenseKeys  = 0;
  SenseDataLength     = 0;
  Status = ScsiWrite10Command (
            ScsiDiskDevice->ScsiIo,
            Timeout,
            NULL,
            &SenseDataLength,
            &HostAdapterStatus,
            &TargetStatus,
            DataBuffer,
            DataLength,
            StartLba,
            SectorSize
            );
  return Status;
}


/**
  Check sense key to find if media presents.

  @param  SenseData   The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts The number of sense key

  @retval TRUE    NOT any media
  @retval FALSE   Media presents
**/
BOOLEAN
ScsiDiskIsNoMedia (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA *SensePtr;
  UINTN               Index;
  BOOLEAN             IsNoMedia;

  IsNoMedia = FALSE;
  SensePtr  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    //
    // Sense Key is EFI_SCSI_SK_NOT_READY (0x2),
    // Additional Sense Code is ASC_NO_MEDIA (0x3A)
    //
    if ((SensePtr->Sense_Key == EFI_SCSI_SK_NOT_READY) &&
        (SensePtr->Addnl_Sense_Code == EFI_SCSI_ASC_NO_MEDIA)) {
      IsNoMedia = TRUE;
    }
    SensePtr++;
  }

  return IsNoMedia;
}


/**
  Parse sense key.

  @param  SenseData    The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts  The number of sense key

  @retval TRUE   Error
  @retval FALSE  NOT error

**/
BOOLEAN
ScsiDiskIsMediaError (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA *SensePtr;
  UINTN               Index;
  BOOLEAN             IsError;

  IsError   = FALSE;
  SensePtr  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {

    switch (SensePtr->Sense_Key) {

    case EFI_SCSI_SK_MEDIUM_ERROR:
      //
      // Sense Key is EFI_SCSI_SK_MEDIUM_ERROR (0x3)
      //
      switch (SensePtr->Addnl_Sense_Code) {

      //
      // fall through
      //
      case EFI_SCSI_ASC_MEDIA_ERR1:

      //
      // fall through
      //
      case EFI_SCSI_ASC_MEDIA_ERR2:

      //
      // fall through
      //
      case EFI_SCSI_ASC_MEDIA_ERR3:
      case EFI_SCSI_ASC_MEDIA_ERR4:
        IsError = TRUE;
        break;

      default:
        break;
      }

      break;

    case EFI_SCSI_SK_NOT_READY:
      //
      // Sense Key is EFI_SCSI_SK_NOT_READY (0x2)
      //
      switch (SensePtr->Addnl_Sense_Code) {
      //
      // Additional Sense Code is ASC_MEDIA_UPSIDE_DOWN (0x6)
      //
      case EFI_SCSI_ASC_MEDIA_UPSIDE_DOWN:
        IsError = TRUE;
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }

    SensePtr++;
  }

  return IsError;
}


/**
  Check sense key to find if hardware error happens.

  @param  SenseData     The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts   The number of sense key

  @retval TRUE  Hardware error exits.
  @retval FALSE NO error.

**/
BOOLEAN
ScsiDiskIsHardwareError (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA *SensePtr;
  UINTN               Index;
  BOOLEAN             IsError;

  IsError   = FALSE;
  SensePtr  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    
    //
    // Sense Key is EFI_SCSI_SK_HARDWARE_ERROR (0x4)
    //
    if (SensePtr->Sense_Key == EFI_SCSI_SK_HARDWARE_ERROR) {
      IsError = TRUE;
    }

    SensePtr++;
  }

  return IsError;
}


/**
  Check sense key to find if media has changed.

  @param  SenseData    The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts  The number of sense key

  @retval TRUE   Media is changed.
  @retval FALSE  Medit is NOT changed.
**/
BOOLEAN
ScsiDiskIsMediaChange (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA *SensePtr;
  UINTN               Index;
  BOOLEAN             IsMediaChanged;

  IsMediaChanged  = FALSE;
  SensePtr        = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    //
    // Sense Key is EFI_SCSI_SK_UNIT_ATTENTION (0x6),
    // Additional sense code is EFI_SCSI_ASC_MEDIA_CHANGE (0x28)
    //
    if ((SensePtr->Sense_Key == EFI_SCSI_SK_UNIT_ATTENTION) &&
        (SensePtr->Addnl_Sense_Code == EFI_SCSI_ASC_MEDIA_CHANGE)) {
      IsMediaChanged = TRUE;
    }

    SensePtr++;
  }

  return IsMediaChanged;
}

/**
  Check sense key to find if reset happens.

  @param  SenseData    The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts  The number of sense key

  @retval TRUE  It is reset before.
  @retval FALSE It is NOT reset before.

**/
BOOLEAN
ScsiDiskIsResetBefore (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA *SensePtr;
  UINTN               Index;
  BOOLEAN             IsResetBefore;

  IsResetBefore = FALSE;
  SensePtr      = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    
    //
    // Sense Key is EFI_SCSI_SK_UNIT_ATTENTION (0x6)
    // Additional Sense Code is EFI_SCSI_ASC_RESET (0x29)
    //
    if ((SensePtr->Sense_Key == EFI_SCSI_SK_UNIT_ATTENTION) &&
        (SensePtr->Addnl_Sense_Code == EFI_SCSI_ASC_RESET)) {
      IsResetBefore = TRUE;
    }

    SensePtr++;
  }

  return IsResetBefore;
}

/**
  Check sense key to find if the drive is ready.

  @param  SenseData    The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts  The number of sense key
  @param  RetryLater   The flag means if need a retry 

  @retval TRUE  Drive is ready.
  @retval FALSE Drive is NOT ready.

**/
BOOLEAN
ScsiDiskIsDriveReady (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts,
  OUT BOOLEAN               *RetryLater
  )
{
  EFI_SCSI_SENSE_DATA *SensePtr;
  UINTN               Index;
  BOOLEAN             IsReady;

  IsReady     = TRUE;
  *RetryLater = FALSE;
  SensePtr    = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {

    switch (SensePtr->Sense_Key) {

    case EFI_SCSI_SK_NOT_READY:
      //
      // Sense Key is EFI_SCSI_SK_NOT_READY (0x2)
      //
      switch (SensePtr->Addnl_Sense_Code) {
      case EFI_SCSI_ASC_NOT_READY:
        //
        // Additional Sense Code is EFI_SCSI_ASC_NOT_READY (0x4)
        //
        switch (SensePtr->Addnl_Sense_Code_Qualifier) {
        case EFI_SCSI_ASCQ_IN_PROGRESS:
          //
          // Additional Sense Code Qualifier is
          // EFI_SCSI_ASCQ_IN_PROGRESS (0x1)
          //
          IsReady     = FALSE;
          *RetryLater = TRUE;
          break;

        default:
          IsReady     = FALSE;
          *RetryLater = FALSE;
          break;
        }
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }

    SensePtr++;
  }

  return IsReady;
}

/**
  Check sense key to find if it has sense key.

  @param  SenseData   - The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts - The number of sense key

  @retval TRUE  It has sense key.
  @retval FALSE It has NOT any sense key.

**/
BOOLEAN
ScsiDiskHaveSenseKey (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA *SensePtr;
  UINTN               Index;
  BOOLEAN             HaveSenseKey;

  if (SenseCounts == 0) {
    HaveSenseKey = FALSE;
  } else {
    HaveSenseKey = TRUE;
  }

  SensePtr = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    
    //
    // Sense Key is SK_NO_SENSE (0x0)
    //
    if ((SensePtr->Sense_Key == EFI_SCSI_SK_NO_SENSE) &&
        (Index == 0)) {
      HaveSenseKey = FALSE;
    }

    SensePtr++;
  }

  return HaveSenseKey;
}

/**
  Release resource about disk device.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV

**/
VOID
ReleaseScsiDiskDeviceResources (
  IN  SCSI_DISK_DEV   *ScsiDiskDevice
  )
{
  if (ScsiDiskDevice == NULL) {
    return ;
  }

  if (ScsiDiskDevice->SenseData != NULL) {
    FreePool (ScsiDiskDevice->SenseData);
    ScsiDiskDevice->SenseData = NULL;
  }

  if (ScsiDiskDevice->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (ScsiDiskDevice->ControllerNameTable);
    ScsiDiskDevice->ControllerNameTable = NULL;
  }

  FreePool (ScsiDiskDevice);

  ScsiDiskDevice = NULL;
}
