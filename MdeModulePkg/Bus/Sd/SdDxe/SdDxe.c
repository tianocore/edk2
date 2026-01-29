/** @file
  The SdDxe driver is used to manage the SD memory card device.

  It produces BlockIo and BlockIo2 protocols to allow upper layer
  access the SD memory card device.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdDxe.h"

//
// SdDxe Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL  gSdDxeDriverBinding = {
  SdDxeDriverBindingSupported,
  SdDxeDriverBindingStart,
  SdDxeDriverBindingStop,
  0x10,
  NULL,
  NULL
};

//
// Template for SD_DEVICE data structure.
//
SD_DEVICE  mSdDeviceTemplate = {
  SD_DEVICE_SIGNATURE,         // Signature
  NULL,                        // Handle
  NULL,                        // DevicePath
  0xFF,                        // Slot
  FALSE,                       // SectorAddressing
  {                            // BlockIo
    EFI_BLOCK_IO_PROTOCOL_REVISION,
    NULL,
    SdReset,
    SdReadBlocks,
    SdWriteBlocks,
    SdFlushBlocks
  },
  {                            // BlockIo2
    NULL,
    SdResetEx,
    SdReadBlocksEx,
    SdWriteBlocksEx,
    SdFlushBlocksEx
  },
  {                            // BlockMedia
    0,                         // MediaId
    FALSE,                     // RemovableMedia
    TRUE,                      // MediaPresent
    FALSE,                     // LogicPartition
    FALSE,                     // ReadOnly
    FALSE,                     // WritingCache
    0x200,                     // BlockSize
    0,                         // IoAlign
    0                          // LastBlock
  },
  {                            // EraseBlock
    EFI_ERASE_BLOCK_PROTOCOL_REVISION,
    1,
    SdEraseBlocks
  },
  {                            // DiskInfo
    EFI_DISK_INFO_SD_MMC_INTERFACE_GUID,
    SdDiskInfoInquiry,
    SdDiskInfoIdentify,
    SdDiskInfoSenseData,
    SdDiskInfoWhichIde
  },
  {                            // Queue
    NULL,
    NULL
  },
  {                            // Csd
    0,
  },
  {                            // Cid
    0,
  },
  NULL,                        // ControllerNameTable
  {                            // ModelName
    0,
  },
  NULL                         // Private
};

/**
  Decode and print SD CSD Register content.

  @param[in] Csd           Pointer to SD_CSD data structure.

  @retval EFI_SUCCESS      The function completed successfully
**/
EFI_STATUS
DumpCsd (
  IN SD_CSD  *Csd
  )
{
  SD_CSD2  *Csd2;

  DEBUG ((DEBUG_INFO, "== Dump Sd Csd Register==\n"));
  DEBUG ((DEBUG_INFO, "  CSD structure                    0x%x\n", Csd->CsdStructure));
  DEBUG ((DEBUG_INFO, "  Data read access-time 1          0x%x\n", Csd->Taac));
  DEBUG ((DEBUG_INFO, "  Data read access-time 2          0x%x\n", Csd->Nsac));
  DEBUG ((DEBUG_INFO, "  Max. bus clock frequency         0x%x\n", Csd->TranSpeed));
  DEBUG ((DEBUG_INFO, "  Device command classes           0x%x\n", Csd->Ccc));
  DEBUG ((DEBUG_INFO, "  Max. read data block length      0x%x\n", Csd->ReadBlLen));
  DEBUG ((DEBUG_INFO, "  Partial blocks for read allowed  0x%x\n", Csd->ReadBlPartial));
  DEBUG ((DEBUG_INFO, "  Write block misalignment         0x%x\n", Csd->WriteBlkMisalign));
  DEBUG ((DEBUG_INFO, "  Read block misalignment          0x%x\n", Csd->ReadBlkMisalign));
  DEBUG ((DEBUG_INFO, "  DSR implemented                  0x%x\n", Csd->DsrImp));
  if (Csd->CsdStructure == 0) {
    DEBUG ((DEBUG_INFO, "  Device size                      0x%x\n", Csd->CSizeLow | (Csd->CSizeHigh << 2)));
    DEBUG ((DEBUG_INFO, "  Max. read current @ VDD min      0x%x\n", Csd->VddRCurrMin));
    DEBUG ((DEBUG_INFO, "  Max. read current @ VDD max      0x%x\n", Csd->VddRCurrMax));
    DEBUG ((DEBUG_INFO, "  Max. write current @ VDD min     0x%x\n", Csd->VddWCurrMin));
    DEBUG ((DEBUG_INFO, "  Max. write current @ VDD max     0x%x\n", Csd->VddWCurrMax));
  } else {
    Csd2 = (SD_CSD2 *)(VOID *)Csd;
    DEBUG ((DEBUG_INFO, "  Device size                      0x%x\n", Csd2->CSizeLow | (Csd->CSizeHigh << 16)));
  }

  DEBUG ((DEBUG_INFO, "  Erase sector size                0x%x\n", Csd->SectorSize));
  DEBUG ((DEBUG_INFO, "  Erase single block enable        0x%x\n", Csd->EraseBlkEn));
  DEBUG ((DEBUG_INFO, "  Write protect group size         0x%x\n", Csd->WpGrpSize));
  DEBUG ((DEBUG_INFO, "  Write protect group enable       0x%x\n", Csd->WpGrpEnable));
  DEBUG ((DEBUG_INFO, "  Write speed factor               0x%x\n", Csd->R2WFactor));
  DEBUG ((DEBUG_INFO, "  Max. write data block length     0x%x\n", Csd->WriteBlLen));
  DEBUG ((DEBUG_INFO, "  Partial blocks for write allowed 0x%x\n", Csd->WriteBlPartial));
  DEBUG ((DEBUG_INFO, "  File format group                0x%x\n", Csd->FileFormatGrp));
  DEBUG ((DEBUG_INFO, "  Copy flag (OTP)                  0x%x\n", Csd->Copy));
  DEBUG ((DEBUG_INFO, "  Permanent write protection       0x%x\n", Csd->PermWriteProtect));
  DEBUG ((DEBUG_INFO, "  Temporary write protection       0x%x\n", Csd->TmpWriteProtect));
  DEBUG ((DEBUG_INFO, "  File format                      0x%x\n", Csd->FileFormat));

  return EFI_SUCCESS;
}

/**
  Get SD device model name.

  @param[in, out] Device   The pointer to the SD_DEVICE data structure.
  @param[in]      Cid      Pointer to SD_CID data structure.

  @retval EFI_SUCCESS      The function completed successfully

**/
EFI_STATUS
GetSdModelName (
  IN OUT SD_DEVICE  *Device,
  IN     SD_CID     *Cid
  )
{
  CHAR8  String[SD_MODEL_NAME_MAX_LEN];

  ZeroMem (String, sizeof (String));
  CopyMem (String, Cid->OemId, sizeof (Cid->OemId));
  String[sizeof (Cid->OemId)] = ' ';
  CopyMem (String + sizeof (Cid->OemId) + 1, Cid->ProductName, sizeof (Cid->ProductName));
  String[sizeof (Cid->OemId) + sizeof (Cid->ProductName)] = ' ';
  CopyMem (String + sizeof (Cid->OemId) + sizeof (Cid->ProductName) + 1, Cid->ProductSerialNumber, sizeof (Cid->ProductSerialNumber));

  AsciiStrToUnicodeStrS (String, Device->ModelName, sizeof (Device->ModelName) / sizeof (Device->ModelName[0]));

  return EFI_SUCCESS;
}

/**
  Discover user area partition in the SD device.

  @param[in] Device          The pointer to the SD_DEVICE data structure.

  @retval EFI_SUCCESS        The user area partition in the SD device is successfully identified.
  @return Others             Some error occurs when identifying the user area.

**/
EFI_STATUS
DiscoverUserArea (
  IN SD_DEVICE  *Device
  )
{
  EFI_STATUS  Status;
  SD_CSD      *Csd;
  SD_CSD2     *Csd2;
  SD_CID      *Cid;
  UINT64      Capacity;
  UINT32      DevStatus;
  UINT16      Rca;
  UINT32      CSize;
  UINT32      CSizeMul;
  UINT32      ReadBlLen;

  //
  // Deselect the device to force it enter stby mode.
  // Note here we don't judge return status as some SD devices return
  // error but the state has been stby.
  //
  SdSelect (Device, 0);

  Status = SdSetRca (Device, &Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DiscoverUserArea(): Assign new Rca = 0x%x fails with %r\n", Rca, Status));
    return Status;
  }

  Csd    = &Device->Csd;
  Status = SdGetCsd (Device, Rca, Csd);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DumpCsd (Csd);

  Cid    = &Device->Cid;
  Status = SdGetCid (Device, Rca, Cid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GetSdModelName (Device, Cid);

  Status = SdSelect (Device, Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DiscoverUserArea(): Reselect the device 0x%x fails with %r\n", Rca, Status));
    return Status;
  }

  Status = SdSendStatus (Device, Rca, &DevStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Csd->CsdStructure == 0) {
    Device->SectorAddressing = FALSE;
    CSize                    = (Csd->CSizeHigh << 2 | Csd->CSizeLow) + 1;
    CSizeMul                 = (1 << (Csd->CSizeMul + 2));
    ReadBlLen                = (1 << (Csd->ReadBlLen));
    Capacity                 = MultU64x32 (MultU64x32 ((UINT64)CSize, CSizeMul), ReadBlLen);
  } else {
    Device->SectorAddressing = TRUE;
    Csd2                     = (SD_CSD2 *)(VOID *)Csd;
    CSize                    = (Csd2->CSizeHigh << 16 | Csd2->CSizeLow) + 1;
    Capacity                 = MultU64x32 ((UINT64)CSize, SIZE_512KB);
  }

  Device->BlockIo.Media               = &Device->BlockMedia;
  Device->BlockIo2.Media              = &Device->BlockMedia;
  Device->BlockMedia.IoAlign          = Device->Private->PassThru->IoAlign;
  Device->BlockMedia.BlockSize        = 0x200;
  Device->BlockMedia.LastBlock        = 0x00;
  Device->BlockMedia.RemovableMedia   = TRUE;
  Device->BlockMedia.MediaPresent     = TRUE;
  Device->BlockMedia.LogicalPartition = FALSE;
  Device->BlockMedia.LastBlock        = DivU64x32 (Capacity, Device->BlockMedia.BlockSize) - 1;

  if (Csd->EraseBlkEn) {
    Device->EraseBlock.EraseLengthGranularity = 1;
  } else {
    Device->EraseBlock.EraseLengthGranularity = (Csd->SectorSize + 1) * (1 << (Csd->WriteBlLen - 9));
  }

  return Status;
}

/**
  Scan SD Bus to discover the device.

  @param[in]  Private             The SD driver private data structure.
  @param[in]  Slot                The slot number to check device present.

  @retval EFI_SUCCESS             Successfully to discover the device and attach
                                  SdMmcIoProtocol to it.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack
                                  of resources.
  @retval EFI_ALREADY_STARTED     The device was discovered before.
  @retval Others                  Fail to discover the device.

**/
EFI_STATUS
EFIAPI
DiscoverSdDevice (
  IN  SD_DRIVER_PRIVATE_DATA  *Private,
  IN  UINT8                   Slot
  )
{
  EFI_STATUS                     Status;
  SD_DEVICE                      *Device;
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL       *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath;
  EFI_HANDLE                     DeviceHandle;
  EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru;

  Device              = NULL;
  DevicePath          = NULL;
  NewDevicePath       = NULL;
  RemainingDevicePath = NULL;
  PassThru            = Private->PassThru;

  //
  // Build Device Path
  //
  Status = PassThru->BuildDevicePath (
                       PassThru,
                       Slot,
                       &DevicePath
                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DevicePath->SubType != MSG_SD_DP) {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }

  NewDevicePath = AppendDevicePathNode (
                    Private->ParentDevicePath,
                    DevicePath
                    );

  if (NewDevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  DeviceHandle        = NULL;
  RemainingDevicePath = NewDevicePath;
  Status              = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &DeviceHandle);
  if (!EFI_ERROR (Status) && (DeviceHandle != NULL) && IsDevicePathEnd (RemainingDevicePath)) {
    //
    // The device has been started, directly return to fast boot.
    //
    Status = EFI_ALREADY_STARTED;
    goto Error;
  }

  //
  // Allocate buffer to store SD_DEVICE private data.
  //
  Device = AllocateCopyPool (sizeof (SD_DEVICE), &mSdDeviceTemplate);
  if (Device == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Device->DevicePath = NewDevicePath;
  Device->Slot       = Slot;
  Device->Private    = Private;
  InitializeListHead (&Device->Queue);

  //
  // Expose user area in the Sd memory card to upper layer.
  //
  Status = DiscoverUserArea (Device);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Device->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gSdDxeComponentName.SupportedLanguages,
    &Device->ControllerNameTable,
    Device->ModelName,
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gSdDxeComponentName2.SupportedLanguages,
    &Device->ControllerNameTable,
    Device->ModelName,
    FALSE
    );

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Device->Handle,
                  &gEfiDevicePathProtocolGuid,
                  Device->DevicePath,
                  &gEfiBlockIoProtocolGuid,
                  &Device->BlockIo,
                  &gEfiBlockIo2ProtocolGuid,
                  &Device->BlockIo2,
                  &gEfiEraseBlockProtocolGuid,
                  &Device->EraseBlock,
                  &gEfiDiskInfoProtocolGuid,
                  &Device->DiskInfo,
                  NULL
                  );

  if (!EFI_ERROR (Status)) {
    gBS->OpenProtocol (
           Private->Controller,
           &gEfiSdMmcPassThruProtocolGuid,
           (VOID **)&(Private->PassThru),
           Private->DriverBindingHandle,
           Device->Handle,
           EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
           );
  }

Error:
  FreePool (DevicePath);

  if (EFI_ERROR (Status) && (NewDevicePath != NULL)) {
    FreePool (NewDevicePath);
  }

  if (EFI_ERROR (Status) && (Device != NULL)) {
    FreePool (Device);
  }

  return Status;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Since ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
SdDxeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                     Status;
  EFI_DEVICE_PATH_PROTOCOL       *ParentDevicePath;
  EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru;
  UINT8                          Slot;

  //
  // Test EFI_SD_MMC_PASS_THRU_PROTOCOL on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSdMmcPassThruProtocolGuid,
                  (VOID **)&PassThru,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Test RemainingDevicePath is valid or not.
  //
  if ((RemainingDevicePath != NULL) && !IsDevicePathEnd (RemainingDevicePath)) {
    Status = PassThru->GetSlotNumber (PassThru, RemainingDevicePath, &Slot);
    if (EFI_ERROR (Status)) {
      //
      // Close the I/O Abstraction(s) used to perform the supported test
      //
      gBS->CloseProtocol (
             Controller,
             &gEfiSdMmcPassThruProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
      return Status;
    }
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiSdMmcPassThruProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  return Status;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
SdDxeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                     Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru;
  EFI_DEVICE_PATH_PROTOCOL       *ParentDevicePath;
  SD_DRIVER_PRIVATE_DATA         *Private;
  UINT8                          Slot;

  Private  = NULL;
  PassThru = NULL;
  Status   = gBS->OpenProtocol (
                    Controller,
                    &gEfiSdMmcPassThruProtocolGuid,
                    (VOID **)&PassThru,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  //
  // Check EFI_ALREADY_STARTED to reuse the original SD_DRIVER_PRIVATE_DATA.
  //
  if (Status != EFI_ALREADY_STARTED) {
    Private = AllocateZeroPool (sizeof (SD_DRIVER_PRIVATE_DATA));
    if (Private == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&ParentDevicePath,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    ASSERT_EFI_ERROR (Status);
    Private->PassThru            = PassThru;
    Private->Controller          = Controller;
    Private->ParentDevicePath    = ParentDevicePath;
    Private->DriverBindingHandle = This->DriverBindingHandle;

    Status = gBS->InstallProtocolInterface (
                    &Controller,
                    &gEfiCallerIdGuid,
                    EFI_NATIVE_INTERFACE,
                    Private
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **)&Private,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  }

  if (RemainingDevicePath == NULL) {
    Slot = 0xFF;
    while (TRUE) {
      Status = PassThru->GetNextSlot (PassThru, &Slot);
      if (EFI_ERROR (Status)) {
        //
        // Cannot find more legal slots.
        //
        Status = EFI_SUCCESS;
        break;
      }

      Status = DiscoverSdDevice (Private, Slot);
      if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
        break;
      }
    }
  } else if (!IsDevicePathEnd (RemainingDevicePath)) {
    Status = PassThru->GetSlotNumber (PassThru, RemainingDevicePath, &Slot);
    if (!EFI_ERROR (Status)) {
      Status = DiscoverSdDevice (Private, Slot);
    }
  }

Error:
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiSdMmcPassThruProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    if (Private != NULL) {
      gBS->UninstallMultipleProtocolInterfaces (
             Controller,
             &gEfiCallerIdGuid,
             Private,
             NULL
             );
      FreePool (Private);
    }
  }

  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
SdDxeDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                     Status;
  BOOLEAN                        AllChildrenStopped;
  UINTN                          Index;
  SD_DRIVER_PRIVATE_DATA         *Private;
  SD_DEVICE                      *Device;
  EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru;
  EFI_BLOCK_IO2_PROTOCOL         *BlockIo2;
  EFI_BLOCK_IO_PROTOCOL          *BlockIo;
  LIST_ENTRY                     *Link;
  LIST_ENTRY                     *NextLink;
  SD_REQUEST                     *Request;
  EFI_TPL                        OldTpl;

  if (NumberOfChildren == 0) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **)&Private,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    gBS->UninstallProtocolInterface (
           Controller,
           &gEfiCallerIdGuid,
           Private
           );
    gBS->CloseProtocol (
           Controller,
           &gEfiSdMmcPassThruProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    FreePool (Private);

    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    BlockIo  = NULL;
    BlockIo2 = NULL;
    Status   = gBS->OpenProtocol (
                      ChildHandleBuffer[Index],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **)&BlockIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
    if (EFI_ERROR (Status)) {
      Status = gBS->OpenProtocol (
                      ChildHandleBuffer[Index],
                      &gEfiBlockIo2ProtocolGuid,
                      (VOID **)&BlockIo2,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (EFI_ERROR (Status)) {
        AllChildrenStopped = FALSE;
        continue;
      }
    }

    if (BlockIo != NULL) {
      Device = SD_DEVICE_DATA_FROM_BLKIO (BlockIo);
    } else {
      ASSERT (BlockIo2 != NULL);
      Device = SD_DEVICE_DATA_FROM_BLKIO2 (BlockIo2);
    }

    //
    // Free all on-going async tasks.
    //
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    for (Link = GetFirstNode (&Device->Queue);
         !IsNull (&Device->Queue, Link);
         Link = NextLink)
    {
      NextLink = GetNextNode (&Device->Queue, Link);
      RemoveEntryList (Link);

      Request = SD_REQUEST_FROM_LINK (Link);

      gBS->CloseEvent (Request->Event);
      Request->Token->TransactionStatus = EFI_ABORTED;

      if (Request->IsEnd) {
        gBS->SignalEvent (Request->Token->Event);
      }

      FreePool (Request);
    }

    gBS->RestoreTPL (OldTpl);

    //
    // Close the child handle
    //
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiSdMmcPassThruProtocolGuid,
                    This->DriverBindingHandle,
                    ChildHandleBuffer[Index]
                    );

    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ChildHandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    Device->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Device->BlockIo,
                    &gEfiBlockIo2ProtocolGuid,
                    &Device->BlockIo2,
                    &gEfiEraseBlockProtocolGuid,
                    &Device->EraseBlock,
                    &gEfiDiskInfoProtocolGuid,
                    &Device->DiskInfo,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
      gBS->OpenProtocol (
             Controller,
             &gEfiSdMmcPassThruProtocolGuid,
             (VOID **)&PassThru,
             This->DriverBindingHandle,
             ChildHandleBuffer[Index],
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
    } else {
      FreePool (Device->DevicePath);
      FreeUnicodeStringTable (Device->ControllerNameTable);
      FreePool (Device);
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  The user Entry Point for module SdDxe. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some errors occur when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeSdDxe (
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
             &gSdDxeDriverBinding,
             ImageHandle,
             &gSdDxeComponentName,
             &gSdDxeComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
