/** @file
  SCSI disk driver that layers on every SCSI IO protocol in the system.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) 1985 - 2022, American Megatrends International LLC.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ScsiDisk.h"

EFI_DRIVER_BINDING_PROTOCOL  gScsiDiskDriverBinding = {
  ScsiDiskDriverBindingSupported,
  ScsiDiskDriverBindingStart,
  ScsiDiskDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_DISK_INFO_PROTOCOL  gScsiDiskInfoProtocolTemplate = {
  EFI_DISK_INFO_SCSI_INTERFACE_GUID,
  ScsiDiskInfoInquiry,
  ScsiDiskInfoIdentify,
  ScsiDiskInfoSenseData,
  ScsiDiskInfoWhichIde
};

/**
  Allocates an aligned buffer for SCSI disk.

  This function allocates an aligned buffer for the SCSI disk to perform
  SCSI IO operations. The alignment requirement is from SCSI IO interface.

  @param  ScsiDiskDevice    The SCSI disk involved for the operation.
  @param  BufferSize        The request buffer size.

  @return A pointer to the aligned buffer or NULL if the allocation fails.

**/
VOID *
AllocateAlignedBuffer (
  IN SCSI_DISK_DEV  *ScsiDiskDevice,
  IN UINTN          BufferSize
  )
{
  return AllocateAlignedPages (EFI_SIZE_TO_PAGES (BufferSize), ScsiDiskDevice->ScsiIo->IoAlign);
}

/**
  Frees an aligned buffer for SCSI disk.

  This function frees an aligned buffer for the SCSI disk to perform
  SCSI IO operations.

  @param  Buffer            The aligned buffer to be freed.
  @param  BufferSize        The request buffer size.

**/
VOID
FreeAlignedBuffer (
  IN VOID   *Buffer,
  IN UINTN  BufferSize
  )
{
  if (Buffer != NULL) {
    FreeAlignedPages (Buffer, EFI_SIZE_TO_PAGES (BufferSize));
  }
}

/**
  Remove trailing spaces from the string.

  @param String   The ASCII string to remove the trailing spaces.

  @retval the new length of the string.
**/
UINTN
RemoveTrailingSpaces (
  IN OUT CHAR8  *String
  )
{
  UINTN  Length;

  Length = AsciiStrLen (String);
  if (Length == 0) {
    return 0;
  }

  while ((Length > 0) && (String[Length-1] == ' ')) {
    Length--;
  }

  String[Length] = '\0';
  return Length;
}

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
InitializeScsiDisk (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

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
                  (VOID **)&ScsiIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ScsiIo->GetDeviceType (ScsiIo, &DeviceType);
  if (!EFI_ERROR (Status)) {
    if ((DeviceType == EFI_SCSI_TYPE_DISK) ||
        (DeviceType == EFI_SCSI_TYPE_CDROM) ||
        (DeviceType == EFI_SCSI_TYPE_WLUN))
    {
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
  BOOLEAN               MustReadCapacity;
  CHAR8                 VendorStr[VENDOR_IDENTIFICATION_LENGTH + 1];
  CHAR8                 ProductStr[PRODUCT_IDENTIFICATION_LENGTH + 1];
  CHAR16                DeviceStr[VENDOR_IDENTIFICATION_LENGTH + PRODUCT_IDENTIFICATION_LENGTH + 2];

  MustReadCapacity = TRUE;

  ScsiDiskDevice = (SCSI_DISK_DEV *)AllocateZeroPool (sizeof (SCSI_DISK_DEV));
  if (ScsiDiskDevice == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiScsiIoProtocolGuid,
                  (VOID **)&ScsiIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    FreePool (ScsiDiskDevice);
    return Status;
  }

  ScsiDiskDevice->Signature                         = SCSI_DISK_DEV_SIGNATURE;
  ScsiDiskDevice->ScsiIo                            = ScsiIo;
  ScsiDiskDevice->BlkIo.Revision                    = EFI_BLOCK_IO_PROTOCOL_REVISION3;
  ScsiDiskDevice->BlkIo.Media                       = &ScsiDiskDevice->BlkIoMedia;
  ScsiDiskDevice->BlkIo.Media->IoAlign              = ScsiIo->IoAlign;
  ScsiDiskDevice->BlkIo.Reset                       = ScsiDiskReset;
  ScsiDiskDevice->BlkIo.ReadBlocks                  = ScsiDiskReadBlocks;
  ScsiDiskDevice->BlkIo.WriteBlocks                 = ScsiDiskWriteBlocks;
  ScsiDiskDevice->BlkIo.FlushBlocks                 = ScsiDiskFlushBlocks;
  ScsiDiskDevice->BlkIo2.Media                      = &ScsiDiskDevice->BlkIoMedia;
  ScsiDiskDevice->BlkIo2.Reset                      = ScsiDiskResetEx;
  ScsiDiskDevice->BlkIo2.ReadBlocksEx               = ScsiDiskReadBlocksEx;
  ScsiDiskDevice->BlkIo2.WriteBlocksEx              = ScsiDiskWriteBlocksEx;
  ScsiDiskDevice->BlkIo2.FlushBlocksEx              = ScsiDiskFlushBlocksEx;
  ScsiDiskDevice->StorageSecurity.ReceiveData       = ScsiDiskReceiveData;
  ScsiDiskDevice->StorageSecurity.SendData          = ScsiDiskSendData;
  ScsiDiskDevice->EraseBlock.Revision               = EFI_ERASE_BLOCK_PROTOCOL_REVISION;
  ScsiDiskDevice->EraseBlock.EraseLengthGranularity = 1;
  ScsiDiskDevice->EraseBlock.EraseBlocks            = ScsiDiskEraseBlocks;
  ScsiDiskDevice->UnmapInfo.MaxBlkDespCnt           = 1;
  ScsiDiskDevice->BlockLimitsVpdSupported           = FALSE;
  ScsiDiskDevice->Handle                            = Controller;
  InitializeListHead (&ScsiDiskDevice->AsyncTaskQueue);

  ScsiIo->GetDeviceType (ScsiIo, &(ScsiDiskDevice->DeviceType));
  switch (ScsiDiskDevice->DeviceType) {
    case EFI_SCSI_TYPE_DISK:
      ScsiDiskDevice->BlkIo.Media->BlockSize = 0x200;
      MustReadCapacity                       = TRUE;
      break;

    case EFI_SCSI_TYPE_CDROM:
      ScsiDiskDevice->BlkIo.Media->BlockSize = 0x800;
      ScsiDiskDevice->BlkIo.Media->ReadOnly  = TRUE;
      MustReadCapacity                       = FALSE;
      break;

    case EFI_SCSI_TYPE_WLUN:
      MustReadCapacity = FALSE;
      break;
  }

  //
  // The Sense Data Array's initial size is 6
  //
  ScsiDiskDevice->SenseDataNumber = 6;
  ScsiDiskDevice->SenseData       = (EFI_SCSI_SENSE_DATA *)AllocateZeroPool (
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
  Status = ScsiDiskDetectMedia (ScsiDiskDevice, MustReadCapacity, &Temp);
  if (!EFI_ERROR (Status)) {
    //
    // Determine if Block IO & Block IO2 should be produced on this controller
    // handle
    //
    if (DetermineInstallBlockIo (Controller)) {
      InitializeInstallDiskInfo (ScsiDiskDevice, Controller);
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Controller,
                      &gEfiBlockIoProtocolGuid,
                      &ScsiDiskDevice->BlkIo,
                      &gEfiBlockIo2ProtocolGuid,
                      &ScsiDiskDevice->BlkIo2,
                      &gEfiDiskInfoProtocolGuid,
                      &ScsiDiskDevice->DiskInfo,
                      NULL
                      );
      if (!EFI_ERROR (Status)) {
        if (DetermineInstallEraseBlock (ScsiDiskDevice, Controller)) {
          Status = gBS->InstallProtocolInterface (
                          &Controller,
                          &gEfiEraseBlockProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          &ScsiDiskDevice->EraseBlock
                          );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "ScsiDisk: Failed to install the Erase Block Protocol! Status = %r\n", Status));
          }
        }

        if (DetermineInstallStorageSecurity (ScsiDiskDevice, Controller)) {
          Status = gBS->InstallProtocolInterface (
                          &Controller,
                          &gEfiStorageSecurityCommandProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          &ScsiDiskDevice->StorageSecurity
                          );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "ScsiDisk: Failed to install the Storage Security Command Protocol! Status = %r\n", Status));
          }
        }

        CopyMem (
          VendorStr,
          &ScsiDiskDevice->InquiryData.Reserved_5_95[VENDOR_IDENTIFICATION_OFFSET],
          VENDOR_IDENTIFICATION_LENGTH
          );
        VendorStr[VENDOR_IDENTIFICATION_LENGTH] = 0;
        RemoveTrailingSpaces (VendorStr);

        CopyMem (
          ProductStr,
          &ScsiDiskDevice->InquiryData.Reserved_5_95[PRODUCT_IDENTIFICATION_OFFSET],
          PRODUCT_IDENTIFICATION_LENGTH
          );
        ProductStr[PRODUCT_IDENTIFICATION_LENGTH] = 0;
        RemoveTrailingSpaces (ProductStr);

        UnicodeSPrint (DeviceStr, sizeof (DeviceStr), L"%a %a", VendorStr, ProductStr);

        ScsiDiskDevice->ControllerNameTable = NULL;
        AddUnicodeString2 (
          "eng",
          gScsiDiskComponentName.SupportedLanguages,
          &ScsiDiskDevice->ControllerNameTable,
          DeviceStr,
          TRUE
          );
        AddUnicodeString2 (
          "en",
          gScsiDiskComponentName2.SupportedLanguages,
          &ScsiDiskDevice->ControllerNameTable,
          DeviceStr,
          FALSE
          );
        return EFI_SUCCESS;
      }
    }
  }

  gBS->FreePool (ScsiDiskDevice->SenseData);
  gBS->FreePool (ScsiDiskDevice);
  gBS->CloseProtocol (
         Controller,
         &gEfiScsiIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;
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
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer   OPTIONAL
  )
{
  EFI_BLOCK_IO_PROTOCOL     *BlkIo;
  EFI_ERASE_BLOCK_PROTOCOL  *EraseBlock;
  SCSI_DISK_DEV             *ScsiDiskDevice;
  EFI_STATUS                Status;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlkIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ScsiDiskDevice = SCSI_DISK_DEV_FROM_BLKIO (BlkIo);

  //
  // Wait for the BlockIo2 requests queue to become empty
  //
  while (!IsListEmpty (&ScsiDiskDevice->AsyncTaskQueue)) {
  }

  //
  // If Erase Block Protocol is installed, then uninstall this protocol.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiEraseBlockProtocolGuid,
                  (VOID **)&EraseBlock,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    Status = gBS->UninstallProtocolInterface (
                    Controller,
                    &gEfiEraseBlockProtocolGuid,
                    &ScsiDiskDevice->EraseBlock
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  &ScsiDiskDevice->BlkIo,
                  &gEfiBlockIo2ProtocolGuid,
                  &ScsiDiskDevice->BlkIo2,
                  &gEfiDiskInfoProtocolGuid,
                  &ScsiDiskDevice->DiskInfo,
                  NULL
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
  @return EFI_STATUS is returned from EFI_SCSI_IO_PROTOCOL.ResetDevice().

**/
EFI_STATUS
EFIAPI
ScsiDiskReset (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  BOOLEAN                ExtendedVerification
  )
{
  EFI_TPL        OldTpl;
  SCSI_DISK_DEV  *ScsiDiskDevice;
  EFI_STATUS     Status;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  ScsiDiskDevice = SCSI_DISK_DEV_FROM_BLKIO (This);

  Status = ScsiDiskDevice->ScsiIo->ResetDevice (ScsiDiskDevice->ScsiIo);

  if (EFI_ERROR (Status)) {
    if (Status == EFI_UNSUPPORTED) {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }
  }

  if (!ExtendedVerification) {
    goto Done;
  }

  Status = ScsiDiskDevice->ScsiIo->ResetBus (ScsiDiskDevice->ScsiIo);

  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

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
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  MediaChange    = FALSE;
  OldTpl         = gBS->RaiseTPL (TPL_CALLBACK);
  ScsiDiskDevice = SCSI_DISK_DEV_FROM_BLKIO (This);
  Media          = ScsiDiskDevice->BlkIo.Media;

  if (!IS_DEVICE_FIXED (ScsiDiskDevice)) {
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
      gBS->ReinstallProtocolInterface (
             ScsiDiskDevice->Handle,
             &gEfiBlockIo2ProtocolGuid,
             &ScsiDiskDevice->BlkIo2,
             &ScsiDiskDevice->BlkIo2
             );
      if (DetermineInstallEraseBlock (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiEraseBlockProtocolGuid,
               &ScsiDiskDevice->EraseBlock,
               &ScsiDiskDevice->EraseBlock
               );
      }

      if (DetermineInstallStorageSecurity (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiStorageSecurityCommandProtocolGuid,
               &ScsiDiskDevice->StorageSecurity,
               &ScsiDiskDevice->StorageSecurity
               );
      }

      if (Media->MediaPresent) {
        Status = EFI_MEDIA_CHANGED;
      } else {
        Status = EFI_NO_MEDIA;
      }

      goto Done;
    }
  }

  //
  // Get the intrinsic block size
  //
  BlockSize = Media->BlockSize;

  if (BlockSize == 0) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  NumberOfBlocks = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (Buffer == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
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

  if ((Media->IoAlign > 1) && (((UINTN)Buffer & (Media->IoAlign - 1)) != 0)) {
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
  @retval EFI_MEDIA_CHANGED     Media has changed.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER Invalid parameter passed in.

**/
EFI_STATUS
EFIAPI
ScsiDiskWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  IN  VOID                   *Buffer
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  MediaChange    = FALSE;
  OldTpl         = gBS->RaiseTPL (TPL_CALLBACK);
  ScsiDiskDevice = SCSI_DISK_DEV_FROM_BLKIO (This);
  Media          = ScsiDiskDevice->BlkIo.Media;

  if (!IS_DEVICE_FIXED (ScsiDiskDevice)) {
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
      gBS->ReinstallProtocolInterface (
             ScsiDiskDevice->Handle,
             &gEfiBlockIo2ProtocolGuid,
             &ScsiDiskDevice->BlkIo2,
             &ScsiDiskDevice->BlkIo2
             );
      if (DetermineInstallEraseBlock (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiEraseBlockProtocolGuid,
               &ScsiDiskDevice->EraseBlock,
               &ScsiDiskDevice->EraseBlock
               );
      }

      if (DetermineInstallStorageSecurity (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiStorageSecurityCommandProtocolGuid,
               &ScsiDiskDevice->StorageSecurity,
               &ScsiDiskDevice->StorageSecurity
               );
      }

      if (Media->MediaPresent) {
        Status = EFI_MEDIA_CHANGED;
      } else {
        Status = EFI_NO_MEDIA;
      }

      goto Done;
    }
  }

  //
  // Get the intrinsic block size
  //
  BlockSize = Media->BlockSize;

  if (BlockSize == 0) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  NumberOfBlocks = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (Media->ReadOnly) {
    Status = EFI_WRITE_PROTECTED;
    goto Done;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  if (Buffer == NULL) {
    Status = EFI_INVALID_PARAMETER;
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

  if ((Media->IoAlign > 1) && (((UINTN)Buffer & (Media->IoAlign - 1)) != 0)) {
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
  IN  EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  //
  // return directly
  //
  return EFI_SUCCESS;
}

/**
  Reset SCSI Disk.

  @param  This                 The pointer of EFI_BLOCK_IO2_PROTOCOL.
  @param  ExtendedVerification The flag about if extend verificate.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.
  @return EFI_STATUS is returned from EFI_SCSI_IO_PROTOCOL.ResetDevice().

**/
EFI_STATUS
EFIAPI
ScsiDiskResetEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  EFI_TPL        OldTpl;
  SCSI_DISK_DEV  *ScsiDiskDevice;
  EFI_STATUS     Status;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  ScsiDiskDevice = SCSI_DISK_DEV_FROM_BLKIO2 (This);

  Status = ScsiDiskDevice->ScsiIo->ResetDevice (ScsiDiskDevice->ScsiIo);

  if (EFI_ERROR (Status)) {
    if (Status == EFI_UNSUPPORTED) {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }
  }

  if (!ExtendedVerification) {
    goto Done;
  }

  Status = ScsiDiskDevice->ScsiIo->ResetBus (ScsiDiskDevice->ScsiIo);

  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  The function is to Read Block from SCSI Disk.

  @param  This       The pointer of EFI_BLOCK_IO_PROTOCOL.
  @param  MediaId    The Id of Media detected.
  @param  Lba        The logic block address.
  @param  Token      A pointer to the token associated with the transaction.
  @param  BufferSize The size of Buffer.
  @param  Buffer     The buffer to fill the read out data.

  @retval EFI_SUCCESS           The read request was queued if Token-> Event is
                                not NULL. The data was read correctly from the
                                device if theToken-> Event is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error while attempting
                                to perform the read operation.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of
                                the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not
                                valid, or the buffer is not on proper
                                alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.

**/
EFI_STATUS
EFIAPI
ScsiDiskReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN     UINT32                  MediaId,
  IN     EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
  IN     UINTN                   BufferSize,
  OUT    VOID                    *Buffer
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  MediaChange    = FALSE;
  OldTpl         = gBS->RaiseTPL (TPL_CALLBACK);
  ScsiDiskDevice = SCSI_DISK_DEV_FROM_BLKIO2 (This);
  Media          = ScsiDiskDevice->BlkIo.Media;

  if (!IS_DEVICE_FIXED (ScsiDiskDevice)) {
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
      gBS->ReinstallProtocolInterface (
             ScsiDiskDevice->Handle,
             &gEfiBlockIo2ProtocolGuid,
             &ScsiDiskDevice->BlkIo2,
             &ScsiDiskDevice->BlkIo2
             );
      if (DetermineInstallEraseBlock (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiEraseBlockProtocolGuid,
               &ScsiDiskDevice->EraseBlock,
               &ScsiDiskDevice->EraseBlock
               );
      }

      if (DetermineInstallStorageSecurity (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiStorageSecurityCommandProtocolGuid,
               &ScsiDiskDevice->StorageSecurity,
               &ScsiDiskDevice->StorageSecurity
               );
      }

      if (Media->MediaPresent) {
        Status = EFI_MEDIA_CHANGED;
      } else {
        Status = EFI_NO_MEDIA;
      }

      goto Done;
    }
  }

  //
  // Get the intrinsic block size
  //
  BlockSize = Media->BlockSize;

  if (BlockSize == 0) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  NumberOfBlocks = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (Buffer == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (BufferSize == 0) {
    if ((Token != NULL) && (Token->Event != NULL)) {
      Token->TransactionStatus = EFI_SUCCESS;
      gBS->SignalEvent (Token->Event);
    }

    Status = EFI_SUCCESS;
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

  if ((Media->IoAlign > 1) && (((UINTN)Buffer & (Media->IoAlign - 1)) != 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  //
  // If all the parameters are valid, then perform read sectors command
  // to transfer data from device to host.
  //
  if ((Token != NULL) && (Token->Event != NULL)) {
    Token->TransactionStatus = EFI_SUCCESS;
    Status                   = ScsiDiskAsyncReadSectors (
                                 ScsiDiskDevice,
                                 Buffer,
                                 Lba,
                                 NumberOfBlocks,
                                 Token
                                 );
  } else {
    Status = ScsiDiskReadSectors (
               ScsiDiskDevice,
               Buffer,
               Lba,
               NumberOfBlocks
               );
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  The function is to Write Block to SCSI Disk.

  @param  This       The pointer of EFI_BLOCK_IO_PROTOCOL.
  @param  MediaId    The Id of Media detected.
  @param  Lba        The logic block address.
  @param  Token      A pointer to the token associated with the transaction.
  @param  BufferSize The size of Buffer.
  @param  Buffer     The buffer to fill the read out data.

  @retval EFI_SUCCESS           The data were written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device cannot be written to.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_DEVICE_ERROR      The device reported an error while attempting
                                to perform the write operation.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of
                                the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not
                                valid, or the buffer is not on proper
                                alignment.

**/
EFI_STATUS
EFIAPI
ScsiDiskWriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN     UINT32                  MediaId,
  IN     EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
  IN     UINTN                   BufferSize,
  IN     VOID                    *Buffer
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  MediaChange    = FALSE;
  OldTpl         = gBS->RaiseTPL (TPL_CALLBACK);
  ScsiDiskDevice = SCSI_DISK_DEV_FROM_BLKIO2 (This);
  Media          = ScsiDiskDevice->BlkIo.Media;

  if (!IS_DEVICE_FIXED (ScsiDiskDevice)) {
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
      gBS->ReinstallProtocolInterface (
             ScsiDiskDevice->Handle,
             &gEfiBlockIo2ProtocolGuid,
             &ScsiDiskDevice->BlkIo2,
             &ScsiDiskDevice->BlkIo2
             );
      if (DetermineInstallEraseBlock (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiEraseBlockProtocolGuid,
               &ScsiDiskDevice->EraseBlock,
               &ScsiDiskDevice->EraseBlock
               );
      }

      if (DetermineInstallStorageSecurity (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiStorageSecurityCommandProtocolGuid,
               &ScsiDiskDevice->StorageSecurity,
               &ScsiDiskDevice->StorageSecurity
               );
      }

      if (Media->MediaPresent) {
        Status = EFI_MEDIA_CHANGED;
      } else {
        Status = EFI_NO_MEDIA;
      }

      goto Done;
    }
  }

  //
  // Get the intrinsic block size
  //
  BlockSize = Media->BlockSize;

  if (BlockSize == 0) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  NumberOfBlocks = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (Media->ReadOnly) {
    Status = EFI_WRITE_PROTECTED;
    goto Done;
  }

  if (BufferSize == 0) {
    if ((Token != NULL) && (Token->Event != NULL)) {
      Token->TransactionStatus = EFI_SUCCESS;
      gBS->SignalEvent (Token->Event);
    }

    Status = EFI_SUCCESS;
    goto Done;
  }

  if (Buffer == NULL) {
    Status = EFI_INVALID_PARAMETER;
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

  if ((Media->IoAlign > 1) && (((UINTN)Buffer & (Media->IoAlign - 1)) != 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  //
  // if all the parameters are valid, then perform write sectors command
  // to transfer data from device to host.
  //
  if ((Token != NULL) && (Token->Event != NULL)) {
    Token->TransactionStatus = EFI_SUCCESS;
    Status                   = ScsiDiskAsyncWriteSectors (
                                 ScsiDiskDevice,
                                 Buffer,
                                 Lba,
                                 NumberOfBlocks,
                                 Token
                                 );
  } else {
    Status = ScsiDiskWriteSectors (
               ScsiDiskDevice,
               Buffer,
               Lba,
               NumberOfBlocks
               );
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Flush the Block Device.

  @param  This       Indicates a pointer to the calling context.
  @param  Token      A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS         All outstanding data was written to the device.
  @retval EFI_DEVICE_ERROR    The device reported an error while attempting to
                              write data.
  @retval EFI_WRITE_PROTECTED The device cannot be written to.
  @retval EFI_NO_MEDIA        There is no media in the device.
  @retval EFI_MEDIA_CHANGED   The MediaId is not for the current media.

**/
EFI_STATUS
EFIAPI
ScsiDiskFlushBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  MediaChange    = FALSE;
  OldTpl         = gBS->RaiseTPL (TPL_CALLBACK);
  ScsiDiskDevice = SCSI_DISK_DEV_FROM_BLKIO2 (This);
  Media          = ScsiDiskDevice->BlkIo.Media;

  if (!IS_DEVICE_FIXED (ScsiDiskDevice)) {
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
      gBS->ReinstallProtocolInterface (
             ScsiDiskDevice->Handle,
             &gEfiBlockIo2ProtocolGuid,
             &ScsiDiskDevice->BlkIo2,
             &ScsiDiskDevice->BlkIo2
             );
      if (DetermineInstallEraseBlock (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiEraseBlockProtocolGuid,
               &ScsiDiskDevice->EraseBlock,
               &ScsiDiskDevice->EraseBlock
               );
      }

      if (DetermineInstallStorageSecurity (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiStorageSecurityCommandProtocolGuid,
               &ScsiDiskDevice->StorageSecurity,
               &ScsiDiskDevice->StorageSecurity
               );
      }

      if (Media->MediaPresent) {
        Status = EFI_MEDIA_CHANGED;
      } else {
        Status = EFI_NO_MEDIA;
      }

      goto Done;
    }
  }

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (Media->ReadOnly) {
    Status = EFI_WRITE_PROTECTED;
    goto Done;
  }

  //
  // Wait for the BlockIo2 requests queue to become empty
  //
  while (!IsListEmpty (&ScsiDiskDevice->AsyncTaskQueue)) {
  }

  Status = EFI_SUCCESS;

  //
  // Signal caller event
  //
  if ((Token != NULL) && (Token->Event != NULL)) {
    Token->TransactionStatus = EFI_SUCCESS;
    gBS->SignalEvent (Token->Event);
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Internal helper notify function which process the result of an asynchronous
  SCSI UNMAP Command and signal the event passed from EraseBlocks.

  @param  Event    The instance of EFI_EVENT.
  @param  Context  The parameter passed in.

**/
VOID
EFIAPI
ScsiDiskAsyncUnmapNotify (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  SCSI_ERASEBLK_REQUEST            *EraseBlkReq;
  EFI_SCSI_IO_SCSI_REQUEST_PACKET  *CommandPacket;
  EFI_ERASE_BLOCK_TOKEN            *Token;
  EFI_STATUS                       Status;

  gBS->CloseEvent (Event);

  EraseBlkReq              = (SCSI_ERASEBLK_REQUEST *)Context;
  CommandPacket            = &EraseBlkReq->CommandPacket;
  Token                    = EraseBlkReq->Token;
  Token->TransactionStatus = EFI_SUCCESS;

  Status = CheckHostAdapterStatus (CommandPacket->HostAdapterStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ScsiDiskAsyncUnmapNotify: Host adapter indicating error status 0x%x.\n",
      CommandPacket->HostAdapterStatus
      ));

    Token->TransactionStatus = Status;
    goto Done;
  }

  Status = CheckTargetStatus (CommandPacket->TargetStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ScsiDiskAsyncUnmapNotify: Target indicating error status 0x%x.\n",
      CommandPacket->HostAdapterStatus
      ));

    Token->TransactionStatus = Status;
    goto Done;
  }

Done:
  RemoveEntryList (&EraseBlkReq->Link);
  FreePool (CommandPacket->OutDataBuffer);
  FreePool (EraseBlkReq->CommandPacket.Cdb);
  FreePool (EraseBlkReq);

  gBS->SignalEvent (Token->Event);
}

/**
  Require the device server to cause one or more LBAs to be unmapped.

  @param  ScsiDiskDevice         The pointer of ScsiDiskDevice.
  @param  Lba                    The start block number.
  @param  Blocks                 Total block number to be unmapped.
  @param  Token                  The pointer to the token associated with the
                                 non-blocking erase block request.

  @retval EFI_SUCCESS            Target blocks have been successfully unmapped.
  @retval EFI_DEVICE_ERROR       Fail to unmap the target blocks.

**/
EFI_STATUS
ScsiDiskUnmap (
  IN SCSI_DISK_DEV          *ScsiDiskDevice,
  IN UINT64                 Lba,
  IN UINTN                  Blocks,
  IN EFI_ERASE_BLOCK_TOKEN  *Token            OPTIONAL
  )
{
  EFI_SCSI_IO_PROTOCOL             *ScsiIo;
  SCSI_ERASEBLK_REQUEST            *EraseBlkReq;
  EFI_SCSI_IO_SCSI_REQUEST_PACKET  *CommandPacket;
  EFI_SCSI_DISK_UNMAP_BLOCK_DESP   *BlkDespPtr;
  EFI_STATUS                       Status;
  EFI_STATUS                       ReturnStatus;
  UINT8                            *Cdb;
  UINT32                           MaxLbaCnt;
  UINT32                           MaxBlkDespCnt;
  UINT32                           BlkDespCnt;
  UINT16                           UnmapParamListLen;
  VOID                             *UnmapParamList;
  EFI_EVENT                        AsyncUnmapEvent;
  EFI_TPL                          OldTpl;

  ScsiIo          = ScsiDiskDevice->ScsiIo;
  MaxLbaCnt       = ScsiDiskDevice->UnmapInfo.MaxLbaCnt;
  MaxBlkDespCnt   = ScsiDiskDevice->UnmapInfo.MaxBlkDespCnt;
  EraseBlkReq     = NULL;
  UnmapParamList  = NULL;
  AsyncUnmapEvent = NULL;
  ReturnStatus    = EFI_SUCCESS;

  if (Blocks / (UINTN)MaxLbaCnt > MaxBlkDespCnt) {
    ReturnStatus = EFI_DEVICE_ERROR;
    goto Done;
  }

  EraseBlkReq = AllocateZeroPool (sizeof (SCSI_ERASEBLK_REQUEST));
  if (EraseBlkReq == NULL) {
    ReturnStatus = EFI_DEVICE_ERROR;
    goto Done;
  }

  EraseBlkReq->CommandPacket.Cdb = AllocateZeroPool (0xA);
  if (EraseBlkReq->CommandPacket.Cdb == NULL) {
    ReturnStatus = EFI_DEVICE_ERROR;
    goto Done;
  }

  BlkDespCnt        = (UINT32)((Blocks - 1) / MaxLbaCnt + 1);
  UnmapParamListLen = (UINT16)(sizeof (EFI_SCSI_DISK_UNMAP_PARAM_LIST_HEADER)
                               + BlkDespCnt * sizeof (EFI_SCSI_DISK_UNMAP_BLOCK_DESP));
  UnmapParamList = AllocateZeroPool (UnmapParamListLen);
  if (UnmapParamList == NULL) {
    ReturnStatus = EFI_DEVICE_ERROR;
    goto Done;
  }

  *((UINT16 *)UnmapParamList)     = SwapBytes16 (UnmapParamListLen - 2);
  *((UINT16 *)UnmapParamList + 1) = SwapBytes16 (UnmapParamListLen - sizeof (EFI_SCSI_DISK_UNMAP_PARAM_LIST_HEADER));

  BlkDespPtr = (EFI_SCSI_DISK_UNMAP_BLOCK_DESP *)((UINT8 *)UnmapParamList + sizeof (EFI_SCSI_DISK_UNMAP_PARAM_LIST_HEADER));
  while (Blocks > 0) {
    if (Blocks > MaxLbaCnt) {
      *(UINT64 *)(&BlkDespPtr->Lba)      = SwapBytes64 (Lba);
      *(UINT32 *)(&BlkDespPtr->BlockNum) = SwapBytes32 (MaxLbaCnt);
      Blocks                            -= MaxLbaCnt;
      Lba                               += MaxLbaCnt;
    } else {
      *(UINT64 *)(&BlkDespPtr->Lba)      = SwapBytes64 (Lba);
      *(UINT32 *)(&BlkDespPtr->BlockNum) = SwapBytes32 ((UINT32)Blocks);
      Blocks                             = 0;
    }

    BlkDespPtr++;
  }

  CommandPacket                    = &EraseBlkReq->CommandPacket;
  CommandPacket->Timeout           = SCSI_DISK_TIMEOUT;
  CommandPacket->OutDataBuffer     = UnmapParamList;
  CommandPacket->OutTransferLength = UnmapParamListLen;
  CommandPacket->CdbLength         = 0xA;
  CommandPacket->DataDirection     = EFI_SCSI_DATA_OUT;
  //
  // Fill Cdb for UNMAP Command
  //
  Cdb    = CommandPacket->Cdb;
  Cdb[0] = EFI_SCSI_OP_UNMAP;
  WriteUnaligned16 ((UINT16 *)&Cdb[7], SwapBytes16 (UnmapParamListLen));

  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // Non-blocking UNMAP request
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    ScsiDiskAsyncUnmapNotify,
                    EraseBlkReq,
                    &AsyncUnmapEvent
                    );
    if (EFI_ERROR (Status)) {
      ReturnStatus = EFI_DEVICE_ERROR;
      goto Done;
    }

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    InsertTailList (&ScsiDiskDevice->AsyncTaskQueue, &EraseBlkReq->Link);
    gBS->RestoreTPL (OldTpl);

    EraseBlkReq->Token = Token;

    Status = ScsiIo->ExecuteScsiCommand (
                       ScsiIo,
                       CommandPacket,
                       AsyncUnmapEvent
                       );
    if (EFI_ERROR (Status)) {
      ReturnStatus = EFI_DEVICE_ERROR;

      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&EraseBlkReq->Link);
      gBS->RestoreTPL (OldTpl);

      goto Done;
    } else {
      //
      // Directly return if the non-blocking UNMAP request is queued.
      //
      return EFI_SUCCESS;
    }
  } else {
    //
    // Blocking UNMAP request
    //
    Status = ScsiIo->ExecuteScsiCommand (
                       ScsiIo,
                       CommandPacket,
                       NULL
                       );
    if (EFI_ERROR (Status)) {
      ReturnStatus = EFI_DEVICE_ERROR;
      goto Done;
    }
  }

  //
  // Only blocking UNMAP request will reach here.
  //
  Status = CheckHostAdapterStatus (CommandPacket->HostAdapterStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ScsiDiskUnmap: Host adapter indicating error status 0x%x.\n",
      CommandPacket->HostAdapterStatus
      ));

    ReturnStatus = EFI_DEVICE_ERROR;
    goto Done;
  }

  Status = CheckTargetStatus (CommandPacket->TargetStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ScsiDiskUnmap: Target indicating error status 0x%x.\n",
      CommandPacket->HostAdapterStatus
      ));

    ReturnStatus = EFI_DEVICE_ERROR;
    goto Done;
  }

Done:
  if (EraseBlkReq != NULL) {
    if (EraseBlkReq->CommandPacket.Cdb != NULL) {
      FreePool (EraseBlkReq->CommandPacket.Cdb);
    }

    FreePool (EraseBlkReq);
  }

  if (UnmapParamList != NULL) {
    FreePool (UnmapParamList);
  }

  if (AsyncUnmapEvent != NULL) {
    gBS->CloseEvent (AsyncUnmapEvent);
  }

  return ReturnStatus;
}

/**
  Erase a specified number of device blocks.

  @param[in]       This           Indicates a pointer to the calling context.
  @param[in]       MediaId        The media ID that the erase request is for.
  @param[in]       Lba            The starting logical block address to be
                                  erased. The caller is responsible for erasing
                                  only legitimate locations.
  @param[in, out]  Token          A pointer to the token associated with the
                                  transaction.
  @param[in]       Size           The size in bytes to be erased. This must be
                                  a multiple of the physical block size of the
                                  device.

  @retval EFI_SUCCESS             The erase request was queued if Event is not
                                  NULL. The data was erased correctly to the
                                  device if the Event is NULL.to the device.
  @retval EFI_WRITE_PROTECTED     The device cannot be erased due to write
                                  protection.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the erase operation.
  @retval EFI_INVALID_PARAMETER   The erase request contains LBAs that are not
                                  valid.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.

**/
EFI_STATUS
EFIAPI
ScsiDiskEraseBlocks (
  IN     EFI_ERASE_BLOCK_PROTOCOL  *This,
  IN     UINT32                    MediaId,
  IN     EFI_LBA                   Lba,
  IN OUT EFI_ERASE_BLOCK_TOKEN     *Token,
  IN     UINTN                     Size
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  MediaChange    = FALSE;
  OldTpl         = gBS->RaiseTPL (TPL_CALLBACK);
  ScsiDiskDevice = SCSI_DISK_DEV_FROM_ERASEBLK (This);

  if (!IS_DEVICE_FIXED (ScsiDiskDevice)) {
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
      gBS->ReinstallProtocolInterface (
             ScsiDiskDevice->Handle,
             &gEfiBlockIo2ProtocolGuid,
             &ScsiDiskDevice->BlkIo2,
             &ScsiDiskDevice->BlkIo2
             );
      if (DetermineInstallEraseBlock (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiEraseBlockProtocolGuid,
               &ScsiDiskDevice->EraseBlock,
               &ScsiDiskDevice->EraseBlock
               );
      }

      if (DetermineInstallStorageSecurity (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiStorageSecurityCommandProtocolGuid,
               &ScsiDiskDevice->StorageSecurity,
               &ScsiDiskDevice->StorageSecurity
               );
      }

      Status = EFI_MEDIA_CHANGED;
      goto Done;
    }
  }

  //
  // Get the intrinsic block size
  //
  Media = ScsiDiskDevice->BlkIo.Media;

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (Media->ReadOnly) {
    Status = EFI_WRITE_PROTECTED;
    goto Done;
  }

  if (Size == 0) {
    if ((Token != NULL) && (Token->Event != NULL)) {
      Token->TransactionStatus = EFI_SUCCESS;
      gBS->SignalEvent (Token->Event);
    }

    Status = EFI_SUCCESS;
    goto Done;
  }

  BlockSize = Media->BlockSize;
  if ((Size % BlockSize) != 0) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  NumberOfBlocks = Size / BlockSize;
  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = ScsiDiskUnmap (ScsiDiskDevice, Lba, NumberOfBlocks, Token);
  } else {
    Status = ScsiDiskUnmap (ScsiDiskDevice, Lba, NumberOfBlocks, NULL);
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Send a security protocol command to a device that receives data and/or the result
  of one or more commands sent by SendData.

  The ReceiveData function sends a security protocol command to the given MediaId.
  The security protocol command sent is defined by SecurityProtocolId and contains
  the security protocol specific data SecurityProtocolSpecificData. The function
  returns the data from the security protocol command in PayloadBuffer.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL IN command defined in SPC-4.

  If PayloadBufferSize is too small to store the available data from the security
  protocol command, the function shall copy PayloadBufferSize bytes into the
  PayloadBuffer and return EFI_WARN_BUFFER_TOO_SMALL.

  If PayloadBuffer or PayloadTransferSize is NULL and PayloadBufferSize is non-zero,
  the function shall return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function shall
  return EFI_UNSUPPORTED. If there is no media in the device, the function returns
  EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the device,
  the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall
  return EFI_SUCCESS. If the security protocol command completes with an error, the
  function shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from. If there is no
                                       block IO protocol supported by the physical device, the
                                       value of MediaId is undefined.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT if the
                                       time required to execute the receive data command is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command. The caller is responsible for having
                                       either implicit or explicit ownership of the buffer.
  @param  PayloadTransferSize          A pointer to a buffer to store the size in bytes of the
                                       data written to the payload data buffer.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL    The PayloadBufferSize was too small to store the available
                                       data from the device. The PayloadBuffer contains the truncated data.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer or PayloadTransferSize is NULL and
                                       PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
ScsiDiskReceiveData (
  IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL  *This,
  IN UINT32                                 MediaId   OPTIONAL,
  IN UINT64                                 Timeout,
  IN UINT8                                  SecurityProtocolId,
  IN UINT16                                 SecurityProtocolSpecificData,
  IN UINTN                                  PayloadBufferSize,
  OUT VOID                                  *PayloadBuffer,
  OUT UINTN                                 *PayloadTransferSize
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;
  UINT8               SenseDataLength;
  UINT8               HostAdapterStatus;
  UINT8               TargetStatus;
  VOID                *AlignedBuffer;
  BOOLEAN             AlignedBufferAllocated;

  AlignedBuffer          = NULL;
  MediaChange            = FALSE;
  AlignedBufferAllocated = FALSE;
  OldTpl                 = gBS->RaiseTPL (TPL_CALLBACK);
  ScsiDiskDevice         = SCSI_DISK_DEV_FROM_STORSEC (This);
  Media                  = ScsiDiskDevice->BlkIo.Media;

  SenseDataLength = (UINT8)(ScsiDiskDevice->SenseDataNumber * sizeof (EFI_SCSI_SENSE_DATA));

  if (!IS_DEVICE_FIXED (ScsiDiskDevice)) {
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
      gBS->ReinstallProtocolInterface (
             ScsiDiskDevice->Handle,
             &gEfiBlockIo2ProtocolGuid,
             &ScsiDiskDevice->BlkIo2,
             &ScsiDiskDevice->BlkIo2
             );
      if (DetermineInstallEraseBlock (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiEraseBlockProtocolGuid,
               &ScsiDiskDevice->EraseBlock,
               &ScsiDiskDevice->EraseBlock
               );
      }

      if (DetermineInstallStorageSecurity (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiStorageSecurityCommandProtocolGuid,
               &ScsiDiskDevice->StorageSecurity,
               &ScsiDiskDevice->StorageSecurity
               );
      }

      if (Media->MediaPresent) {
        Status = EFI_MEDIA_CHANGED;
      } else {
        Status = EFI_NO_MEDIA;
      }

      goto Done;
    }
  }

  //
  // Validate Media
  //
  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if ((MediaId != 0) && (MediaId != Media->MediaId)) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (PayloadBufferSize != 0) {
    if ((PayloadBuffer == NULL) || (PayloadTransferSize == NULL)) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    if ((ScsiDiskDevice->ScsiIo->IoAlign > 1) && !ADDRESS_IS_ALIGNED (PayloadBuffer, ScsiDiskDevice->ScsiIo->IoAlign)) {
      AlignedBuffer = AllocateAlignedBuffer (ScsiDiskDevice, PayloadBufferSize);
      if (AlignedBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      ZeroMem (AlignedBuffer, PayloadBufferSize);
      AlignedBufferAllocated = TRUE;
    } else {
      AlignedBuffer = PayloadBuffer;
    }
  }

  Status = ScsiSecurityProtocolInCommand (
             ScsiDiskDevice->ScsiIo,
             Timeout,
             ScsiDiskDevice->SenseData,
             &SenseDataLength,
             &HostAdapterStatus,
             &TargetStatus,
             SecurityProtocolId,
             SecurityProtocolSpecificData,
             FALSE,
             PayloadBufferSize,
             AlignedBuffer,
             PayloadTransferSize
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (AlignedBufferAllocated) {
    CopyMem (PayloadBuffer, AlignedBuffer, PayloadBufferSize);
  }

  if (PayloadBufferSize < *PayloadTransferSize) {
    Status = EFI_WARN_BUFFER_TOO_SMALL;
    goto Done;
  }

  Status = CheckHostAdapterStatus (HostAdapterStatus);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = CheckTargetStatus (TargetStatus);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

Done:
  if (AlignedBufferAllocated) {
    ZeroMem (AlignedBuffer, PayloadBufferSize);
    FreeAlignedBuffer (AlignedBuffer, PayloadBufferSize);
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Send a security protocol command to a device.

  The SendData function sends a security protocol command containing the payload
  PayloadBuffer to the given MediaId. The security protocol command sent is
  defined by SecurityProtocolId and contains the security protocol specific data
  SecurityProtocolSpecificData. If the underlying protocol command requires a
  specific padding for the command payload, the SendData function shall add padding
  bytes to the command payload to satisfy the padding requirements.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL OUT command defined in SPC-4.

  If PayloadBuffer is NULL and PayloadBufferSize is non-zero, the function shall
  return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function
  shall return EFI_UNSUPPORTED. If there is no media in the device, the function
  returns EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the
  device, the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall return
  EFI_SUCCESS. If the security protocol command completes with an error, the function
  shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from. If there is no
                                       block IO protocol supported by the physical device, the
                                       value of MediaId is undefined.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT if the
                                       time required to execute the receive data command is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer is NULL and PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
ScsiDiskSendData (
  IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL  *This,
  IN UINT32                                 MediaId   OPTIONAL,
  IN UINT64                                 Timeout,
  IN UINT8                                  SecurityProtocolId,
  IN UINT16                                 SecurityProtocolSpecificData,
  IN UINTN                                  PayloadBufferSize,
  OUT VOID                                  *PayloadBuffer
  )
{
  SCSI_DISK_DEV       *ScsiDiskDevice;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;
  UINT8               SenseDataLength;
  UINT8               HostAdapterStatus;
  UINT8               TargetStatus;
  VOID                *AlignedBuffer;
  BOOLEAN             AlignedBufferAllocated;

  AlignedBuffer          = NULL;
  MediaChange            = FALSE;
  AlignedBufferAllocated = FALSE;
  OldTpl                 = gBS->RaiseTPL (TPL_CALLBACK);
  ScsiDiskDevice         = SCSI_DISK_DEV_FROM_STORSEC (This);
  Media                  = ScsiDiskDevice->BlkIo.Media;

  SenseDataLength = (UINT8)(ScsiDiskDevice->SenseDataNumber * sizeof (EFI_SCSI_SENSE_DATA));

  if (!IS_DEVICE_FIXED (ScsiDiskDevice)) {
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
      gBS->ReinstallProtocolInterface (
             ScsiDiskDevice->Handle,
             &gEfiBlockIo2ProtocolGuid,
             &ScsiDiskDevice->BlkIo2,
             &ScsiDiskDevice->BlkIo2
             );
      if (DetermineInstallEraseBlock (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiEraseBlockProtocolGuid,
               &ScsiDiskDevice->EraseBlock,
               &ScsiDiskDevice->EraseBlock
               );
      }

      if (DetermineInstallStorageSecurity (ScsiDiskDevice, ScsiDiskDevice->Handle)) {
        gBS->ReinstallProtocolInterface (
               ScsiDiskDevice->Handle,
               &gEfiStorageSecurityCommandProtocolGuid,
               &ScsiDiskDevice->StorageSecurity,
               &ScsiDiskDevice->StorageSecurity
               );
      }

      if (Media->MediaPresent) {
        Status = EFI_MEDIA_CHANGED;
      } else {
        Status = EFI_NO_MEDIA;
      }

      goto Done;
    }
  }

  //
  // Validate Media
  //
  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if ((MediaId != 0) && (MediaId != Media->MediaId)) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (Media->ReadOnly) {
    Status = EFI_WRITE_PROTECTED;
    goto Done;
  }

  if (PayloadBufferSize != 0) {
    if (PayloadBuffer == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    if ((ScsiDiskDevice->ScsiIo->IoAlign > 1) && !ADDRESS_IS_ALIGNED (PayloadBuffer, ScsiDiskDevice->ScsiIo->IoAlign)) {
      AlignedBuffer = AllocateAlignedBuffer (ScsiDiskDevice, PayloadBufferSize);
      if (AlignedBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      CopyMem (AlignedBuffer, PayloadBuffer, PayloadBufferSize);
      AlignedBufferAllocated = TRUE;
    } else {
      AlignedBuffer = PayloadBuffer;
    }
  }

  Status = ScsiSecurityProtocolOutCommand (
             ScsiDiskDevice->ScsiIo,
             Timeout,
             ScsiDiskDevice->SenseData,
             &SenseDataLength,
             &HostAdapterStatus,
             &TargetStatus,
             SecurityProtocolId,
             SecurityProtocolSpecificData,
             FALSE,
             PayloadBufferSize,
             AlignedBuffer
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = CheckHostAdapterStatus (HostAdapterStatus);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = CheckTargetStatus (TargetStatus);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

Done:
  if (AlignedBufferAllocated) {
    ZeroMem (AlignedBuffer, PayloadBufferSize);
    FreeAlignedBuffer (AlignedBuffer, PayloadBufferSize);
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Detect Device and read out capacity ,if error occurs, parse the sense key.

  @param  ScsiDiskDevice    The pointer of SCSI_DISK_DEV
  @param  MustReadCapacity  The flag about reading device capacity
  @param  MediaChange       The pointer of flag indicates if media has changed

  @retval EFI_DEVICE_ERROR  Indicates that error occurs
  @retval EFI_SUCCESS       Successfully to detect media

**/
EFI_STATUS
ScsiDiskDetectMedia (
  IN   SCSI_DISK_DEV  *ScsiDiskDevice,
  IN   BOOLEAN        MustReadCapacity,
  OUT  BOOLEAN        *MediaChange
  )
{
  EFI_STATUS           Status;
  EFI_SCSI_SENSE_DATA  *SenseData;
  UINTN                NumberOfSenseKeys;
  BOOLEAN              NeedRetry;
  BOOLEAN              NeedReadCapacity;
  UINT8                Retry;
  UINT8                MaxRetry;
  EFI_BLOCK_IO_MEDIA   OldMedia;
  UINTN                Action;
  EFI_EVENT            TimeoutEvt;

  Status            = EFI_SUCCESS;
  SenseData         = NULL;
  NumberOfSenseKeys = 0;
  Retry             = 0;
  MaxRetry          = 3;
  Action            = ACTION_NO_ACTION;
  NeedReadCapacity  = FALSE;
  *MediaChange      = FALSE;
  TimeoutEvt        = NULL;

  CopyMem (&OldMedia, ScsiDiskDevice->BlkIo.Media, sizeof (OldMedia));

  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimeoutEvt
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (TimeoutEvt, TimerRelative, EFI_TIMER_PERIOD_SECONDS (120));
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Sending Test_Unit cmd to poll device status.
  // If the sense data shows the drive is not ready or reset before, we need poll the device status again.
  // We limit the upper boundary to 120 seconds.
  //
  while (EFI_ERROR (gBS->CheckEvent (TimeoutEvt))) {
    Status = ScsiDiskTestUnitReady (
               ScsiDiskDevice,
               &NeedRetry,
               &SenseData,
               &NumberOfSenseKeys
               );
    if (!EFI_ERROR (Status)) {
      Status = DetectMediaParsingSenseKeys (
                 ScsiDiskDevice,
                 SenseData,
                 NumberOfSenseKeys,
                 &Action
                 );
      if (EFI_ERROR (Status)) {
        goto EXIT;
      } else if (Action == ACTION_RETRY_COMMAND_LATER) {
        continue;
      } else {
        break;
      }
    } else {
      Retry++;
      if (!NeedRetry || (Retry >= MaxRetry)) {
        goto EXIT;
      }
    }
  }

  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // ACTION_NO_ACTION: need not read capacity
  // other action code: need read capacity
  //
  if (Action == ACTION_READ_CAPACITY) {
    NeedReadCapacity = TRUE;
  }

  //
  // READ_CAPACITY command is not supported by any of the UFS WLUNs.
  //
  if (ScsiDiskDevice->DeviceType == EFI_SCSI_TYPE_WLUN) {
    NeedReadCapacity                          = FALSE;
    MustReadCapacity                          = FALSE;
    ScsiDiskDevice->BlkIo.Media->MediaPresent = TRUE;
  }

  //
  // either NeedReadCapacity is TRUE, or MustReadCapacity is TRUE,
  // retrieve capacity via Read Capacity command
  //
  if (NeedReadCapacity || MustReadCapacity) {
    //
    // retrieve media information
    //
    for (Retry = 0; Retry < MaxRetry; Retry++) {
      Status = ScsiDiskReadCapacity (
                 ScsiDiskDevice,
                 &NeedRetry,
                 &SenseData,
                 &NumberOfSenseKeys
                 );
      if (!EFI_ERROR (Status)) {
        //
        // analyze sense key to action
        //
        Status = DetectMediaParsingSenseKeys (
                   ScsiDiskDevice,
                   SenseData,
                   NumberOfSenseKeys,
                   &Action
                   );
        if (EFI_ERROR (Status)) {
          //
          // if Status is error, it may indicate crisis error,
          // so return without retry.
          //
          goto EXIT;
        } else if (Action == ACTION_RETRY_COMMAND_LATER) {
          Retry = 0;
          continue;
        } else {
          break;
        }
      } else {
        Retry++;
        if (!NeedRetry || (Retry >= MaxRetry)) {
          goto EXIT;
        }
      }
    }

    if (EFI_ERROR (Status)) {
      goto EXIT;
    }
  }

  if (ScsiDiskDevice->BlkIo.Media->MediaId != OldMedia.MediaId) {
    //
    // Media change information got from the device
    //
    *MediaChange = TRUE;
  }

  if (ScsiDiskDevice->BlkIo.Media->ReadOnly != OldMedia.ReadOnly) {
    *MediaChange                          = TRUE;
    ScsiDiskDevice->BlkIo.Media->MediaId += 1;
  }

  if (ScsiDiskDevice->BlkIo.Media->BlockSize != OldMedia.BlockSize) {
    *MediaChange                          = TRUE;
    ScsiDiskDevice->BlkIo.Media->MediaId += 1;
  }

  if (ScsiDiskDevice->BlkIo.Media->LastBlock != OldMedia.LastBlock) {
    *MediaChange                          = TRUE;
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

EXIT:
  if (TimeoutEvt != NULL) {
    gBS->CloseEvent (TimeoutEvt);
  }

  return Status;
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
  IN OUT  SCSI_DISK_DEV  *ScsiDiskDevice,
  OUT  BOOLEAN           *NeedRetry
  )
{
  UINT32                                 InquiryDataLength;
  UINT8                                  SenseDataLength;
  UINT8                                  HostAdapterStatus;
  UINT8                                  TargetStatus;
  EFI_SCSI_SENSE_DATA                    *SenseDataArray;
  UINTN                                  NumberOfSenseKeys;
  EFI_STATUS                             Status;
  UINT8                                  MaxRetry;
  UINT8                                  Index;
  EFI_SCSI_SUPPORTED_VPD_PAGES_VPD_PAGE  *SupportedVpdPages;
  EFI_SCSI_BLOCK_LIMITS_VPD_PAGE         *BlockLimits;
  UINTN                                  PageLength;

  InquiryDataLength = sizeof (EFI_SCSI_INQUIRY_DATA);
  SenseDataLength   = 0;

  Status = ScsiInquiryCommand (
             ScsiDiskDevice->ScsiIo,
             SCSI_DISK_TIMEOUT,
             NULL,
             &SenseDataLength,
             &HostAdapterStatus,
             &TargetStatus,
             (VOID *)&(ScsiDiskDevice->InquiryData),
             &InquiryDataLength,
             FALSE
             );
  //
  // no need to check HostAdapterStatus and TargetStatus
  //
  if ((Status == EFI_SUCCESS) || (Status == EFI_WARN_BUFFER_TOO_SMALL)) {
    ParseInquiryData (ScsiDiskDevice);

    if (ScsiDiskDevice->DeviceType == EFI_SCSI_TYPE_DISK) {
      //
      // Check whether the device supports Block Limits VPD page (0xB0)
      //
      SupportedVpdPages = AllocateAlignedBuffer (ScsiDiskDevice, sizeof (EFI_SCSI_SUPPORTED_VPD_PAGES_VPD_PAGE));
      if (SupportedVpdPages == NULL) {
        *NeedRetry = FALSE;
        return EFI_DEVICE_ERROR;
      }

      ZeroMem (SupportedVpdPages, sizeof (EFI_SCSI_SUPPORTED_VPD_PAGES_VPD_PAGE));
      InquiryDataLength = sizeof (EFI_SCSI_SUPPORTED_VPD_PAGES_VPD_PAGE);
      SenseDataLength   = 0;
      Status            = ScsiInquiryCommandEx (
                            ScsiDiskDevice->ScsiIo,
                            SCSI_DISK_TIMEOUT,
                            NULL,
                            &SenseDataLength,
                            &HostAdapterStatus,
                            &TargetStatus,
                            (VOID *)SupportedVpdPages,
                            &InquiryDataLength,
                            TRUE,
                            EFI_SCSI_PAGE_CODE_SUPPORTED_VPD
                            );
      if (!EFI_ERROR (Status)) {
        PageLength = (SupportedVpdPages->PageLength2 << 8)
                     |  SupportedVpdPages->PageLength1;

        //
        // Sanity checks for coping with broken devices
        //
        if (PageLength > sizeof SupportedVpdPages->SupportedVpdPageList) {
          DEBUG ((
            DEBUG_WARN,
            "%a: invalid PageLength (%u) in Supported VPD Pages page\n",
            __func__,
            (UINT32)PageLength
            ));
          PageLength = 0;
        }

        if ((PageLength > 0) &&
            (SupportedVpdPages->SupportedVpdPageList[0] !=
             EFI_SCSI_PAGE_CODE_SUPPORTED_VPD))
        {
          DEBUG ((
            DEBUG_WARN,
            "%a: Supported VPD Pages page doesn't start with code 0x%02x\n",
            __func__,
            EFI_SCSI_PAGE_CODE_SUPPORTED_VPD
            ));
          PageLength = 0;
        }

        //
        // Locate the code for the Block Limits VPD page
        //
        for (Index = 0; Index < PageLength; Index++) {
          //
          // Sanity check
          //
          if ((Index > 0) &&
              (SupportedVpdPages->SupportedVpdPageList[Index] <=
               SupportedVpdPages->SupportedVpdPageList[Index - 1]))
          {
            DEBUG ((
              DEBUG_WARN,
              "%a: non-ascending code in Supported VPD Pages page @ %u\n",
              __func__,
              Index
              ));
            Index      = 0;
            PageLength = 0;
            break;
          }

          if (SupportedVpdPages->SupportedVpdPageList[Index] == EFI_SCSI_PAGE_CODE_BLOCK_LIMITS_VPD) {
            break;
          }
        }

        //
        // Query the Block Limits VPD page
        //
        if (Index < PageLength) {
          BlockLimits = AllocateAlignedBuffer (ScsiDiskDevice, sizeof (EFI_SCSI_BLOCK_LIMITS_VPD_PAGE));
          if (BlockLimits == NULL) {
            FreeAlignedBuffer (SupportedVpdPages, sizeof (EFI_SCSI_SUPPORTED_VPD_PAGES_VPD_PAGE));
            *NeedRetry = FALSE;
            return EFI_DEVICE_ERROR;
          }

          ZeroMem (BlockLimits, sizeof (EFI_SCSI_BLOCK_LIMITS_VPD_PAGE));
          InquiryDataLength = sizeof (EFI_SCSI_BLOCK_LIMITS_VPD_PAGE);
          SenseDataLength   = 0;
          Status            = ScsiInquiryCommandEx (
                                ScsiDiskDevice->ScsiIo,
                                SCSI_DISK_TIMEOUT,
                                NULL,
                                &SenseDataLength,
                                &HostAdapterStatus,
                                &TargetStatus,
                                (VOID *)BlockLimits,
                                &InquiryDataLength,
                                TRUE,
                                EFI_SCSI_PAGE_CODE_BLOCK_LIMITS_VPD
                                );
          if (!EFI_ERROR (Status)) {
            ScsiDiskDevice->BlkIo.Media->OptimalTransferLengthGranularity =
              (BlockLimits->OptimalTransferLengthGranularity2 << 8) |
              BlockLimits->OptimalTransferLengthGranularity1;

            ScsiDiskDevice->UnmapInfo.MaxLbaCnt =
              (BlockLimits->MaximumUnmapLbaCount4 << 24) |
              (BlockLimits->MaximumUnmapLbaCount3 << 16) |
              (BlockLimits->MaximumUnmapLbaCount2 << 8)  |
              BlockLimits->MaximumUnmapLbaCount1;
            ScsiDiskDevice->UnmapInfo.MaxBlkDespCnt =
              (BlockLimits->MaximumUnmapBlockDescriptorCount4 << 24) |
              (BlockLimits->MaximumUnmapBlockDescriptorCount3 << 16) |
              (BlockLimits->MaximumUnmapBlockDescriptorCount2 << 8)  |
              BlockLimits->MaximumUnmapBlockDescriptorCount1;
            ScsiDiskDevice->EraseBlock.EraseLengthGranularity =
              (BlockLimits->OptimalUnmapGranularity4 << 24) |
              (BlockLimits->OptimalUnmapGranularity3 << 16) |
              (BlockLimits->OptimalUnmapGranularity2 << 8)  |
              BlockLimits->OptimalUnmapGranularity1;
            if (BlockLimits->UnmapGranularityAlignmentValid != 0) {
              ScsiDiskDevice->UnmapInfo.GranularityAlignment =
                (BlockLimits->UnmapGranularityAlignment4 << 24) |
                (BlockLimits->UnmapGranularityAlignment3 << 16) |
                (BlockLimits->UnmapGranularityAlignment2 << 8)  |
                BlockLimits->UnmapGranularityAlignment1;
            }

            if (ScsiDiskDevice->EraseBlock.EraseLengthGranularity == 0) {
              //
              // A value of 0 indicates that the optimal unmap granularity is
              // not reported.
              //
              ScsiDiskDevice->EraseBlock.EraseLengthGranularity = 1;
            }

            ScsiDiskDevice->BlockLimitsVpdSupported = TRUE;
          }

          FreeAlignedBuffer (BlockLimits, sizeof (EFI_SCSI_BLOCK_LIMITS_VPD_PAGE));
        }
      }

      FreeAlignedBuffer (SupportedVpdPages, sizeof (EFI_SCSI_SUPPORTED_VPD_PAGES_VPD_PAGE));
    }
  }

  if (!EFI_ERROR (Status)) {
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
  // if goes here, meant ScsiInquiryCommand() failed.
  // if ScsiDiskRequestSenseKeys() succeeds at last,
  // better retry ScsiInquiryCommand(). (by setting *NeedRetry = TRUE)
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
  To test device.

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
  IN  SCSI_DISK_DEV        *ScsiDiskDevice,
  OUT BOOLEAN              *NeedRetry,
  OUT EFI_SCSI_SENSE_DATA  **SenseDataArray,
  OUT UINTN                *NumberOfSenseKeys
  )
{
  EFI_STATUS  Status;
  UINT8       SenseDataLength;
  UINT8       HostAdapterStatus;
  UINT8       TargetStatus;
  UINT8       Index;
  UINT8       MaxRetry;

  SenseDataLength    = (UINT8)(ScsiDiskDevice->SenseDataNumber * sizeof (EFI_SCSI_SENSE_DATA));
  *NumberOfSenseKeys = 0;

  //
  // Parameter 3 and 4: do not require sense data, retrieve it when needed.
  //
  Status = ScsiTestUnitReadyCommand (
             ScsiDiskDevice->ScsiIo,
             SCSI_DISK_TIMEOUT,
             ScsiDiskDevice->SenseData,
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

  if (SenseDataLength != 0) {
    *NumberOfSenseKeys = SenseDataLength / sizeof (EFI_SCSI_SENSE_DATA);
    *SenseDataArray    = ScsiDiskDevice->SenseData;
    return EFI_SUCCESS;
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
  OUT  SCSI_DISK_DEV        *ScsiDiskDevice,
  IN   EFI_SCSI_SENSE_DATA  *SenseData,
  IN   UINTN                NumberOfSenseKeys,
  OUT  UINTN                *Action
  )
{
  BOOLEAN  RetryLater;

  //
  // Default is to read capacity, unless..
  //
  *Action = ACTION_READ_CAPACITY;

  if (NumberOfSenseKeys == 0) {
    if (ScsiDiskDevice->BlkIo.Media->MediaPresent == TRUE) {
      *Action = ACTION_NO_ACTION;
    }

    return EFI_SUCCESS;
  }

  if (!ScsiDiskHaveSenseKey (SenseData, NumberOfSenseKeys)) {
    //
    // No Sense Key returned from last submitted command
    //
    if (ScsiDiskDevice->BlkIo.Media->MediaPresent == TRUE) {
      *Action = ACTION_NO_ACTION;
    }

    return EFI_SUCCESS;
  }

  if (ScsiDiskIsNoMedia (SenseData, NumberOfSenseKeys)) {
    ScsiDiskDevice->BlkIo.Media->MediaPresent = FALSE;
    ScsiDiskDevice->BlkIo.Media->LastBlock    = 0;
    *Action                                   = ACTION_NO_ACTION;
    DEBUG ((DEBUG_VERBOSE, "ScsiDisk: ScsiDiskIsNoMedia\n"));
    return EFI_SUCCESS;
  }

  if (ScsiDiskIsMediaChange (SenseData, NumberOfSenseKeys)) {
    ScsiDiskDevice->BlkIo.Media->MediaId++;
    DEBUG ((DEBUG_VERBOSE, "ScsiDisk: ScsiDiskIsMediaChange!\n"));
    return EFI_SUCCESS;
  }

  if (ScsiDiskIsResetBefore (SenseData, NumberOfSenseKeys)) {
    *Action = ACTION_RETRY_COMMAND_LATER;
    DEBUG ((DEBUG_VERBOSE, "ScsiDisk: ScsiDiskIsResetBefore!\n"));
    return EFI_SUCCESS;
  }

  if (ScsiDiskIsMediaError (SenseData, NumberOfSenseKeys)) {
    DEBUG ((DEBUG_VERBOSE, "ScsiDisk: ScsiDiskIsMediaError\n"));
    *Action = ACTION_RETRY_WITH_BACKOFF_ALGO;
    return EFI_DEVICE_ERROR;
  }

  if (ScsiDiskIsHardwareError (SenseData, NumberOfSenseKeys)) {
    DEBUG ((DEBUG_VERBOSE, "ScsiDisk: ScsiDiskIsHardwareError\n"));
    *Action = ACTION_RETRY_WITH_BACKOFF_ALGO;
    return EFI_DEVICE_ERROR;
  }

  if (!ScsiDiskIsDriveReady (SenseData, NumberOfSenseKeys, &RetryLater)) {
    if (RetryLater) {
      *Action = ACTION_RETRY_COMMAND_LATER;
      DEBUG ((DEBUG_VERBOSE, "ScsiDisk: ScsiDiskDriveNotReady!\n"));
      return EFI_SUCCESS;
    }

    *Action = ACTION_NO_ACTION;
    return EFI_DEVICE_ERROR;
  }

  *Action = ACTION_RETRY_WITH_BACKOFF_ALGO;
  DEBUG ((DEBUG_VERBOSE, "ScsiDisk: Sense Key = 0x%x ASC = 0x%x!\n", SenseData->Sense_Key, SenseData->Addnl_Sense_Code));
  return EFI_SUCCESS;
}

/**
  Send read capacity command to device and get the device parameter.

  @param  ScsiDiskDevice     The pointer of SCSI_DISK_DEV
  @param  NeedRetry          The pointer of flag indicates if need a retry
  @param  SenseDataArray     The pointer of an array of sense data
  @param  NumberOfSenseKeys  The number of sense key

  @retval EFI_DEVICE_ERROR   Indicates that error occurs
  @retval EFI_SUCCESS        Successfully to read capacity or sense data is received.

**/
EFI_STATUS
ScsiDiskReadCapacity (
  IN  OUT  SCSI_DISK_DEV    *ScsiDiskDevice,
  OUT  BOOLEAN              *NeedRetry,
  OUT  EFI_SCSI_SENSE_DATA  **SenseDataArray,
  OUT  UINTN                *NumberOfSenseKeys
  )
{
  UINT8                          HostAdapterStatus;
  UINT8                          TargetStatus;
  EFI_STATUS                     CommandStatus;
  EFI_STATUS                     Status;
  UINT8                          Index;
  UINT8                          MaxRetry;
  UINT8                          SenseDataLength;
  UINT32                         DataLength10;
  UINT32                         DataLength16;
  EFI_SCSI_DISK_CAPACITY_DATA    *CapacityData10;
  EFI_SCSI_DISK_CAPACITY_DATA16  *CapacityData16;

  CapacityData10 = AllocateAlignedBuffer (ScsiDiskDevice, sizeof (EFI_SCSI_DISK_CAPACITY_DATA));
  if (CapacityData10 == NULL) {
    *NeedRetry = FALSE;
    return EFI_DEVICE_ERROR;
  }

  CapacityData16 = AllocateAlignedBuffer (ScsiDiskDevice, sizeof (EFI_SCSI_DISK_CAPACITY_DATA16));
  if (CapacityData16 == NULL) {
    FreeAlignedBuffer (CapacityData10, sizeof (EFI_SCSI_DISK_CAPACITY_DATA));
    *NeedRetry = FALSE;
    return EFI_DEVICE_ERROR;
  }

  SenseDataLength = 0;
  DataLength10    = sizeof (EFI_SCSI_DISK_CAPACITY_DATA);
  DataLength16    = sizeof (EFI_SCSI_DISK_CAPACITY_DATA16);
  ZeroMem (CapacityData10, sizeof (EFI_SCSI_DISK_CAPACITY_DATA));
  ZeroMem (CapacityData16, sizeof (EFI_SCSI_DISK_CAPACITY_DATA16));

  *NumberOfSenseKeys = 0;
  *NeedRetry         = FALSE;

  //
  // submit Read Capacity(10) Command. If it returns capacity of FFFFFFFFh,
  // 16 byte command should be used to access large hard disk >2TB
  //
  CommandStatus = ScsiReadCapacityCommand (
                    ScsiDiskDevice->ScsiIo,
                    SCSI_DISK_TIMEOUT,
                    NULL,
                    &SenseDataLength,
                    &HostAdapterStatus,
                    &TargetStatus,
                    (VOID *)CapacityData10,
                    &DataLength10,
                    FALSE
                    );

  ScsiDiskDevice->Cdb16Byte = FALSE;
  if ((!EFI_ERROR (CommandStatus)) && (CapacityData10->LastLba3 == 0xff) && (CapacityData10->LastLba2 == 0xff) &&
      (CapacityData10->LastLba1 == 0xff) && (CapacityData10->LastLba0 == 0xff))
  {
    //
    // use Read Capacity (16), Read (16) and Write (16) next when hard disk size > 2TB
    //
    ScsiDiskDevice->Cdb16Byte = TRUE;
    //
    // submit Read Capacity(16) Command to get parameter LogicalBlocksPerPhysicalBlock
    // and LowestAlignedLba
    //
    CommandStatus = ScsiReadCapacity16Command (
                      ScsiDiskDevice->ScsiIo,
                      SCSI_DISK_TIMEOUT,
                      NULL,
                      &SenseDataLength,
                      &HostAdapterStatus,
                      &TargetStatus,
                      (VOID *)CapacityData16,
                      &DataLength16,
                      FALSE
                      );
  }

  //
  // no need to check HostAdapterStatus and TargetStatus
  //
  if (CommandStatus == EFI_SUCCESS) {
    GetMediaInfo (ScsiDiskDevice, CapacityData10, CapacityData16);
    FreeAlignedBuffer (CapacityData10, sizeof (EFI_SCSI_DISK_CAPACITY_DATA));
    FreeAlignedBuffer (CapacityData16, sizeof (EFI_SCSI_DISK_CAPACITY_DATA16));
    return EFI_SUCCESS;
  }

  FreeAlignedBuffer (CapacityData10, sizeof (EFI_SCSI_DISK_CAPACITY_DATA));
  FreeAlignedBuffer (CapacityData16, sizeof (EFI_SCSI_DISK_CAPACITY_DATA16));

  if (CommandStatus == EFI_NOT_READY) {
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
  // if goes here, meant ScsiReadCapacityCommand() failed.
  // if ScsiDiskRequestSenseKeys() succeeds at last,
  // better retry ScsiReadCapacityCommand(). (by setting *NeedRetry = TRUE)
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
  Check the HostAdapter status and re-interpret it in EFI_STATUS.

  @param  HostAdapterStatus  Host Adapter status

  @retval  EFI_SUCCESS       Host adapter is OK.
  @retval  EFI_TIMEOUT       Timeout.
  @retval  EFI_NOT_READY     Adapter NOT ready.
  @retval  EFI_DEVICE_ERROR  Adapter device error.

**/
EFI_STATUS
CheckHostAdapterStatus (
  IN UINT8  HostAdapterStatus
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
  IN  UINT8  TargetStatus
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

    default:
      return EFI_SUCCESS;
  }
}

/**
  Retrieve all sense keys from the device.

  When encountering error during the process, if retrieve sense keys before
  error encountered, it returns the sense keys with return status set to EFI_SUCCESS,
  and NeedRetry set to FALSE; otherwise, return the proper return status.

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
  IN  OUT  SCSI_DISK_DEV    *ScsiDiskDevice,
  OUT  BOOLEAN              *NeedRetry,
  OUT  EFI_SCSI_SENSE_DATA  **SenseDataArray,
  OUT  UINTN                *NumberOfSenseKeys,
  IN       BOOLEAN          AskResetIfError
  )
{
  EFI_SCSI_SENSE_DATA  *PtrSenseData;
  UINT8                SenseDataLength;
  BOOLEAN              SenseReq;
  EFI_STATUS           Status;
  EFI_STATUS           FallStatus;
  UINT8                HostAdapterStatus;
  UINT8                TargetStatus;

  FallStatus      = EFI_SUCCESS;
  SenseDataLength = (UINT8)sizeof (EFI_SCSI_SENSE_DATA);

  ZeroMem (
    ScsiDiskDevice->SenseData,
    sizeof (EFI_SCSI_SENSE_DATA) * (ScsiDiskDevice->SenseDataNumber)
    );

  *NumberOfSenseKeys = 0;
  *SenseDataArray    = ScsiDiskDevice->SenseData;
  Status             = EFI_SUCCESS;
  PtrSenseData       = AllocateAlignedBuffer (ScsiDiskDevice, sizeof (EFI_SCSI_SENSE_DATA));
  if (PtrSenseData == NULL) {
    return EFI_DEVICE_ERROR;
  }

  for (SenseReq = TRUE; SenseReq;) {
    ZeroMem (PtrSenseData, sizeof (EFI_SCSI_SENSE_DATA));
    Status = ScsiRequestSenseCommand (
               ScsiDiskDevice->ScsiIo,
               SCSI_DISK_TIMEOUT,
               PtrSenseData,
               &SenseDataLength,
               &HostAdapterStatus,
               &TargetStatus
               );
    if ((Status == EFI_SUCCESS) || (Status == EFI_WARN_BUFFER_TOO_SMALL)) {
      FallStatus = EFI_SUCCESS;
    } else if ((Status == EFI_TIMEOUT) || (Status == EFI_NOT_READY)) {
      *NeedRetry = TRUE;
      FallStatus = EFI_DEVICE_ERROR;
    } else if ((Status == EFI_INVALID_PARAMETER) || (Status == EFI_UNSUPPORTED)) {
      *NeedRetry = FALSE;
      FallStatus = EFI_DEVICE_ERROR;
    } else if (Status == EFI_DEVICE_ERROR) {
      if (AskResetIfError) {
        ScsiDiskDevice->ScsiIo->ResetDevice (ScsiDiskDevice->ScsiIo);
      }

      FallStatus = EFI_DEVICE_ERROR;
    }

    if (EFI_ERROR (FallStatus)) {
      if (*NumberOfSenseKeys != 0) {
        *NeedRetry = FALSE;
        Status     = EFI_SUCCESS;
        goto EXIT;
      } else {
        Status = EFI_DEVICE_ERROR;
        goto EXIT;
      }
    }

    CopyMem (ScsiDiskDevice->SenseData + *NumberOfSenseKeys, PtrSenseData, SenseDataLength);
    (*NumberOfSenseKeys) += 1;

    //
    // no more sense key or number of sense keys exceeds predefined,
    // skip the loop.
    //
    if ((PtrSenseData->Sense_Key == EFI_SCSI_SK_NO_SENSE) ||
        (*NumberOfSenseKeys == ScsiDiskDevice->SenseDataNumber))
    {
      SenseReq = FALSE;
    }
  }

EXIT:
  FreeAlignedBuffer (PtrSenseData, sizeof (EFI_SCSI_SENSE_DATA));
  return Status;
}

/**
  Get information from media read capacity command.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV
  @param  Capacity10      The pointer of EFI_SCSI_DISK_CAPACITY_DATA
  @param  Capacity16      The pointer of EFI_SCSI_DISK_CAPACITY_DATA16

**/
VOID
GetMediaInfo (
  IN OUT SCSI_DISK_DEV                  *ScsiDiskDevice,
  IN     EFI_SCSI_DISK_CAPACITY_DATA    *Capacity10,
  IN     EFI_SCSI_DISK_CAPACITY_DATA16  *Capacity16
  )
{
  UINT8  *Ptr;

  if (!ScsiDiskDevice->Cdb16Byte) {
    ScsiDiskDevice->BlkIo.Media->LastBlock =  ((UINT32)Capacity10->LastLba3 << 24) |
                                             (Capacity10->LastLba2 << 16) |
                                             (Capacity10->LastLba1 << 8)  |
                                             Capacity10->LastLba0;

    ScsiDiskDevice->BlkIo.Media->BlockSize = (Capacity10->BlockSize3 << 24) |
                                             (Capacity10->BlockSize2 << 16) |
                                             (Capacity10->BlockSize1 << 8)  |
                                             Capacity10->BlockSize0;
    ScsiDiskDevice->BlkIo.Media->LowestAlignedLba              = 0;
    ScsiDiskDevice->BlkIo.Media->LogicalBlocksPerPhysicalBlock = 0;
    if (!ScsiDiskDevice->BlockLimitsVpdSupported) {
      ScsiDiskDevice->UnmapInfo.MaxLbaCnt = (UINT32)ScsiDiskDevice->BlkIo.Media->LastBlock;
    }
  } else {
    Ptr    = (UINT8 *)&ScsiDiskDevice->BlkIo.Media->LastBlock;
    *Ptr++ = Capacity16->LastLba0;
    *Ptr++ = Capacity16->LastLba1;
    *Ptr++ = Capacity16->LastLba2;
    *Ptr++ = Capacity16->LastLba3;
    *Ptr++ = Capacity16->LastLba4;
    *Ptr++ = Capacity16->LastLba5;
    *Ptr++ = Capacity16->LastLba6;
    *Ptr   = Capacity16->LastLba7;

    ScsiDiskDevice->BlkIo.Media->BlockSize = (Capacity16->BlockSize3 << 24) |
                                             (Capacity16->BlockSize2 << 16) |
                                             (Capacity16->BlockSize1 << 8)  |
                                             Capacity16->BlockSize0;

    ScsiDiskDevice->BlkIo.Media->LowestAlignedLba = (Capacity16->LowestAlignLogic2 << 8) |
                                                    Capacity16->LowestAlignLogic1;
    ScsiDiskDevice->BlkIo.Media->LogicalBlocksPerPhysicalBlock = (1 << Capacity16->LogicPerPhysical);
    if (!ScsiDiskDevice->BlockLimitsVpdSupported) {
      if (ScsiDiskDevice->BlkIo.Media->LastBlock > (UINT32)-1) {
        ScsiDiskDevice->UnmapInfo.MaxLbaCnt = (UINT32)-1;
      } else {
        ScsiDiskDevice->UnmapInfo.MaxLbaCnt = (UINT32)ScsiDiskDevice->BlkIo.Media->LastBlock;
      }
    }
  }

  ScsiDiskDevice->BlkIo.Media->MediaPresent = TRUE;
}

/**
  Parse Inquiry data.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV

**/
VOID
ParseInquiryData (
  IN OUT SCSI_DISK_DEV  *ScsiDiskDevice
  )
{
  ScsiDiskDevice->FixedDevice               = (BOOLEAN)((ScsiDiskDevice->InquiryData.Rmb == 1) ? 0 : 1);
  ScsiDiskDevice->BlkIoMedia.RemovableMedia = (BOOLEAN)(!ScsiDiskDevice->FixedDevice);
}

/**
  Read sector from SCSI Disk.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV
  @param  Buffer          The buffer to fill in the read out data
  @param  Lba             Logic block address
  @param  NumberOfBlocks  The number of blocks to read

  @retval EFI_DEVICE_ERROR  Indicates a device error.
  @retval EFI_SUCCESS       Operation is successful.

**/
EFI_STATUS
ScsiDiskReadSectors (
  IN   SCSI_DISK_DEV  *ScsiDiskDevice,
  OUT  VOID           *Buffer,
  IN   EFI_LBA        Lba,
  IN   UINTN          NumberOfBlocks
  )
{
  UINTN       BlocksRemaining;
  UINT8       *PtrBuffer;
  UINT32      BlockSize;
  UINT32      ByteCount;
  UINT32      MaxBlock;
  UINT32      SectorCount;
  UINT32      NextSectorCount;
  UINT64      Timeout;
  EFI_STATUS  Status;
  UINT8       Index;
  UINT8       MaxRetry;
  BOOLEAN     NeedRetry;

  Status = EFI_SUCCESS;

  BlocksRemaining = NumberOfBlocks;
  BlockSize       = ScsiDiskDevice->BlkIo.Media->BlockSize;

  //
  // limit the data bytes that can be transferred by one Read(10) or Read(16) Command
  //
  if (!ScsiDiskDevice->Cdb16Byte) {
    MaxBlock = 0xFFFF;
  } else {
    MaxBlock = 0xFFFFFFFF;
  }

  PtrBuffer = Buffer;

  while (BlocksRemaining > 0) {
    if (BlocksRemaining <= MaxBlock) {
      if (!ScsiDiskDevice->Cdb16Byte) {
        SectorCount = (UINT16)BlocksRemaining;
      } else {
        SectorCount = (UINT32)BlocksRemaining;
      }
    } else {
      SectorCount = MaxBlock;
    }

    ByteCount = SectorCount * BlockSize;
    //
    // |------------------------|-----------------|------------------|-----------------|
    // |   ATA Transfer Mode    |  Transfer Rate  |  SCSI Interface  |  Transfer Rate  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 0       |  3.3Mbytes/sec  |     SCSI-1       |    5Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 1       |  5.2Mbytes/sec  |    Fast SCSI     |   10Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 2       |  8.3Mbytes/sec  |  Fast-Wide SCSI  |   20Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 3       | 11.1Mbytes/sec  |    Ultra SCSI    |   20Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 4       | 16.6Mbytes/sec  |  Ultra Wide SCSI |   40Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 0 |  2.1Mbytes/sec  |    Ultra2 SCSI   |   40Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 1 |  4.2Mbytes/sec  | Ultra2 Wide SCSI |   80Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 2 |  8.4Mbytes/sec  |    Ultra3 SCSI   |  160Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Multi-word DMA Mode 0  |  4.2Mbytes/sec  |  Ultra-320 SCSI  |  320Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Multi-word DMA Mode 1  | 13.3Mbytes/sec  |  Ultra-640 SCSI  |  640Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    //
    // As ScsiDisk and ScsiBus driver are used to manage SCSI or ATAPI devices, we have to use
    // the lowest transfer rate to calculate the possible maximum timeout value for each operation.
    // From the above table, we could know 2.1Mbytes per second is lowest one.
    // The timeout value is rounded up to nearest integer and here an additional 30s is added
    // to follow ATA spec in which it mentioned that the device may take up to 30s to respond
    // commands in the Standby/Idle mode.
    //
    Timeout = EFI_TIMER_PERIOD_SECONDS (ByteCount / 2100000 + 31);

    MaxRetry = 2;
    for (Index = 0; Index < MaxRetry; Index++) {
      if (!ScsiDiskDevice->Cdb16Byte) {
        Status = ScsiDiskRead10 (
                   ScsiDiskDevice,
                   &NeedRetry,
                   Timeout,
                   PtrBuffer,
                   &ByteCount,
                   (UINT32)Lba,
                   SectorCount
                   );
      } else {
        Status = ScsiDiskRead16 (
                   ScsiDiskDevice,
                   &NeedRetry,
                   Timeout,
                   PtrBuffer,
                   &ByteCount,
                   Lba,
                   SectorCount
                   );
      }

      if (!EFI_ERROR (Status)) {
        break;
      }

      if (!NeedRetry) {
        return EFI_DEVICE_ERROR;
      }

      //
      // We need to retry. However, if ScsiDiskRead10() or ScsiDiskRead16() has
      // lowered ByteCount on output, we must make sure that we lower
      // SectorCount accordingly. SectorCount will be encoded in the CDB, and
      // it is invalid to request more sectors in the CDB than the entire
      // transfer (ie. ByteCount) can carry.
      //
      // In addition, ByteCount is only expected to go down, or stay unchanged.
      // Therefore we don't need to update Timeout: the original timeout should
      // accommodate shorter transfers too.
      //
      NextSectorCount = ByteCount / BlockSize;
      if (NextSectorCount < SectorCount) {
        SectorCount = NextSectorCount;
        //
        // Account for any rounding down.
        //
        ByteCount = SectorCount * BlockSize;
      }
    }

    if ((Index == MaxRetry) && (Status != EFI_SUCCESS)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // actual transferred sectors
    //
    SectorCount = ByteCount / BlockSize;

    Lba             += SectorCount;
    PtrBuffer        = PtrBuffer + SectorCount * BlockSize;
    BlocksRemaining -= SectorCount;
  }

  return EFI_SUCCESS;
}

/**
  Write sector to SCSI Disk.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV
  @param  Buffer          The buffer of data to be written into SCSI Disk
  @param  Lba             Logic block address
  @param  NumberOfBlocks  The number of blocks to read

  @retval EFI_DEVICE_ERROR  Indicates a device error.
  @retval EFI_SUCCESS       Operation is successful.

**/
EFI_STATUS
ScsiDiskWriteSectors (
  IN  SCSI_DISK_DEV  *ScsiDiskDevice,
  IN  VOID           *Buffer,
  IN  EFI_LBA        Lba,
  IN  UINTN          NumberOfBlocks
  )
{
  UINTN       BlocksRemaining;
  UINT8       *PtrBuffer;
  UINT32      BlockSize;
  UINT32      ByteCount;
  UINT32      MaxBlock;
  UINT32      SectorCount;
  UINT32      NextSectorCount;
  UINT64      Timeout;
  EFI_STATUS  Status;
  UINT8       Index;
  UINT8       MaxRetry;
  BOOLEAN     NeedRetry;

  Status = EFI_SUCCESS;

  BlocksRemaining = NumberOfBlocks;
  BlockSize       = ScsiDiskDevice->BlkIo.Media->BlockSize;

  //
  // limit the data bytes that can be transferred by one Read(10) or Read(16) Command
  //
  if (!ScsiDiskDevice->Cdb16Byte) {
    MaxBlock = 0xFFFF;
  } else {
    MaxBlock = 0xFFFFFFFF;
  }

  PtrBuffer = Buffer;

  while (BlocksRemaining > 0) {
    if (BlocksRemaining <= MaxBlock) {
      if (!ScsiDiskDevice->Cdb16Byte) {
        SectorCount = (UINT16)BlocksRemaining;
      } else {
        SectorCount = (UINT32)BlocksRemaining;
      }
    } else {
      SectorCount = MaxBlock;
    }

    ByteCount = SectorCount * BlockSize;
    //
    // |------------------------|-----------------|------------------|-----------------|
    // |   ATA Transfer Mode    |  Transfer Rate  |  SCSI Interface  |  Transfer Rate  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 0       |  3.3Mbytes/sec  |     SCSI-1       |    5Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 1       |  5.2Mbytes/sec  |    Fast SCSI     |   10Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 2       |  8.3Mbytes/sec  |  Fast-Wide SCSI  |   20Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 3       | 11.1Mbytes/sec  |    Ultra SCSI    |   20Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 4       | 16.6Mbytes/sec  |  Ultra Wide SCSI |   40Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 0 |  2.1Mbytes/sec  |    Ultra2 SCSI   |   40Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 1 |  4.2Mbytes/sec  | Ultra2 Wide SCSI |   80Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 2 |  8.4Mbytes/sec  |    Ultra3 SCSI   |  160Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Multi-word DMA Mode 0  |  4.2Mbytes/sec  |  Ultra-320 SCSI  |  320Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Multi-word DMA Mode 1  | 13.3Mbytes/sec  |  Ultra-640 SCSI  |  640Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    //
    // As ScsiDisk and ScsiBus driver are used to manage SCSI or ATAPI devices, we have to use
    // the lowest transfer rate to calculate the possible maximum timeout value for each operation.
    // From the above table, we could know 2.1Mbytes per second is lowest one.
    // The timeout value is rounded up to nearest integer and here an additional 30s is added
    // to follow ATA spec in which it mentioned that the device may take up to 30s to respond
    // commands in the Standby/Idle mode.
    //
    Timeout  = EFI_TIMER_PERIOD_SECONDS (ByteCount / 2100000 + 31);
    MaxRetry = 2;
    for (Index = 0; Index < MaxRetry; Index++) {
      if (!ScsiDiskDevice->Cdb16Byte) {
        Status = ScsiDiskWrite10 (
                   ScsiDiskDevice,
                   &NeedRetry,
                   Timeout,
                   PtrBuffer,
                   &ByteCount,
                   (UINT32)Lba,
                   SectorCount
                   );
      } else {
        Status = ScsiDiskWrite16 (
                   ScsiDiskDevice,
                   &NeedRetry,
                   Timeout,
                   PtrBuffer,
                   &ByteCount,
                   Lba,
                   SectorCount
                   );
      }

      if (!EFI_ERROR (Status)) {
        break;
      }

      if (!NeedRetry) {
        return EFI_DEVICE_ERROR;
      }

      //
      // We need to retry. However, if ScsiDiskWrite10() or ScsiDiskWrite16()
      // has lowered ByteCount on output, we must make sure that we lower
      // SectorCount accordingly. SectorCount will be encoded in the CDB, and
      // it is invalid to request more sectors in the CDB than the entire
      // transfer (ie. ByteCount) can carry.
      //
      // In addition, ByteCount is only expected to go down, or stay unchanged.
      // Therefore we don't need to update Timeout: the original timeout should
      // accommodate shorter transfers too.
      //
      NextSectorCount = ByteCount / BlockSize;
      if (NextSectorCount < SectorCount) {
        SectorCount = NextSectorCount;
        //
        // Account for any rounding down.
        //
        ByteCount = SectorCount * BlockSize;
      }
    }

    if ((Index == MaxRetry) && (Status != EFI_SUCCESS)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // actual transferred sectors
    //
    SectorCount = ByteCount / BlockSize;

    Lba             += SectorCount;
    PtrBuffer        = PtrBuffer + SectorCount * BlockSize;
    BlocksRemaining -= SectorCount;
  }

  return EFI_SUCCESS;
}

/**
  Asynchronously read sector from SCSI Disk.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV.
  @param  Buffer          The buffer to fill in the read out data.
  @param  Lba             Logic block address.
  @param  NumberOfBlocks  The number of blocks to read.
  @param  Token           A pointer to the token associated with the
                          non-blocking read request.

  @retval EFI_INVALID_PARAMETER  Token is NULL or Token->Event is NULL.
  @retval EFI_DEVICE_ERROR       Indicates a device error.
  @retval EFI_SUCCESS            Operation is successful.

**/
EFI_STATUS
ScsiDiskAsyncReadSectors (
  IN   SCSI_DISK_DEV        *ScsiDiskDevice,
  OUT  VOID                 *Buffer,
  IN   EFI_LBA              Lba,
  IN   UINTN                NumberOfBlocks,
  IN   EFI_BLOCK_IO2_TOKEN  *Token
  )
{
  UINTN                BlocksRemaining;
  UINT8                *PtrBuffer;
  UINT32               BlockSize;
  UINT32               ByteCount;
  UINT32               MaxBlock;
  UINT32               SectorCount;
  UINT64               Timeout;
  SCSI_BLKIO2_REQUEST  *BlkIo2Req;
  EFI_STATUS           Status;
  EFI_TPL              OldTpl;

  if ((Token == NULL) || (Token->Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BlkIo2Req = AllocateZeroPool (sizeof (SCSI_BLKIO2_REQUEST));
  if (BlkIo2Req == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BlkIo2Req->Token = Token;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&ScsiDiskDevice->AsyncTaskQueue, &BlkIo2Req->Link);
  gBS->RestoreTPL (OldTpl);

  InitializeListHead (&BlkIo2Req->ScsiRWQueue);

  Status = EFI_SUCCESS;

  BlocksRemaining = NumberOfBlocks;
  BlockSize       = ScsiDiskDevice->BlkIo.Media->BlockSize;

  //
  // Limit the data bytes that can be transferred by one Read(10) or Read(16)
  // Command
  //
  if (!ScsiDiskDevice->Cdb16Byte) {
    MaxBlock = 0xFFFF;
  } else {
    MaxBlock = 0xFFFFFFFF;
  }

  PtrBuffer = Buffer;

  while (BlocksRemaining > 0) {
    if (BlocksRemaining <= MaxBlock) {
      if (!ScsiDiskDevice->Cdb16Byte) {
        SectorCount = (UINT16)BlocksRemaining;
      } else {
        SectorCount = (UINT32)BlocksRemaining;
      }
    } else {
      SectorCount = MaxBlock;
    }

    ByteCount = SectorCount * BlockSize;
    //
    // |------------------------|-----------------|------------------|-----------------|
    // |   ATA Transfer Mode    |  Transfer Rate  |  SCSI Interface  |  Transfer Rate  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 0       |  3.3Mbytes/sec  |     SCSI-1       |    5Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 1       |  5.2Mbytes/sec  |    Fast SCSI     |   10Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 2       |  8.3Mbytes/sec  |  Fast-Wide SCSI  |   20Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 3       | 11.1Mbytes/sec  |    Ultra SCSI    |   20Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 4       | 16.6Mbytes/sec  |  Ultra Wide SCSI |   40Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 0 |  2.1Mbytes/sec  |    Ultra2 SCSI   |   40Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 1 |  4.2Mbytes/sec  | Ultra2 Wide SCSI |   80Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 2 |  8.4Mbytes/sec  |    Ultra3 SCSI   |  160Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Multi-word DMA Mode 0  |  4.2Mbytes/sec  |  Ultra-320 SCSI  |  320Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Multi-word DMA Mode 1  | 13.3Mbytes/sec  |  Ultra-640 SCSI  |  640Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    //
    // As ScsiDisk and ScsiBus driver are used to manage SCSI or ATAPI devices,
    // we have to use the lowest transfer rate to calculate the possible
    // maximum timeout value for each operation.
    // From the above table, we could know 2.1Mbytes per second is lowest one.
    // The timeout value is rounded up to nearest integer and here an additional
    // 30s is added to follow ATA spec in which it mentioned that the device
    // may take up to 30s to respond commands in the Standby/Idle mode.
    //
    Timeout = EFI_TIMER_PERIOD_SECONDS (ByteCount / 2100000 + 31);

    if (!ScsiDiskDevice->Cdb16Byte) {
      Status = ScsiDiskAsyncRead10 (
                 ScsiDiskDevice,
                 Timeout,
                 0,
                 PtrBuffer,
                 ByteCount,
                 (UINT32)Lba,
                 SectorCount,
                 BlkIo2Req,
                 Token
                 );
    } else {
      Status = ScsiDiskAsyncRead16 (
                 ScsiDiskDevice,
                 Timeout,
                 0,
                 PtrBuffer,
                 ByteCount,
                 Lba,
                 SectorCount,
                 BlkIo2Req,
                 Token
                 );
    }

    if (EFI_ERROR (Status)) {
      //
      // Some devices will return EFI_DEVICE_ERROR or EFI_TIMEOUT when the data
      // length of a SCSI I/O command is too large.
      // In this case, we retry sending the SCSI command with a data length
      // half of its previous value.
      //
      if ((Status == EFI_DEVICE_ERROR) || (Status == EFI_TIMEOUT)) {
        if ((MaxBlock > 1) && (SectorCount > 1)) {
          MaxBlock = MIN (MaxBlock, SectorCount) >> 1;
          continue;
        }
      }

      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      if (IsListEmpty (&BlkIo2Req->ScsiRWQueue)) {
        //
        // Free the SCSI_BLKIO2_REQUEST structure only when there is no other
        // SCSI sub-task running. Otherwise, it will be freed in the callback
        // function ScsiDiskNotify().
        //
        RemoveEntryList (&BlkIo2Req->Link);
        FreePool (BlkIo2Req);
        BlkIo2Req = NULL;
        gBS->RestoreTPL (OldTpl);

        //
        // It is safe to return error status to the caller, since there is no
        // previous SCSI sub-task executing.
        //
        Status = EFI_DEVICE_ERROR;
        goto Done;
      } else {
        gBS->RestoreTPL (OldTpl);

        //
        // There are previous SCSI commands still running, EFI_SUCCESS should
        // be returned to make sure that the caller does not free resources
        // still using by these SCSI commands.
        //
        Status = EFI_SUCCESS;
        goto Done;
      }
    }

    //
    // Sectors submitted for transfer
    //
    SectorCount = ByteCount / BlockSize;

    Lba             += SectorCount;
    PtrBuffer        = PtrBuffer + SectorCount * BlockSize;
    BlocksRemaining -= SectorCount;
  }

  Status = EFI_SUCCESS;

Done:
  if (BlkIo2Req != NULL) {
    BlkIo2Req->LastScsiRW = TRUE;

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    if (IsListEmpty (&BlkIo2Req->ScsiRWQueue)) {
      RemoveEntryList (&BlkIo2Req->Link);
      FreePool (BlkIo2Req);
      BlkIo2Req = NULL;

      gBS->SignalEvent (Token->Event);
    }

    gBS->RestoreTPL (OldTpl);
  }

  return Status;
}

/**
  Asynchronously write sector to SCSI Disk.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV.
  @param  Buffer          The buffer of data to be written into SCSI Disk.
  @param  Lba             Logic block address.
  @param  NumberOfBlocks  The number of blocks to read.
  @param  Token           A pointer to the token associated with the
                          non-blocking read request.

  @retval EFI_INVALID_PARAMETER  Token is NULL or Token->Event is NULL
  @retval EFI_DEVICE_ERROR  Indicates a device error.
  @retval EFI_SUCCESS       Operation is successful.

**/
EFI_STATUS
ScsiDiskAsyncWriteSectors (
  IN  SCSI_DISK_DEV        *ScsiDiskDevice,
  IN  VOID                 *Buffer,
  IN  EFI_LBA              Lba,
  IN  UINTN                NumberOfBlocks,
  IN  EFI_BLOCK_IO2_TOKEN  *Token
  )
{
  UINTN                BlocksRemaining;
  UINT8                *PtrBuffer;
  UINT32               BlockSize;
  UINT32               ByteCount;
  UINT32               MaxBlock;
  UINT32               SectorCount;
  UINT64               Timeout;
  SCSI_BLKIO2_REQUEST  *BlkIo2Req;
  EFI_STATUS           Status;
  EFI_TPL              OldTpl;

  if ((Token == NULL) || (Token->Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BlkIo2Req = AllocateZeroPool (sizeof (SCSI_BLKIO2_REQUEST));
  if (BlkIo2Req == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BlkIo2Req->Token = Token;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&ScsiDiskDevice->AsyncTaskQueue, &BlkIo2Req->Link);
  gBS->RestoreTPL (OldTpl);

  InitializeListHead (&BlkIo2Req->ScsiRWQueue);

  Status = EFI_SUCCESS;

  BlocksRemaining = NumberOfBlocks;
  BlockSize       = ScsiDiskDevice->BlkIo.Media->BlockSize;

  //
  // Limit the data bytes that can be transferred by one Read(10) or Read(16)
  // Command
  //
  if (!ScsiDiskDevice->Cdb16Byte) {
    MaxBlock = 0xFFFF;
  } else {
    MaxBlock = 0xFFFFFFFF;
  }

  PtrBuffer = Buffer;

  while (BlocksRemaining > 0) {
    if (BlocksRemaining <= MaxBlock) {
      if (!ScsiDiskDevice->Cdb16Byte) {
        SectorCount = (UINT16)BlocksRemaining;
      } else {
        SectorCount = (UINT32)BlocksRemaining;
      }
    } else {
      SectorCount = MaxBlock;
    }

    ByteCount = SectorCount * BlockSize;
    //
    // |------------------------|-----------------|------------------|-----------------|
    // |   ATA Transfer Mode    |  Transfer Rate  |  SCSI Interface  |  Transfer Rate  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 0       |  3.3Mbytes/sec  |     SCSI-1       |    5Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 1       |  5.2Mbytes/sec  |    Fast SCSI     |   10Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 2       |  8.3Mbytes/sec  |  Fast-Wide SCSI  |   20Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 3       | 11.1Mbytes/sec  |    Ultra SCSI    |   20Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // |       PIO Mode 4       | 16.6Mbytes/sec  |  Ultra Wide SCSI |   40Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 0 |  2.1Mbytes/sec  |    Ultra2 SCSI   |   40Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 1 |  4.2Mbytes/sec  | Ultra2 Wide SCSI |   80Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Single-word DMA Mode 2 |  8.4Mbytes/sec  |    Ultra3 SCSI   |  160Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Multi-word DMA Mode 0  |  4.2Mbytes/sec  |  Ultra-320 SCSI  |  320Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    // | Multi-word DMA Mode 1  | 13.3Mbytes/sec  |  Ultra-640 SCSI  |  640Mbytes/sec  |
    // |------------------------|-----------------|------------------|-----------------|
    //
    // As ScsiDisk and ScsiBus driver are used to manage SCSI or ATAPI devices,
    // we have to use the lowest transfer rate to calculate the possible
    // maximum timeout value for each operation.
    // From the above table, we could know 2.1Mbytes per second is lowest one.
    // The timeout value is rounded up to nearest integer and here an additional
    // 30s is added to follow ATA spec in which it mentioned that the device
    // may take up to 30s to respond commands in the Standby/Idle mode.
    //
    Timeout = EFI_TIMER_PERIOD_SECONDS (ByteCount / 2100000 + 31);

    if (!ScsiDiskDevice->Cdb16Byte) {
      Status = ScsiDiskAsyncWrite10 (
                 ScsiDiskDevice,
                 Timeout,
                 0,
                 PtrBuffer,
                 ByteCount,
                 (UINT32)Lba,
                 SectorCount,
                 BlkIo2Req,
                 Token
                 );
    } else {
      Status = ScsiDiskAsyncWrite16 (
                 ScsiDiskDevice,
                 Timeout,
                 0,
                 PtrBuffer,
                 ByteCount,
                 Lba,
                 SectorCount,
                 BlkIo2Req,
                 Token
                 );
    }

    if (EFI_ERROR (Status)) {
      //
      // Some devices will return EFI_DEVICE_ERROR or EFI_TIMEOUT when the data
      // length of a SCSI I/O command is too large.
      // In this case, we retry sending the SCSI command with a data length
      // half of its previous value.
      //
      if ((Status == EFI_DEVICE_ERROR) || (Status == EFI_TIMEOUT)) {
        if ((MaxBlock > 1) && (SectorCount > 1)) {
          MaxBlock = MIN (MaxBlock, SectorCount) >> 1;
          continue;
        }
      }

      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      if (IsListEmpty (&BlkIo2Req->ScsiRWQueue)) {
        //
        // Free the SCSI_BLKIO2_REQUEST structure only when there is no other
        // SCSI sub-task running. Otherwise, it will be freed in the callback
        // function ScsiDiskNotify().
        //
        RemoveEntryList (&BlkIo2Req->Link);
        FreePool (BlkIo2Req);
        BlkIo2Req = NULL;
        gBS->RestoreTPL (OldTpl);

        //
        // It is safe to return error status to the caller, since there is no
        // previous SCSI sub-task executing.
        //
        Status = EFI_DEVICE_ERROR;
        goto Done;
      } else {
        gBS->RestoreTPL (OldTpl);

        //
        // There are previous SCSI commands still running, EFI_SUCCESS should
        // be returned to make sure that the caller does not free resources
        // still using by these SCSI commands.
        //
        Status = EFI_SUCCESS;
        goto Done;
      }
    }

    //
    // Sectors submitted for transfer
    //
    SectorCount = ByteCount / BlockSize;

    Lba             += SectorCount;
    PtrBuffer        = PtrBuffer + SectorCount * BlockSize;
    BlocksRemaining -= SectorCount;
  }

  Status = EFI_SUCCESS;

Done:
  if (BlkIo2Req != NULL) {
    BlkIo2Req->LastScsiRW = TRUE;

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    if (IsListEmpty (&BlkIo2Req->ScsiRWQueue)) {
      RemoveEntryList (&BlkIo2Req->Link);
      FreePool (BlkIo2Req);
      BlkIo2Req = NULL;

      gBS->SignalEvent (Token->Event);
    }

    gBS->RestoreTPL (OldTpl);
  }

  return Status;
}

/**
  Submit Read(10) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorCount        The number of blocks to read

  @return  EFI_STATUS is returned by calling ScsiRead10Command().
**/
EFI_STATUS
ScsiDiskRead10 (
  IN     SCSI_DISK_DEV  *ScsiDiskDevice,
  OUT BOOLEAN           *NeedRetry,
  IN     UINT64         Timeout,
  OUT UINT8             *DataBuffer,
  IN OUT UINT32         *DataLength,
  IN     UINT32         StartLba,
  IN     UINT32         SectorCount
  )
{
  UINT8       SenseDataLength;
  EFI_STATUS  Status;
  EFI_STATUS  ReturnStatus;
  UINT8       HostAdapterStatus;
  UINT8       TargetStatus;
  UINTN       Action;

  //
  // Implement a backoff algorithm to resolve some compatibility issues that
  // some SCSI targets or ATAPI devices couldn't correctly response reading/writing
  // big data in a single operation.
  // This algorithm will at first try to execute original request. If the request fails
  // with media error sense data or else, it will reduce the transfer length to half and
  // try again till the operation succeeds or fails with one sector transfer length.
  //
BackOff:
  *NeedRetry      = FALSE;
  Action          = ACTION_NO_ACTION;
  SenseDataLength = (UINT8)(ScsiDiskDevice->SenseDataNumber * sizeof (EFI_SCSI_SENSE_DATA));
  ReturnStatus    = ScsiRead10Command (
                      ScsiDiskDevice->ScsiIo,
                      Timeout,
                      ScsiDiskDevice->SenseData,
                      &SenseDataLength,
                      &HostAdapterStatus,
                      &TargetStatus,
                      DataBuffer,
                      DataLength,
                      StartLba,
                      SectorCount
                      );

  if ((ReturnStatus == EFI_NOT_READY) || (ReturnStatus == EFI_BAD_BUFFER_SIZE)) {
    *NeedRetry = TRUE;
    return EFI_DEVICE_ERROR;
  } else if ((ReturnStatus == EFI_INVALID_PARAMETER) || (ReturnStatus == EFI_UNSUPPORTED)) {
    *NeedRetry = FALSE;
    return ReturnStatus;
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

  if ((TargetStatus == EFI_EXT_SCSI_STATUS_TARGET_CHECK_CONDITION) || (EFI_ERROR (ReturnStatus))) {
    DEBUG ((DEBUG_ERROR, "ScsiDiskRead10: Check Condition happened!\n"));
    DetectMediaParsingSenseKeys (ScsiDiskDevice, ScsiDiskDevice->SenseData, SenseDataLength / sizeof (EFI_SCSI_SENSE_DATA), &Action);
    if (Action == ACTION_RETRY_COMMAND_LATER) {
      *NeedRetry = TRUE;
      return EFI_DEVICE_ERROR;
    } else if (Action == ACTION_RETRY_WITH_BACKOFF_ALGO) {
      if (SectorCount <= 1) {
        //
        // Jump out if the operation still fails with one sector transfer length.
        //
        *NeedRetry = FALSE;
        return EFI_DEVICE_ERROR;
      }

      //
      // Try again with half length if the sense data shows we need to retry.
      //
      SectorCount >>= 1;
      *DataLength   = SectorCount * ScsiDiskDevice->BlkIo.Media->BlockSize;
      goto BackOff;
    } else {
      *NeedRetry = FALSE;
      return EFI_DEVICE_ERROR;
    }
  }

  return ReturnStatus;
}

/**
  Submit Write(10) Command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorCount        The number of blocks to write

  @return  EFI_STATUS is returned by calling ScsiWrite10Command().

**/
EFI_STATUS
ScsiDiskWrite10 (
  IN     SCSI_DISK_DEV  *ScsiDiskDevice,
  OUT BOOLEAN           *NeedRetry,
  IN     UINT64         Timeout,
  IN     UINT8          *DataBuffer,
  IN OUT UINT32         *DataLength,
  IN     UINT32         StartLba,
  IN     UINT32         SectorCount
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  ReturnStatus;
  UINT8       SenseDataLength;
  UINT8       HostAdapterStatus;
  UINT8       TargetStatus;
  UINTN       Action;

  //
  // Implement a backoff algorithm to resolve some compatibility issues that
  // some SCSI targets or ATAPI devices couldn't correctly response reading/writing
  // big data in a single operation.
  // This algorithm will at first try to execute original request. If the request fails
  // with media error sense data or else, it will reduce the transfer length to half and
  // try again till the operation succeeds or fails with one sector transfer length.
  //
BackOff:
  *NeedRetry      = FALSE;
  Action          = ACTION_NO_ACTION;
  SenseDataLength = (UINT8)(ScsiDiskDevice->SenseDataNumber * sizeof (EFI_SCSI_SENSE_DATA));
  ReturnStatus    = ScsiWrite10Command (
                      ScsiDiskDevice->ScsiIo,
                      Timeout,
                      ScsiDiskDevice->SenseData,
                      &SenseDataLength,
                      &HostAdapterStatus,
                      &TargetStatus,
                      DataBuffer,
                      DataLength,
                      StartLba,
                      SectorCount
                      );
  if ((ReturnStatus == EFI_NOT_READY) || (ReturnStatus == EFI_BAD_BUFFER_SIZE)) {
    *NeedRetry = TRUE;
    return EFI_DEVICE_ERROR;
  } else if ((ReturnStatus == EFI_INVALID_PARAMETER) || (ReturnStatus == EFI_UNSUPPORTED)) {
    *NeedRetry = FALSE;
    return ReturnStatus;
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

  if ((TargetStatus == EFI_EXT_SCSI_STATUS_TARGET_CHECK_CONDITION) || (EFI_ERROR (ReturnStatus))) {
    DEBUG ((DEBUG_ERROR, "ScsiDiskWrite10: Check Condition happened!\n"));
    DetectMediaParsingSenseKeys (ScsiDiskDevice, ScsiDiskDevice->SenseData, SenseDataLength / sizeof (EFI_SCSI_SENSE_DATA), &Action);
    if (Action == ACTION_RETRY_COMMAND_LATER) {
      *NeedRetry = TRUE;
      return EFI_DEVICE_ERROR;
    } else if (Action == ACTION_RETRY_WITH_BACKOFF_ALGO) {
      if (SectorCount <= 1) {
        //
        // Jump out if the operation still fails with one sector transfer length.
        //
        *NeedRetry = FALSE;
        return EFI_DEVICE_ERROR;
      }

      //
      // Try again with half length if the sense data shows we need to retry.
      //
      SectorCount >>= 1;
      *DataLength   = SectorCount * ScsiDiskDevice->BlkIo.Media->BlockSize;
      goto BackOff;
    } else {
      *NeedRetry = FALSE;
      return EFI_DEVICE_ERROR;
    }
  }

  return ReturnStatus;
}

/**
  Submit Read(16) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorCount        The number of blocks to read

  @return  EFI_STATUS is returned by calling ScsiRead16Command().
**/
EFI_STATUS
ScsiDiskRead16 (
  IN     SCSI_DISK_DEV  *ScsiDiskDevice,
  OUT BOOLEAN           *NeedRetry,
  IN     UINT64         Timeout,
  OUT UINT8             *DataBuffer,
  IN OUT UINT32         *DataLength,
  IN     UINT64         StartLba,
  IN     UINT32         SectorCount
  )
{
  UINT8       SenseDataLength;
  EFI_STATUS  Status;
  EFI_STATUS  ReturnStatus;
  UINT8       HostAdapterStatus;
  UINT8       TargetStatus;
  UINTN       Action;

  //
  // Implement a backoff algorithm to resolve some compatibility issues that
  // some SCSI targets or ATAPI devices couldn't correctly response reading/writing
  // big data in a single operation.
  // This algorithm will at first try to execute original request. If the request fails
  // with media error sense data or else, it will reduce the transfer length to half and
  // try again till the operation succeeds or fails with one sector transfer length.
  //
BackOff:
  *NeedRetry      = FALSE;
  Action          = ACTION_NO_ACTION;
  SenseDataLength = (UINT8)(ScsiDiskDevice->SenseDataNumber * sizeof (EFI_SCSI_SENSE_DATA));
  ReturnStatus    = ScsiRead16Command (
                      ScsiDiskDevice->ScsiIo,
                      Timeout,
                      ScsiDiskDevice->SenseData,
                      &SenseDataLength,
                      &HostAdapterStatus,
                      &TargetStatus,
                      DataBuffer,
                      DataLength,
                      StartLba,
                      SectorCount
                      );
  if ((ReturnStatus == EFI_NOT_READY) || (ReturnStatus == EFI_BAD_BUFFER_SIZE)) {
    *NeedRetry = TRUE;
    return EFI_DEVICE_ERROR;
  } else if ((ReturnStatus == EFI_INVALID_PARAMETER) || (ReturnStatus == EFI_UNSUPPORTED)) {
    *NeedRetry = FALSE;
    return ReturnStatus;
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

  if ((TargetStatus == EFI_EXT_SCSI_STATUS_TARGET_CHECK_CONDITION) || (EFI_ERROR (ReturnStatus))) {
    DEBUG ((DEBUG_ERROR, "ScsiDiskRead16: Check Condition happened!\n"));
    DetectMediaParsingSenseKeys (ScsiDiskDevice, ScsiDiskDevice->SenseData, SenseDataLength / sizeof (EFI_SCSI_SENSE_DATA), &Action);
    if (Action == ACTION_RETRY_COMMAND_LATER) {
      *NeedRetry = TRUE;
      return EFI_DEVICE_ERROR;
    } else if (Action == ACTION_RETRY_WITH_BACKOFF_ALGO) {
      if (SectorCount <= 1) {
        //
        // Jump out if the operation still fails with one sector transfer length.
        //
        *NeedRetry = FALSE;
        return EFI_DEVICE_ERROR;
      }

      //
      // Try again with half length if the sense data shows we need to retry.
      //
      SectorCount >>= 1;
      *DataLength   = SectorCount * ScsiDiskDevice->BlkIo.Media->BlockSize;
      goto BackOff;
    } else {
      *NeedRetry = FALSE;
      return EFI_DEVICE_ERROR;
    }
  }

  return ReturnStatus;
}

/**
  Submit Write(16) Command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorCount        The number of blocks to write

  @return  EFI_STATUS is returned by calling ScsiWrite16Command().

**/
EFI_STATUS
ScsiDiskWrite16 (
  IN     SCSI_DISK_DEV  *ScsiDiskDevice,
  OUT BOOLEAN           *NeedRetry,
  IN     UINT64         Timeout,
  IN     UINT8          *DataBuffer,
  IN OUT UINT32         *DataLength,
  IN     UINT64         StartLba,
  IN     UINT32         SectorCount
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  ReturnStatus;
  UINT8       SenseDataLength;
  UINT8       HostAdapterStatus;
  UINT8       TargetStatus;
  UINTN       Action;

  //
  // Implement a backoff algorithm to resolve some compatibility issues that
  // some SCSI targets or ATAPI devices couldn't correctly response reading/writing
  // big data in a single operation.
  // This algorithm will at first try to execute original request. If the request fails
  // with media error sense data or else, it will reduce the transfer length to half and
  // try again till the operation succeeds or fails with one sector transfer length.
  //
BackOff:
  *NeedRetry      = FALSE;
  Action          = ACTION_NO_ACTION;
  SenseDataLength = (UINT8)(ScsiDiskDevice->SenseDataNumber * sizeof (EFI_SCSI_SENSE_DATA));
  ReturnStatus    = ScsiWrite16Command (
                      ScsiDiskDevice->ScsiIo,
                      Timeout,
                      ScsiDiskDevice->SenseData,
                      &SenseDataLength,
                      &HostAdapterStatus,
                      &TargetStatus,
                      DataBuffer,
                      DataLength,
                      StartLba,
                      SectorCount
                      );
  if ((ReturnStatus == EFI_NOT_READY) || (ReturnStatus == EFI_BAD_BUFFER_SIZE)) {
    *NeedRetry = TRUE;
    return EFI_DEVICE_ERROR;
  } else if ((ReturnStatus == EFI_INVALID_PARAMETER) || (ReturnStatus == EFI_UNSUPPORTED)) {
    *NeedRetry = FALSE;
    return ReturnStatus;
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

  if ((TargetStatus == EFI_EXT_SCSI_STATUS_TARGET_CHECK_CONDITION) || (EFI_ERROR (ReturnStatus))) {
    DEBUG ((DEBUG_ERROR, "ScsiDiskWrite16: Check Condition happened!\n"));
    DetectMediaParsingSenseKeys (ScsiDiskDevice, ScsiDiskDevice->SenseData, SenseDataLength / sizeof (EFI_SCSI_SENSE_DATA), &Action);
    if (Action == ACTION_RETRY_COMMAND_LATER) {
      *NeedRetry = TRUE;
      return EFI_DEVICE_ERROR;
    } else if (Action == ACTION_RETRY_WITH_BACKOFF_ALGO) {
      if (SectorCount <= 1) {
        //
        // Jump out if the operation still fails with one sector transfer length.
        //
        *NeedRetry = FALSE;
        return EFI_DEVICE_ERROR;
      }

      //
      // Try again with half length if the sense data shows we need to retry.
      //
      SectorCount >>= 1;
      *DataLength   = SectorCount * ScsiDiskDevice->BlkIo.Media->BlockSize;
      goto BackOff;
    } else {
      *NeedRetry = FALSE;
      return EFI_DEVICE_ERROR;
    }
  }

  return ReturnStatus;
}

/**
  Internal helper notify function in which determine whether retry of a SCSI
  Read/Write command is needed and signal the event passed from Block I/O(2) if
  the SCSI I/O operation completes.

  @param  Event    The instance of EFI_EVENT.
  @param  Context  The parameter passed in.

**/
VOID
EFIAPI
ScsiDiskNotify (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS             Status;
  SCSI_ASYNC_RW_REQUEST  *Request;
  SCSI_DISK_DEV          *ScsiDiskDevice;
  EFI_BLOCK_IO2_TOKEN    *Token;
  UINTN                  Action;
  UINT32                 OldDataLength;
  UINT32                 OldSectorCount;
  UINT8                  MaxRetry;

  gBS->CloseEvent (Event);

  Request        = (SCSI_ASYNC_RW_REQUEST *)Context;
  ScsiDiskDevice = Request->ScsiDiskDevice;
  Token          = Request->BlkIo2Req->Token;
  OldDataLength  = Request->DataLength;
  OldSectorCount = Request->SectorCount;
  MaxRetry       = 2;

  //
  // If previous sub-tasks already fails, no need to process this sub-task.
  //
  if (Token->TransactionStatus != EFI_SUCCESS) {
    goto Exit;
  }

  //
  // Check HostAdapterStatus and TargetStatus
  // (EFI_TIMEOUT, EFI_DEVICE_ERROR, EFI_WARN_BUFFER_TOO_SMALL)
  //
  Status = CheckHostAdapterStatus (Request->HostAdapterStatus);
  if ((Status == EFI_TIMEOUT) || (Status == EFI_NOT_READY)) {
    if (++Request->TimesRetry > MaxRetry) {
      Token->TransactionStatus = EFI_DEVICE_ERROR;
      goto Exit;
    } else {
      goto Retry;
    }
  } else if (Status == EFI_DEVICE_ERROR) {
    //
    // reset the scsi channel
    //
    ScsiDiskDevice->ScsiIo->ResetBus (ScsiDiskDevice->ScsiIo);
    Token->TransactionStatus = EFI_DEVICE_ERROR;
    goto Exit;
  }

  Status = CheckTargetStatus (Request->TargetStatus);
  if (Status == EFI_NOT_READY) {
    //
    // reset the scsi device
    //
    ScsiDiskDevice->ScsiIo->ResetDevice (ScsiDiskDevice->ScsiIo);
    if (++Request->TimesRetry > MaxRetry) {
      Token->TransactionStatus = EFI_DEVICE_ERROR;
      goto Exit;
    } else {
      goto Retry;
    }
  } else if (Status == EFI_DEVICE_ERROR) {
    Token->TransactionStatus = EFI_DEVICE_ERROR;
    goto Exit;
  }

  if (Request->TargetStatus == EFI_EXT_SCSI_STATUS_TARGET_CHECK_CONDITION) {
    DEBUG ((DEBUG_ERROR, "ScsiDiskNotify: Check Condition happened!\n"));

    DetectMediaParsingSenseKeys (
      ScsiDiskDevice,
      Request->SenseData,
      Request->SenseDataLength / sizeof (EFI_SCSI_SENSE_DATA),
      &Action
      );
    if (Action == ACTION_RETRY_COMMAND_LATER) {
      if (++Request->TimesRetry > MaxRetry) {
        Token->TransactionStatus = EFI_DEVICE_ERROR;
        goto Exit;
      } else {
        goto Retry;
      }
    } else if (Action == ACTION_RETRY_WITH_BACKOFF_ALGO) {
      if (Request->SectorCount <= 1) {
        //
        // Jump out if the operation still fails with one sector transfer
        // length.
        //
        Token->TransactionStatus = EFI_DEVICE_ERROR;
        goto Exit;
      }

      //
      // Try again with two half length request if the sense data shows we need
      // to retry.
      //
      Request->SectorCount >>= 1;
      Request->DataLength    = Request->SectorCount * ScsiDiskDevice->BlkIo.Media->BlockSize;
      Request->TimesRetry    = 0;

      goto Retry;
    } else {
      Token->TransactionStatus = EFI_DEVICE_ERROR;
      goto Exit;
    }
  }

  //
  // This sub-task succeeds, no need to retry.
  //
  goto Exit;

Retry:
  if (Request->InBuffer != NULL) {
    //
    // SCSI read command
    //
    if (!ScsiDiskDevice->Cdb16Byte) {
      Status = ScsiDiskAsyncRead10 (
                 ScsiDiskDevice,
                 Request->Timeout,
                 Request->TimesRetry,
                 Request->InBuffer,
                 Request->DataLength,
                 (UINT32)Request->StartLba,
                 Request->SectorCount,
                 Request->BlkIo2Req,
                 Token
                 );
    } else {
      Status = ScsiDiskAsyncRead16 (
                 ScsiDiskDevice,
                 Request->Timeout,
                 Request->TimesRetry,
                 Request->InBuffer,
                 Request->DataLength,
                 Request->StartLba,
                 Request->SectorCount,
                 Request->BlkIo2Req,
                 Token
                 );
    }

    if (EFI_ERROR (Status)) {
      Token->TransactionStatus = EFI_DEVICE_ERROR;
      goto Exit;
    } else if (OldSectorCount != Request->SectorCount) {
      //
      // Original sub-task will be split into two new sub-tasks with smaller
      // DataLength
      //
      if (!ScsiDiskDevice->Cdb16Byte) {
        Status = ScsiDiskAsyncRead10 (
                   ScsiDiskDevice,
                   Request->Timeout,
                   0,
                   Request->InBuffer + Request->SectorCount * ScsiDiskDevice->BlkIo.Media->BlockSize,
                   OldDataLength - Request->DataLength,
                   (UINT32)Request->StartLba + Request->SectorCount,
                   OldSectorCount - Request->SectorCount,
                   Request->BlkIo2Req,
                   Token
                   );
      } else {
        Status = ScsiDiskAsyncRead16 (
                   ScsiDiskDevice,
                   Request->Timeout,
                   0,
                   Request->InBuffer + Request->SectorCount * ScsiDiskDevice->BlkIo.Media->BlockSize,
                   OldDataLength - Request->DataLength,
                   Request->StartLba + Request->SectorCount,
                   OldSectorCount - Request->SectorCount,
                   Request->BlkIo2Req,
                   Token
                   );
      }

      if (EFI_ERROR (Status)) {
        Token->TransactionStatus = EFI_DEVICE_ERROR;
        goto Exit;
      }
    }
  } else {
    //
    // SCSI write command
    //
    if (!ScsiDiskDevice->Cdb16Byte) {
      Status = ScsiDiskAsyncWrite10 (
                 ScsiDiskDevice,
                 Request->Timeout,
                 Request->TimesRetry,
                 Request->OutBuffer,
                 Request->DataLength,
                 (UINT32)Request->StartLba,
                 Request->SectorCount,
                 Request->BlkIo2Req,
                 Token
                 );
    } else {
      Status = ScsiDiskAsyncWrite16 (
                 ScsiDiskDevice,
                 Request->Timeout,
                 Request->TimesRetry,
                 Request->OutBuffer,
                 Request->DataLength,
                 Request->StartLba,
                 Request->SectorCount,
                 Request->BlkIo2Req,
                 Token
                 );
    }

    if (EFI_ERROR (Status)) {
      Token->TransactionStatus = EFI_DEVICE_ERROR;
      goto Exit;
    } else if (OldSectorCount != Request->SectorCount) {
      //
      // Original sub-task will be split into two new sub-tasks with smaller
      // DataLength
      //
      if (!ScsiDiskDevice->Cdb16Byte) {
        Status = ScsiDiskAsyncWrite10 (
                   ScsiDiskDevice,
                   Request->Timeout,
                   0,
                   Request->OutBuffer + Request->SectorCount * ScsiDiskDevice->BlkIo.Media->BlockSize,
                   OldDataLength - Request->DataLength,
                   (UINT32)Request->StartLba + Request->SectorCount,
                   OldSectorCount - Request->SectorCount,
                   Request->BlkIo2Req,
                   Token
                   );
      } else {
        Status = ScsiDiskAsyncWrite16 (
                   ScsiDiskDevice,
                   Request->Timeout,
                   0,
                   Request->OutBuffer + Request->SectorCount * ScsiDiskDevice->BlkIo.Media->BlockSize,
                   OldDataLength - Request->DataLength,
                   Request->StartLba + Request->SectorCount,
                   OldSectorCount - Request->SectorCount,
                   Request->BlkIo2Req,
                   Token
                   );
      }

      if (EFI_ERROR (Status)) {
        Token->TransactionStatus = EFI_DEVICE_ERROR;
        goto Exit;
      }
    }
  }

Exit:
  RemoveEntryList (&Request->Link);
  if ((IsListEmpty (&Request->BlkIo2Req->ScsiRWQueue)) &&
      (Request->BlkIo2Req->LastScsiRW))
  {
    //
    // The last SCSI R/W command of a BlockIo2 request completes
    //
    RemoveEntryList (&Request->BlkIo2Req->Link);
    FreePool (Request->BlkIo2Req);  // Should be freed only once
    gBS->SignalEvent (Token->Event);
  }

  FreePool (Request->SenseData);
  FreePool (Request);
}

/**
  Submit Async Read(10) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice.
  @param  Timeout            The time to complete the command.
  @param  TimesRetry         The number of times the command has been retried.
  @param  DataBuffer         The buffer to fill with the read out data.
  @param  DataLength         The length of buffer.
  @param  StartLba           The start logic block address.
  @param  SectorCount        The number of blocks to read.
  @param  BlkIo2Req          The upstream BlockIo2 request.
  @param  Token              The pointer to the token associated with the
                             non-blocking read request.

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @return others                Status returned by calling
                                ScsiRead10CommandEx().

**/
EFI_STATUS
ScsiDiskAsyncRead10 (
  IN     SCSI_DISK_DEV        *ScsiDiskDevice,
  IN     UINT64               Timeout,
  IN     UINT8                TimesRetry,
  OUT UINT8                   *DataBuffer,
  IN     UINT32               DataLength,
  IN     UINT32               StartLba,
  IN     UINT32               SectorCount,
  IN OUT SCSI_BLKIO2_REQUEST  *BlkIo2Req,
  IN     EFI_BLOCK_IO2_TOKEN  *Token
  )
{
  EFI_STATUS             Status;
  SCSI_ASYNC_RW_REQUEST  *Request;
  EFI_EVENT              AsyncIoEvent;
  EFI_TPL                OldTpl;

  AsyncIoEvent = NULL;

  Request = AllocateZeroPool (sizeof (SCSI_ASYNC_RW_REQUEST));
  if (Request == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&BlkIo2Req->ScsiRWQueue, &Request->Link);
  gBS->RestoreTPL (OldTpl);

  Request->SenseDataLength = (UINT8)(6 * sizeof (EFI_SCSI_SENSE_DATA));
  Request->SenseData       = AllocateZeroPool (Request->SenseDataLength);
  if (Request->SenseData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  Request->ScsiDiskDevice = ScsiDiskDevice;
  Request->Timeout        = Timeout;
  Request->TimesRetry     = TimesRetry;
  Request->InBuffer       = DataBuffer;
  Request->DataLength     = DataLength;
  Request->StartLba       = StartLba;
  Request->SectorCount    = SectorCount;
  Request->BlkIo2Req      = BlkIo2Req;

  //
  // Create Event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ScsiDiskNotify,
                  Request,
                  &AsyncIoEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = ScsiRead10CommandEx (
             ScsiDiskDevice->ScsiIo,
             Request->Timeout,
             Request->SenseData,
             &Request->SenseDataLength,
             &Request->HostAdapterStatus,
             &Request->TargetStatus,
             Request->InBuffer,
             &Request->DataLength,
             (UINT32)Request->StartLba,
             Request->SectorCount,
             AsyncIoEvent
             );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  if (AsyncIoEvent != NULL) {
    gBS->CloseEvent (AsyncIoEvent);
  }

  if (Request != NULL) {
    if (Request->SenseData != NULL) {
      FreePool (Request->SenseData);
    }

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    RemoveEntryList (&Request->Link);
    gBS->RestoreTPL (OldTpl);

    FreePool (Request);
  }

  return Status;
}

/**
  Submit Async Write(10) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice.
  @param  Timeout            The time to complete the command.
  @param  TimesRetry         The number of times the command has been retried.
  @param  DataBuffer         The buffer contains the data to write.
  @param  DataLength         The length of buffer.
  @param  StartLba           The start logic block address.
  @param  SectorCount        The number of blocks to write.
  @param  BlkIo2Req          The upstream BlockIo2 request.
  @param  Token              The pointer to the token associated with the
                             non-blocking read request.

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @return others                Status returned by calling
                                ScsiWrite10CommandEx().

**/
EFI_STATUS
ScsiDiskAsyncWrite10 (
  IN     SCSI_DISK_DEV        *ScsiDiskDevice,
  IN     UINT64               Timeout,
  IN     UINT8                TimesRetry,
  IN     UINT8                *DataBuffer,
  IN     UINT32               DataLength,
  IN     UINT32               StartLba,
  IN     UINT32               SectorCount,
  IN OUT SCSI_BLKIO2_REQUEST  *BlkIo2Req,
  IN     EFI_BLOCK_IO2_TOKEN  *Token
  )
{
  EFI_STATUS             Status;
  SCSI_ASYNC_RW_REQUEST  *Request;
  EFI_EVENT              AsyncIoEvent;
  EFI_TPL                OldTpl;

  AsyncIoEvent = NULL;

  Request = AllocateZeroPool (sizeof (SCSI_ASYNC_RW_REQUEST));
  if (Request == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&BlkIo2Req->ScsiRWQueue, &Request->Link);
  gBS->RestoreTPL (OldTpl);

  Request->SenseDataLength = (UINT8)(6 * sizeof (EFI_SCSI_SENSE_DATA));
  Request->SenseData       = AllocateZeroPool (Request->SenseDataLength);
  if (Request->SenseData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  Request->ScsiDiskDevice = ScsiDiskDevice;
  Request->Timeout        = Timeout;
  Request->TimesRetry     = TimesRetry;
  Request->OutBuffer      = DataBuffer;
  Request->DataLength     = DataLength;
  Request->StartLba       = StartLba;
  Request->SectorCount    = SectorCount;
  Request->BlkIo2Req      = BlkIo2Req;

  //
  // Create Event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ScsiDiskNotify,
                  Request,
                  &AsyncIoEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = ScsiWrite10CommandEx (
             ScsiDiskDevice->ScsiIo,
             Request->Timeout,
             Request->SenseData,
             &Request->SenseDataLength,
             &Request->HostAdapterStatus,
             &Request->TargetStatus,
             Request->OutBuffer,
             &Request->DataLength,
             (UINT32)Request->StartLba,
             Request->SectorCount,
             AsyncIoEvent
             );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  if (AsyncIoEvent != NULL) {
    gBS->CloseEvent (AsyncIoEvent);
  }

  if (Request != NULL) {
    if (Request->SenseData != NULL) {
      FreePool (Request->SenseData);
    }

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    RemoveEntryList (&Request->Link);
    gBS->RestoreTPL (OldTpl);

    FreePool (Request);
  }

  return Status;
}

/**
  Submit Async Read(16) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice.
  @param  Timeout            The time to complete the command.
  @param  TimesRetry         The number of times the command has been retried.
  @param  DataBuffer         The buffer to fill with the read out data.
  @param  DataLength         The length of buffer.
  @param  StartLba           The start logic block address.
  @param  SectorCount        The number of blocks to read.
  @param  BlkIo2Req          The upstream BlockIo2 request.
  @param  Token              The pointer to the token associated with the
                             non-blocking read request.

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @return others                Status returned by calling
                                ScsiRead16CommandEx().

**/
EFI_STATUS
ScsiDiskAsyncRead16 (
  IN     SCSI_DISK_DEV        *ScsiDiskDevice,
  IN     UINT64               Timeout,
  IN     UINT8                TimesRetry,
  OUT UINT8                   *DataBuffer,
  IN     UINT32               DataLength,
  IN     UINT64               StartLba,
  IN     UINT32               SectorCount,
  IN OUT SCSI_BLKIO2_REQUEST  *BlkIo2Req,
  IN     EFI_BLOCK_IO2_TOKEN  *Token
  )
{
  EFI_STATUS             Status;
  SCSI_ASYNC_RW_REQUEST  *Request;
  EFI_EVENT              AsyncIoEvent;
  EFI_TPL                OldTpl;

  AsyncIoEvent = NULL;

  Request = AllocateZeroPool (sizeof (SCSI_ASYNC_RW_REQUEST));
  if (Request == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&BlkIo2Req->ScsiRWQueue, &Request->Link);
  gBS->RestoreTPL (OldTpl);

  Request->SenseDataLength = (UINT8)(6 * sizeof (EFI_SCSI_SENSE_DATA));
  Request->SenseData       = AllocateZeroPool (Request->SenseDataLength);
  if (Request->SenseData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  Request->ScsiDiskDevice = ScsiDiskDevice;
  Request->Timeout        = Timeout;
  Request->TimesRetry     = TimesRetry;
  Request->InBuffer       = DataBuffer;
  Request->DataLength     = DataLength;
  Request->StartLba       = StartLba;
  Request->SectorCount    = SectorCount;
  Request->BlkIo2Req      = BlkIo2Req;

  //
  // Create Event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ScsiDiskNotify,
                  Request,
                  &AsyncIoEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = ScsiRead16CommandEx (
             ScsiDiskDevice->ScsiIo,
             Request->Timeout,
             Request->SenseData,
             &Request->SenseDataLength,
             &Request->HostAdapterStatus,
             &Request->TargetStatus,
             Request->InBuffer,
             &Request->DataLength,
             Request->StartLba,
             Request->SectorCount,
             AsyncIoEvent
             );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  if (AsyncIoEvent != NULL) {
    gBS->CloseEvent (AsyncIoEvent);
  }

  if (Request != NULL) {
    if (Request->SenseData != NULL) {
      FreePool (Request->SenseData);
    }

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    RemoveEntryList (&Request->Link);
    gBS->RestoreTPL (OldTpl);

    FreePool (Request);
  }

  return Status;
}

/**
  Submit Async Write(16) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice.
  @param  Timeout            The time to complete the command.
  @param  TimesRetry         The number of times the command has been retried.
  @param  DataBuffer         The buffer contains the data to write.
  @param  DataLength         The length of buffer.
  @param  StartLba           The start logic block address.
  @param  SectorCount        The number of blocks to write.
  @param  BlkIo2Req          The upstream BlockIo2 request.
  @param  Token              The pointer to the token associated with the
                             non-blocking read request.

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @return others                Status returned by calling
                                ScsiWrite16CommandEx().

**/
EFI_STATUS
ScsiDiskAsyncWrite16 (
  IN     SCSI_DISK_DEV        *ScsiDiskDevice,
  IN     UINT64               Timeout,
  IN     UINT8                TimesRetry,
  IN     UINT8                *DataBuffer,
  IN     UINT32               DataLength,
  IN     UINT64               StartLba,
  IN     UINT32               SectorCount,
  IN OUT SCSI_BLKIO2_REQUEST  *BlkIo2Req,
  IN     EFI_BLOCK_IO2_TOKEN  *Token
  )
{
  EFI_STATUS             Status;
  SCSI_ASYNC_RW_REQUEST  *Request;
  EFI_EVENT              AsyncIoEvent;
  EFI_TPL                OldTpl;

  AsyncIoEvent = NULL;

  Request = AllocateZeroPool (sizeof (SCSI_ASYNC_RW_REQUEST));
  if (Request == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&BlkIo2Req->ScsiRWQueue, &Request->Link);
  gBS->RestoreTPL (OldTpl);

  Request->SenseDataLength = (UINT8)(6 * sizeof (EFI_SCSI_SENSE_DATA));
  Request->SenseData       = AllocateZeroPool (Request->SenseDataLength);
  if (Request->SenseData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  Request->ScsiDiskDevice = ScsiDiskDevice;
  Request->Timeout        = Timeout;
  Request->TimesRetry     = TimesRetry;
  Request->OutBuffer      = DataBuffer;
  Request->DataLength     = DataLength;
  Request->StartLba       = StartLba;
  Request->SectorCount    = SectorCount;
  Request->BlkIo2Req      = BlkIo2Req;

  //
  // Create Event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ScsiDiskNotify,
                  Request,
                  &AsyncIoEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = ScsiWrite16CommandEx (
             ScsiDiskDevice->ScsiIo,
             Request->Timeout,
             Request->SenseData,
             &Request->SenseDataLength,
             &Request->HostAdapterStatus,
             &Request->TargetStatus,
             Request->OutBuffer,
             &Request->DataLength,
             Request->StartLba,
             Request->SectorCount,
             AsyncIoEvent
             );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  if (AsyncIoEvent != NULL) {
    gBS->CloseEvent (AsyncIoEvent);
  }

  if (Request != NULL) {
    if (Request->SenseData != NULL) {
      FreePool (Request->SenseData);
    }

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    RemoveEntryList (&Request->Link);
    gBS->RestoreTPL (OldTpl);

    FreePool (Request);
  }

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
  IN  EFI_SCSI_SENSE_DATA  *SenseData,
  IN  UINTN                SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA  *SensePtr;
  UINTN                Index;
  BOOLEAN              IsNoMedia;

  IsNoMedia = FALSE;
  SensePtr  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    //
    // Sense Key is EFI_SCSI_SK_NOT_READY (0x2),
    // Additional Sense Code is ASC_NO_MEDIA (0x3A)
    //
    if ((SensePtr->Sense_Key == EFI_SCSI_SK_NOT_READY) &&
        (SensePtr->Addnl_Sense_Code == EFI_SCSI_ASC_NO_MEDIA))
    {
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
  IN  EFI_SCSI_SENSE_DATA  *SenseData,
  IN  UINTN                SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA  *SensePtr;
  UINTN                Index;
  BOOLEAN              IsError;

  IsError  = FALSE;
  SensePtr = SenseData;

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
  IN  EFI_SCSI_SENSE_DATA  *SenseData,
  IN  UINTN                SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA  *SensePtr;
  UINTN                Index;
  BOOLEAN              IsError;

  IsError  = FALSE;
  SensePtr = SenseData;

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
  @retval FALSE  Media is NOT changed.
**/
BOOLEAN
ScsiDiskIsMediaChange (
  IN  EFI_SCSI_SENSE_DATA  *SenseData,
  IN  UINTN                SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA  *SensePtr;
  UINTN                Index;
  BOOLEAN              IsMediaChanged;

  IsMediaChanged = FALSE;
  SensePtr       = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    //
    // Sense Key is EFI_SCSI_SK_UNIT_ATTENTION (0x6),
    // Additional sense code is EFI_SCSI_ASC_MEDIA_CHANGE (0x28)
    //
    if ((SensePtr->Sense_Key == EFI_SCSI_SK_UNIT_ATTENTION) &&
        (SensePtr->Addnl_Sense_Code == EFI_SCSI_ASC_MEDIA_CHANGE))
    {
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
  IN  EFI_SCSI_SENSE_DATA  *SenseData,
  IN  UINTN                SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA  *SensePtr;
  UINTN                Index;
  BOOLEAN              IsResetBefore;

  IsResetBefore = FALSE;
  SensePtr      = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {
    //
    // Sense Key is EFI_SCSI_SK_UNIT_ATTENTION (0x6)
    // Additional Sense Code is EFI_SCSI_ASC_RESET (0x29)
    //
    if ((SensePtr->Sense_Key == EFI_SCSI_SK_UNIT_ATTENTION) &&
        (SensePtr->Addnl_Sense_Code == EFI_SCSI_ASC_RESET))
    {
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
  IN  EFI_SCSI_SENSE_DATA  *SenseData,
  IN  UINTN                SenseCounts,
  OUT BOOLEAN              *RetryLater
  )
{
  EFI_SCSI_SENSE_DATA  *SensePtr;
  UINTN                Index;
  BOOLEAN              IsReady;

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
  IN  EFI_SCSI_SENSE_DATA  *SenseData,
  IN  UINTN                SenseCounts
  )
{
  EFI_SCSI_SENSE_DATA  *SensePtr;
  UINTN                Index;
  BOOLEAN              HaveSenseKey;

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
        (Index == 0))
    {
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
  IN  SCSI_DISK_DEV  *ScsiDiskDevice
  )
{
  if (ScsiDiskDevice == NULL) {
    return;
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

/**
  Determine if Block Io & Block Io2 should be produced.


  @param  ChildHandle  Child Handle to retrieve Parent information.

  @retval  TRUE    Should produce Block Io & Block Io2.
  @retval  FALSE   Should not produce Block Io & Block Io2.

**/
BOOLEAN
DetermineInstallBlockIo (
  IN  EFI_HANDLE  ChildHandle
  )
{
  EFI_SCSI_PASS_THRU_PROTOCOL      *ScsiPassThru;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *ExtScsiPassThru;

  //
  // Firstly, check if ExtScsiPassThru Protocol parent handle exists. If existence,
  // check its attribute, logic or physical.
  //
  ExtScsiPassThru = (EFI_EXT_SCSI_PASS_THRU_PROTOCOL *)GetParentProtocol (&gEfiExtScsiPassThruProtocolGuid, ChildHandle);
  if (ExtScsiPassThru != NULL) {
    if ((ExtScsiPassThru->Mode->Attributes & EFI_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL) != 0) {
      return TRUE;
    }
  }

  //
  // Secondly, check if ScsiPassThru Protocol parent handle exists. If existence,
  // check its attribute, logic or physical.
  //
  ScsiPassThru = (EFI_SCSI_PASS_THRU_PROTOCOL *)GetParentProtocol (&gEfiScsiPassThruProtocolGuid, ChildHandle);
  if (ScsiPassThru != NULL) {
    if ((ScsiPassThru->Mode->Attributes & EFI_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL) != 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Search protocol database and check to see if the protocol
  specified by ProtocolGuid is present on a ControllerHandle and opened by
  ChildHandle with an attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
  If the ControllerHandle is found, then the protocol specified by ProtocolGuid
  will be opened on it.


  @param  ProtocolGuid   ProtocolGuid pointer.
  @param  ChildHandle    Child Handle to retrieve Parent information.

**/
VOID *
EFIAPI
GetParentProtocol (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  ChildHandle
  )
{
  UINTN       Index;
  UINTN       HandleCount;
  VOID        *Interface;
  EFI_STATUS  Status;
  EFI_HANDLE  *HandleBuffer;

  //
  // Retrieve the list of all handles from the handle database
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Iterate to find who is parent handle that is opened with ProtocolGuid by ChildHandle
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = EfiTestChildHandle (HandleBuffer[Index], ChildHandle, ProtocolGuid);
    if (!EFI_ERROR (Status)) {
      Status = gBS->HandleProtocol (HandleBuffer[Index], ProtocolGuid, (VOID **)&Interface);
      if (!EFI_ERROR (Status)) {
        gBS->FreePool (HandleBuffer);
        return Interface;
      }
    }
  }

  gBS->FreePool (HandleBuffer);
  return NULL;
}

/**
  Determine if EFI Erase Block Protocol should be produced.

  @param   ScsiDiskDevice    The pointer of SCSI_DISK_DEV.
  @param   ChildHandle       Handle of device.

  @retval  TRUE    Should produce EFI Erase Block Protocol.
  @retval  FALSE   Should not produce EFI Erase Block Protocol.

**/
BOOLEAN
DetermineInstallEraseBlock (
  IN  SCSI_DISK_DEV  *ScsiDiskDevice,
  IN  EFI_HANDLE     ChildHandle
  )
{
  UINT8                          HostAdapterStatus;
  UINT8                          TargetStatus;
  EFI_STATUS                     CommandStatus;
  EFI_STATUS                     Status;
  BOOLEAN                        UfsDevice;
  BOOLEAN                        RetVal;
  EFI_DEVICE_PATH_PROTOCOL       *DevicePathNode;
  UINT8                          SenseDataLength;
  UINT32                         DataLength16;
  EFI_SCSI_DISK_CAPACITY_DATA16  *CapacityData16;

  UfsDevice      = FALSE;
  RetVal         = TRUE;
  CapacityData16 = NULL;

  //
  // UNMAP command is not supported by any of the UFS WLUNs.
  //
  if (ScsiDiskDevice->DeviceType == EFI_SCSI_TYPE_WLUN) {
    RetVal = FALSE;
    goto Done;
  }

  Status = gBS->HandleProtocol (
                  ChildHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePathNode
                  );
  //
  // Device Path protocol must be installed on the device handle.
  //
  ASSERT_EFI_ERROR (Status);

  while (!IsDevicePathEndType (DevicePathNode)) {
    //
    // For now, only support Erase Block Protocol on UFS devices.
    //
    if ((DevicePathNode->Type == MESSAGING_DEVICE_PATH) &&
        (DevicePathNode->SubType == MSG_UFS_DP))
    {
      UfsDevice = TRUE;
      break;
    }

    DevicePathNode = NextDevicePathNode (DevicePathNode);
  }

  if (!UfsDevice) {
    RetVal = FALSE;
    goto Done;
  }

  //
  // Check whether the erase functionality is enabled on the UFS device.
  //
  CapacityData16 = AllocateAlignedBuffer (ScsiDiskDevice, sizeof (EFI_SCSI_DISK_CAPACITY_DATA16));
  if (CapacityData16 == NULL) {
    RetVal = FALSE;
    goto Done;
  }

  SenseDataLength = 0;
  DataLength16    = sizeof (EFI_SCSI_DISK_CAPACITY_DATA16);
  ZeroMem (CapacityData16, sizeof (EFI_SCSI_DISK_CAPACITY_DATA16));

  CommandStatus = ScsiReadCapacity16Command (
                    ScsiDiskDevice->ScsiIo,
                    SCSI_DISK_TIMEOUT,
                    NULL,
                    &SenseDataLength,
                    &HostAdapterStatus,
                    &TargetStatus,
                    (VOID *)CapacityData16,
                    &DataLength16,
                    FALSE
                    );

  if (CommandStatus == EFI_SUCCESS) {
    //
    // Universal Flash Storage (UFS) Version 2.0
    // Section 11.3.9.2
    // Bits TPE and TPRZ should both be set to enable the erase feature on UFS.
    //
    if (((CapacityData16->LowestAlignLogic2 & BIT7) == 0) ||
        ((CapacityData16->LowestAlignLogic2 & BIT6) == 0))
    {
      DEBUG ((
        DEBUG_VERBOSE,
        "ScsiDisk EraseBlock: Either TPE or TPRZ is not set: 0x%x.\n",
        CapacityData16->LowestAlignLogic2
        ));

      RetVal = FALSE;
      goto Done;
    }
  } else {
    DEBUG ((
      DEBUG_VERBOSE,
      "ScsiDisk EraseBlock: ReadCapacity16 failed with status %r.\n",
      CommandStatus
      ));

    RetVal = FALSE;
    goto Done;
  }

  //
  // Check whether the UFS device server implements the UNMAP command.
  //
  if ((ScsiDiskDevice->UnmapInfo.MaxLbaCnt == 0) ||
      (ScsiDiskDevice->UnmapInfo.MaxBlkDespCnt == 0))
  {
    DEBUG ((
      DEBUG_VERBOSE,
      "ScsiDisk EraseBlock: The device server does not implement the UNMAP command.\n"
      ));

    RetVal = FALSE;
    goto Done;
  }

Done:
  if (CapacityData16 != NULL) {
    FreeAlignedBuffer (CapacityData16, sizeof (EFI_SCSI_DISK_CAPACITY_DATA16));
  }

  return RetVal;
}

/**
  Determine if EFI Storage Security Command Protocol should be produced.

  @param   ScsiDiskDevice    The pointer of SCSI_DISK_DEV.
  @param   ChildHandle       Handle of device.

  @retval  TRUE    Should produce EFI Storage Security Command Protocol.
  @retval  FALSE   Should not produce EFI Storage Security Command Protocol.

**/
BOOLEAN
DetermineInstallStorageSecurity (
  IN  SCSI_DISK_DEV  *ScsiDiskDevice,
  IN  EFI_HANDLE     ChildHandle
  )
{
  EFI_STATUS                Status;
  UFS_DEVICE_PATH           *UfsDevice;
  BOOLEAN                   RetVal;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode;

  UfsDevice = NULL;
  RetVal    = TRUE;

  Status = gBS->HandleProtocol (
                  ChildHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePathNode
                  );
  //
  // Device Path protocol must be installed on the device handle.
  //
  ASSERT_EFI_ERROR (Status);

  while (!IsDevicePathEndType (DevicePathNode)) {
    //
    // For now, only support Storage Security Command Protocol on UFS devices.
    //
    if ((DevicePathNode->Type == MESSAGING_DEVICE_PATH) &&
        (DevicePathNode->SubType == MSG_UFS_DP))
    {
      UfsDevice = (UFS_DEVICE_PATH *)DevicePathNode;
      break;
    }

    DevicePathNode = NextDevicePathNode (DevicePathNode);
  }

  if (UfsDevice == NULL) {
    RetVal = FALSE;
    goto Done;
  }

  if (UfsDevice->Lun != UFS_WLUN_RPMB) {
    RetVal = FALSE;
  }

Done:
  return RetVal;
}

/**
  Provides inquiry information for the controller type.

  This function is used by the IDE bus driver to get inquiry data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] InquiryData       Pointer to a buffer for the inquiry data.
  @param[in, out] InquiryDataSize   Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class
  @retval EFI_DEVICE_ERROR       Error reading InquiryData from device
  @retval EFI_BUFFER_TOO_SMALL   InquiryDataSize not big enough

**/
EFI_STATUS
EFIAPI
ScsiDiskInfoInquiry (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *InquiryData,
  IN OUT UINT32                  *InquiryDataSize
  )
{
  EFI_STATUS     Status;
  SCSI_DISK_DEV  *ScsiDiskDevice;

  ScsiDiskDevice = SCSI_DISK_DEV_FROM_DISKINFO (This);

  Status = EFI_BUFFER_TOO_SMALL;
  if (*InquiryDataSize >= sizeof (ScsiDiskDevice->InquiryData)) {
    Status = EFI_SUCCESS;
    CopyMem (InquiryData, &ScsiDiskDevice->InquiryData, sizeof (ScsiDiskDevice->InquiryData));
  }

  *InquiryDataSize = sizeof (ScsiDiskDevice->InquiryData);
  return Status;
}

/**
  Provides identify information for the controller type.

  This function is used by the IDE bus driver to get identify data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL
                                    instance.
  @param[in, out] IdentifyData      Pointer to a buffer for the identify data.
  @param[in, out] IdentifyDataSize  Pointer to the value for the identify data
                                    size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class
  @retval EFI_DEVICE_ERROR       Error reading IdentifyData from device
  @retval EFI_BUFFER_TOO_SMALL   IdentifyDataSize not big enough

**/
EFI_STATUS
EFIAPI
ScsiDiskInfoIdentify (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *IdentifyData,
  IN OUT UINT32                  *IdentifyDataSize
  )
{
  EFI_STATUS     Status;
  SCSI_DISK_DEV  *ScsiDiskDevice;

  if (CompareGuid (&This->Interface, &gEfiDiskInfoScsiInterfaceGuid) || CompareGuid (&This->Interface, &gEfiDiskInfoUfsInterfaceGuid)) {
    //
    // Physical SCSI bus does not support this data class.
    //
    return EFI_NOT_FOUND;
  }

  ScsiDiskDevice = SCSI_DISK_DEV_FROM_DISKINFO (This);

  Status = EFI_BUFFER_TOO_SMALL;
  if (*IdentifyDataSize >= sizeof (ScsiDiskDevice->IdentifyData)) {
    Status = EFI_SUCCESS;
    CopyMem (IdentifyData, &ScsiDiskDevice->IdentifyData, sizeof (ScsiDiskDevice->IdentifyData));
  }

  *IdentifyDataSize = sizeof (ScsiDiskDevice->IdentifyData);
  return Status;
}

/**
  Provides sense data information for the controller type.

  This function is used by the IDE bus driver to get sense data.
  Data format of Sense data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] SenseData         Pointer to the SenseData.
  @param[in, out] SenseDataSize     Size of SenseData in bytes.
  @param[out]     SenseDataNumber   Pointer to the value for the sense data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class.
  @retval EFI_DEVICE_ERROR       Error reading SenseData from device.
  @retval EFI_BUFFER_TOO_SMALL   SenseDataSize not big enough.

**/
EFI_STATUS
EFIAPI
ScsiDiskInfoSenseData (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *SenseData,
  IN OUT UINT32                  *SenseDataSize,
  OUT    UINT8                   *SenseDataNumber
  )
{
  return EFI_NOT_FOUND;
}

/**
  This function is used by the IDE bus driver to get controller information.

  @param[in]  This         Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[out] IdeChannel   Pointer to the Ide Channel number.  Primary or secondary.
  @param[out] IdeDevice    Pointer to the Ide Device number.  Master or slave.

  @retval EFI_SUCCESS       IdeChannel and IdeDevice are valid.
  @retval EFI_UNSUPPORTED   This is not an IDE device.

**/
EFI_STATUS
EFIAPI
ScsiDiskInfoWhichIde (
  IN  EFI_DISK_INFO_PROTOCOL  *This,
  OUT UINT32                  *IdeChannel,
  OUT UINT32                  *IdeDevice
  )
{
  SCSI_DISK_DEV  *ScsiDiskDevice;

  if (CompareGuid (&This->Interface, &gEfiDiskInfoScsiInterfaceGuid) || CompareGuid (&This->Interface, &gEfiDiskInfoUfsInterfaceGuid)) {
    //
    // This is not an IDE physical device.
    //
    return EFI_UNSUPPORTED;
  }

  ScsiDiskDevice = SCSI_DISK_DEV_FROM_DISKINFO (This);
  *IdeChannel    = ScsiDiskDevice->Channel;
  *IdeDevice     = ScsiDiskDevice->Device;

  return EFI_SUCCESS;
}

/**
  Issues ATA IDENTIFY DEVICE command to identify ATAPI device.

  This function tries to fill 512-byte ATAPI_IDENTIFY_DATA for ATAPI device to
  implement Identify() interface for DiskInfo protocol. The ATA command is sent
  via SCSI Request Packet.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV

  @retval EFI_SUCCESS     The ATAPI device identify data were retrieved successfully.
  @retval others          Some error occurred during the identification that ATAPI device.

**/
EFI_STATUS
AtapiIdentifyDevice (
  IN OUT SCSI_DISK_DEV  *ScsiDiskDevice
  )
{
  EFI_SCSI_IO_SCSI_REQUEST_PACKET  CommandPacket;
  UINT8                            Cdb[6];

  //
  // Initialize SCSI REQUEST_PACKET and 6-byte Cdb
  //
  ZeroMem (&CommandPacket, sizeof (CommandPacket));
  ZeroMem (Cdb, sizeof (Cdb));

  Cdb[0]                         = ATA_CMD_IDENTIFY_DEVICE;
  CommandPacket.Timeout          = SCSI_DISK_TIMEOUT;
  CommandPacket.Cdb              = Cdb;
  CommandPacket.CdbLength        = (UINT8)sizeof (Cdb);
  CommandPacket.InDataBuffer     = &ScsiDiskDevice->IdentifyData;
  CommandPacket.InTransferLength = sizeof (ScsiDiskDevice->IdentifyData);

  return ScsiDiskDevice->ScsiIo->ExecuteScsiCommand (ScsiDiskDevice->ScsiIo, &CommandPacket, NULL);
}

/**
  Initialize the installation of DiskInfo protocol.

  This function prepares for the installation of DiskInfo protocol on the child handle.
  By default, it installs DiskInfo protocol with SCSI interface GUID. If it further
  detects that the physical device is an ATAPI/AHCI device, it then updates interface GUID
  to be IDE/AHCI interface GUID.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV.
  @param  ChildHandle     Child handle to install DiskInfo protocol.

**/
VOID
InitializeInstallDiskInfo (
  IN  SCSI_DISK_DEV  *ScsiDiskDevice,
  IN  EFI_HANDLE     ChildHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *ChildDevicePathNode;
  ATAPI_DEVICE_PATH         *AtapiDevicePath;
  SATA_DEVICE_PATH          *SataDevicePath;
  UINTN                     IdentifyRetry;

  Status = gBS->HandleProtocol (ChildHandle, &gEfiDevicePathProtocolGuid, (VOID **)&DevicePathNode);
  //
  // Device Path protocol must be installed on the device handle.
  //
  ASSERT_EFI_ERROR (Status);
  //
  // Copy the DiskInfo protocol template.
  //
  CopyMem (&ScsiDiskDevice->DiskInfo, &gScsiDiskInfoProtocolTemplate, sizeof (gScsiDiskInfoProtocolTemplate));

  while (!IsDevicePathEnd (DevicePathNode)) {
    ChildDevicePathNode = NextDevicePathNode (DevicePathNode);
    if ((DevicePathType (DevicePathNode) == HARDWARE_DEVICE_PATH) &&
        (DevicePathSubType (DevicePathNode) == HW_PCI_DP) &&
        (DevicePathType (ChildDevicePathNode) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (ChildDevicePathNode) == MSG_ATAPI_DP) ||
         (DevicePathSubType (ChildDevicePathNode) == MSG_SATA_DP)))
    {
      IdentifyRetry = 3;
      do {
        //
        // Issue ATA Identify Device Command via SCSI command, which is required to publish DiskInfo protocol
        // with IDE/AHCI interface GUID.
        //
        Status = AtapiIdentifyDevice (ScsiDiskDevice);
        if (!EFI_ERROR (Status)) {
          if (DevicePathSubType (ChildDevicePathNode) == MSG_ATAPI_DP) {
            //
            // We find the valid ATAPI device path
            //
            AtapiDevicePath         = (ATAPI_DEVICE_PATH *)ChildDevicePathNode;
            ScsiDiskDevice->Channel = AtapiDevicePath->PrimarySecondary;
            ScsiDiskDevice->Device  = AtapiDevicePath->SlaveMaster;
            //
            // Update the DiskInfo.Interface to IDE interface GUID for the physical ATAPI device.
            //
            CopyGuid (&ScsiDiskDevice->DiskInfo.Interface, &gEfiDiskInfoIdeInterfaceGuid);
          } else {
            //
            // We find the valid SATA device path
            //
            SataDevicePath          = (SATA_DEVICE_PATH *)ChildDevicePathNode;
            ScsiDiskDevice->Channel = SataDevicePath->HBAPortNumber;
            ScsiDiskDevice->Device  = SataDevicePath->PortMultiplierPortNumber;
            //
            // Update the DiskInfo.Interface to AHCI interface GUID for the physical AHCI device.
            //
            CopyGuid (&ScsiDiskDevice->DiskInfo.Interface, &gEfiDiskInfoAhciInterfaceGuid);
          }

          return;
        }
      } while (--IdentifyRetry > 0);
    } else if ((DevicePathType (ChildDevicePathNode) == MESSAGING_DEVICE_PATH) &&
               (DevicePathSubType (ChildDevicePathNode) == MSG_UFS_DP))
    {
      CopyGuid (&ScsiDiskDevice->DiskInfo.Interface, &gEfiDiskInfoUfsInterfaceGuid);
      break;
    }

    DevicePathNode = ChildDevicePathNode;
  }

  return;
}
