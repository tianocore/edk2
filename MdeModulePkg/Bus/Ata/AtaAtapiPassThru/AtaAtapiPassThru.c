/** @file
  This file implements ATA_PASSTHRU_PROTOCOL and EXT_SCSI_PASSTHRU_PROTOCOL interfaces
  for managed ATA controllers.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AtaAtapiPassThru.h"

//
//  EFI_DRIVER_BINDING_PROTOCOL instance
//
EFI_DRIVER_BINDING_PROTOCOL  gAtaAtapiPassThruDriverBinding = {
  AtaAtapiPassThruSupported,
  AtaAtapiPassThruStart,
  AtaAtapiPassThruStop,
  0x10,
  NULL,
  NULL
};

ATA_ATAPI_PASS_THRU_INSTANCE  gAtaAtapiPassThruInstanceTemplate = {
  ATA_ATAPI_PASS_THRU_SIGNATURE,
  0,                  // Controller Handle
  NULL,               // PciIo Protocol
  NULL,               // IdeControllerInit Protocol
  {                   // AtaPassThruMode
    //
    // According to UEFI2.3 spec Section 12.10, Drivers for non-RAID ATA controllers should set
    // both EFI_ATA_PASS_THRU_ATTRIBUTES_PHYSICAL and EFI_ATA_PASS_THRU_ATTRIBUTES_LOGICAL
    // bits.
    // Note that the driver doesn't support AtaPassThru non blocking I/O.
    //
    EFI_ATA_PASS_THRU_ATTRIBUTES_PHYSICAL | EFI_ATA_PASS_THRU_ATTRIBUTES_LOGICAL | EFI_ATA_PASS_THRU_ATTRIBUTES_NONBLOCKIO,
    //
    // IoAlign
    //
    sizeof (UINTN)
  },
  {                   // AtaPassThru
    NULL,
    AtaPassThruPassThru,
    AtaPassThruGetNextPort,
    AtaPassThruGetNextDevice,
    AtaPassThruBuildDevicePath,
    AtaPassThruGetDevice,
    AtaPassThruResetPort,
    AtaPassThruResetDevice
  },
  {                   // ExtScsiPassThruMode
    //
    // AdapterId
    //
    0,
    //
    // According to UEFI2.3 spec Section 14.7, Drivers for non-RAID SCSI controllers should set
    // both EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL and EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL
    // bits.
    // Note that the driver doesn't support ExtScsiPassThru non blocking I/O.
    //
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL | EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL,
    //
    // IoAlign
    //
    sizeof (UINTN)
  },
  {                   // ExtScsiPassThru
    NULL,
    ExtScsiPassThruPassThru,
    ExtScsiPassThruGetNextTargetLun,
    ExtScsiPassThruBuildDevicePath,
    ExtScsiPassThruGetTargetLun,
    ExtScsiPassThruResetChannel,
    ExtScsiPassThruResetTargetLun,
    ExtScsiPassThruGetNextTarget
  },
  EfiAtaUnknownMode,  // Work Mode
  {                   // IdeRegisters
    { 0 },
    { 0 }
  },
  {                   // AhciRegisters
    0
  },
  {                   // DeviceList
    NULL,
    NULL
  },
  0,                  // EnabledPciAttributes
  0,                  // OriginalAttributes
  0,                  // PreviousPort
  0,                  // PreviousPortMultiplier
  0,                  // PreviousTargetId
  0,                  // PreviousLun
  NULL,               // Timer event
  {                   // NonBlocking TaskList
    NULL,
    NULL
  }
};

ATAPI_DEVICE_PATH  mAtapiDevicePathTemplate = {
  {
    MESSAGING_DEVICE_PATH,
    MSG_ATAPI_DP,
    {
      (UINT8)(sizeof (ATAPI_DEVICE_PATH)),
      (UINT8)((sizeof (ATAPI_DEVICE_PATH)) >> 8)
    }
  },
  0,
  0,
  0
};

SATA_DEVICE_PATH  mSataDevicePathTemplate = {
  {
    MESSAGING_DEVICE_PATH,
    MSG_SATA_DP,
    {
      (UINT8)(sizeof (SATA_DEVICE_PATH)),
      (UINT8)((sizeof (SATA_DEVICE_PATH)) >> 8)
    }
  },
  0,
  0,
  0
};

UINT8  mScsiId[TARGET_MAX_BYTES] = {
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF
};

EDKII_ATA_ATAPI_POLICY_PROTOCOL  *mAtaAtapiPolicy;
EDKII_ATA_ATAPI_POLICY_PROTOCOL  mDefaultAtaAtapiPolicy = {
  EDKII_ATA_ATAPI_POLICY_VERSION,
  2,  // PuisEnable
  0,  // DeviceSleepEnable
  0,  // AggressiveDeviceSleepEnable
  0   // Reserved
};

/**
  Sends an ATA command to an ATA device that is attached to the ATA controller. This function
  supports both blocking I/O and non-blocking I/O. The blocking I/O functionality is required,
  and the non-blocking I/O functionality is optional.

  @param[in]      Port               The port number of the ATA device to send the command.
  @param[in]      PortMultiplierPort The port multiplier port number of the ATA device to send the command.
                                     If there is no port multiplier, then specify 0xFFFF.
  @param[in, out] Packet             A pointer to the ATA command to send to the ATA device specified by Port
                                     and PortMultiplierPort.
  @param[in]      Instance           Pointer to the ATA_ATAPI_PASS_THRU_INSTANCE.
  @param[in]      Task               Optional. Pointer to the ATA_NONBLOCK_TASK
                                     used by non-blocking mode.

  @retval EFI_SUCCESS                The ATA command was sent by the host. For
                                     bi-directional commands, InTransferLength bytes
                                     were transferred from InDataBuffer. For
                                     write and bi-directional commands, OutTransferLength
                                     bytes were transferred by OutDataBuffer.
  @retval EFI_BAD_BUFFER_SIZE        The ATA command was not executed. The number
                                     of bytes that could be transferred is returned
                                     in InTransferLength. For write and bi-directional
                                     commands, OutTransferLength bytes were transferred
                                     by OutDataBuffer.
  @retval EFI_NOT_READY              The ATA command could not be sent because
                                     there are too many ATA commands already
                                     queued. The caller may retry again later.
  @retval EFI_DEVICE_ERROR           A device error occurred while attempting
                                     to send the ATA command.
  @retval EFI_INVALID_PARAMETER      Port, PortMultiplierPort, or the contents
                                     of Acb are invalid. The ATA command was
                                     not sent, so no additional status information
                                     is available.

**/
EFI_STATUS
EFIAPI
AtaPassThruPassThruExecute (
  IN     UINT16                            Port,
  IN     UINT16                            PortMultiplierPort,
  IN OUT EFI_ATA_PASS_THRU_COMMAND_PACKET  *Packet,
  IN     ATA_ATAPI_PASS_THRU_INSTANCE      *Instance,
  IN     ATA_NONBLOCK_TASK                 *Task OPTIONAL
  )
{
  EFI_ATA_PASS_THRU_CMD_PROTOCOL  Protocol;
  EFI_ATA_HC_WORK_MODE            Mode;
  EFI_STATUS                      Status;

  Protocol = Packet->Protocol;

  Mode = Instance->Mode;
  switch (Mode) {
    case EfiAtaIdeMode:
      //
      // Reassign IDE mode io port registers' base addresses
      //
      Status = GetIdeRegisterIoAddr (Instance->PciIo, Instance->IdeRegisters);

      if (EFI_ERROR (Status)) {
        return Status;
      }

      switch (Protocol) {
        case EFI_ATA_PASS_THRU_PROTOCOL_ATA_NON_DATA:
          Status = AtaNonDataCommandIn (
                     Instance->PciIo,
                     &Instance->IdeRegisters[Port],
                     Packet->Acb,
                     Packet->Asb,
                     Packet->Timeout,
                     Task
                     );
          break;
        case EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN:
          Status = AtaPioDataInOut (
                     Instance->PciIo,
                     &Instance->IdeRegisters[Port],
                     Packet->InDataBuffer,
                     Packet->InTransferLength,
                     TRUE,
                     Packet->Acb,
                     Packet->Asb,
                     Packet->Timeout,
                     Task
                     );
          break;
        case EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT:
          Status = AtaPioDataInOut (
                     Instance->PciIo,
                     &Instance->IdeRegisters[Port],
                     Packet->OutDataBuffer,
                     Packet->OutTransferLength,
                     FALSE,
                     Packet->Acb,
                     Packet->Asb,
                     Packet->Timeout,
                     Task
                     );
          break;
        case EFI_ATA_PASS_THRU_PROTOCOL_UDMA_DATA_IN:
          Status = AtaUdmaInOut (
                     Instance,
                     &Instance->IdeRegisters[Port],
                     TRUE,
                     Packet->InDataBuffer,
                     Packet->InTransferLength,
                     Packet->Acb,
                     Packet->Asb,
                     Packet->Timeout,
                     Task
                     );
          break;
        case EFI_ATA_PASS_THRU_PROTOCOL_UDMA_DATA_OUT:
          Status = AtaUdmaInOut (
                     Instance,
                     &Instance->IdeRegisters[Port],
                     FALSE,
                     Packet->OutDataBuffer,
                     Packet->OutTransferLength,
                     Packet->Acb,
                     Packet->Asb,
                     Packet->Timeout,
                     Task
                     );
          break;
        default:
          return EFI_UNSUPPORTED;
      }

      break;
    case EfiAtaAhciMode:
      if (PortMultiplierPort == 0xFFFF) {
        //
        // If there is no port multiplier, PortMultiplierPort will be 0xFFFF
        // according to UEFI spec. Here, we convert its value to 0 to follow
        // AHCI spec.
        //
        PortMultiplierPort = 0;
      }

      switch (Protocol) {
        case EFI_ATA_PASS_THRU_PROTOCOL_ATA_NON_DATA:
          Status = AhciNonDataTransfer (
                     Instance->PciIo,
                     &Instance->AhciRegisters,
                     (UINT8)Port,
                     (UINT8)PortMultiplierPort,
                     NULL,
                     0,
                     Packet->Acb,
                     Packet->Asb,
                     Packet->Timeout,
                     Task
                     );
          break;
        case EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN:
          Status = AhciPioTransfer (
                     Instance->PciIo,
                     &Instance->AhciRegisters,
                     (UINT8)Port,
                     (UINT8)PortMultiplierPort,
                     NULL,
                     0,
                     TRUE,
                     Packet->Acb,
                     Packet->Asb,
                     Packet->InDataBuffer,
                     Packet->InTransferLength,
                     Packet->Timeout,
                     Task
                     );
          break;
        case EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT:
          Status = AhciPioTransfer (
                     Instance->PciIo,
                     &Instance->AhciRegisters,
                     (UINT8)Port,
                     (UINT8)PortMultiplierPort,
                     NULL,
                     0,
                     FALSE,
                     Packet->Acb,
                     Packet->Asb,
                     Packet->OutDataBuffer,
                     Packet->OutTransferLength,
                     Packet->Timeout,
                     Task
                     );
          break;
        case EFI_ATA_PASS_THRU_PROTOCOL_UDMA_DATA_IN:
          Status = AhciDmaTransfer (
                     Instance,
                     &Instance->AhciRegisters,
                     (UINT8)Port,
                     (UINT8)PortMultiplierPort,
                     NULL,
                     0,
                     TRUE,
                     Packet->Acb,
                     Packet->Asb,
                     Packet->InDataBuffer,
                     Packet->InTransferLength,
                     Packet->Timeout,
                     Task
                     );
          break;
        case EFI_ATA_PASS_THRU_PROTOCOL_UDMA_DATA_OUT:
          Status = AhciDmaTransfer (
                     Instance,
                     &Instance->AhciRegisters,
                     (UINT8)Port,
                     (UINT8)PortMultiplierPort,
                     NULL,
                     0,
                     FALSE,
                     Packet->Acb,
                     Packet->Asb,
                     Packet->OutDataBuffer,
                     Packet->OutTransferLength,
                     Packet->Timeout,
                     Task
                     );
          break;
        default:
          return EFI_UNSUPPORTED;
      }

      break;

    default:
      Status = EFI_DEVICE_ERROR;
      break;
  }

  return Status;
}

/**
  Call back function when the timer event is signaled.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the
                        Event.

**/
VOID
EFIAPI
AsyncNonBlockingTransferRoutine (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  LIST_ENTRY                    *Entry;
  LIST_ENTRY                    *EntryHeader;
  ATA_NONBLOCK_TASK             *Task;
  EFI_STATUS                    Status;
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;

  Instance    = (ATA_ATAPI_PASS_THRU_INSTANCE *)Context;
  EntryHeader = &Instance->NonBlockingTaskList;
  //
  // Get the Tasks from the Tasks List and execute it, until there is
  // no task in the list or the device is busy with task (EFI_NOT_READY).
  //
  while (TRUE) {
    if (!IsListEmpty (EntryHeader)) {
      Entry = GetFirstNode (EntryHeader);
      Task  = ATA_NON_BLOCK_TASK_FROM_ENTRY (Entry);
    } else {
      return;
    }

    Status = AtaPassThruPassThruExecute (
               Task->Port,
               Task->PortMultiplier,
               Task->Packet,
               Instance,
               Task
               );

    //
    // If the data transfer meet a error, remove all tasks in the list since these tasks are
    // associated with one task from Ata Bus and signal the event with error status.
    //
    if ((Status != EFI_NOT_READY) && (Status != EFI_SUCCESS)) {
      DestroyAsynTaskList (Instance, TRUE);
      break;
    }

    //
    // For Non blocking mode, the Status of EFI_NOT_READY means the operation
    // is not finished yet. Otherwise the operation is successful.
    //
    if (Status == EFI_NOT_READY) {
      break;
    } else {
      RemoveEntryList (&Task->Link);
      gBS->SignalEvent (Task->Event);
      FreePool (Task);
    }
  }
}

/**
  The Entry Point of module.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeAtaAtapiPassThru (
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
             &gAtaAtapiPassThruDriverBinding,
             ImageHandle,
             &gAtaAtapiPassThruComponentName,
             &gAtaAtapiPassThruComponentName2
             );
  ASSERT_EFI_ERROR (Status);

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
  Because ControllerHandle may have been previously started by the same driver, if a protocol is
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
AtaAtapiPassThruSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *ParentDevicePath;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  PCI_TYPE00                        PciData;
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeControllerInit;

  //
  // SATA Controller is a device driver, and should ignore the
  // "RemainingDevicePath" according to UEFI spec
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID *)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    //
    // EFI_ALREADY_STARTED is also an error
    //
    return Status;
  }

  //
  // Close the protocol because we don't use it here
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIdeControllerInitProtocolGuid,
                  (VOID **)&IdeControllerInit,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    //
    // EFI_ALREADY_STARTED is also an error
    //
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiIdeControllerInitProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Now test the EfiPciIoProtocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Now further check the PCI header: Base class (offset 0x0B) and
  // Sub Class (offset 0x0A). This controller should be an ATA controller
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (PciData.Hdr.ClassCode),
                        PciData.Hdr.ClassCode
                        );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (IS_PCI_IDE (&PciData) || IS_PCI_SATADPA (&PciData)) {
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
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
AtaAtapiPassThruStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeControllerInit;
  ATA_ATAPI_PASS_THRU_INSTANCE      *Instance;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UINT64                            EnabledPciAttributes;
  UINT64                            OriginalPciAttributes;

  Status                = EFI_SUCCESS;
  IdeControllerInit     = NULL;
  Instance              = NULL;
  OriginalPciAttributes = 0;

  DEBUG ((DEBUG_INFO, "==AtaAtapiPassThru Start== Controller = %x\n", Controller));

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIdeControllerInitProtocolGuid,
                  (VOID **)&IdeControllerInit,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Open Ide_Controller_Init Error, Status=%r", Status));
    goto ErrorExit;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get Pci_Io Protocol Error, Status=%r", Status));
    goto ErrorExit;
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    &OriginalPciAttributes
                    );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &EnabledPciAttributes
                    );
  if (!EFI_ERROR (Status)) {
    EnabledPciAttributes &= (UINT64)EFI_PCI_DEVICE_ENABLE;
    Status                = PciIo->Attributes (
                                     PciIo,
                                     EfiPciIoAttributeOperationEnable,
                                     EnabledPciAttributes,
                                     NULL
                                     );
  }

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->LocateProtocol (&gEdkiiAtaAtapiPolicyProtocolGuid, NULL, (VOID **)&mAtaAtapiPolicy);
  if (EFI_ERROR (Status)) {
    //
    // If there is no AtaAtapiPolicy exposed, use the default policy.
    //
    mAtaAtapiPolicy = &mDefaultAtaAtapiPolicy;
  }

  //
  // Allocate a buffer to store the ATA_ATAPI_PASS_THRU_INSTANCE data structure
  //
  Instance = AllocateCopyPool (sizeof (ATA_ATAPI_PASS_THRU_INSTANCE), &gAtaAtapiPassThruInstanceTemplate);
  if (Instance == NULL) {
    goto ErrorExit;
  }

  Instance->ControllerHandle      = Controller;
  Instance->IdeControllerInit     = IdeControllerInit;
  Instance->PciIo                 = PciIo;
  Instance->EnabledPciAttributes  = EnabledPciAttributes;
  Instance->OriginalPciAttributes = OriginalPciAttributes;
  Instance->AtaPassThru.Mode      = &Instance->AtaPassThruMode;
  Instance->ExtScsiPassThru.Mode  = &Instance->ExtScsiPassThruMode;
  InitializeListHead (&Instance->DeviceList);
  InitializeListHead (&Instance->NonBlockingTaskList);

  Instance->TimerEvent = NULL;

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  AsyncNonBlockingTransferRoutine,
                  Instance,
                  &Instance->TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Set 1ms timer.
  //
  Status = gBS->SetTimer (Instance->TimerEvent, TimerPeriodic, 10000);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Enumerate all inserted ATA devices.
  //
  Status = EnumerateAttachedDevice (Instance);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiAtaPassThruProtocolGuid,
                  &(Instance->AtaPassThru),
                  &gEfiExtScsiPassThruProtocolGuid,
                  &(Instance->ExtScsiPassThru),
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;

ErrorExit:
  if (IdeControllerInit != NULL) {
    gBS->CloseProtocol (
           Controller,
           &gEfiIdeControllerInitProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  if ((Instance != NULL) && (Instance->TimerEvent != NULL)) {
    gBS->CloseEvent (Instance->TimerEvent);
  }

  if (Instance != NULL) {
    //
    // Remove all inserted ATA devices.
    //
    DestroyDeviceInfoList (Instance);
    FreePool (Instance);
  }

  return EFI_UNSUPPORTED;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
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
AtaAtapiPassThruStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                    Status;
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  EFI_AHCI_REGISTERS            *AhciRegisters;

  DEBUG ((DEBUG_INFO, "==AtaAtapiPassThru Stop== Controller = %x\n", Controller));

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiAtaPassThruProtocolGuid,
                  (VOID **)&AtaPassThru,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (AtaPassThru);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiAtaPassThruProtocolGuid,
                  &(Instance->AtaPassThru),
                  &gEfiExtScsiPassThruProtocolGuid,
                  &(Instance->ExtScsiPassThru),
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Close protocols opened by AtaAtapiPassThru controller driver
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiIdeControllerInitProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Close Non-Blocking timer and free Task list.
  //
  if (Instance->TimerEvent != NULL) {
    gBS->CloseEvent (Instance->TimerEvent);
    Instance->TimerEvent = NULL;
  }

  DestroyAsynTaskList (Instance, FALSE);
  //
  // Free allocated resource
  //
  DestroyDeviceInfoList (Instance);

  PciIo = Instance->PciIo;

  //
  // Disable this ATA host controller.
  //
  PciIo->Attributes (
           PciIo,
           EfiPciIoAttributeOperationDisable,
           Instance->EnabledPciAttributes,
           NULL
           );

  //
  // If the current working mode is AHCI mode, then pre-allocated resource
  // for AHCI initialization should be released.
  //
  if (Instance->Mode == EfiAtaAhciMode) {
    AhciRegisters = &Instance->AhciRegisters;
    PciIo->Unmap (
             PciIo,
             AhciRegisters->MapCommandTable
             );
    PciIo->FreeBuffer (
             PciIo,
             EFI_SIZE_TO_PAGES ((UINTN)AhciRegisters->MaxCommandTableSize),
             AhciRegisters->AhciCommandTable
             );
    PciIo->Unmap (
             PciIo,
             AhciRegisters->MapCmdList
             );
    PciIo->FreeBuffer (
             PciIo,
             EFI_SIZE_TO_PAGES ((UINTN)AhciRegisters->MaxCommandListSize),
             AhciRegisters->AhciCmdList
             );
    PciIo->Unmap (
             PciIo,
             AhciRegisters->MapRFis
             );
    PciIo->FreeBuffer (
             PciIo,
             EFI_SIZE_TO_PAGES ((UINTN)AhciRegisters->MaxReceiveFisSize),
             AhciRegisters->AhciRFis
             );
  }

  //
  // Restore original PCI attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSet,
                    Instance->OriginalPciAttributes,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  FreePool (Instance);

  return Status;
}

/**
  Traverse the attached ATA devices list to find out the device to access.

  @param[in]  Instance            A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.
  @param[in]  Port                The port number of the ATA device to send the command.
  @param[in]  PortMultiplierPort  The port multiplier port number of the ATA device to send the command.
                                  If there is no port multiplier, then specify 0xFFFF.
  @param[in]  DeviceType          The device type of the ATA device.

  @retval     The pointer to the data structure of the device info to access.

**/
LIST_ENTRY *
EFIAPI
SearchDeviceInfoList (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplier,
  IN  EFI_ATA_DEVICE_TYPE           DeviceType
  )
{
  EFI_ATA_DEVICE_INFO  *DeviceInfo;
  LIST_ENTRY           *Node;

  Node = GetFirstNode (&Instance->DeviceList);
  while (!IsNull (&Instance->DeviceList, Node)) {
    DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

    //
    // For CD-ROM working in the AHCI mode, only 8 bits are used to record
    // the PortMultiplier information. If the CD-ROM is directly attached
    // on a SATA port, the PortMultiplier should be translated from 0xFF
    // to 0xFFFF according to the UEFI spec.
    //
    if ((Instance->Mode == EfiAtaAhciMode) &&
        (DeviceInfo->Type == EfiIdeCdrom) &&
        (PortMultiplier == 0xFF))
    {
      PortMultiplier = 0xFFFF;
    }

    if ((DeviceInfo->Type == DeviceType) &&
        (Port == DeviceInfo->Port) &&
        (PortMultiplier == DeviceInfo->PortMultiplier))
    {
      return Node;
    }

    Node = GetNextNode (&Instance->DeviceList, Node);
  }

  return NULL;
}

/**
  Allocate device info data structure to contain device info.
  And insert the data structure to the tail of device list for tracing.

  @param[in]  Instance            A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.
  @param[in]  Port                The port number of the ATA device to send the command.
  @param[in]  PortMultiplierPort  The port multiplier port number of the ATA device to send the command.
                                  If there is no port multiplier, then specify 0xFFFF.
  @param[in]  DeviceType          The device type of the ATA device.
  @param[in]  IdentifyData        The data buffer to store the output of the IDENTIFY cmd.

  @retval EFI_SUCCESS             Successfully insert the ata device to the tail of device list.
  @retval EFI_OUT_OF_RESOURCES    Can not allocate enough resource for use.

**/
EFI_STATUS
EFIAPI
CreateNewDeviceInfo (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplier,
  IN  EFI_ATA_DEVICE_TYPE           DeviceType,
  IN  EFI_IDENTIFY_DATA             *IdentifyData
  )
{
  EFI_ATA_DEVICE_INFO  *DeviceInfo;

  DeviceInfo = AllocateZeroPool (sizeof (EFI_ATA_DEVICE_INFO));

  if (DeviceInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DeviceInfo->Signature      = ATA_ATAPI_DEVICE_SIGNATURE;
  DeviceInfo->Port           = Port;
  DeviceInfo->PortMultiplier = PortMultiplier;
  DeviceInfo->Type           = DeviceType;

  if (IdentifyData != NULL) {
    DeviceInfo->IdentifyData = AllocateCopyPool (sizeof (EFI_IDENTIFY_DATA), IdentifyData);
    if (DeviceInfo->IdentifyData == NULL) {
      FreePool (DeviceInfo);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  InsertTailList (&Instance->DeviceList, &DeviceInfo->Link);

  return EFI_SUCCESS;
}

/**
  Destroy all attached ATA devices info.

  @param[in]  Instance          A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.

**/
VOID
EFIAPI
DestroyDeviceInfoList (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance
  )
{
  EFI_ATA_DEVICE_INFO  *DeviceInfo;
  LIST_ENTRY           *Node;

  Node = GetFirstNode (&Instance->DeviceList);
  while (!IsNull (&Instance->DeviceList, Node)) {
    DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

    Node = GetNextNode (&Instance->DeviceList, Node);

    RemoveEntryList (&DeviceInfo->Link);
    if (DeviceInfo->IdentifyData != NULL) {
      FreePool (DeviceInfo->IdentifyData);
    }

    FreePool (DeviceInfo);
  }
}

/**
  Destroy all pending non blocking tasks.

  @param[in]  Instance    A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.
  @param[in]  IsSigEvent  Indicate whether signal the task event when remove the
                          task.

**/
VOID
EFIAPI
DestroyAsynTaskList (
  IN ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN BOOLEAN                       IsSigEvent
  )
{
  LIST_ENTRY         *Entry;
  LIST_ENTRY         *DelEntry;
  ATA_NONBLOCK_TASK  *Task;
  EFI_TPL            OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (!IsListEmpty (&Instance->NonBlockingTaskList)) {
    //
    // Free the Subtask list.
    //
    for (Entry = (&Instance->NonBlockingTaskList)->ForwardLink;
         Entry != (&Instance->NonBlockingTaskList);
         )
    {
      DelEntry = Entry;
      Entry    = Entry->ForwardLink;
      Task     = ATA_NON_BLOCK_TASK_FROM_ENTRY (DelEntry);

      RemoveEntryList (DelEntry);
      if (IsSigEvent) {
        Task->Packet->Asb->AtaStatus = 0x01;
        gBS->SignalEvent (Task->Event);
      }

      FreePool (Task);
    }
  }

  gBS->RestoreTPL (OldTpl);
}

/**
  Enumerate all attached ATA devices at IDE mode or AHCI mode separately.

  The function is designed to enumerate all attached ATA devices.

  @param[in]  Instance          A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.

  @retval EFI_SUCCESS           Successfully enumerate attached ATA devices.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
EnumerateAttachedDevice (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance
  )
{
  EFI_STATUS  Status;
  PCI_TYPE00  PciData;
  UINT8       ClassCode;

  Status = EFI_SUCCESS;

  Status = Instance->PciIo->Pci.Read (
                                  Instance->PciIo,
                                  EfiPciIoWidthUint8,
                                  PCI_CLASSCODE_OFFSET,
                                  sizeof (PciData.Hdr.ClassCode),
                                  PciData.Hdr.ClassCode
                                  );
  ASSERT_EFI_ERROR (Status);

  ClassCode = PciData.Hdr.ClassCode[1];

  switch (ClassCode) {
    case PCI_CLASS_MASS_STORAGE_IDE:
      //
      // The ATA controller is working at IDE mode
      //
      Instance->Mode = EfiAtaIdeMode;

      Status = IdeModeInitialization (Instance);
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto Done;
      }

      break;
    case PCI_CLASS_MASS_STORAGE_SATADPA:
      //
      // The ATA controller is working at AHCI mode
      //
      Instance->Mode = EfiAtaAhciMode;

      Status = AhciModeInitialization (Instance);

      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto Done;
      }

      break;
    default:
      Status = EFI_UNSUPPORTED;
  }

Done:
  return Status;
}

/**
  Sends an ATA command to an ATA device that is attached to the ATA controller. This function
  supports both blocking I/O and non-blocking I/O. The blocking I/O functionality is required,
  and the non-blocking I/O functionality is optional.

  @param[in]      This               A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]      Port               The port number of the ATA device to send the command.
  @param[in]      PortMultiplierPort The port multiplier port number of the ATA device to send the command.
                                     If there is no port multiplier, then specify 0xFFFF.
  @param[in, out] Packet             A pointer to the ATA command to send to the ATA device specified by Port
                                     and PortMultiplierPort.
  @param[in]      Event               If non-blocking I/O is not supported then Event is ignored, and blocking
                                     I/O is performed. If Event is NULL, then blocking I/O is performed. If
                                     Event is not NULL and non blocking I/O is supported, then non-blocking
                                     I/O is performed, and Event will be signaled when the ATA command completes.

  @retval EFI_SUCCESS                The ATA command was sent by the host. For bi-directional commands,
                                     InTransferLength bytes were transferred from InDataBuffer. For write and
                                     bi-directional commands, OutTransferLength bytes were transferred by OutDataBuffer.
  @retval EFI_BAD_BUFFER_SIZE        The ATA command was not executed. The number of bytes that could be transferred
                                     is returned in InTransferLength. For write and bi-directional commands,
                                     OutTransferLength bytes were transferred by OutDataBuffer.
  @retval EFI_NOT_READY              The ATA command could not be sent because there are too many ATA commands
                                     already queued. The caller may retry again later.
  @retval EFI_DEVICE_ERROR           A device error occurred while attempting to send the ATA command.
  @retval EFI_INVALID_PARAMETER      Port, PortMultiplierPort, or the contents of Acb are invalid. The ATA
                                     command was not sent, so no additional status information is available.

**/
EFI_STATUS
EFIAPI
AtaPassThruPassThru (
  IN     EFI_ATA_PASS_THRU_PROTOCOL        *This,
  IN     UINT16                            Port,
  IN     UINT16                            PortMultiplierPort,
  IN OUT EFI_ATA_PASS_THRU_COMMAND_PACKET  *Packet,
  IN     EFI_EVENT                         Event OPTIONAL
  )
{
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;
  EFI_ATA_DEVICE_INFO           *DeviceInfo;
  EFI_IDENTIFY_DATA             *IdentifyData;
  UINT64                        Capacity;
  UINT32                        MaxSectorCount;
  ATA_NONBLOCK_TASK             *Task;
  EFI_TPL                       OldTpl;
  UINT32                        BlockSize;

  Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  if ((This->Mode->IoAlign > 1) && !ADDRESS_IS_ALIGNED (Packet->InDataBuffer, This->Mode->IoAlign)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((This->Mode->IoAlign > 1) && !ADDRESS_IS_ALIGNED (Packet->OutDataBuffer, This->Mode->IoAlign)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((This->Mode->IoAlign > 1) && !ADDRESS_IS_ALIGNED (Packet->Asb, This->Mode->IoAlign)) {
    return EFI_INVALID_PARAMETER;
  }

  Node = SearchDeviceInfoList (Instance, Port, PortMultiplierPort, EfiIdeHarddisk);

  if (Node == NULL) {
    Node = SearchDeviceInfoList (Instance, Port, PortMultiplierPort, EfiIdeCdrom);
    if (Node == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Check whether this device needs 48-bit addressing (ATAPI-6 ata device).
  // Per ATA-6 spec, word83: bit15 is zero and bit14 is one.
  // If bit10 is one, it means the ata device support 48-bit addressing.
  //
  DeviceInfo     = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);
  IdentifyData   = DeviceInfo->IdentifyData;
  MaxSectorCount = 0x100;
  if ((IdentifyData->AtaData.command_set_supported_83 & (BIT10 | BIT15 | BIT14)) == 0x4400) {
    Capacity = *((UINT64 *)IdentifyData->AtaData.maximum_lba_for_48bit_addressing);
    if (Capacity > 0xFFFFFFF) {
      //
      // Capacity exceeds 120GB. 48-bit addressing is really needed
      // In this case, the max sector count is 0x10000
      //
      MaxSectorCount = 0x10000;
    }
  }

  BlockSize = 0x200;
  if ((IdentifyData->AtaData.phy_logic_sector_support & (BIT14 | BIT15)) == BIT14) {
    //
    // Check logical block size
    //
    if ((IdentifyData->AtaData.phy_logic_sector_support & BIT12) != 0) {
      BlockSize = (UINT32)(((UINT32)(IdentifyData->AtaData.logic_sector_size_hi << 16) | IdentifyData->AtaData.logic_sector_size_lo) * sizeof (UINT16));
    }
  }

  //
  // convert the transfer length from sector count to byte.
  //
  if (((Packet->Length & EFI_ATA_PASS_THRU_LENGTH_BYTES) == 0) &&
      (Packet->InTransferLength != 0))
  {
    Packet->InTransferLength = Packet->InTransferLength * BlockSize;
  }

  //
  // convert the transfer length from sector count to byte.
  //
  if (((Packet->Length & EFI_ATA_PASS_THRU_LENGTH_BYTES) == 0) &&
      (Packet->OutTransferLength != 0))
  {
    Packet->OutTransferLength = Packet->OutTransferLength * BlockSize;
  }

  //
  // If the data buffer described by InDataBuffer/OutDataBuffer and InTransferLength/OutTransferLength
  // is too big to be transferred in a single command, then no data is transferred and EFI_BAD_BUFFER_SIZE
  // is returned.
  //
  if (((Packet->InTransferLength != 0) && (Packet->InTransferLength > MaxSectorCount * BlockSize)) ||
      ((Packet->OutTransferLength != 0) && (Packet->OutTransferLength > MaxSectorCount * BlockSize)))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // For non-blocking mode, queue the Task into the list.
  //
  if (Event != NULL) {
    Task = AllocateZeroPool (sizeof (ATA_NONBLOCK_TASK));
    if (Task == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Task->Signature      = ATA_NONBLOCKING_TASK_SIGNATURE;
    Task->Port           = Port;
    Task->PortMultiplier = PortMultiplierPort;
    Task->Packet         = Packet;
    Task->Event          = Event;
    Task->IsStart        = FALSE;
    Task->RetryTimes     = DivU64x32 (Packet->Timeout, 1000) + 1;
    if (Packet->Timeout == 0) {
      Task->InfiniteWait = TRUE;
    } else {
      Task->InfiniteWait = FALSE;
    }

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    InsertTailList (&Instance->NonBlockingTaskList, &Task->Link);
    gBS->RestoreTPL (OldTpl);

    return EFI_SUCCESS;
  } else {
    return AtaPassThruPassThruExecute (
             Port,
             PortMultiplierPort,
             Packet,
             Instance,
             NULL
             );
  }
}

/**
  Used to retrieve the list of legal port numbers for ATA devices on an ATA controller.
  These can either be the list of ports where ATA devices are actually present or the
  list of legal port numbers for the ATA controller. Regardless, the caller of this
  function must probe the port number returned to see if an ATA device is actually
  present at that location on the ATA controller.

  The GetNextPort() function retrieves the port number on an ATA controller. If on input
  Port is 0xFFFF, then the port number of the first port on the ATA controller is returned
  in Port and EFI_SUCCESS is returned.

  If Port is a port number that was returned on a previous call to GetNextPort(), then the
  port number of the next port on the ATA controller is returned in Port, and EFI_SUCCESS
  is returned. If Port is not 0xFFFF and Port was not returned on a previous call to
  GetNextPort(), then EFI_INVALID_PARAMETER is returned.

  If Port is the port number of the last port on the ATA controller, then EFI_NOT_FOUND is
  returned.

  @param[in]      This          A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in, out] Port          On input, a pointer to the port number on the ATA controller.
                                On output, a pointer to the next port number on the ATA
                                controller. An input value of 0xFFFF retrieves the first port
                                number on the ATA controller.

  @retval EFI_SUCCESS           The next port number on the ATA controller was returned in Port.
  @retval EFI_NOT_FOUND         There are no more ports on this ATA controller.
  @retval EFI_INVALID_PARAMETER Port is not 0xFFFF and Port was not returned on a previous call
                                to GetNextPort().

**/
EFI_STATUS
EFIAPI
AtaPassThruGetNextPort (
  IN EFI_ATA_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT16                  *Port
  )
{
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;
  EFI_ATA_DEVICE_INFO           *DeviceInfo;

  Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  if (Port == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Port == 0xFFFF) {
    //
    // If the Port is all 0xFF's, start to traverse the device list from the beginning
    //
    Node = GetFirstNode (&Instance->DeviceList);

    while (!IsNull (&Instance->DeviceList, Node)) {
      DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

      if (DeviceInfo->Type == EfiIdeHarddisk) {
        *Port = DeviceInfo->Port;
        goto Exit;
      }

      Node = GetNextNode (&Instance->DeviceList, Node);
    }

    return EFI_NOT_FOUND;
  } else if (*Port == Instance->PreviousPort) {
    Node = GetFirstNode (&Instance->DeviceList);

    while (!IsNull (&Instance->DeviceList, Node)) {
      DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

      if ((DeviceInfo->Type == EfiIdeHarddisk) &&
          (DeviceInfo->Port > *Port))
      {
        *Port = DeviceInfo->Port;
        goto Exit;
      }

      Node = GetNextNode (&Instance->DeviceList, Node);
    }

    return EFI_NOT_FOUND;
  } else {
    //
    // Port is not equal to 0xFFFF and also not equal to previous return value
    //
    return EFI_INVALID_PARAMETER;
  }

Exit:
  //
  // Update the PreviousPort and PreviousPortMultiplier.
  //
  Instance->PreviousPort = *Port;

  return EFI_SUCCESS;
}

/**
  Used to retrieve the list of legal port multiplier port numbers for ATA devices on a port of an ATA
  controller. These can either be the list of port multiplier ports where ATA devices are actually
  present on port or the list of legal port multiplier ports on that port. Regardless, the caller of this
  function must probe the port number and port multiplier port number returned to see if an ATA
  device is actually present.

  The GetNextDevice() function retrieves the port multiplier port number of an ATA device
  present on a port of an ATA controller.

  If PortMultiplierPort points to a port multiplier port number value that was returned on a
  previous call to GetNextDevice(), then the port multiplier port number of the next ATA device
  on the port of the ATA controller is returned in PortMultiplierPort, and EFI_SUCCESS is
  returned.

  If PortMultiplierPort points to 0xFFFF, then the port multiplier port number of the first
  ATA device on port of the ATA controller is returned in PortMultiplierPort and
  EFI_SUCCESS is returned.

  If PortMultiplierPort is not 0xFFFF and the value pointed to by PortMultiplierPort
  was not returned on a previous call to GetNextDevice(), then EFI_INVALID_PARAMETER
  is returned.

  If PortMultiplierPort is the port multiplier port number of the last ATA device on the port of
  the ATA controller, then EFI_NOT_FOUND is returned.

  @param[in]      This               A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]      Port               The port number present on the ATA controller.
  @param[in, out] PortMultiplierPort On input, a pointer to the port multiplier port number of an
                                     ATA device present on the ATA controller.
                                     If on input a PortMultiplierPort of 0xFFFF is specified,
                                     then the port multiplier port number of the first ATA device
                                     is returned. On output, a pointer to the port multiplier port
                                     number of the next ATA device present on an ATA controller.

  @retval EFI_SUCCESS                The port multiplier port number of the next ATA device on the port
                                     of the ATA controller was returned in PortMultiplierPort.
  @retval EFI_NOT_FOUND              There are no more ATA devices on this port of the ATA controller.
  @retval EFI_INVALID_PARAMETER      PortMultiplierPort is not 0xFFFF, and PortMultiplierPort was not
                                     returned on a previous call to GetNextDevice().

**/
EFI_STATUS
EFIAPI
AtaPassThruGetNextDevice (
  IN EFI_ATA_PASS_THRU_PROTOCOL  *This,
  IN UINT16                      Port,
  IN OUT UINT16                  *PortMultiplierPort
  )
{
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;
  EFI_ATA_DEVICE_INFO           *DeviceInfo;

  Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  if (PortMultiplierPort == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Instance->PreviousPortMultiplier == 0xFFFF) {
    //
    // If a device is directly attached on a port, previous call to this
    // function will return the value 0xFFFF for PortMultiplierPort. In
    // this case, there should be no more device on the port multiplier.
    //
    Instance->PreviousPortMultiplier = 0;
    return EFI_NOT_FOUND;
  }

  if (*PortMultiplierPort == Instance->PreviousPortMultiplier) {
    Node = GetFirstNode (&Instance->DeviceList);

    while (!IsNull (&Instance->DeviceList, Node)) {
      DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

      if ((DeviceInfo->Type == EfiIdeHarddisk) &&
          (DeviceInfo->Port == Port) &&
          (DeviceInfo->PortMultiplier > *PortMultiplierPort))
      {
        *PortMultiplierPort = DeviceInfo->PortMultiplier;
        goto Exit;
      }

      Node = GetNextNode (&Instance->DeviceList, Node);
    }

    return EFI_NOT_FOUND;
  } else if (*PortMultiplierPort == 0xFFFF) {
    //
    // If the PortMultiplierPort is all 0xFF's, start to traverse the device list from the beginning
    //
    Node = GetFirstNode (&Instance->DeviceList);

    while (!IsNull (&Instance->DeviceList, Node)) {
      DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

      if ((DeviceInfo->Type == EfiIdeHarddisk) &&
          (DeviceInfo->Port == Port))
      {
        *PortMultiplierPort = DeviceInfo->PortMultiplier;
        goto Exit;
      }

      Node = GetNextNode (&Instance->DeviceList, Node);
    }

    return EFI_NOT_FOUND;
  } else {
    //
    // PortMultiplierPort is not equal to 0xFFFF and also not equal to previous return value
    //
    return EFI_INVALID_PARAMETER;
  }

Exit:
  //
  // Update the PreviousPort and PreviousPortMultiplier.
  //
  Instance->PreviousPortMultiplier = *PortMultiplierPort;

  return EFI_SUCCESS;
}

/**
  Used to allocate and build a device path node for an ATA device on an ATA controller.

  The BuildDevicePath() function allocates and builds a single device node for the ATA
  device specified by Port and PortMultiplierPort. If the ATA device specified by Port and
  PortMultiplierPort is not present on the ATA controller, then EFI_NOT_FOUND is returned.
  If DevicePath is NULL, then EFI_INVALID_PARAMETER is returned. If there are not enough
  resources to allocate the device path node, then EFI_OUT_OF_RESOURCES is returned.

  Otherwise, DevicePath is allocated with the boot service AllocatePool(), the contents of
  DevicePath are initialized to describe the ATA device specified by Port and PortMultiplierPort,
  and EFI_SUCCESS is returned.

  @param[in]      This               A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]      Port               Port specifies the port number of the ATA device for which a
                                     device path node is to be allocated and built.
  @param[in]      PortMultiplierPort The port multiplier port number of the ATA device for which a
                                     device path node is to be allocated and built. If there is no
                                     port multiplier, then specify 0xFFFF.
  @param[in, out] DevicePath         A pointer to a single device path node that describes the ATA
                                     device specified by Port and PortMultiplierPort. This function
                                     is responsible for allocating the buffer DevicePath with the
                                     boot service AllocatePool(). It is the caller's responsibility
                                     to free DevicePath when the caller is finished with DevicePath.
  @retval EFI_SUCCESS                The device path node that describes the ATA device specified by
                                     Port and PortMultiplierPort was allocated and returned in DevicePath.
  @retval EFI_NOT_FOUND              The ATA device specified by Port and PortMultiplierPort does not
                                     exist on the ATA controller.
  @retval EFI_INVALID_PARAMETER      DevicePath is NULL.
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources to allocate DevicePath.

**/
EFI_STATUS
EFIAPI
AtaPassThruBuildDevicePath (
  IN     EFI_ATA_PASS_THRU_PROTOCOL  *This,
  IN     UINT16                      Port,
  IN     UINT16                      PortMultiplierPort,
  IN OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath
  )
{
  EFI_DEV_PATH                  *DevicePathNode;
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;

  Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  //
  // Validate parameters passed in.
  //
  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Node = SearchDeviceInfoList (Instance, Port, PortMultiplierPort, EfiIdeHarddisk);
  if (Node == NULL) {
    return EFI_NOT_FOUND;
  }

  if (Instance->Mode == EfiAtaIdeMode) {
    DevicePathNode = AllocateCopyPool (sizeof (ATAPI_DEVICE_PATH), &mAtapiDevicePathTemplate);
    if (DevicePathNode == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DevicePathNode->Atapi.PrimarySecondary = (UINT8)Port;
    DevicePathNode->Atapi.SlaveMaster      = (UINT8)PortMultiplierPort;
    DevicePathNode->Atapi.Lun              = 0;
  } else {
    DevicePathNode = AllocateCopyPool (sizeof (SATA_DEVICE_PATH), &mSataDevicePathTemplate);
    if (DevicePathNode == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DevicePathNode->Sata.HBAPortNumber            = Port;
    DevicePathNode->Sata.PortMultiplierPortNumber = PortMultiplierPort;
    DevicePathNode->Sata.Lun                      = 0;
  }

  *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)DevicePathNode;

  return EFI_SUCCESS;
}

/**
  Used to translate a device path node to a port number and port multiplier port number.

  The GetDevice() function determines the port and port multiplier port number associated with
  the ATA device described by DevicePath. If DevicePath is a device path node type that the
  ATA Pass Thru driver supports, then the ATA Pass Thru driver will attempt to translate the contents
  DevicePath into a port number and port multiplier port number.

  If this translation is successful, then that port number and port multiplier port number are returned
  in Port and PortMultiplierPort, and EFI_SUCCESS is returned.

  If DevicePath, Port, or PortMultiplierPort are NULL, then EFI_INVALID_PARAMETER is returned.

  If DevicePath is not a device path node type that the ATA Pass Thru driver supports, then
  EFI_UNSUPPORTED is returned.

  If DevicePath is a device path node type that the ATA Pass Thru driver supports, but there is not
  a valid translation from DevicePath to a port number and port multiplier port number, then
  EFI_NOT_FOUND is returned.

  @param[in]  This                A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]  DevicePath          A pointer to the device path node that describes an ATA device on the
                                  ATA controller.
  @param[out] Port                On return, points to the port number of an ATA device on the ATA controller.
  @param[out] PortMultiplierPort  On return, points to the port multiplier port number of an ATA device
                                  on the ATA controller.

  @retval EFI_SUCCESS             DevicePath was successfully translated to a port number and port multiplier
                                  port number, and they were returned in Port and PortMultiplierPort.
  @retval EFI_INVALID_PARAMETER   DevicePath is NULL.
  @retval EFI_INVALID_PARAMETER   Port is NULL.
  @retval EFI_INVALID_PARAMETER   PortMultiplierPort is NULL.
  @retval EFI_UNSUPPORTED         This driver does not support the device path node type in DevicePath.
  @retval EFI_NOT_FOUND           A valid translation from DevicePath to a port number and port multiplier
                                  port number does not exist.
**/
EFI_STATUS
EFIAPI
AtaPassThruGetDevice (
  IN  EFI_ATA_PASS_THRU_PROTOCOL  *This,
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  OUT UINT16                      *Port,
  OUT UINT16                      *PortMultiplierPort
  )
{
  EFI_DEV_PATH                  *DevicePathNode;
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;

  Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  //
  // Validate parameters passed in.
  //
  if ((DevicePath == NULL) || (Port == NULL) || (PortMultiplierPort == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether the DevicePath belongs to SCSI_DEVICE_PATH or ATAPI_DEVICE_PATH
  //
  if ((DevicePath->Type != MESSAGING_DEVICE_PATH) ||
      ((DevicePath->SubType != MSG_SATA_DP) &&
       (DevicePath->SubType != MSG_ATAPI_DP)) ||
      ((DevicePathNodeLength (DevicePath) != sizeof (ATAPI_DEVICE_PATH)) &&
       (DevicePathNodeLength (DevicePath) != sizeof (SATA_DEVICE_PATH))))
  {
    return EFI_UNSUPPORTED;
  }

  DevicePathNode = (EFI_DEV_PATH *)DevicePath;

  if (Instance->Mode == EfiAtaIdeMode) {
    *Port               = DevicePathNode->Atapi.PrimarySecondary;
    *PortMultiplierPort = DevicePathNode->Atapi.SlaveMaster;
  } else {
    *Port               = DevicePathNode->Sata.HBAPortNumber;
    *PortMultiplierPort = DevicePathNode->Sata.PortMultiplierPortNumber;
  }

  Node = SearchDeviceInfoList (Instance, *Port, *PortMultiplierPort, EfiIdeHarddisk);

  if (Node == NULL) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Resets a specific port on the ATA controller. This operation also resets all the ATA devices
  connected to the port.

  The ResetChannel() function resets an a specific port on an ATA controller. This operation
  resets all the ATA devices connected to that port. If this ATA controller does not support
  a reset port operation, then EFI_UNSUPPORTED is returned.

  If a device error occurs while executing that port reset operation, then EFI_DEVICE_ERROR is
  returned.

  If a timeout occurs during the execution of the port reset operation, then EFI_TIMEOUT is returned.

  If the port reset operation is completed, then EFI_SUCCESS is returned.

  @param[in]  This          A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]  Port          The port number on the ATA controller.

  @retval EFI_SUCCESS       The ATA controller port was reset.
  @retval EFI_UNSUPPORTED   The ATA controller does not support a port reset operation.
  @retval EFI_DEVICE_ERROR  A device error occurred while attempting to reset the ATA port.
  @retval EFI_TIMEOUT       A timeout occurred while attempting to reset the ATA port.

**/
EFI_STATUS
EFIAPI
AtaPassThruResetPort (
  IN EFI_ATA_PASS_THRU_PROTOCOL  *This,
  IN UINT16                      Port
  )
{
  //
  // Return success directly then upper layer driver could think reset port operation is done.
  //
  return EFI_SUCCESS;
}

/**
  Resets an ATA device that is connected to an ATA controller.

  The ResetDevice() function resets the ATA device specified by Port and PortMultiplierPort.
  If this ATA controller does not support a device reset operation, then EFI_UNSUPPORTED is
  returned.

  If Port or PortMultiplierPort are not in a valid range for this ATA controller, then
  EFI_INVALID_PARAMETER is returned.

  If a device error occurs while executing that device reset operation, then EFI_DEVICE_ERROR
  is returned.

  If a timeout occurs during the execution of the device reset operation, then EFI_TIMEOUT is
  returned.

  If the device reset operation is completed, then EFI_SUCCESS is returned.

  @param[in] This                A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in] Port                Port represents the port number of the ATA device to be reset.
  @param[in] PortMultiplierPort  The port multiplier port number of the ATA device to reset.
                                 If there is no port multiplier, then specify 0xFFFF.
  @retval EFI_SUCCESS            The ATA device specified by Port and PortMultiplierPort was reset.
  @retval EFI_UNSUPPORTED        The ATA controller does not support a device reset operation.
  @retval EFI_INVALID_PARAMETER  Port or PortMultiplierPort are invalid.
  @retval EFI_DEVICE_ERROR       A device error occurred while attempting to reset the ATA device
                                 specified by Port and PortMultiplierPort.
  @retval EFI_TIMEOUT            A timeout occurred while attempting to reset the ATA device
                                 specified by Port and PortMultiplierPort.

**/
EFI_STATUS
EFIAPI
AtaPassThruResetDevice (
  IN EFI_ATA_PASS_THRU_PROTOCOL  *This,
  IN UINT16                      Port,
  IN UINT16                      PortMultiplierPort
  )
{
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;

  Instance = ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  Node = SearchDeviceInfoList (Instance, Port, PortMultiplierPort, EfiIdeHarddisk);

  if (Node == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Return success directly then upper layer driver could think reset device operation is done.
  //
  return EFI_SUCCESS;
}

/**
  Submit ATAPI request sense command.

  @param[in] This            A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param[in] Target          The Target is an array of size TARGET_MAX_BYTES and it represents
                             the id of the SCSI device to send the SCSI Request Packet. Each
                             transport driver may choose to utilize a subset of this size to suit the needs
                             of transport target representation. For example, a Fibre Channel driver
                             may use only 8 bytes (WWN) to represent an FC target.
  @param[in] Lun             The LUN of the SCSI device to send the SCSI Request Packet.
  @param[in] SenseData       A pointer to store sense data.
  @param[in] SenseDataLength The sense data length.
  @param[in] Timeout         The timeout value to execute this cmd, uses 100ns as a unit.

  @retval EFI_SUCCESS        Send out the ATAPI packet command successfully.
  @retval EFI_DEVICE_ERROR   The device failed to send data.

**/
EFI_STATUS
EFIAPI
AtaPacketRequestSense (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN  UINT8                            *Target,
  IN  UINT64                           Lun,
  IN  VOID                             *SenseData,
  IN  UINT8                            SenseDataLength,
  IN  UINT64                           Timeout
  )
{
  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  Packet;
  UINT8                                       Cdb[12];
  EFI_STATUS                                  Status;

  ZeroMem (&Packet, sizeof (EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET));
  ZeroMem (Cdb, 12);

  Cdb[0] = ATA_CMD_REQUEST_SENSE;
  Cdb[4] = SenseDataLength;

  Packet.Timeout          = Timeout;
  Packet.Cdb              = Cdb;
  Packet.CdbLength        = 12;
  Packet.DataDirection    = EFI_EXT_SCSI_DATA_DIRECTION_READ;
  Packet.InDataBuffer     = SenseData;
  Packet.InTransferLength = SenseDataLength;

  Status = ExtScsiPassThruPassThru (This, Target, Lun, &Packet, NULL);

  return Status;
}

/**
  Sends a SCSI Request Packet to a SCSI device that is attached to the SCSI channel. This function
  supports both blocking I/O and nonblocking I/O. The blocking I/O functionality is required, and the
  nonblocking I/O functionality is optional.

  @param  This    A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target  The Target is an array of size TARGET_MAX_BYTES and it represents
                  the id of the SCSI device to send the SCSI Request Packet. Each
                  transport driver may choose to utilize a subset of this size to suit the needs
                  of transport target representation. For example, a Fibre Channel driver
                  may use only 8 bytes (WWN) to represent an FC target.
  @param  Lun     The LUN of the SCSI device to send the SCSI Request Packet.
  @param  Packet  A pointer to the SCSI Request Packet to send to the SCSI device
                  specified by Target and Lun.
  @param  Event   If nonblocking I/O is not supported then Event is ignored, and blocking
                  I/O is performed. If Event is NULL, then blocking I/O is performed. If
                  Event is not NULL and non blocking I/O is supported, then
                  nonblocking I/O is performed, and Event will be signaled when the
                  SCSI Request Packet completes.

  @retval EFI_SUCCESS           The SCSI Request Packet was sent by the host. For bi-directional
                                commands, InTransferLength bytes were transferred from
                                InDataBuffer. For write and bi-directional commands,
                                OutTransferLength bytes were transferred by
                                OutDataBuffer.
  @retval EFI_BAD_BUFFER_SIZE   The SCSI Request Packet was not executed. The number of bytes that
                                could be transferred is returned in InTransferLength. For write
                                and bi-directional commands, OutTransferLength bytes were
                                transferred by OutDataBuffer.
  @retval EFI_NOT_READY         The SCSI Request Packet could not be sent because there are too many
                                SCSI Request Packets already queued. The caller may retry again later.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SCSI Request
                                Packet.
  @retval EFI_INVALID_PARAMETER Target, Lun, or the contents of ScsiRequestPacket are invalid.
  @retval EFI_UNSUPPORTED       The command described by the SCSI Request Packet is not supported
                                by the host adapter. This includes the case of Bi-directional SCSI
                                commands not supported by the implementation. The SCSI Request
                                Packet was not sent, so no additional status information is available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI Request Packet to execute.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                 *This,
  IN UINT8                                           *Target,
  IN UINT64                                          Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet,
  IN EFI_EVENT                                       Event OPTIONAL
  )
{
  EFI_STATUS                    Status;
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  UINT8                         Port;
  UINT8                         PortMultiplier;
  EFI_ATA_HC_WORK_MODE          Mode;
  LIST_ENTRY                    *Node;
  EFI_ATA_DEVICE_INFO           *DeviceInfo;
  BOOLEAN                       SenseReq;
  EFI_SCSI_SENSE_DATA           *PtrSenseData;
  UINTN                         SenseDataLen;
  EFI_STATUS                    SenseStatus;

  SenseDataLen = 0;
  Instance     = EXT_SCSI_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  if ((Packet == NULL) || (Packet->Cdb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Don't support variable length CDB
  //
  if ((Packet->CdbLength != 6) && (Packet->CdbLength != 10) &&
      (Packet->CdbLength != 12) && (Packet->CdbLength != 16))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->SenseDataLength != 0) && (Packet->SenseData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((This->Mode->IoAlign > 1) && !ADDRESS_IS_ALIGNED (Packet->InDataBuffer, This->Mode->IoAlign)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((This->Mode->IoAlign > 1) && !ADDRESS_IS_ALIGNED (Packet->OutDataBuffer, This->Mode->IoAlign)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((This->Mode->IoAlign > 1) && !ADDRESS_IS_ALIGNED (Packet->SenseData, This->Mode->IoAlign)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For ATAPI device, doesn't support multiple LUN device.
  //
  if (Lun != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The layout of Target array:
  //  ________________________________________________________________________
  // |       Byte 0        |       Byte 1        | ... | TARGET_MAX_BYTES - 1 |
  // |_____________________|_____________________|_____|______________________|
  // |                     | The port multiplier |     |                      |
  // |   The port number   |    port number      | N/A |         N/A          |
  // |_____________________|_____________________|_____|______________________|
  //
  // For ATAPI device, 2 bytes is enough to represent the location of SCSI device.
  //
  Port           = Target[0];
  PortMultiplier = Target[1];

  Node = SearchDeviceInfoList (Instance, Port, PortMultiplier, EfiIdeCdrom);
  if (Node == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

  //
  // ATA_CMD_IDENTIFY_DEVICE cmd is a ATA cmd but not a SCSI cmd.
  // Normally it should NOT be passed down through ExtScsiPassThru protocol interface.
  // But to response EFI_DISK_INFO.Identify() request from ScsiDisk, we should handle this command.
  //
  if (*((UINT8 *)Packet->Cdb) == ATA_CMD_IDENTIFY_DEVICE) {
    CopyMem (Packet->InDataBuffer, DeviceInfo->IdentifyData, sizeof (EFI_IDENTIFY_DATA));
    //
    // For IDENTIFY DEVICE cmd, we don't need to get sense data.
    //
    Packet->SenseDataLength = 0;
    return EFI_SUCCESS;
  }

  Mode = Instance->Mode;
  switch (Mode) {
    case EfiAtaIdeMode:
      //
      // Reassign IDE mode io port registers' base addresses
      //
      Status = GetIdeRegisterIoAddr (Instance->PciIo, Instance->IdeRegisters);

      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = AtaPacketCommandExecute (Instance->PciIo, &Instance->IdeRegisters[Port], Port, PortMultiplier, Packet);
      break;
    case EfiAtaAhciMode:
      if (PortMultiplier == 0xFF) {
        //
        // If there is no port multiplier, the PortMultiplier will be 0xFF
        // Here, we convert its value to 0 to follow the AHCI spec.
        //
        PortMultiplier = 0;
      }

      Status = AhciPacketCommandExecute (Instance->PciIo, &Instance->AhciRegisters, Port, PortMultiplier, Packet);
      break;
    default:
      Status = EFI_DEVICE_ERROR;
      break;
  }

  //
  // If the cmd doesn't get executed correctly, then check sense data.
  //
  if (EFI_ERROR (Status) && (Packet->SenseDataLength != 0) && (*((UINT8 *)Packet->Cdb) != ATA_CMD_REQUEST_SENSE)) {
    PtrSenseData = AllocateAlignedPages (EFI_SIZE_TO_PAGES (sizeof (EFI_SCSI_SENSE_DATA)), This->Mode->IoAlign);
    if (PtrSenseData == NULL) {
      return EFI_DEVICE_ERROR;
    }

    for (SenseReq = TRUE; SenseReq;) {
      SenseStatus = AtaPacketRequestSense (
                      This,
                      Target,
                      Lun,
                      PtrSenseData,
                      sizeof (EFI_SCSI_SENSE_DATA),
                      Packet->Timeout
                      );
      if (EFI_ERROR (SenseStatus)) {
        break;
      }

      CopyMem ((UINT8 *)Packet->SenseData + SenseDataLen, PtrSenseData, sizeof (EFI_SCSI_SENSE_DATA));
      SenseDataLen += sizeof (EFI_SCSI_SENSE_DATA);

      //
      // no more sense key or number of sense keys exceeds predefined,
      // skip the loop.
      //
      if ((PtrSenseData->Sense_Key == EFI_SCSI_SK_NO_SENSE) ||
          (SenseDataLen + sizeof (EFI_SCSI_SENSE_DATA) > Packet->SenseDataLength))
      {
        SenseReq = FALSE;
      }
    }

    FreeAlignedPages (PtrSenseData, EFI_SIZE_TO_PAGES (sizeof (EFI_SCSI_SENSE_DATA)));
  }

  //
  // Update the SenseDataLength field to the data length received.
  //
  Packet->SenseDataLength = (UINT8)SenseDataLen;
  return Status;
}

/**
  Used to retrieve the list of legal Target IDs and LUNs for SCSI devices on a SCSI channel. These
  can either be the list SCSI devices that are actually present on the SCSI channel, or the list of legal
  Target Ids and LUNs for the SCSI channel. Regardless, the caller of this function must probe the
  Target ID and LUN returned to see if a SCSI device is actually present at that location on the SCSI
  channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target On input, a pointer to the Target ID (an array of size
                 TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                 On output, a pointer to the Target ID (an array of
                 TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                 channel. An input value of 0xF(all bytes in the array are 0xF) in the
                 Target array retrieves the Target ID of the first SCSI device present on a
                 SCSI channel.
  @param  Lun    On input, a pointer to the LUN of a SCSI device present on the SCSI
                 channel. On output, a pointer to the LUN of the next SCSI device present
                 on a SCSI channel.

  @retval EFI_SUCCESS           The Target ID and LUN of the next SCSI device on the SCSI
                                channel was returned in Target and Lun.
  @retval EFI_INVALID_PARAMETER Target array is not all 0xF, and Target and Lun were
                                not returned on a previous call to GetNextTargetLun().
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruGetNextTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                         **Target,
  IN OUT UINT64                        *Lun
  )
{
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;
  EFI_ATA_DEVICE_INFO           *DeviceInfo;
  UINT8                         *Target8;
  UINT16                        *Target16;

  Instance = EXT_SCSI_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  if ((Target == NULL) || (Lun == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Target8  = *Target;
  Target16 = (UINT16 *)*Target;

  if (CompareMem (Target8, mScsiId, TARGET_MAX_BYTES) != 0) {
    //
    // For ATAPI device, we use 2 least significant bytes to represent the location of SCSI device.
    // So the higher bytes in Target array should be 0xFF.
    //
    if (CompareMem (&Target8[2], &mScsiId[2], TARGET_MAX_BYTES - 2) != 0) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // When Target is not all 0xFF's, compare 2 least significant bytes with
    // previous target id to see if it is returned by previous call.
    //
    if ((*Target16 != Instance->PreviousTargetId) ||
        (*Lun != Instance->PreviousLun))
    {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Traverse the whole device list to find the next cdrom closed to
    // the device signified by Target[0] and Target[1].
    //
    // Note that we here use a tricky way to find the next cdrom :
    // All ata devices are detected and inserted into the device list
    // sequentially.
    //
    Node = GetFirstNode (&Instance->DeviceList);

    while (!IsNull (&Instance->DeviceList, Node)) {
      DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

      if ((DeviceInfo->Type == EfiIdeCdrom) &&
          ((Target8[0] < DeviceInfo->Port) ||
           ((Target8[0] == DeviceInfo->Port) &&
            (Target8[1] < (UINT8)DeviceInfo->PortMultiplier))))
      {
        Target8[0] = (UINT8)DeviceInfo->Port;
        Target8[1] = (UINT8)DeviceInfo->PortMultiplier;
        goto Exit;
      }

      Node = GetNextNode (&Instance->DeviceList, Node);
    }

    return EFI_NOT_FOUND;
  } else {
    //
    // If the array is all 0xFF's, start to traverse the device list from the beginning
    //
    Node = GetFirstNode (&Instance->DeviceList);
    while (!IsNull (&Instance->DeviceList, Node)) {
      DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

      if (DeviceInfo->Type == EfiIdeCdrom) {
        Target8[0] = (UINT8)DeviceInfo->Port;
        Target8[1] = (UINT8)DeviceInfo->PortMultiplier;
        goto Exit;
      }

      Node = GetNextNode (&Instance->DeviceList, Node);
    }

    return EFI_NOT_FOUND;
  }

Exit:
  *Lun = 0;

  //
  // Update the PreviousTargetId.
  //
  Instance->PreviousTargetId = *Target16;
  Instance->PreviousLun      = *Lun;

  return EFI_SUCCESS;
}

/**
  Used to allocate and build a device path node for a SCSI device on a SCSI channel.

  @param  This       A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target     The Target is an array of size TARGET_MAX_BYTES and it specifies the
                     Target ID of the SCSI device for which a device path node is to be
                     allocated and built. Transport drivers may chose to utilize a subset of
                     this size to suit the representation of targets. For example, a Fibre
                     Channel driver may use only 8 bytes (WWN) in the array to represent a
                     FC target.
  @param  Lun        The LUN of the SCSI device for which a device path node is to be
                     allocated and built.
  @param  DevicePath A pointer to a single device path node that describes the SCSI device
                     specified by Target and Lun. This function is responsible for
                     allocating the buffer DevicePath with the boot service
                     AllocatePool(). It is the caller's responsibility to free
                     DevicePath when the caller is finished with DevicePath.

  @retval EFI_SUCCESS           The device path node that describes the SCSI device specified by
                                Target and Lun was allocated and returned in
                                DevicePath.
  @retval EFI_INVALID_PARAMETER DevicePath is NULL.
  @retval EFI_NOT_FOUND         The SCSI devices specified by Target and Lun does not exist
                                on the SCSI channel.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to allocate DevicePath.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruBuildDevicePath (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN     UINT8                            *Target,
  IN     UINT64                           Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL         **DevicePath
  )
{
  EFI_DEV_PATH                  *DevicePathNode;
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  UINT8                         Port;
  UINT8                         PortMultiplier;

  Instance = EXT_SCSI_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  Port           = Target[0];
  PortMultiplier = Target[1];

  //
  // Validate parameters passed in.
  //
  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // can not build device path for the SCSI Host Controller.
  //
  if (Lun != 0) {
    return EFI_NOT_FOUND;
  }

  if (SearchDeviceInfoList (Instance, Port, PortMultiplier, EfiIdeCdrom) == NULL) {
    return EFI_NOT_FOUND;
  }

  if (Instance->Mode == EfiAtaIdeMode) {
    DevicePathNode = AllocateCopyPool (sizeof (ATAPI_DEVICE_PATH), &mAtapiDevicePathTemplate);
    if (DevicePathNode == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DevicePathNode->Atapi.PrimarySecondary = Port;
    DevicePathNode->Atapi.SlaveMaster      = PortMultiplier;
    DevicePathNode->Atapi.Lun              = (UINT16)Lun;
  } else {
    DevicePathNode = AllocateCopyPool (sizeof (SATA_DEVICE_PATH), &mSataDevicePathTemplate);
    if (DevicePathNode == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DevicePathNode->Sata.HBAPortNumber = Port;
    //
    // For CD-ROM working in the AHCI mode, only 8 bits are used to record
    // the PortMultiplier information. If the CD-ROM is directly attached
    // on a SATA port, the PortMultiplier should be translated from 0xFF
    // to 0xFFFF according to the UEFI spec.
    //
    DevicePathNode->Sata.PortMultiplierPortNumber = PortMultiplier == 0xFF ? 0xFFFF : PortMultiplier;
    DevicePathNode->Sata.Lun                      = (UINT16)Lun;
  }

  *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)DevicePathNode;

  return EFI_SUCCESS;
}

/**
  Used to translate a device path node to a Target ID and LUN.

  @param  This       A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  DevicePath A pointer to a single device path node that describes the SCSI device
                     on the SCSI channel.
  @param  Target     A pointer to the Target Array which represents the ID of a SCSI device
                     on the SCSI channel.
  @param  Lun        A pointer to the LUN of a SCSI device on the SCSI channel.

  @retval EFI_SUCCESS           DevicePath was successfully translated to a Target ID and
                                LUN, and they were returned in Target and Lun.
  @retval EFI_INVALID_PARAMETER DevicePath or Target or Lun is NULL.
  @retval EFI_NOT_FOUND         A valid translation from DevicePath to a Target ID and LUN
                                does not exist.
  @retval EFI_UNSUPPORTED       This driver does not support the device path node type in
                                 DevicePath.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN  EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT UINT8                            **Target,
  OUT UINT64                           *Lun
  )
{
  EFI_DEV_PATH                  *DevicePathNode;
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;

  Instance = EXT_SCSI_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  //
  // Validate parameters passed in.
  //
  if ((DevicePath == NULL) || (Target == NULL) || (Lun == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether the DevicePath belongs to SCSI_DEVICE_PATH
  //
  if ((DevicePath->Type != MESSAGING_DEVICE_PATH) ||
      ((DevicePath->SubType != MSG_ATAPI_DP) &&
       (DevicePath->SubType != MSG_SATA_DP)) ||
      ((DevicePathNodeLength (DevicePath) != sizeof (ATAPI_DEVICE_PATH)) &&
       (DevicePathNodeLength (DevicePath) != sizeof (SATA_DEVICE_PATH))))
  {
    return EFI_UNSUPPORTED;
  }

  SetMem (*Target, TARGET_MAX_BYTES, 0xFF);

  DevicePathNode = (EFI_DEV_PATH *)DevicePath;

  if (Instance->Mode == EfiAtaIdeMode) {
    (*Target)[0] = (UINT8)DevicePathNode->Atapi.PrimarySecondary;
    (*Target)[1] = (UINT8)DevicePathNode->Atapi.SlaveMaster;
    *Lun         = (UINT8)DevicePathNode->Atapi.Lun;
  } else {
    (*Target)[0] = (UINT8)DevicePathNode->Sata.HBAPortNumber;
    (*Target)[1] = (UINT8)DevicePathNode->Sata.PortMultiplierPortNumber;
    *Lun         = (UINT8)DevicePathNode->Sata.Lun;
  }

  Node = SearchDeviceInfoList (Instance, (*Target)[0], (*Target)[1], EfiIdeCdrom);

  if (Node == NULL) {
    return EFI_NOT_FOUND;
  }

  if (*Lun != 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Resets a SCSI channel. This operation resets all the SCSI devices connected to the SCSI channel.

  @param  This A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.

  @retval EFI_SUCCESS      The SCSI channel was reset.
  @retval EFI_DEVICE_ERROR A device error occurred while attempting to reset the SCSI channel.
  @retval EFI_TIMEOUT      A timeout occurred while attempting to reset the SCSI channel.
  @retval EFI_UNSUPPORTED  The SCSI channel does not support a channel reset operation.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruResetChannel (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This
  )
{
  //
  // Return success directly then upper layer driver could think reset channel operation is done.
  //
  return EFI_SUCCESS;
}

/**
  Resets a SCSI logical unit that is connected to a SCSI channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target The Target is an array of size TARGET_MAX_BYTE and it represents the
                 target port ID of the SCSI device containing the SCSI logical unit to
                 reset. Transport drivers may chose to utilize a subset of this array to suit
                 the representation of their targets.
  @param  Lun    The LUN of the SCSI device to reset.

  @retval EFI_SUCCESS           The SCSI device specified by Target and Lun was reset.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           A timeout occurred while attempting to reset the SCSI device
                                specified by Target and Lun.
  @retval EFI_UNSUPPORTED       The SCSI channel does not support a target reset operation.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to reset the SCSI device
                                 specified by Target and Lun.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun
  )
{
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;
  UINT8                         Port;
  UINT8                         PortMultiplier;

  Instance = EXT_SCSI_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);
  //
  // For ATAPI device, doesn't support multiple LUN device.
  //
  if (Lun != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The layout of Target array:
  //  ________________________________________________________________________
  // |       Byte 0        |       Byte 1        | ... | TARGET_MAX_BYTES - 1 |
  // |_____________________|_____________________|_____|______________________|
  // |                     | The port multiplier |     |                      |
  // |   The port number   |    port number      | N/A |         N/A          |
  // |_____________________|_____________________|_____|______________________|
  //
  // For ATAPI device, 2 bytes is enough to represent the location of SCSI device.
  //
  Port           = Target[0];
  PortMultiplier = Target[1];

  Node = SearchDeviceInfoList (Instance, Port, PortMultiplier, EfiIdeCdrom);
  if (Node == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Return success directly then upper layer driver could think reset target LUN operation is done.
  //
  return EFI_SUCCESS;
}

/**
  Used to retrieve the list of legal Target IDs for SCSI devices on a SCSI channel. These can either
  be the list SCSI devices that are actually present on the SCSI channel, or the list of legal Target IDs
  for the SCSI channel. Regardless, the caller of this function must probe the Target ID returned to
  see if a SCSI device is actually present at that location on the SCSI channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target (TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                 On output, a pointer to the Target ID (an array of
                 TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                 channel. An input value of 0xF(all bytes in the array are 0xF) in the
                 Target array retrieves the Target ID of the first SCSI device present on a
                 SCSI channel.

  @retval EFI_SUCCESS           The Target ID of the next SCSI device on the SCSI
                                channel was returned in Target.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           Target array is not all 0xF, and Target was not
                                returned on a previous call to GetNextTarget().
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruGetNextTarget (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                         **Target
  )
{
  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance;
  LIST_ENTRY                    *Node;
  EFI_ATA_DEVICE_INFO           *DeviceInfo;
  UINT8                         *Target8;
  UINT16                        *Target16;

  Instance = EXT_SCSI_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  if ((Target == NULL) || (*Target == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Target8  = *Target;
  Target16 = (UINT16 *)*Target;

  if (CompareMem (Target8, mScsiId, TARGET_MAX_BYTES) != 0) {
    //
    // For ATAPI device, we use 2 least significant bytes to represent the location of SCSI device.
    // So the higher bytes in Target array should be 0xFF.
    //
    if (CompareMem (&Target8[2], &mScsiId[2], TARGET_MAX_BYTES - 2) != 0) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // When Target is not all 0xFF's, compare 2 least significant bytes with
    // previous target id to see if it is returned by previous call.
    //
    if (*Target16 != Instance->PreviousTargetId) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Traverse the whole device list to find the next cdrom closed to
    // the device signified by Target[0] and Target[1].
    //
    // Note that we here use a tricky way to find the next cdrom :
    // All ata devices are detected and inserted into the device list
    // sequentially.
    //
    Node = GetFirstNode (&Instance->DeviceList);
    while (!IsNull (&Instance->DeviceList, Node)) {
      DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

      if ((DeviceInfo->Type == EfiIdeCdrom) &&
          ((Target8[0] < DeviceInfo->Port) ||
           ((Target8[0] == DeviceInfo->Port) &&
            (Target8[1] < (UINT8)DeviceInfo->PortMultiplier))))
      {
        Target8[0] = (UINT8)DeviceInfo->Port;
        Target8[1] = (UINT8)DeviceInfo->PortMultiplier;
        goto Exit;
      }

      Node = GetNextNode (&Instance->DeviceList, Node);
    }

    return EFI_NOT_FOUND;
  } else {
    //
    // If the array is all 0xFF's, start to traverse the device list from the beginning
    //
    Node = GetFirstNode (&Instance->DeviceList);

    while (!IsNull (&Instance->DeviceList, Node)) {
      DeviceInfo = ATA_ATAPI_DEVICE_INFO_FROM_THIS (Node);

      if (DeviceInfo->Type == EfiIdeCdrom) {
        Target8[0] = (UINT8)DeviceInfo->Port;
        Target8[1] = (UINT8)DeviceInfo->PortMultiplier;
        goto Exit;
      }

      Node = GetNextNode (&Instance->DeviceList, Node);
    }

    return EFI_NOT_FOUND;
  }

Exit:
  //
  // Update the PreviousTargetId.
  //
  Instance->PreviousTargetId = *Target16;

  return EFI_SUCCESS;
}
