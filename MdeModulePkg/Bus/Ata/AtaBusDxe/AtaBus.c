/** @file
  This file implements protocol interfaces for ATA bus driver.

  This file implements protocol interfaces: Driver Binding protocol,
  Block IO protocol and DiskInfo protocol.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "AtaBus.h"

//
// ATA Bus Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gAtaBusDriverBinding = {
  AtaBusDriverBindingSupported,
  AtaBusDriverBindingStart,
  AtaBusDriverBindingStop,
  0x10,
  NULL,
  NULL
};

//
// Template for ATA Child Device.
//
ATA_DEVICE gAtaDeviceTemplate = {
  ATA_DEVICE_SIGNATURE,        // Signature
  NULL,                        // Handle
  {                            // BlockIo
    EFI_BLOCK_IO_PROTOCOL_REVISION,
    NULL,
    AtaBlockIoReset,
    AtaBlockIoReadBlocks,
    AtaBlockIoWriteBlocks,
    AtaBlockIoFlushBlocks
  },
  {                            // BlockIo2
    NULL,
    AtaBlockIoResetEx,
    AtaBlockIoReadBlocksEx,
    AtaBlockIoWriteBlocksEx,
    AtaBlockIoFlushBlocksEx
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
    0,                         // LastBlock
    0,                         // LowestAlignedLba
    1                          // LogicalBlocksPerPhysicalBlock
  },
  {                            // DiskInfo
    EFI_DISK_INFO_IDE_INTERFACE_GUID,
    AtaDiskInfoInquiry,
    AtaDiskInfoIdentify,
    AtaDiskInfoSenseData,
    AtaDiskInfoWhichIde
  },
  NULL,                        // DevicePath
  {
    AtaStorageSecurityReceiveData,
    AtaStorageSecuritySendData
  },
  NULL,                        // AtaBusDriverData
  0,                           // Port
  0,                           // PortMultiplierPort
  { 0, },                      // Packet
  {{ 0}, },                    // Acb
  NULL,                        // Asb
  FALSE,                       // UdmaValid
  FALSE,                       // Lba48Bit
  NULL,                        // IdentifyData
  NULL,                        // ControllerNameTable
  {L'\0', },                   // ModelName
  {NULL, NULL},                // AtaTaskList
  {NULL, NULL},                // AtaSubTaskList
  FALSE                        // Abort
};

/**
  Allocates an aligned buffer for ATA device.

  This function allocates an aligned buffer for the ATA device to perform
  ATA pass through operations. The alignment requirement is from ATA pass
  through interface.

  @param  AtaDevice         The ATA child device involved for the operation.
  @param  BufferSize        The request buffer size.

  @return A pointer to the aligned buffer or NULL if the allocation fails.

**/
VOID *
AllocateAlignedBuffer (
  IN ATA_DEVICE               *AtaDevice,
  IN UINTN                    BufferSize
  )
{
  return AllocateAlignedPages (EFI_SIZE_TO_PAGES (BufferSize), AtaDevice->AtaBusDriverData->AtaPassThru->Mode->IoAlign);
}

/**
  Frees an aligned buffer for ATA device.

  This function frees an aligned buffer for the ATA device to perform
  ATA pass through operations.

  @param  Buffer            The aligned buffer to be freed.
  @param  BufferSize        The request buffer size.

**/
VOID
FreeAlignedBuffer (
  IN VOID                     *Buffer,
  IN UINTN                    BufferSize
  )
{
  if (Buffer != NULL) {
    FreeAlignedPages (Buffer, EFI_SIZE_TO_PAGES (BufferSize));
  }
}


/**
  Release all the resources allocated for the ATA device.

  This function releases all the resources allocated for the ATA device.

  @param  AtaDevice         The ATA child device involved for the operation.

**/
VOID
ReleaseAtaResources (
  IN ATA_DEVICE  *AtaDevice
  )
{
  ATA_BUS_ASYN_SUB_TASK *SubTask;
  ATA_BUS_ASYN_TASK     *AtaTask;
  LIST_ENTRY            *Entry;
  LIST_ENTRY            *DelEntry;
  EFI_TPL               OldTpl;

  FreeUnicodeStringTable (AtaDevice->ControllerNameTable);
  FreeAlignedBuffer (AtaDevice->Asb, sizeof (EFI_ATA_STATUS_BLOCK));
  FreeAlignedBuffer (AtaDevice->IdentifyData, sizeof (ATA_IDENTIFY_DATA));
  if (AtaDevice->DevicePath != NULL) {
    FreePool (AtaDevice->DevicePath);
  }
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (!IsListEmpty (&AtaDevice->AtaSubTaskList)) {
    //
    // Free the Subtask list.
    //
    for(Entry = AtaDevice->AtaSubTaskList.ForwardLink;
        Entry != (&AtaDevice->AtaSubTaskList);
       ) {
      DelEntry = Entry;
      Entry    = Entry->ForwardLink;
      SubTask  = ATA_ASYN_SUB_TASK_FROM_ENTRY (DelEntry);

      RemoveEntryList (DelEntry);
      FreeAtaSubTask (SubTask);
    }
  }
  if (!IsListEmpty (&AtaDevice->AtaTaskList)) {
    //
    // Free the Subtask list.
    //
    for(Entry = AtaDevice->AtaTaskList.ForwardLink;
        Entry != (&AtaDevice->AtaTaskList);
       ) {
      DelEntry = Entry;
      Entry    = Entry->ForwardLink;
      AtaTask  = ATA_ASYN_TASK_FROM_ENTRY (DelEntry);

      RemoveEntryList (DelEntry);
      FreePool (AtaTask);
    }
  }
  gBS->RestoreTPL (OldTpl);
  FreePool (AtaDevice);
}


/**
  Registers an ATA device.

  This function allocates an ATA device structure for the ATA device specified by
  Port and PortMultiplierPort if the ATA device is identified as a valid one.
  Then it will create child handle and install Block IO and Disk Info protocol on
  it.

  @param  AtaBusDriverData      The parent ATA bus driver data structure.
  @param  Port                  The port number of the ATA device.
  @param  PortMultiplierPort    The port multiplier port number of the ATA device.

  @retval EFI_SUCCESS           The ATA device is successfully registered.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to allocate the ATA device
                                and related data structures.
  @return Others                Some error occurs when registering the ATA device.
**/
EFI_STATUS
RegisterAtaDevice (
  IN OUT ATA_BUS_DRIVER_DATA        *AtaBusDriverData,
  IN     UINT16                     Port,
  IN     UINT16                     PortMultiplierPort
  )
{
  EFI_STATUS                        Status;
  ATA_DEVICE                        *AtaDevice;
  EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
  EFI_DEVICE_PATH_PROTOCOL          *NewDevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL          *RemainingDevicePath;
  EFI_HANDLE                        DeviceHandle;

  AtaDevice         = NULL;
  NewDevicePathNode = NULL;
  DevicePath        = NULL;
  RemainingDevicePath = NULL;

  //
  // Build device path
  //
  AtaPassThru = AtaBusDriverData->AtaPassThru;
  Status = AtaPassThru->BuildDevicePath (AtaPassThru, Port, PortMultiplierPort, &NewDevicePathNode);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  DevicePath = AppendDevicePathNode (AtaBusDriverData->ParentDevicePath, NewDevicePathNode);
  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  DeviceHandle = NULL;
  RemainingDevicePath = DevicePath;
  Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &DeviceHandle);
  if (!EFI_ERROR (Status) && (DeviceHandle != NULL) && IsDevicePathEnd(RemainingDevicePath)) {
    Status = EFI_ALREADY_STARTED;
    FreePool (DevicePath);
    goto Done;
  }

  //
  // Allocate ATA device from the template.
  //
  AtaDevice = AllocateCopyPool (sizeof (ATA_DEVICE), &gAtaDeviceTemplate);
  if (AtaDevice == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Initializes ATA device structures and allocates the required buffer.
  //
  AtaDevice->BlockIo.Media      = &AtaDevice->BlockMedia;
  AtaDevice->BlockIo2.Media     = &AtaDevice->BlockMedia;
  AtaDevice->AtaBusDriverData   = AtaBusDriverData;
  AtaDevice->DevicePath         = DevicePath;
  AtaDevice->Port               = Port;
  AtaDevice->PortMultiplierPort = PortMultiplierPort;
  AtaDevice->Asb = AllocateAlignedBuffer (AtaDevice, sizeof (EFI_ATA_STATUS_BLOCK));
  if (AtaDevice->Asb == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  AtaDevice->IdentifyData = AllocateAlignedBuffer (AtaDevice, sizeof (ATA_IDENTIFY_DATA));
  if (AtaDevice->IdentifyData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Initial Ata Task List
  //
  InitializeListHead (&AtaDevice->AtaTaskList);
  InitializeListHead (&AtaDevice->AtaSubTaskList);

  //
  // Report Status Code to indicate the ATA device will be enabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_ENABLE),
    AtaBusDriverData->ParentDevicePath
    );

  //
  // Try to identify the ATA device via the ATA pass through command.
  //
  Status = DiscoverAtaDevice (AtaDevice);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Build controller name for Component Name (2) protocol.
  //
  Status = AddUnicodeString2 (
             "eng",
             gAtaBusComponentName.SupportedLanguages,
             &AtaDevice->ControllerNameTable,
             AtaDevice->ModelName,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = AddUnicodeString2 (
             "en",
             gAtaBusComponentName2.SupportedLanguages,
             &AtaDevice->ControllerNameTable,
             AtaDevice->ModelName,
             FALSE
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Update to AHCI interface GUID based on device path node. The default one
  // is IDE interface GUID copied from template.
  //
  if (NewDevicePathNode->SubType == MSG_SATA_DP) {
    CopyGuid (&AtaDevice->DiskInfo.Interface, &gEfiDiskInfoAhciInterfaceGuid);
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &AtaDevice->Handle,
                  &gEfiDevicePathProtocolGuid,
                  AtaDevice->DevicePath,
                  &gEfiBlockIoProtocolGuid,
                  &AtaDevice->BlockIo,
                  &gEfiBlockIo2ProtocolGuid,
                  &AtaDevice->BlockIo2,
                  &gEfiDiskInfoProtocolGuid,
                  &AtaDevice->DiskInfo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // See if the ata device support trust computing feature or not.
  // If yes, then install Storage Security Protocol at the ata device handle.
  //
  if ((AtaDevice->IdentifyData->trusted_computing_support & BIT0) != 0) {
    DEBUG ((EFI_D_INFO, "Found TCG support in Port %x PortMultiplierPort %x\n", Port, PortMultiplierPort));
    Status = gBS->InstallProtocolInterface (
                    &AtaDevice->Handle,
                    &gEfiStorageSecurityCommandProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &AtaDevice->StorageSecurity
                    );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    DEBUG ((EFI_D_INFO, "Successfully Install Storage Security Protocol on the ATA device\n"));
  }

  gBS->OpenProtocol (
         AtaBusDriverData->Controller,
         &gEfiAtaPassThruProtocolGuid,
         (VOID **) &AtaPassThru,
         AtaBusDriverData->DriverBindingHandle,
         AtaDevice->Handle,
         EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
         );

Done:
  if (NewDevicePathNode != NULL) {
    FreePool (NewDevicePathNode);
  }

  if (EFI_ERROR (Status) && (AtaDevice != NULL)) {
    ReleaseAtaResources (AtaDevice);
    DEBUG ((EFI_D_ERROR | EFI_D_INIT, "Failed to initialize Port %x PortMultiplierPort %x, status = %r\n", Port, PortMultiplierPort, Status));
  }
  return Status;
}


/**
  Unregisters an ATA device.

  This function removes the protocols installed on the controller handle and
  frees the resources allocated for the ATA device.

  @param  This                  The pointer to EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller            The controller handle of the ATA device.
  @param  Handle                The child handle.

  @retval EFI_SUCCESS           The ATA device is successfully unregistered.
  @return Others                Some error occurs when unregistering the ATA device.

**/
EFI_STATUS
UnregisterAtaDevice (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  EFI_HANDLE                     Handle
  )
{
  EFI_STATUS                               Status;
  EFI_BLOCK_IO_PROTOCOL                    *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL                   *BlockIo2;
  ATA_DEVICE                               *AtaDevice;
  EFI_ATA_PASS_THRU_PROTOCOL               *AtaPassThru;
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *StorageSecurity;

  BlockIo2             =     NULL;
  BlockIo              =     NULL;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlockIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    //
    // Locate BlockIo2 protocol
    //
    Status = gBS->OpenProtocol (
                    Handle,
                    &gEfiBlockIo2ProtocolGuid,
                    (VOID **) &BlockIo2,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Get AtaDevice data.
  //
  if (BlockIo != NULL) {
    AtaDevice = ATA_DEVICE_FROM_BLOCK_IO (BlockIo);
  } else {
    ASSERT (BlockIo2 != NULL);
    AtaDevice = ATA_DEVICE_FROM_BLOCK_IO2 (BlockIo2);
  }

  //
  // Close the child handle
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiAtaPassThruProtocolGuid,
         This->DriverBindingHandle,
         Handle
         );

  //
  // The Ata Bus driver installs the BlockIo and BlockIo2 in the DriverBindingStart().
  // Here should uninstall both of them.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  AtaDevice->DevicePath,
                  &gEfiBlockIoProtocolGuid,
                  &AtaDevice->BlockIo,
                  &gEfiBlockIo2ProtocolGuid,
                  &AtaDevice->BlockIo2,
                  &gEfiDiskInfoProtocolGuid,
                  &AtaDevice->DiskInfo,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    gBS->OpenProtocol (
          Controller,
          &gEfiAtaPassThruProtocolGuid,
          (VOID **) &AtaPassThru,
          This->DriverBindingHandle,
          Handle,
          EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
          );
    return Status;
  }

  //
  // If Storage Security Command Protocol is installed, then uninstall this protocol.
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiStorageSecurityCommandProtocolGuid,
                  (VOID **) &StorageSecurity,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    Status = gBS->UninstallProtocolInterface (
                    Handle,
                    &gEfiStorageSecurityCommandProtocolGuid,
                    &AtaDevice->StorageSecurity
                    );
    if (EFI_ERROR (Status)) {
      gBS->OpenProtocol (
        Controller,
        &gEfiAtaPassThruProtocolGuid,
        (VOID **) &AtaPassThru,
        This->DriverBindingHandle,
        Handle,
        EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
        );
      return Status;
    }
  }

  ReleaseAtaResources (AtaDevice);
  return EFI_SUCCESS;
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
AtaBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *ParentDevicePath;
  EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
  UINT16                            Port;
  UINT16                            PortMultiplierPort;

  //
  // Test EFI_ATA_PASS_THRU_PROTOCOL on controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiAtaPassThruProtocolGuid,
                  (VOID **) &AtaPassThru,
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
  // Test to see if this ATA Pass Thru Protocol is for a LOGICAL channel
  //
  if ((AtaPassThru->Mode->Attributes & EFI_ATA_PASS_THRU_ATTRIBUTES_LOGICAL) == 0) {
    //
    // Close the I/O Abstraction(s) used to perform the supported test
    //
    gBS->CloseProtocol (
          Controller,
          &gEfiAtaPassThruProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return EFI_UNSUPPORTED;
  }

  //
  // Test RemainingDevicePath is valid or not.
  //
  if ((RemainingDevicePath != NULL) && !IsDevicePathEnd (RemainingDevicePath)) {
    Status = AtaPassThru->GetDevice (AtaPassThru, RemainingDevicePath, &Port, &PortMultiplierPort);
    if (EFI_ERROR (Status)) {
      //
      // Close the I/O Abstraction(s) used to perform the supported test
      //
      gBS->CloseProtocol (
            Controller,
            &gEfiAtaPassThruProtocolGuid,
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
        &gEfiAtaPassThruProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
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
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
AtaBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
  EFI_DEVICE_PATH_PROTOCOL          *ParentDevicePath;
  ATA_BUS_DRIVER_DATA               *AtaBusDriverData;
  UINT16                            Port;
  UINT16                            PortMultiplierPort;

  AtaBusDriverData = NULL;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Report Status Code to indicate ATA bus starts
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_INIT),
    ParentDevicePath
    );

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiAtaPassThruProtocolGuid,
                  (VOID **) &AtaPassThru,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    goto ErrorExit;
  }

  //
  // Check EFI_ALREADY_STARTED to reuse the original ATA_BUS_DRIVER_DATA.
  //
  if (Status != EFI_ALREADY_STARTED) {
    AtaBusDriverData = AllocateZeroPool (sizeof (ATA_BUS_DRIVER_DATA));
    if (AtaBusDriverData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    AtaBusDriverData->AtaPassThru = AtaPassThru;
    AtaBusDriverData->Controller  = Controller;
    AtaBusDriverData->ParentDevicePath = ParentDevicePath;
    AtaBusDriverData->DriverBindingHandle = This->DriverBindingHandle;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Controller,
                    &gEfiCallerIdGuid,
                    AtaBusDriverData,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

  } else {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **) &AtaBusDriverData,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      AtaBusDriverData = NULL;
      goto ErrorExit;
    }
  }

  //
  // Report Status Code to indicate detecting devices on bus
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_DETECT),
    ParentDevicePath
    );

  if (RemainingDevicePath == NULL) {
    Port = 0xFFFF;
    while (TRUE) {
      Status = AtaPassThru->GetNextPort (AtaPassThru, &Port);
      if (EFI_ERROR (Status)) {
        //
        // We cannot find more legal port then we are done.
        //
        break;
      }

      PortMultiplierPort = 0xFFFF;
      while (TRUE) {
        Status = AtaPassThru->GetNextDevice (AtaPassThru, Port, &PortMultiplierPort);
        if (EFI_ERROR (Status)) {
          //
          // We cannot find more legal port multiplier port number for ATA device
          // on the port, then we are done.
          //
          break;
        }
        RegisterAtaDevice (AtaBusDriverData, Port, PortMultiplierPort);
      }
    }
    Status = EFI_SUCCESS;
  } else if (!IsDevicePathEnd (RemainingDevicePath)) {
    Status = AtaPassThru->GetDevice (AtaPassThru, RemainingDevicePath, &Port, &PortMultiplierPort);
    if (!EFI_ERROR (Status)) {
      Status = RegisterAtaDevice (AtaBusDriverData,Port, PortMultiplierPort);
    }
  }

  return Status;

ErrorExit:

  if (AtaBusDriverData != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           Controller,
           &gEfiCallerIdGuid,
           AtaBusDriverData,
           NULL
           );
    FreePool (AtaBusDriverData);
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiAtaPassThruProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

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
AtaBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  BOOLEAN                     AllChildrenStopped;
  UINTN                       Index;
  ATA_BUS_DRIVER_DATA         *AtaBusDriverData;

  if (NumberOfChildren == 0) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **) &AtaBusDriverData,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      gBS->UninstallMultipleProtocolInterfaces (
            Controller,
            &gEfiCallerIdGuid,
            AtaBusDriverData,
            NULL
            );
      FreePool (AtaBusDriverData);
    }

    gBS->CloseProtocol (
          Controller,
          &gEfiAtaPassThruProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {

    Status = UnregisterAtaDevice (This, Controller, ChildHandleBuffer[Index]);
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

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
AtaBlockIoReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  EFI_STATUS      Status;
  ATA_DEVICE      *AtaDevice;
  EFI_TPL         OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  AtaDevice = ATA_DEVICE_FROM_BLOCK_IO (This);

  Status = ResetAtaDevice (AtaDevice);

  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Read/Write BufferSize bytes from Lba from/into Buffer.

  @param[in]       This       Indicates a pointer to the calling context. Either be
                              block I/O or block I/O2.
  @param[in]       MediaId    The media ID that the read/write request is for.
  @param[in]       Lba        The starting logical block address to be read/written.
                              The caller is responsible for reading/writing to only
                              legitimate locations.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[out]      Buffer     A pointer to the destination/source buffer for the data.
  @param[in]       IsBlockIo2 Indicate the calling is from BlockIO or BlockIO2. TRUE is
                              from BlockIO2, FALSE is for BlockIO.
  @param[in]       IsWrite    Indicates whether it is a write operation.

  @retval EFI_SUCCESS           The data was read/written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be read/written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read/write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read/write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
BlockIoReadWrite (
  IN     VOID                    *This,
  IN     UINT32                  MediaId,
  IN     EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
  IN     UINTN                   BufferSize,
  OUT    VOID                    *Buffer,
  IN     BOOLEAN                 IsBlockIo2,
  IN     BOOLEAN                 IsWrite
  )
{
  ATA_DEVICE                        *AtaDevice;
  EFI_STATUS                        Status;
  EFI_TPL                           OldTpl;
  EFI_BLOCK_IO_MEDIA                *Media;
  UINTN                             BlockSize;
  UINTN                             NumberOfBlocks;
  UINTN                             IoAlign;

  if (IsBlockIo2) {
   Media     = ((EFI_BLOCK_IO2_PROTOCOL *) This)->Media;
   AtaDevice = ATA_DEVICE_FROM_BLOCK_IO2 (This);
  } else {
   Media     = ((EFI_BLOCK_IO_PROTOCOL *) This)->Media;
   AtaDevice = ATA_DEVICE_FROM_BLOCK_IO (This);
  }

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  //
  // Check parameters.
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    if ((Token != NULL) && (Token->Event != NULL)) {
      Token->TransactionStatus = EFI_SUCCESS;
      gBS->SignalEvent (Token->Event);
    }
    return EFI_SUCCESS;
  }

  BlockSize = Media->BlockSize;
  if ((BufferSize % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NumberOfBlocks  = BufferSize / BlockSize;
  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  IoAlign = Media->IoAlign;
  if (IoAlign > 0 && (((UINTN) Buffer & (IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Invoke low level AtaDevice Access Routine.
  //
  Status = AccessAtaDevice (AtaDevice, Buffer, Lba, NumberOfBlocks, IsWrite, Token);

  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
AtaBlockIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
{
  return BlockIoReadWrite ((VOID *) This, MediaId, Lba, NULL, BufferSize, Buffer, FALSE, FALSE);
}


/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
AtaBlockIoWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  return BlockIoReadWrite ((VOID *) This, MediaId, Lba, NULL, BufferSize, Buffer, FALSE, TRUE);
}


/**
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writing back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
AtaBlockIoFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
{
  //
  // return directly
  //
  return EFI_SUCCESS;
}

/**
  Reset the Block Device.

  @param[in]  This                 Indicates a pointer to the calling context.
  @param[in]  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
AtaBlockIoResetEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  EFI_STATUS      Status;
  ATA_DEVICE      *AtaDevice;
  EFI_TPL         OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  AtaDevice = ATA_DEVICE_FROM_BLOCK_IO2 (This);

  AtaTerminateNonBlockingTask (AtaDevice);

  Status = ResetAtaDevice (AtaDevice);

  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Read BufferSize bytes from Lba into Buffer.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    Id of the media, changes every time the media is replaced.
  @param[in]       Lba        The starting Logical Block Address to read from.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[out]      Buffer     A pointer to the destination buffer for the data. The caller is
                              responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The read request was queued if Event is not NULL.
                                The data was read correctly from the device if
                                the Event is NULL.
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
AtaBlockIoReadBlocksEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN  *Token,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
{
  return BlockIoReadWrite ((VOID *) This, MediaId, Lba, Token, BufferSize, Buffer, TRUE, FALSE);
}


/**
  Write BufferSize bytes from Lba into Buffer.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    The media ID that the write request is for.
  @param[in]       Lba        The starting logical block address to be written. The
                              caller is responsible for writing to only legitimate
                              locations.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[in]       Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
AtaBlockIoWriteBlocksEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN  *Token,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  return BlockIoReadWrite ((VOID *) This, MediaId, Lba, Token, BufferSize, Buffer, TRUE, TRUE);
}


/**
  Flush the Block Device.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in, out]  Token      A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writing back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
AtaBlockIoFlushBlocksEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN OUT EFI_BLOCK_IO2_TOKEN  *Token
  )
{
  //
  // Signal event and return directly.
  //
  if (Token != NULL && Token->Event != NULL) {
    Token->TransactionStatus = EFI_SUCCESS;
    gBS->SignalEvent (Token->Event);
  }
  return EFI_SUCCESS;
}
/**
  Provides inquiry information for the controller type.

  This function is used by the IDE bus driver to get inquiry data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This             Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] InquiryData      Pointer to a buffer for the inquiry data.
  @param[in, out] InquiryDataSize  Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class
  @retval EFI_DEVICE_ERROR       Error reading InquiryData from device
  @retval EFI_BUFFER_TOO_SMALL   InquiryDataSize not big enough

**/
EFI_STATUS
EFIAPI
AtaDiskInfoInquiry (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *InquiryData,
  IN OUT UINT32                   *InquiryDataSize
  )
{
  return EFI_NOT_FOUND;
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
AtaDiskInfoIdentify (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *IdentifyData,
  IN OUT UINT32                   *IdentifyDataSize
  )
{
  EFI_STATUS                      Status;
  ATA_DEVICE                      *AtaDevice;

  AtaDevice = ATA_DEVICE_FROM_DISK_INFO (This);

  Status = EFI_BUFFER_TOO_SMALL;
  if (*IdentifyDataSize >= sizeof (ATA_IDENTIFY_DATA)) {
    Status = EFI_SUCCESS;
    CopyMem (IdentifyData, AtaDevice->IdentifyData, sizeof (ATA_IDENTIFY_DATA));
  }
  *IdentifyDataSize = sizeof (ATA_IDENTIFY_DATA);

  return Status;
}


/**
  Provides sense data information for the controller type.

  This function is used by the IDE bus driver to get sense data.
  Data format of Sense data is defined by the Interface GUID.

  @param[in]      This             Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] SenseData        Pointer to the SenseData.
  @param[in, out] SenseDataSize    Size of SenseData in bytes.
  @param[out]     SenseDataNumber  Pointer to the value for the sense data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class.
  @retval EFI_DEVICE_ERROR       Error reading SenseData from device.
  @retval EFI_BUFFER_TOO_SMALL   SenseDataSize not big enough.

**/
EFI_STATUS
EFIAPI
AtaDiskInfoSenseData (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *SenseData,
  IN OUT UINT32                   *SenseDataSize,
  OUT    UINT8                    *SenseDataNumber
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
AtaDiskInfoWhichIde (
  IN  EFI_DISK_INFO_PROTOCOL   *This,
  OUT UINT32                   *IdeChannel,
  OUT UINT32                   *IdeDevice
  )
{
  ATA_DEVICE                   *AtaDevice;

  AtaDevice       = ATA_DEVICE_FROM_DISK_INFO (This);
  *IdeChannel     = AtaDevice->Port;
  *IdeDevice      = AtaDevice->PortMultiplierPort;

  return EFI_SUCCESS;
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

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED RECEIVE commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero.

  If the PayloadBufferSize is zero, the security protocol command is sent using the
  Trusted Non-Data command defined in ATA8-ACS.

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
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
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
AtaStorageSecurityReceiveData (
  IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
  IN UINT32                                   MediaId,
  IN UINT64                                   Timeout,
  IN UINT8                                    SecurityProtocolId,
  IN UINT16                                   SecurityProtocolSpecificData,
  IN UINTN                                    PayloadBufferSize,
  OUT VOID                                    *PayloadBuffer,
  OUT UINTN                                   *PayloadTransferSize
  )
{
  EFI_STATUS                       Status;
  ATA_DEVICE                       *Private;
  EFI_TPL                          OldTpl;

  DEBUG ((EFI_D_INFO, "EFI Storage Security Protocol - Read\n"));
  if ((PayloadBuffer == NULL || PayloadTransferSize == NULL) && PayloadBufferSize != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status  = EFI_SUCCESS;
  Private = ATA_DEVICE_FROM_STORAGE_SECURITY (This);

  if (MediaId != Private->BlockIo.Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (!Private->BlockIo.Media->MediaPresent) {
    return EFI_NO_MEDIA;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = TrustTransferAtaDevice (
             Private,
             PayloadBuffer,
             SecurityProtocolId,
             SecurityProtocolSpecificData,
             PayloadBufferSize,
             FALSE,
             Timeout,
             PayloadTransferSize
             );

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

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED SEND commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero. If the PayloadBufferSize is zero, the security protocol command is
  sent using the Trusted Non-Data command defined in ATA8-ACS.

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
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
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
AtaStorageSecuritySendData (
  IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
  IN UINT32                                   MediaId,
  IN UINT64                                   Timeout,
  IN UINT8                                    SecurityProtocolId,
  IN UINT16                                   SecurityProtocolSpecificData,
  IN UINTN                                    PayloadBufferSize,
  IN VOID                                     *PayloadBuffer
  )
{
  EFI_STATUS                       Status;
  ATA_DEVICE                       *Private;
  EFI_TPL                          OldTpl;

  DEBUG ((EFI_D_INFO, "EFI Storage Security Protocol - Send\n"));
  if ((PayloadBuffer == NULL) && (PayloadBufferSize != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status  = EFI_SUCCESS;
  Private = ATA_DEVICE_FROM_STORAGE_SECURITY (This);

  if (MediaId != Private->BlockIo.Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  Status = TrustTransferAtaDevice (
             Private,
             PayloadBuffer,
             SecurityProtocolId,
             SecurityProtocolSpecificData,
             PayloadBufferSize,
             TRUE,
             Timeout,
             NULL
             );

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  The user Entry Point for module AtaBus. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeAtaBus(
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
             &gAtaBusDriverBinding,
             ImageHandle,
             &gAtaBusComponentName,
             &gAtaBusComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

