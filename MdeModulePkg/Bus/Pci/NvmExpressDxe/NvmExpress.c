/** @file
  NvmExpressDxe driver is used to manage non-volatile memory subsystem which follows
  NVM Express specification.

  Copyright (c) 2013 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvmExpress.h"

//
// NVM Express Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL  gNvmExpressDriverBinding = {
  NvmExpressDriverBindingSupported,
  NvmExpressDriverBindingStart,
  NvmExpressDriverBindingStop,
  0x10,
  NULL,
  NULL
};

//
// NVM Express EFI Driver Supported EFI Version Protocol Instance
//
EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL  gNvmExpressDriverSupportedEfiVersion = {
  sizeof (EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL), // Size of Protocol structure.
  0                                                   // Version number to be filled at start up.
};

//
// Template for NVM Express Pass Thru Mode data structure.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_NVM_EXPRESS_PASS_THRU_MODE  gEfiNvmExpressPassThruMode = {
  EFI_NVM_EXPRESS_PASS_THRU_ATTRIBUTES_PHYSICAL   |
  EFI_NVM_EXPRESS_PASS_THRU_ATTRIBUTES_LOGICAL    |
  EFI_NVM_EXPRESS_PASS_THRU_ATTRIBUTES_NONBLOCKIO |
  EFI_NVM_EXPRESS_PASS_THRU_ATTRIBUTES_CMD_SET_NVM,
  sizeof (UINTN),
  0x10100
};

/**
  Check if the specified Nvm Express device namespace is active, and create child handles
  for them with BlockIo and DiskInfo protocol instances.

  @param[in] Private         The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] NamespaceId     The NVM Express namespace ID  for which a device path node is to be
                             allocated and built. Caller must set the NamespaceId to zero if the
                             device path node will contain a valid UUID.

  @retval EFI_SUCCESS        All the namespaces in the device are successfully enumerated.
  @return Others             Some error occurs when enumerating the namespaces.

**/
EFI_STATUS
EnumerateNvmeDevNamespace (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                           NamespaceId
  )
{
  NVME_ADMIN_NAMESPACE_DATA  *NamespaceData;
  EFI_DEVICE_PATH_PROTOCOL   *NewDevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_HANDLE                 DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL   *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath;
  NVME_DEVICE_PRIVATE_DATA   *Device;
  EFI_STATUS                 Status;
  UINT32                     Lbads;
  UINT32                     Flbas;
  UINT32                     LbaFmtIdx;
  UINT8                      Sn[21];
  UINT8                      Mn[41];
  VOID                       *DummyInterface;

  NewDevicePathNode = NULL;
  DevicePath        = NULL;
  Device            = NULL;

  //
  // Allocate a buffer for Identify Namespace data
  //
  NamespaceData = AllocateZeroPool (sizeof (NVME_ADMIN_NAMESPACE_DATA));
  if (NamespaceData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ParentDevicePath = Private->ParentDevicePath;
  //
  // Identify Namespace
  //
  Status = NvmeIdentifyNamespace (
             Private,
             NamespaceId,
             (VOID *)NamespaceData
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Validate Namespace
  //
  if (NamespaceData->Ncap == 0) {
    Status = EFI_DEVICE_ERROR;
  } else {
    //
    // allocate device private data for each discovered namespace
    //
    Device = AllocateZeroPool (sizeof (NVME_DEVICE_PRIVATE_DATA));
    if (Device == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    //
    // Initialize SSD namespace instance data
    //
    Device->Signature     = NVME_DEVICE_PRIVATE_DATA_SIGNATURE;
    Device->NamespaceId   = NamespaceId;
    Device->NamespaceUuid = NamespaceData->Eui64;

    Device->ControllerHandle    = Private->ControllerHandle;
    Device->DriverBindingHandle = Private->DriverBindingHandle;
    Device->Controller          = Private;

    //
    // Build BlockIo media structure
    //
    Device->Media.MediaId          = 0;
    Device->Media.RemovableMedia   = FALSE;
    Device->Media.MediaPresent     = TRUE;
    Device->Media.LogicalPartition = FALSE;
    Device->Media.ReadOnly         = FALSE;
    Device->Media.WriteCaching     = FALSE;
    Device->Media.IoAlign          = Private->PassThruMode.IoAlign;

    Flbas                   = NamespaceData->Flbas;
    LbaFmtIdx               = Flbas & 0xF;
    Lbads                   = NamespaceData->LbaFormat[LbaFmtIdx].Lbads;
    Device->Media.BlockSize = (UINT32)1 << Lbads;

    Device->Media.LastBlock                     = NamespaceData->Nsze - 1;
    Device->Media.LogicalBlocksPerPhysicalBlock = 1;
    Device->Media.LowestAlignedLba              = 1;

    //
    // Create BlockIo Protocol instance
    //
    Device->BlockIo.Revision    = EFI_BLOCK_IO_PROTOCOL_REVISION2;
    Device->BlockIo.Media       = &Device->Media;
    Device->BlockIo.Reset       = NvmeBlockIoReset;
    Device->BlockIo.ReadBlocks  = NvmeBlockIoReadBlocks;
    Device->BlockIo.WriteBlocks = NvmeBlockIoWriteBlocks;
    Device->BlockIo.FlushBlocks = NvmeBlockIoFlushBlocks;

    //
    // Create BlockIo2 Protocol instance
    //
    Device->BlockIo2.Media         = &Device->Media;
    Device->BlockIo2.Reset         = NvmeBlockIoResetEx;
    Device->BlockIo2.ReadBlocksEx  = NvmeBlockIoReadBlocksEx;
    Device->BlockIo2.WriteBlocksEx = NvmeBlockIoWriteBlocksEx;
    Device->BlockIo2.FlushBlocksEx = NvmeBlockIoFlushBlocksEx;
    InitializeListHead (&Device->AsyncQueue);

    //
    // Create StorageSecurityProtocol Instance
    //
    Device->StorageSecurity.ReceiveData = NvmeStorageSecurityReceiveData;
    Device->StorageSecurity.SendData    = NvmeStorageSecuritySendData;

    //
    // Create DiskInfo Protocol instance
    //
    CopyMem (&Device->NamespaceData, NamespaceData, sizeof (NVME_ADMIN_NAMESPACE_DATA));
    InitializeDiskInfo (Device);

    //
    // Create a Nvm Express Namespace Device Path Node
    //
    Status = Private->Passthru.BuildDevicePath (
                                 &Private->Passthru,
                                 Device->NamespaceId,
                                 &NewDevicePathNode
                                 );

    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    //
    // Append the SSD node to the controller's device path
    //
    DevicePath = AppendDevicePathNode (ParentDevicePath, NewDevicePathNode);
    if (DevicePath == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    DeviceHandle        = NULL;
    RemainingDevicePath = DevicePath;
    Status              = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &DeviceHandle);
    if (!EFI_ERROR (Status) && (DeviceHandle != NULL) && IsDevicePathEnd (RemainingDevicePath)) {
      Status = EFI_ALREADY_STARTED;
      FreePool (DevicePath);
      goto Exit;
    }

    Device->DevicePath = DevicePath;

    //
    // Make sure the handle is NULL so we create a new handle
    //
    Device->DeviceHandle = NULL;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Device->DeviceHandle,
                    &gEfiDevicePathProtocolGuid,
                    Device->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Device->BlockIo,
                    &gEfiBlockIo2ProtocolGuid,
                    &Device->BlockIo2,
                    &gEfiDiskInfoProtocolGuid,
                    &Device->DiskInfo,
                    NULL
                    );

    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    //
    // Check if the NVMe controller supports the Security Send and Security Receive commands
    //
    if ((Private->ControllerData->Oacs & SECURITY_SEND_RECEIVE_SUPPORTED) != 0) {
      Status = gBS->InstallProtocolInterface (
                      &Device->DeviceHandle,
                      &gEfiStorageSecurityCommandProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &Device->StorageSecurity
                      );
      if (EFI_ERROR (Status)) {
        gBS->UninstallMultipleProtocolInterfaces (
               Device->DeviceHandle,
               &gEfiDevicePathProtocolGuid,
               Device->DevicePath,
               &gEfiBlockIoProtocolGuid,
               &Device->BlockIo,
               &gEfiBlockIo2ProtocolGuid,
               &Device->BlockIo2,
               &gEfiDiskInfoProtocolGuid,
               &Device->DiskInfo,
               NULL
               );
        goto Exit;
      }
    }

    gBS->OpenProtocol (
           Private->ControllerHandle,
           &gEfiNvmExpressPassThruProtocolGuid,
           (VOID **)&DummyInterface,
           Private->DriverBindingHandle,
           Device->DeviceHandle,
           EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
           );

    //
    // Dump NvmExpress Identify Namespace Data
    //
    DEBUG ((DEBUG_INFO, " == NVME IDENTIFY NAMESPACE [%d] DATA ==\n", NamespaceId));
    DEBUG ((DEBUG_INFO, "    NSZE        : 0x%x\n", NamespaceData->Nsze));
    DEBUG ((DEBUG_INFO, "    NCAP        : 0x%x\n", NamespaceData->Ncap));
    DEBUG ((DEBUG_INFO, "    NUSE        : 0x%x\n", NamespaceData->Nuse));
    DEBUG ((DEBUG_INFO, "    LBAF0.LBADS : 0x%x\n", (NamespaceData->LbaFormat[0].Lbads)));

    //
    // Build controller name for Component Name (2) protocol.
    //
    CopyMem (Sn, Private->ControllerData->Sn, sizeof (Private->ControllerData->Sn));
    Sn[20] = 0;
    CopyMem (Mn, Private->ControllerData->Mn, sizeof (Private->ControllerData->Mn));
    Mn[40] = 0;
    UnicodeSPrintAsciiFormat (Device->ModelName, sizeof (Device->ModelName), "%a-%a-%x", Sn, Mn, NamespaceData->Eui64);

    AddUnicodeString2 (
      "eng",
      gNvmExpressComponentName.SupportedLanguages,
      &Device->ControllerNameTable,
      Device->ModelName,
      TRUE
      );

    AddUnicodeString2 (
      "en",
      gNvmExpressComponentName2.SupportedLanguages,
      &Device->ControllerNameTable,
      Device->ModelName,
      FALSE
      );
  }

Exit:
  if (NamespaceData != NULL) {
    FreePool (NamespaceData);
  }

  if (NewDevicePathNode != NULL) {
    FreePool (NewDevicePathNode);
  }

  if (EFI_ERROR (Status) && (Device != NULL) && (Device->DevicePath != NULL)) {
    FreePool (Device->DevicePath);
  }

  if (EFI_ERROR (Status) && (Device != NULL)) {
    FreePool (Device);
  }

  return Status;
}

/**
  Discover all Nvm Express device namespaces, and create child handles for them with BlockIo
  and DiskInfo protocol instances.

  @param[in] Private         The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS        All the namespaces in the device are successfully enumerated.
  @return Others             Some error occurs when enumerating the namespaces.

**/
EFI_STATUS
DiscoverAllNamespaces (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                          Status;
  UINT32                              NamespaceId;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  *Passthru;

  NamespaceId = 0xFFFFFFFF;
  Passthru    = &Private->Passthru;

  while (TRUE) {
    Status = Passthru->GetNextNamespace (
                         Passthru,
                         (UINT32 *)&NamespaceId
                         );

    if (EFI_ERROR (Status)) {
      break;
    }

    Status = EnumerateNvmeDevNamespace (
               Private,
               NamespaceId
               );

    if (EFI_ERROR (Status)) {
      continue;
    }
  }

  return EFI_SUCCESS;
}

/**
  Unregisters a Nvm Express device namespace.

  This function removes the protocols installed on the controller handle and
  frees the resources allocated for the namespace.

  @param  This                  The pointer to EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller            The controller handle of the namespace.
  @param  Handle                The child handle.

  @retval EFI_SUCCESS           The namespace is successfully unregistered.
  @return Others                Some error occurs when unregistering the namespace.

**/
EFI_STATUS
UnregisterNvmeNamespace (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  EFI_HANDLE                   Handle
  )
{
  EFI_STATUS                             Status;
  EFI_BLOCK_IO_PROTOCOL                  *BlockIo;
  NVME_DEVICE_PRIVATE_DATA               *Device;
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL  *StorageSecurity;
  BOOLEAN                                IsEmpty;
  EFI_TPL                                OldTpl;
  VOID                                   *DummyInterface;

  BlockIo = NULL;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Device = NVME_DEVICE_PRIVATE_DATA_FROM_BLOCK_IO (BlockIo);

  //
  // Wait for the device's asynchronous I/O queue to become empty.
  //
  while (TRUE) {
    OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);
    IsEmpty = IsListEmpty (&Device->AsyncQueue);
    gBS->RestoreTPL (OldTpl);

    if (IsEmpty) {
      break;
    }

    gBS->Stall (100);
  }

  //
  // Close the child handle
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiNvmExpressPassThruProtocolGuid,
         This->DriverBindingHandle,
         Handle
         );

  //
  // The Nvm Express driver installs the BlockIo and DiskInfo in the DriverBindingStart().
  // Here should uninstall both of them.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  Device->DevicePath,
                  &gEfiBlockIoProtocolGuid,
                  &Device->BlockIo,
                  &gEfiBlockIo2ProtocolGuid,
                  &Device->BlockIo2,
                  &gEfiDiskInfoProtocolGuid,
                  &Device->DiskInfo,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    gBS->OpenProtocol (
           Controller,
           &gEfiNvmExpressPassThruProtocolGuid,
           (VOID **)&DummyInterface,
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
                  (VOID **)&StorageSecurity,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    Status = gBS->UninstallProtocolInterface (
                    Handle,
                    &gEfiStorageSecurityCommandProtocolGuid,
                    &Device->StorageSecurity
                    );
    if (EFI_ERROR (Status)) {
      gBS->OpenProtocol (
             Controller,
             &gEfiNvmExpressPassThruProtocolGuid,
             (VOID **)&DummyInterface,
             This->DriverBindingHandle,
             Handle,
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
      return Status;
    }
  }

  if (Device->DevicePath != NULL) {
    FreePool (Device->DevicePath);
  }

  if (Device->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (Device->ControllerNameTable);
  }

  FreePool (Device);

  return EFI_SUCCESS;
}

/**
  Call back function when the timer event is signaled.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the
                        Event.

**/
VOID
EFIAPI
ProcessAsyncTaskList (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NVME_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  NVME_CQ                       *Cq;
  UINT16                        QueueId;
  UINT32                        Data;
  LIST_ENTRY                    *Link;
  LIST_ENTRY                    *NextLink;
  NVME_PASS_THRU_ASYNC_REQ      *AsyncRequest;
  NVME_BLKIO2_SUBTASK           *Subtask;
  NVME_BLKIO2_REQUEST           *BlkIo2Request;
  EFI_BLOCK_IO2_TOKEN           *Token;
  BOOLEAN                       HasNewItem;
  EFI_STATUS                    Status;

  Private    = (NVME_CONTROLLER_PRIVATE_DATA *)Context;
  QueueId    = 2;
  Cq         = Private->CqBuffer[QueueId] + Private->CqHdbl[QueueId].Cqh;
  HasNewItem = FALSE;
  PciIo      = Private->PciIo;

  //
  // Submit asynchronous subtasks to the NVMe Submission Queue
  //
  for (Link = GetFirstNode (&Private->UnsubmittedSubtasks);
       !IsNull (&Private->UnsubmittedSubtasks, Link);
       Link = NextLink)
  {
    NextLink      = GetNextNode (&Private->UnsubmittedSubtasks, Link);
    Subtask       = NVME_BLKIO2_SUBTASK_FROM_LINK (Link);
    BlkIo2Request = Subtask->BlockIo2Request;
    Token         = BlkIo2Request->Token;
    RemoveEntryList (Link);
    BlkIo2Request->UnsubmittedSubtaskNum--;

    //
    // If any previous subtask fails, do not process subsequent ones.
    //
    if (Token->TransactionStatus != EFI_SUCCESS) {
      if (IsListEmpty (&BlkIo2Request->SubtasksQueue) &&
          BlkIo2Request->LastSubtaskSubmitted &&
          (BlkIo2Request->UnsubmittedSubtaskNum == 0))
      {
        //
        // Remove the BlockIo2 request from the device asynchronous queue.
        //
        RemoveEntryList (&BlkIo2Request->Link);
        FreePool (BlkIo2Request);
        gBS->SignalEvent (Token->Event);
      }

      FreePool (Subtask->CommandPacket->NvmeCmd);
      FreePool (Subtask->CommandPacket->NvmeCompletion);
      FreePool (Subtask->CommandPacket);
      FreePool (Subtask);

      continue;
    }

    Status = Private->Passthru.PassThru (
                                 &Private->Passthru,
                                 Subtask->NamespaceId,
                                 Subtask->CommandPacket,
                                 Subtask->Event
                                 );
    if (Status == EFI_NOT_READY) {
      InsertHeadList (&Private->UnsubmittedSubtasks, Link);
      BlkIo2Request->UnsubmittedSubtaskNum++;
      break;
    } else if (EFI_ERROR (Status)) {
      Token->TransactionStatus = EFI_DEVICE_ERROR;

      if (IsListEmpty (&BlkIo2Request->SubtasksQueue) &&
          Subtask->IsLast)
      {
        //
        // Remove the BlockIo2 request from the device asynchronous queue.
        //
        RemoveEntryList (&BlkIo2Request->Link);
        FreePool (BlkIo2Request);
        gBS->SignalEvent (Token->Event);
      }

      FreePool (Subtask->CommandPacket->NvmeCmd);
      FreePool (Subtask->CommandPacket->NvmeCompletion);
      FreePool (Subtask->CommandPacket);
      FreePool (Subtask);
    } else {
      InsertTailList (&BlkIo2Request->SubtasksQueue, Link);
      if (Subtask->IsLast) {
        BlkIo2Request->LastSubtaskSubmitted = TRUE;
      }
    }
  }

  while (Cq->Pt != Private->Pt[QueueId]) {
    ASSERT (Cq->Sqid == QueueId);

    HasNewItem = TRUE;

    //
    // Find the command with given Command Id.
    //
    for (Link = GetFirstNode (&Private->AsyncPassThruQueue);
         !IsNull (&Private->AsyncPassThruQueue, Link);
         Link = NextLink)
    {
      NextLink     = GetNextNode (&Private->AsyncPassThruQueue, Link);
      AsyncRequest = NVME_PASS_THRU_ASYNC_REQ_FROM_THIS (Link);
      if (AsyncRequest->CommandId == Cq->Cid) {
        //
        // Copy the Respose Queue entry for this command to the callers
        // response buffer.
        //
        CopyMem (
          AsyncRequest->Packet->NvmeCompletion,
          Cq,
          sizeof (EFI_NVM_EXPRESS_COMPLETION)
          );

        //
        // Free the resources allocated before cmd submission
        //
        if (AsyncRequest->MapData != NULL) {
          PciIo->Unmap (PciIo, AsyncRequest->MapData);
        }

        if (AsyncRequest->MapMeta != NULL) {
          PciIo->Unmap (PciIo, AsyncRequest->MapMeta);
        }

        if (AsyncRequest->MapPrpList != NULL) {
          PciIo->Unmap (PciIo, AsyncRequest->MapPrpList);
        }

        if (AsyncRequest->PrpListHost != NULL) {
          PciIo->FreeBuffer (
                   PciIo,
                   AsyncRequest->PrpListNo,
                   AsyncRequest->PrpListHost
                   );
        }

        RemoveEntryList (Link);
        gBS->SignalEvent (AsyncRequest->CallerEvent);
        FreePool (AsyncRequest);

        //
        // Update submission queue head.
        //
        Private->AsyncSqHead = Cq->Sqhd;
        break;
      }
    }

    Private->CqHdbl[QueueId].Cqh++;
    if (Private->CqHdbl[QueueId].Cqh > MIN (NVME_ASYNC_CCQ_SIZE, Private->Cap.Mqes)) {
      Private->CqHdbl[QueueId].Cqh = 0;
      Private->Pt[QueueId]        ^= 1;
    }

    Cq = Private->CqBuffer[QueueId] + Private->CqHdbl[QueueId].Cqh;
  }

  if (HasNewItem) {
    Data = ReadUnaligned32 ((UINT32 *)&Private->CqHdbl[QueueId]);
    PciIo->Mem.Write (
                 PciIo,
                 EfiPciIoWidthUint32,
                 NVME_BAR,
                 NVME_CQHDBL_OFFSET (QueueId, Private->Cap.Dstrd),
                 1,
                 &Data
                 );
  }
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
NvmExpressDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEV_PATH_PTR          DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT8                     ClassCode[3];

  //
  // Check whether device path is valid
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
      DevicePathNode.DevPath = RemainingDevicePath;

      if ((DevicePathNode.DevPath->Type    != MESSAGING_DEVICE_PATH) ||
          (DevicePathNode.DevPath->SubType != MSG_NVME_NAMESPACE_DP) ||
          (DevicePathNodeLength (DevicePathNode.DevPath) != sizeof (NVME_NAMESPACE_DEVICE_PATH)))
      {
        return EFI_UNSUPPORTED;
      }
    }
  }

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
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
  // Close protocol, don't use device path protocol in the Support() function
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Attempt to Open PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
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
  // Now further check the PCI header: Base class (offset 0x0B) and Sub Class (offset 0x0A).
  // This controller should be a Nvm Express controller.
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (ClassCode),
                        ClassCode
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Examine Nvm Express controller PCI Configuration table fields
  //
  if ((ClassCode[0] != PCI_IF_NVMHCI) || (ClassCode[1] != PCI_CLASS_MASS_STORAGE_NVM) || (ClassCode[2] != PCI_CLASS_MASS_STORAGE)) {
    Status = EFI_UNSUPPORTED;
  }

Done:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
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
NvmExpressDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_PCI_IO_PROTOCOL                 *PciIo;
  NVME_CONTROLLER_PRIVATE_DATA        *Private;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;
  UINT32                              NamespaceId;
  EFI_PHYSICAL_ADDRESS                MappedAddr;
  UINTN                               Bytes;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  *Passthru;

  DEBUG ((DEBUG_INFO, "NvmExpressDriverBindingStart: start\n"));

  Private          = NULL;
  Passthru         = NULL;
  ParentDevicePath = NULL;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  //
  // Check EFI_ALREADY_STARTED to reuse the original NVME_CONTROLLER_PRIVATE_DATA.
  //
  if (Status != EFI_ALREADY_STARTED) {
    Private = AllocateZeroPool (sizeof (NVME_CONTROLLER_PRIVATE_DATA));

    if (Private == NULL) {
      DEBUG ((DEBUG_ERROR, "NvmExpressDriverBindingStart: allocating pool for Nvme Private Data failed!\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    //
    // 6 x 4kB aligned buffers will be carved out of this buffer.
    // 1st 4kB boundary is the start of the admin submission queue.
    // 2nd 4kB boundary is the start of the admin completion queue.
    // 3rd 4kB boundary is the start of I/O submission queue #1.
    // 4th 4kB boundary is the start of I/O completion queue #1.
    // 5th 4kB boundary is the start of I/O submission queue #2.
    // 6th 4kB boundary is the start of I/O completion queue #2.
    //
    // Allocate 6 pages of memory, then map it for bus master read and write.
    //
    Status = PciIo->AllocateBuffer (
                      PciIo,
                      AllocateAnyPages,
                      EfiBootServicesData,
                      6,
                      (VOID **)&Private->Buffer,
                      0
                      );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Bytes  = EFI_PAGES_TO_SIZE (6);
    Status = PciIo->Map (
                      PciIo,
                      EfiPciIoOperationBusMasterCommonBuffer,
                      Private->Buffer,
                      &Bytes,
                      &MappedAddr,
                      &Private->Mapping
                      );

    if (EFI_ERROR (Status) || (Bytes != EFI_PAGES_TO_SIZE (6))) {
      goto Exit;
    }

    Private->BufferPciAddr = (UINT8 *)(UINTN)MappedAddr;

    Private->Signature                 = NVME_CONTROLLER_PRIVATE_DATA_SIGNATURE;
    Private->ControllerHandle          = Controller;
    Private->ImageHandle               = This->DriverBindingHandle;
    Private->DriverBindingHandle       = This->DriverBindingHandle;
    Private->PciIo                     = PciIo;
    Private->ParentDevicePath          = ParentDevicePath;
    Private->Passthru.Mode             = &Private->PassThruMode;
    Private->Passthru.PassThru         = NvmExpressPassThru;
    Private->Passthru.GetNextNamespace = NvmExpressGetNextNamespace;
    Private->Passthru.BuildDevicePath  = NvmExpressBuildDevicePath;
    Private->Passthru.GetNamespace     = NvmExpressGetNamespace;
    CopyMem (&Private->PassThruMode, &gEfiNvmExpressPassThruMode, sizeof (EFI_NVM_EXPRESS_PASS_THRU_MODE));
    InitializeListHead (&Private->AsyncPassThruQueue);
    InitializeListHead (&Private->UnsubmittedSubtasks);

    Status = NvmeControllerInit (Private);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    //
    // Start the asynchronous I/O completion monitor
    //
    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    ProcessAsyncTaskList,
                    Private,
                    &Private->TimerEvent
                    );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = gBS->SetTimer (
                    Private->TimerEvent,
                    TimerPeriodic,
                    NVME_HC_ASYNC_TIMER
                    );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Controller,
                    &gEfiNvmExpressPassThruProtocolGuid,
                    &Private->Passthru,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    NvmeRegisterShutdownNotification ();
  } else {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiNvmExpressPassThruProtocolGuid,
                    (VOID **)&Passthru,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Private = NVME_CONTROLLER_PRIVATE_DATA_FROM_PASS_THRU (Passthru);
  }

  if (RemainingDevicePath == NULL) {
    //
    // Enumerate all NVME namespaces in the controller
    //
    Status = DiscoverAllNamespaces (
               Private
               );
  } else if (!IsDevicePathEnd (RemainingDevicePath)) {
    //
    // Enumerate the specified NVME namespace
    //
    Status = Private->Passthru.GetNamespace (
                                 &Private->Passthru,
                                 RemainingDevicePath,
                                 &NamespaceId
                                 );

    if (!EFI_ERROR (Status)) {
      Status = EnumerateNvmeDevNamespace (
                 Private,
                 NamespaceId
                 );
    }
  }

  DEBUG ((DEBUG_INFO, "NvmExpressDriverBindingStart: end successfully\n"));
  return EFI_SUCCESS;

Exit:
  if ((Private != NULL) && (Private->Mapping != NULL)) {
    PciIo->Unmap (PciIo, Private->Mapping);
  }

  if ((Private != NULL) && (Private->Buffer != NULL)) {
    PciIo->FreeBuffer (PciIo, 6, Private->Buffer);
  }

  if ((Private != NULL) && (Private->ControllerData != NULL)) {
    FreePool (Private->ControllerData);
  }

  if (Private != NULL) {
    if (Private->TimerEvent != NULL) {
      gBS->CloseEvent (Private->TimerEvent);
    }

    FreePool (Private);
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  DEBUG ((DEBUG_INFO, "NvmExpressDriverBindingStart: end with %r\n", Status));

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
NvmExpressDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                          Status;
  BOOLEAN                             AllChildrenStopped;
  UINTN                               Index;
  NVME_CONTROLLER_PRIVATE_DATA        *Private;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  *PassThru;
  BOOLEAN                             IsEmpty;
  EFI_TPL                             OldTpl;

  if (NumberOfChildren == 0) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiNvmExpressPassThruProtocolGuid,
                    (VOID **)&PassThru,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );

    if (!EFI_ERROR (Status)) {
      Private = NVME_CONTROLLER_PRIVATE_DATA_FROM_PASS_THRU (PassThru);

      //
      // Wait for the asynchronous PassThru queue to become empty.
      //
      while (TRUE) {
        OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);
        IsEmpty = IsListEmpty (&Private->AsyncPassThruQueue) &&
                  IsListEmpty (&Private->UnsubmittedSubtasks);
        gBS->RestoreTPL (OldTpl);

        if (IsEmpty) {
          break;
        }

        gBS->Stall (100);
      }

      gBS->UninstallMultipleProtocolInterfaces (
             Controller,
             &gEfiNvmExpressPassThruProtocolGuid,
             PassThru,
             NULL
             );

      if (Private->TimerEvent != NULL) {
        gBS->CloseEvent (Private->TimerEvent);
      }

      if (Private->Mapping != NULL) {
        Private->PciIo->Unmap (Private->PciIo, Private->Mapping);
      }

      if (Private->Buffer != NULL) {
        Private->PciIo->FreeBuffer (Private->PciIo, 6, Private->Buffer);
      }

      FreePool (Private->ControllerData);
      FreePool (Private);
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    NvmeUnregisterShutdownNotification ();

    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = UnregisterNvmeNamespace (This, Controller, ChildHandleBuffer[Index]);
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
  This is the unload handle for the NVM Express driver.

  Disconnect the driver specified by ImageHandle from the NVMe device in the handle database.
  Uninstall all the protocols installed in the driver.

  @param[in]  ImageHandle       The drivers' driver image.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
NvmExpressUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *DeviceHandleBuffer;
  UINTN                         DeviceHandleCount;
  UINTN                         Index;
  EFI_COMPONENT_NAME_PROTOCOL   *ComponentName;
  EFI_COMPONENT_NAME2_PROTOCOL  *ComponentName2;

  //
  // Get the list of the device handles managed by this driver.
  // If there is an error getting the list, then means the driver
  // doesn't manage any device. At this way, we would only close
  // those protocols installed at image handle.
  //
  DeviceHandleBuffer = NULL;
  Status             = gBS->LocateHandleBuffer (
                              ByProtocol,
                              &gEfiNvmExpressPassThruProtocolGuid,
                              NULL,
                              &DeviceHandleCount,
                              &DeviceHandleBuffer
                              );

  if (!EFI_ERROR (Status)) {
    //
    // Disconnect the driver specified by ImageHandle from all
    // the devices in the handle database.
    //
    for (Index = 0; Index < DeviceHandleCount; Index++) {
      Status = gBS->DisconnectController (
                      DeviceHandleBuffer[Index],
                      ImageHandle,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        goto EXIT;
      }
    }
  }

  //
  // Uninstall all the protocols installed in the driver entry point
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gNvmExpressDriverBinding,
                  &gEfiDriverSupportedEfiVersionProtocolGuid,
                  &gNvmExpressDriverSupportedEfiVersion,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Note we have to one by one uninstall the following protocols.
  // It's because some of them are optionally installed based on
  // the following PCD settings.
  //   gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnosticsDisable
  //   gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable
  //   gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable
  //   gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable
  //
  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiComponentNameProtocolGuid,
                  (VOID **)&ComponentName
                  );
  if (!EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
           ImageHandle,
           &gEfiComponentNameProtocolGuid,
           ComponentName
           );
  }

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **)&ComponentName2
                  );
  if (!EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
           ImageHandle,
           &gEfiComponentName2ProtocolGuid,
           ComponentName2
           );
  }

  Status = EFI_SUCCESS;

EXIT:
  //
  // Free the buffer containing the list of handles from the handle database
  //
  if (DeviceHandleBuffer != NULL) {
    gBS->FreePool (DeviceHandleBuffer);
  }

  return Status;
}

/**
  The entry point for Nvm Express driver, used to install Nvm Express driver on the ImageHandle.

  @param  ImageHandle   The firmware allocated handle for this driver image.
  @param  SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS   Driver loaded.
  @retval other         Driver not loaded.

**/
EFI_STATUS
EFIAPI
NvmExpressDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gNvmExpressDriverBinding,
             ImageHandle,
             &gNvmExpressComponentName,
             &gNvmExpressComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Install EFI Driver Supported EFI Version Protocol required for
  // EFI drivers that are on PCI and other plug in cards.
  //
  gNvmExpressDriverSupportedEfiVersion.FirmwareVersion = 0x00020028;
  Status                                               = gBS->InstallMultipleProtocolInterfaces (
                                                                &ImageHandle,
                                                                &gEfiDriverSupportedEfiVersionProtocolGuid,
                                                                &gNvmExpressDriverSupportedEfiVersion,
                                                                NULL
                                                                );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
