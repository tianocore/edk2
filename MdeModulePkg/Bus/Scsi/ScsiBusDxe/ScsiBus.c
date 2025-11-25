/** @file
  SCSI Bus driver that layers on every SCSI Pass Thru and
  Extended SCSI Pass Thru protocol in the system.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) 1985 - 2022, American Megatrends International LLC.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ScsiBus.h"

EFI_DRIVER_BINDING_PROTOCOL  gSCSIBusDriverBinding = {
  SCSIBusDriverBindingSupported,
  SCSIBusDriverBindingStart,
  SCSIBusDriverBindingStop,
  0xa,
  NULL,
  NULL
};

VOID  *mWorkingBuffer;

/**
  Allocates an aligned buffer for SCSI device.

  This function allocates an aligned buffer for the SCSI device to perform
  SCSI pass through operations. The alignment requirement is from SCSI pass
  through interface.

  @param  ScsiIoDevice      The SCSI child device involved for the operation.
  @param  BufferSize        The request buffer size.

  @return A pointer to the aligned buffer or NULL if the allocation fails.

**/
VOID *
AllocateAlignedBuffer (
  IN SCSI_IO_DEV  *ScsiIoDevice,
  IN UINTN        BufferSize
  )
{
  return AllocateAlignedPages (EFI_SIZE_TO_PAGES (BufferSize), ScsiIoDevice->ScsiIo.IoAlign);
}

/**
  Frees an aligned buffer for SCSI device.

  This function frees an aligned buffer for the SCSI device to perform
  SCSI pass through operations.

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
  The user Entry Point for module ScsiBus. The user code starts with this function.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeScsiBus (
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
             &gSCSIBusDriverBinding,
             ImageHandle,
             &gScsiBusComponentName,
             &gScsiBusComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Test to see if this driver supports ControllerHandle.

  This service is called by the EFI boot service ConnectController(). In order
  to make drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these calling restrictions. If
  any other agent wishes to call Supported() it must also follow these calling
  restrictions.

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
SCSIBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                       Status;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *ExtPassThru;
  UINT64                           Lun;
  UINT8                            *TargetId;
  SCSI_TARGET_ID                   ScsiTargetId;

  TargetId = &ScsiTargetId.ScsiId.ExtScsi[0];
  SetMem (TargetId, TARGET_MAX_BYTES, 0xFF);

  //
  // To keep backward compatibility, UEFI ExtPassThru Protocol is supported as well as
  // EFI PassThru Protocol. From priority perspective, ExtPassThru Protocol is firstly
  // tried to open on host controller handle. If fails, then PassThru Protocol is tried instead.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiExtScsiPassThruProtocolGuid,
                  (VOID **)&ExtPassThru,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  } else if (!EFI_ERROR (Status)) {
    //
    // Check if RemainingDevicePath is NULL or the End of Device Path Node,
    // if yes, return EFI_SUCCESS.
    //
    if ((RemainingDevicePath == NULL) || IsDevicePathEnd (RemainingDevicePath)) {
      //
      // Close protocol regardless of RemainingDevicePath validation
      //
      gBS->CloseProtocol (
             Controller,
             &gEfiExtScsiPassThruProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
      return EFI_SUCCESS;
    } else {
      //
      // If RemainingDevicePath isn't the End of Device Path Node, check its validation
      //
      Status = ExtPassThru->GetTargetLun (ExtPassThru, RemainingDevicePath, &TargetId, &Lun);
      //
      // Close protocol regardless of RemainingDevicePath validation
      //
      gBS->CloseProtocol (
             Controller,
             &gEfiExtScsiPassThruProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
      if (!EFI_ERROR (Status)) {
        return EFI_SUCCESS;
      }
    }
  }

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
SCSIBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  UINT64                           Lun;
  UINT8                            *TargetId;
  BOOLEAN                          ScanOtherPuns;
  BOOLEAN                          FromFirstTarget;
  EFI_STATUS                       Status;
  EFI_STATUS                       DevicePathStatus;
  SCSI_BUS_DEVICE                  *ScsiBusDev;
  SCSI_TARGET_ID                   ScsiTargetId;
  EFI_DEVICE_PATH_PROTOCOL         *ParentDevicePath;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *ExtScsiInterface;
  EFI_SCSI_BUS_PROTOCOL            *BusIdentify;

  TargetId        = NULL;
  ScanOtherPuns   = TRUE;
  FromFirstTarget = FALSE;

  TargetId = &ScsiTargetId.ScsiId.ExtScsi[0];
  SetMem (TargetId, TARGET_MAX_BYTES, 0xFF);

  DevicePathStatus = gBS->OpenProtocol (
                            Controller,
                            &gEfiDevicePathProtocolGuid,
                            (VOID **)&ParentDevicePath,
                            This->DriverBindingHandle,
                            Controller,
                            EFI_OPEN_PROTOCOL_BY_DRIVER
                            );
  if (EFI_ERROR (DevicePathStatus) && (DevicePathStatus != EFI_ALREADY_STARTED)) {
    return DevicePathStatus;
  }

  //
  // Report Status Code to indicate SCSI bus starts
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_SCSI | EFI_IOB_PC_INIT),
    ParentDevicePath
    );

  //
  // Try ExtPassThru Protocol to open on host controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiExtScsiPassThruProtocolGuid,
                  (VOID **)&ExtScsiInterface,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (Status != EFI_ALREADY_STARTED) {
    //
    // Going through here means that ExtPassThru Protocol is successfully opened
    // on this handle for this time. Then construct Host controller private data.
    //
    ScsiBusDev = NULL;
    ScsiBusDev = AllocateZeroPool (sizeof (SCSI_BUS_DEVICE));
    if (ScsiBusDev == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    ScsiBusDev->Signature        = SCSI_BUS_DEVICE_SIGNATURE;
    ScsiBusDev->DevicePath       = ParentDevicePath;
    ScsiBusDev->ExtScsiInterface = ExtScsiInterface;

    //
    // Install EFI_SCSI_BUS_PROTOCOL to the controller handle, So ScsiBusDev could be
    // retrieved on this controller handle. With ScsiBusDev, we can know which PassThru
    // Protocol is present on the handle, UEFI ExtPassThru Protocol or EFI PassThru Protocol.
    //
    Status = gBS->InstallProtocolInterface (
                    &Controller,
                    &gEfiCallerIdGuid,
                    EFI_NATIVE_INTERFACE,
                    &ScsiBusDev->BusIdentify
                    );
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }
  } else {
    //
    // Go through here means Start() is re-invoked again, nothing special is required to do except
    // picking up Host controller private information.
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **)&BusIdentify,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    ScsiBusDev = SCSI_BUS_CONTROLLER_DEVICE_FROM_THIS (BusIdentify);
  }

  //
  // Report Status Code to indicate detecting devices on bus
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_SCSI | EFI_IOB_PC_DETECT),
    ParentDevicePath
    );

  Lun = 0;
  if (RemainingDevicePath == NULL) {
    //
    // If RemainingDevicePath is NULL,
    // must enumerate all SCSI devices anyway
    //
    FromFirstTarget = TRUE;
  } else if (!IsDevicePathEnd (RemainingDevicePath)) {
    //
    // If RemainingDevicePath isn't the End of Device Path Node,
    // only scan the specified device by RemainingDevicePath
    //
    Status = ScsiBusDev->ExtScsiInterface->GetTargetLun (ScsiBusDev->ExtScsiInterface, RemainingDevicePath, &TargetId, &Lun);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    //
    // If RemainingDevicePath is the End of Device Path Node,
    // skip enumerate any device and return EFI_SUCCESS
    //
    ScanOtherPuns = FALSE;
  }

  while (ScanOtherPuns) {
    if (FromFirstTarget) {
      //
      // Remaining Device Path is NULL, scan all the possible Puns in the
      // SCSI Channel.
      //
      Status = ScsiBusDev->ExtScsiInterface->GetNextTargetLun (ScsiBusDev->ExtScsiInterface, &TargetId, &Lun);

      if (EFI_ERROR (Status)) {
        //
        // no legal Pun and Lun found any more
        //
        break;
      }
    } else {
      ScanOtherPuns = FALSE;
    }

    //
    // Avoid creating handle for the host adapter.
    //
    if ((ScsiTargetId.ScsiId.Scsi) == ScsiBusDev->ExtScsiInterface->Mode->AdapterId) {
      continue;
    }

    //
    // Scan for the scsi device, if it attaches to the scsi bus,
    // then create handle and install scsi i/o protocol.
    //
    Status = ScsiScanCreateDevice (This, Controller, &ScsiTargetId, Lun, ScsiBusDev);
    if (Status == EFI_OUT_OF_RESOURCES) {
      goto ErrorExit;
    }
  }

  return EFI_SUCCESS;

ErrorExit:

  if (ScsiBusDev != NULL) {
    FreePool (ScsiBusDev);
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiExtScsiPassThruProtocolGuid,
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
  calling restrictions. If any other agent wishes to call Stop() it must also
  follow these calling restrictions.

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
SCSIBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS             Status;
  BOOLEAN                AllChildrenStopped;
  UINTN                  Index;
  EFI_SCSI_IO_PROTOCOL   *ScsiIo;
  SCSI_IO_DEV            *ScsiIoDevice;
  VOID                   *ExtScsiPassThru;
  EFI_SCSI_BUS_PROTOCOL  *Scsidentifier;
  SCSI_BUS_DEVICE        *ScsiBusDev;

  if (NumberOfChildren == 0) {
    //
    // Get the SCSI_BUS_DEVICE
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **)&Scsidentifier,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );

    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    ScsiBusDev = SCSI_BUS_CONTROLLER_DEVICE_FROM_THIS (Scsidentifier);

    //
    // Uninstall SCSI Bus Protocol
    //
    gBS->UninstallProtocolInterface (
           Controller,
           &gEfiCallerIdGuid,
           &ScsiBusDev->BusIdentify
           );

    //
    // Close the bus driver
    //
    //
    // Close ExtPassThru Protocol from this controller handle
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiExtScsiPassThruProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    FreePool (ScsiBusDev);
    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiScsiIoProtocolGuid,
                    (VOID **)&ScsiIo,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
      continue;
    }

    ScsiIoDevice = SCSI_IO_DEV_FROM_THIS (ScsiIo);
    //
    // Close the child handle
    //
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiExtScsiPassThruProtocolGuid,
                    This->DriverBindingHandle,
                    ChildHandleBuffer[Index]
                    );

    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ChildHandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    ScsiIoDevice->DevicePath,
                    &gEfiScsiIoProtocolGuid,
                    &ScsiIoDevice->ScsiIo,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
      gBS->OpenProtocol (
             Controller,
             &gEfiExtScsiPassThruProtocolGuid,
             &ExtScsiPassThru,
             This->DriverBindingHandle,
             ChildHandleBuffer[Index],
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
    } else {
      FreePool (ScsiIoDevice);
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Retrieves the device type information of the SCSI Controller.

  @param  This          Protocol instance pointer.
  @param  DeviceType    A pointer to the device type information retrieved from
                        the SCSI Controller.

  @retval EFI_SUCCESS             Retrieves the device type information successfully.
  @retval EFI_INVALID_PARAMETER   The DeviceType is NULL.

**/
EFI_STATUS
EFIAPI
ScsiGetDeviceType (
  IN  EFI_SCSI_IO_PROTOCOL  *This,
  OUT UINT8                 *DeviceType
  )
{
  SCSI_IO_DEV  *ScsiIoDevice;

  if (DeviceType == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ScsiIoDevice = SCSI_IO_DEV_FROM_THIS (This);
  *DeviceType  = ScsiIoDevice->ScsiDeviceType;
  return EFI_SUCCESS;
}

/**
  Retrieves the device location in the SCSI channel.

  @param  This   Protocol instance pointer.
  @param  Target A pointer to the Target ID of a SCSI device
                 on the SCSI channel.
  @param  Lun    A pointer to the LUN of the SCSI device on
                 the SCSI channel.

  @retval EFI_SUCCESS           Retrieves the device location successfully.
  @retval EFI_INVALID_PARAMETER The Target or Lun is NULL.

**/
EFI_STATUS
EFIAPI
ScsiGetDeviceLocation (
  IN  EFI_SCSI_IO_PROTOCOL  *This,
  IN OUT UINT8              **Target,
  OUT UINT64                *Lun
  )
{
  SCSI_IO_DEV  *ScsiIoDevice;

  if ((Target == NULL) || (Lun == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ScsiIoDevice = SCSI_IO_DEV_FROM_THIS (This);

  CopyMem (*Target, &ScsiIoDevice->Pun, TARGET_MAX_BYTES);

  *Lun = ScsiIoDevice->Lun;

  return EFI_SUCCESS;
}

/**
  Resets the SCSI Bus that the SCSI Controller is attached to.

  @param  This  Protocol instance pointer.

  @retval  EFI_SUCCESS       The SCSI bus is reset successfully.
  @retval  EFI_DEVICE_ERROR  Errors encountered when resetting the SCSI bus.
  @retval  EFI_UNSUPPORTED   The bus reset operation is not supported by the
                             SCSI Host Controller.
  @retval  EFI_TIMEOUT       A timeout occurred while attempting to reset
                             the SCSI bus.
**/
EFI_STATUS
EFIAPI
ScsiResetBus (
  IN  EFI_SCSI_IO_PROTOCOL  *This
  )
{
  SCSI_IO_DEV  *ScsiIoDevice;

  ScsiIoDevice = SCSI_IO_DEV_FROM_THIS (This);

  //
  // Report Status Code to indicate reset happens
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_RESET),
    ScsiIoDevice->ScsiBusDeviceData->DevicePath
    );

  if (ScsiIoDevice->ExtScsiPassThru != NULL) {
    return ScsiIoDevice->ExtScsiPassThru->ResetChannel (ScsiIoDevice->ExtScsiPassThru);
  } else {
    return EFI_UNSUPPORTED;
  }
}

/**
  Resets the SCSI Controller that the device handle specifies.

  @param  This  Protocol instance pointer.

  @retval  EFI_SUCCESS       Reset the SCSI controller successfully.
  @retval  EFI_DEVICE_ERROR  Errors are encountered when resetting the SCSI Controller.
  @retval  EFI_UNSUPPORTED   The SCSI bus does not support a device reset operation.
  @retval  EFI_TIMEOUT       A timeout occurred while attempting to reset the
                             SCSI Controller.
**/
EFI_STATUS
EFIAPI
ScsiResetDevice (
  IN  EFI_SCSI_IO_PROTOCOL  *This
  )
{
  SCSI_IO_DEV  *ScsiIoDevice;
  UINT8        Target[TARGET_MAX_BYTES];

  ScsiIoDevice = SCSI_IO_DEV_FROM_THIS (This);

  //
  // Report Status Code to indicate reset happens
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_RESET),
    ScsiIoDevice->ScsiBusDeviceData->DevicePath
    );

  CopyMem (Target, &ScsiIoDevice->Pun, TARGET_MAX_BYTES);

  if (ScsiIoDevice->ExtScsiPassThru != NULL) {
    return ScsiIoDevice->ExtScsiPassThru->ResetTargetLun (
                                            ScsiIoDevice->ExtScsiPassThru,
                                            Target,
                                            ScsiIoDevice->Lun
                                            );
  } else {
    return EFI_UNSUPPORTED;
  }
}

/**
  Sends a SCSI Request Packet to the SCSI Controller for execution.

  @param  This            Protocol instance pointer.
  @param  CommandPacket   The SCSI request packet to send to the SCSI
                          Controller specified by the device handle.
  @param  Event           If the SCSI bus where the SCSI device is attached
                          does not support non-blocking I/O, then Event is
                          ignored, and blocking I/O is performed.
                          If Event is NULL, then blocking I/O is performed.
                          If Event is not NULL and non-blocking I/O is
                          supported, then non-blocking I/O is performed,
                          and Event will be signaled when the SCSI Request
                          Packet completes.

  @retval EFI_SUCCESS         The SCSI Request Packet was sent by the host
                              successfully, and TransferLength bytes were
                              transferred to/from DataBuffer.See
                              HostAdapterStatus, TargetStatus,
                              SenseDataLength, and SenseData in that order
                              for additional status information.
  @retval EFI_BAD_BUFFER_SIZE The SCSI Request Packet was executed,
                              but the entire DataBuffer could not be transferred.
                              The actual number of bytes transferred is returned
                              in TransferLength. See HostAdapterStatus,
                              TargetStatus, SenseDataLength, and SenseData in
                              that order for additional status information.
  @retval EFI_NOT_READY       The SCSI Request Packet could not be sent because
                              there are too many SCSI Command Packets already
                              queued.The caller may retry again later.
  @retval EFI_DEVICE_ERROR    A device error occurred while attempting to send
                              the SCSI Request Packet. See HostAdapterStatus,
                              TargetStatus, SenseDataLength, and SenseData in
                              that order for additional status information.
  @retval EFI_INVALID_PARAMETER  The contents of CommandPacket are invalid.
                                 The SCSI Request Packet was not sent, so no
                                 additional status information is available.
  @retval EFI_UNSUPPORTED     The command described by the SCSI Request Packet
                              is not supported by the SCSI initiator(i.e., SCSI
                              Host Controller). The SCSI Request Packet was not
                              sent, so no additional status information is
                              available.
  @retval EFI_TIMEOUT         A timeout occurred while waiting for the SCSI
                              Request Packet to execute. See HostAdapterStatus,
                              TargetStatus, SenseDataLength, and SenseData in
                              that order for additional status information.
**/
EFI_STATUS
EFIAPI
ScsiExecuteSCSICommand (
  IN     EFI_SCSI_IO_PROTOCOL             *This,
  IN OUT EFI_SCSI_IO_SCSI_REQUEST_PACKET  *Packet,
  IN     EFI_EVENT                        Event  OPTIONAL
  )
{
  SCSI_IO_DEV                                 *ScsiIoDevice;
  EFI_STATUS                                  Status;
  UINT8                                       Target[TARGET_MAX_BYTES];
  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *ExtRequestPacket;

  if (Packet == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ScsiIoDevice = SCSI_IO_DEV_FROM_THIS (This);
  CopyMem (Target, &ScsiIoDevice->Pun, TARGET_MAX_BYTES);

  ExtRequestPacket = (EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *)Packet;
  if (ScsiIoDevice->ExtScsiPassThru != NULL) {
    if (((ScsiIoDevice->ExtScsiPassThru->Mode->Attributes & EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_NONBLOCKIO) != 0) && (Event !=  NULL)) {
      Status = ScsiIoDevice->ExtScsiPassThru->PassThru (
                                                ScsiIoDevice->ExtScsiPassThru,
                                                Target,
                                                ScsiIoDevice->Lun,
                                                ExtRequestPacket,
                                                Event
                                                );
    } else {
      //
      // If there's no event or the SCSI Device doesn't support NON-BLOCKING,
      // let the 'Event' parameter for PassThru() be NULL.
      //
      Status = ScsiIoDevice->ExtScsiPassThru->PassThru (
                                                ScsiIoDevice->ExtScsiPassThru,
                                                Target,
                                                ScsiIoDevice->Lun,
                                                ExtRequestPacket,
                                                NULL
                                                );
      if ((!EFI_ERROR (Status)) && (Event != NULL)) {
        //
        // Signal Event to tell caller to pick up the SCSI IO packet if the
        // PassThru() succeeds.
        //
        gBS->SignalEvent (Event);
      }
    }

    return Status;
  } else {
    return EFI_UNSUPPORTED;
  }
}

/**
  Scan SCSI Bus to discover the device, and attach ScsiIoProtocol to it.

  @param  This           Protocol instance pointer
  @param  Controller     Controller handle
  @param  TargetId       Target to be scanned
  @param  Lun            The Lun of the SCSI device on the SCSI channel.
  @param  ScsiBusDev     The pointer of SCSI_BUS_DEVICE

  @retval EFI_SUCCESS           Successfully to discover the device and attach
                                ScsiIoProtocol to it.
  @retval EFI_NOT_FOUND         Fail to discover the device.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate memory resources.

**/
EFI_STATUS
EFIAPI
ScsiScanCreateDevice (
  IN     EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN     EFI_HANDLE                   Controller,
  IN     SCSI_TARGET_ID               *TargetId,
  IN     UINT64                       Lun,
  IN OUT SCSI_BUS_DEVICE              *ScsiBusDev
  )
{
  EFI_STATUS                Status;
  SCSI_IO_DEV               *ScsiIoDevice;
  EFI_DEVICE_PATH_PROTOCOL  *ScsiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_HANDLE                DeviceHandle;

  DevicePath          = NULL;
  RemainingDevicePath = NULL;
  ScsiDevicePath      = NULL;
  ScsiIoDevice        = NULL;

  //
  // Build Device Path
  //
  Status = ScsiBusDev->ExtScsiInterface->BuildDevicePath (
                                           ScsiBusDev->ExtScsiInterface,
                                           &TargetId->ScsiId.ExtScsi[0],
                                           Lun,
                                           &ScsiDevicePath
                                           );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DevicePath = AppendDevicePathNode (
                 ScsiBusDev->DevicePath,
                 ScsiDevicePath
                 );

  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  DeviceHandle        = NULL;
  RemainingDevicePath = DevicePath;
  Status              = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &DeviceHandle);
  if (!EFI_ERROR (Status) && (DeviceHandle != NULL) && IsDevicePathEnd (RemainingDevicePath)) {
    //
    // The device has been started, directly return to fast boot.
    //
    Status = EFI_ALREADY_STARTED;
    goto ErrorExit;
  }

  ScsiIoDevice = AllocateZeroPool (sizeof (SCSI_IO_DEV));
  if (ScsiIoDevice == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  ScsiIoDevice->Signature         = SCSI_IO_DEV_SIGNATURE;
  ScsiIoDevice->ScsiBusDeviceData = ScsiBusDev;
  CopyMem (&ScsiIoDevice->Pun, TargetId, TARGET_MAX_BYTES);
  ScsiIoDevice->Lun = Lun;

  ScsiIoDevice->ExtScsiPassThru = ScsiBusDev->ExtScsiInterface;
  ScsiIoDevice->ScsiIo.IoAlign  = ScsiIoDevice->ExtScsiPassThru->Mode->IoAlign;

  ScsiIoDevice->ScsiIo.GetDeviceType      = ScsiGetDeviceType;
  ScsiIoDevice->ScsiIo.GetDeviceLocation  = ScsiGetDeviceLocation;
  ScsiIoDevice->ScsiIo.ResetBus           = ScsiResetBus;
  ScsiIoDevice->ScsiIo.ResetDevice        = ScsiResetDevice;
  ScsiIoDevice->ScsiIo.ExecuteScsiCommand = ScsiExecuteSCSICommand;

  //
  // Report Status Code here since the new SCSI device will be discovered
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_SCSI | EFI_IOB_PC_ENABLE),
    ScsiBusDev->DevicePath
    );

  Status = DiscoverScsiDevice (ScsiIoDevice);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  ScsiIoDevice->DevicePath = DevicePath;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ScsiIoDevice->Handle,
                  &gEfiDevicePathProtocolGuid,
                  ScsiIoDevice->DevicePath,
                  &gEfiScsiIoProtocolGuid,
                  &ScsiIoDevice->ScsiIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  } else {
    gBS->OpenProtocol (
           Controller,
           &gEfiExtScsiPassThruProtocolGuid,
           (VOID **)&(ScsiBusDev->ExtScsiInterface),
           This->DriverBindingHandle,
           ScsiIoDevice->Handle,
           EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
           );
  }

  return EFI_SUCCESS;

ErrorExit:

  //
  // The memory space for ScsiDevicePath is allocated in
  // ExtScsiPassThru->BuildDevicePath() function; It is no longer used
  // after AppendDevicePathNode,so free the memory it occupies.
  //
  FreePool (ScsiDevicePath);

  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }

  if (ScsiIoDevice != NULL) {
    FreePool (ScsiIoDevice);
  }

  return Status;
}

/**
  Discovery SCSI Device

  @param  ScsiIoDevice    The pointer of SCSI_IO_DEV

  @retval EFI_SUCCESS           Find SCSI Device and verify it.
  @retval EFI_NOT_FOUND         Unable to find SCSI Device.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate memory resources.

**/
EFI_STATUS
DiscoverScsiDevice (
  IN OUT  SCSI_IO_DEV  *ScsiIoDevice
  )
{
  EFI_STATUS             Status;
  UINT32                 InquiryDataLength;
  UINT8                  SenseDataLength;
  UINT8                  HostAdapterStatus;
  UINT8                  TargetStatus;
  EFI_SCSI_INQUIRY_DATA  *InquiryData;
  EFI_SCSI_SENSE_DATA    *SenseData;
  UINT8                  MaxRetry;
  UINT8                  Index;

  HostAdapterStatus = 0;
  TargetStatus      = 0;
  SenseData         = NULL;

  InquiryData = AllocateAlignedBuffer (ScsiIoDevice, sizeof (EFI_SCSI_INQUIRY_DATA));
  if (InquiryData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  SenseData = AllocateAlignedBuffer (
                ScsiIoDevice,
                sizeof (EFI_SCSI_SENSE_DATA)
                );
  if (SenseData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Using Inquiry command to scan for the device
  //
  InquiryDataLength = sizeof (EFI_SCSI_INQUIRY_DATA);
  SenseDataLength   = sizeof (EFI_SCSI_SENSE_DATA);
  ZeroMem (InquiryData, InquiryDataLength);
  ZeroMem (SenseData, SenseDataLength);

  MaxRetry = 2;
  for (Index = 0; Index < MaxRetry; Index++) {
    Status = ScsiInquiryCommand (
               &ScsiIoDevice->ScsiIo,
               SCSI_BUS_TIMEOUT,
               SenseData,
               &SenseDataLength,
               &HostAdapterStatus,
               &TargetStatus,
               (VOID *)InquiryData,
               &InquiryDataLength,
               FALSE
               );
    if (!EFI_ERROR (Status)) {
      if ((HostAdapterStatus == EFI_SCSI_IO_STATUS_HOST_ADAPTER_OK) &&
          (TargetStatus == EFI_SCSI_IO_STATUS_TARGET_CHECK_CONDITION) &&
          (SenseData->Error_Code == 0x70) &&
          (SenseData->Sense_Key == EFI_SCSI_SK_ILLEGAL_REQUEST))
      {
        Status = EFI_NOT_FOUND;
        goto Done;
      }

      break;
    }

    if ((Status == EFI_BAD_BUFFER_SIZE) ||
        (Status == EFI_INVALID_PARAMETER) ||
        (Status == EFI_UNSUPPORTED))
    {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
  }

  if (Index == MaxRetry) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Retrieved inquiry data successfully
  //
  if (InquiryData->Peripheral_Qualifier != 0) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  if ((InquiryData->Peripheral_Type >= EFI_SCSI_TYPE_RESERVED_LOW) &&
      (InquiryData->Peripheral_Type <= EFI_SCSI_TYPE_RESERVED_HIGH))
  {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // valid device type and peripheral qualifier combination.
  //
  ScsiIoDevice->ScsiDeviceType  = InquiryData->Peripheral_Type;
  ScsiIoDevice->RemovableDevice = InquiryData->Rmb;
  if (InquiryData->Version == 0) {
    ScsiIoDevice->ScsiVersion = 0;
  } else {
    //
    // ANSI-approved version
    //
    ScsiIoDevice->ScsiVersion = (UINT8)(InquiryData->Version & 0x07);
  }

  Status = EFI_SUCCESS;

Done:
  FreeAlignedBuffer (SenseData, sizeof (EFI_SCSI_SENSE_DATA));
  FreeAlignedBuffer (InquiryData, sizeof (EFI_SCSI_INQUIRY_DATA));

  return Status;
}
