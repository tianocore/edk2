/** @file
  SCSI Bus driver that layers on every SCSI Pass Thru and
  Extended SCSI Pass Thru protocol in the system.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "ScsiBus.h"


EFI_DRIVER_BINDING_PROTOCOL gSCSIBusDriverBinding = {
  SCSIBusDriverBindingSupported,
  SCSIBusDriverBindingStart,
  SCSIBusDriverBindingStop,
  0xa,
  NULL,
  NULL
};


//
// The ScsiBusProtocol is just used to locate ScsiBusDev
// structure in the SCSIBusDriverBindingStop(). Then we can
// Close all opened protocols and release this structure.
//
EFI_GUID  mScsiBusProtocolGuid = EFI_SCSI_BUS_PROTOCOL_GUID;

VOID  *mWorkingBuffer;

/**
  Convert EFI_SCSI_IO_SCSI_REQUEST_PACKET packet to EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET packet.

  @param  Packet         The pointer of EFI_SCSI_IO_SCSI_REQUEST_PACKET
  @param  CommandPacket  The pointer of EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET

**/
EFI_STATUS
EFIAPI
ScsiioToPassThruPacket (
  IN      EFI_SCSI_IO_SCSI_REQUEST_PACKET         *Packet,
  OUT     EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *CommandPacket
  );

/**
  Convert EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET packet to EFI_SCSI_IO_SCSI_REQUEST_PACKET packet.

  @param  ScsiPacket  The pointer of EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET
  @param  Packet      The pointer of EFI_SCSI_IO_SCSI_REQUEST_PACKET

**/
EFI_STATUS
EFIAPI
PassThruToScsiioPacket (
  IN     EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *ScsiPacket,
  OUT    EFI_SCSI_IO_SCSI_REQUEST_PACKET         *Packet
  );

/**
  Notify Function in which convert EFI1.0 PassThru Packet back to UEF2.0
  SCSI IO Packet.

  @param  Event    The instance of EFI_EVENT.
  @param  Context  The parameter passed in.

**/
VOID
EFIAPI
NotifyFunction (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/**
  The user Entry Point for module ScsiBus. The user code starts with this function.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeScsiBus(
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
  EFI_STATUS  Status;
  EFI_SCSI_PASS_THRU_PROTOCOL *PassThru;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *ExtPassThru;
  //
  // Check for the existence of Extended SCSI Pass Thru Protocol and SCSI Pass Thru Protocol
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
  }

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiScsiPassThruProtocolGuid,
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

    gBS->CloseProtocol (
      Controller,
      &gEfiScsiPassThruProtocolGuid,
      This->DriverBindingHandle,
      Controller
      );
    return EFI_SUCCESS;
  }

  gBS->CloseProtocol (
    Controller,
    &gEfiExtScsiPassThruProtocolGuid,
    This->DriverBindingHandle,
    Controller
    );

  return EFI_SUCCESS;
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
  UINT64                                Lun;
  UINT8                                 *TargetId;
  BOOLEAN                               ScanOtherPuns;
  BOOLEAN                               FromFirstTarget;
  BOOLEAN                               ExtScsiSupport;
  EFI_STATUS                            Status;
  EFI_STATUS                            DevicePathStatus;
  EFI_STATUS                            PassThruStatus;
  SCSI_BUS_DEVICE                       *ScsiBusDev;
  SCSI_TARGET_ID                        *ScsiTargetId;
  EFI_DEVICE_PATH_PROTOCOL              *ParentDevicePath;
  EFI_SCSI_PASS_THRU_PROTOCOL           *ScsiInterface;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL       *ExtScsiInterface;
  EFI_SCSI_BUS_PROTOCOL                 *BusIdentify;

  TargetId        = NULL;
  ScsiTargetId    = NULL;
  ScanOtherPuns   = TRUE;
  FromFirstTarget = FALSE;
  ExtScsiSupport  = FALSE;
  PassThruStatus  = EFI_SUCCESS;
  
  ScsiTargetId = AllocateZeroPool(sizeof(SCSI_TARGET_ID));
  if (ScsiTargetId == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TargetId = &ScsiTargetId->ScsiId.ExtScsi[0];
  
  DevicePathStatus = gBS->OpenProtocol (
                            Controller,
                            &gEfiDevicePathProtocolGuid,
                            (VOID **) &ParentDevicePath,
                            This->DriverBindingHandle,
                            Controller,
                            EFI_OPEN_PROTOCOL_BY_DRIVER
                            );
  if (EFI_ERROR (DevicePathStatus) && (DevicePathStatus != EFI_ALREADY_STARTED)) {
    return DevicePathStatus;
  }

  //
  // To keep backward compatibility, UEFI ExtPassThru Protocol is supported as well as 
  // EFI PassThru Protocol. From priority perspective, ExtPassThru Protocol is firstly
  // tried to open on host controller handle. If fails, then PassThru Protocol is tried instead.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiExtScsiPassThruProtocolGuid,
                  (VOID **) &ExtScsiInterface,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  //
  // Fail to open UEFI ExtendPassThru Protocol, then try to open EFI PassThru Protocol instead.
  //
  if (EFI_ERROR(Status) && (Status != EFI_ALREADY_STARTED)) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiScsiPassThruProtocolGuid,
                    (VOID **) &ScsiInterface,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
    //
    // Fail to open EFI PassThru Protocol, Close the DevicePathProtocol if it is opened by this time.
    //
    if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
      if (!EFI_ERROR(DevicePathStatus)) {
        gBS->CloseProtocol (
               Controller,
               &gEfiDevicePathProtocolGuid,
               This->DriverBindingHandle,
               Controller
               );
      } 
      return Status;
    } 
  } else {
    //
    // Succeed to open ExtPassThru Protocol, and meanwhile open PassThru Protocol 
    // with BY_DRIVER if it is also present on the handle. The intent is to prevent 
    // another SCSI Bus Driver to work on the same host handle.
    //
    ExtScsiSupport = TRUE;
    PassThruStatus = gBS->OpenProtocol (
                            Controller,
                            &gEfiScsiPassThruProtocolGuid,
                            (VOID **) &ScsiInterface,
                            This->DriverBindingHandle,
                            Controller,
                            EFI_OPEN_PROTOCOL_BY_DRIVER
                            );
  }
    
  if (Status != EFI_ALREADY_STARTED) {
    //
    // Go through here means either ExtPassThru or PassThru Protocol is successfully opened
    // on this handle for this time. Then construct Host controller private data.
    //
    ScsiBusDev = NULL;
    ScsiBusDev = AllocateZeroPool(sizeof(SCSI_BUS_DEVICE));
    if (ScsiBusDev == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }
    ScsiBusDev->Signature        = SCSI_BUS_DEVICE_SIGNATURE;
    ScsiBusDev->ExtScsiSupport   = ExtScsiSupport;
    ScsiBusDev->DevicePath       = ParentDevicePath;
    if (ScsiBusDev->ExtScsiSupport) {
      ScsiBusDev->ExtScsiInterface = ExtScsiInterface;
    } else {
      ScsiBusDev->ScsiInterface    = ScsiInterface;    
    }

    //
    // Install EFI_SCSI_BUS_PROTOCOL to the controller handle, So ScsiBusDev could be
    // retrieved on this controller handle. With ScsiBusDev, we can know which PassThru
    // Protocol is present on the handle, UEFI ExtPassThru Protocol or EFI PassThru Protocol.
    // 
    Status = gBS->InstallProtocolInterface (
                    &Controller,
                    &mScsiBusProtocolGuid,
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
                    &mScsiBusProtocolGuid,
                    (VOID **) &BusIdentify,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }
    ScsiBusDev = SCSI_BUS_CONTROLLER_DEVICE_FROM_THIS (BusIdentify);
  }

  if (RemainingDevicePath == NULL) {
    SetMem (ScsiTargetId, TARGET_MAX_BYTES,0xFF);
    Lun  = 0;
    FromFirstTarget = TRUE;
  } else {
    if (ScsiBusDev->ExtScsiSupport) {
      ScsiBusDev->ExtScsiInterface->GetTargetLun (ScsiBusDev->ExtScsiInterface, RemainingDevicePath, &TargetId, &Lun);  
    } else {
      ScsiBusDev->ScsiInterface->GetTargetLun (ScsiBusDev->ScsiInterface, RemainingDevicePath, &ScsiTargetId->ScsiId.Scsi, &Lun);
    }
  }

  while(ScanOtherPuns) {
    if (FromFirstTarget) {
      //
      // Remaining Device Path is NULL, scan all the possible Puns in the
      // SCSI Channel.
      //
      if (ScsiBusDev->ExtScsiSupport) {
        Status = ScsiBusDev->ExtScsiInterface->GetNextTargetLun (ScsiBusDev->ExtScsiInterface, &TargetId, &Lun);
      } else {
        Status = ScsiBusDev->ScsiInterface->GetNextDevice (ScsiBusDev->ScsiInterface, &ScsiTargetId->ScsiId.Scsi, &Lun);
      }
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
    if (ScsiBusDev->ExtScsiSupport) {
      if ((ScsiTargetId->ScsiId.Scsi) == ScsiBusDev->ExtScsiInterface->Mode->AdapterId) {
        continue;
      }
    } else {
      if ((ScsiTargetId->ScsiId.Scsi) == ScsiBusDev->ScsiInterface->Mode->AdapterId) {
        continue;
      }
    }
    //
    // Scan for the scsi device, if it attaches to the scsi bus,
    // then create handle and install scsi i/o protocol.
    //
    Status = ScsiScanCreateDevice (This, Controller, ScsiTargetId, Lun, ScsiBusDev);
  }
  FreePool (ScsiTargetId);
  return EFI_SUCCESS;

ErrorExit:
  
  if (ScsiBusDev != NULL) {
    FreePool (ScsiBusDev);
  }
  
  if (ExtScsiSupport) {
    gBS->CloseProtocol (
           Controller,
           &gEfiExtScsiPassThruProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    if (!EFI_ERROR (PassThruStatus)) {
      gBS->CloseProtocol (
             Controller,
             &gEfiScsiPassThruProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }
  } else {
    gBS->CloseProtocol (
           Controller,
           &gEfiScsiPassThruProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }
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
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  BOOLEAN                     AllChildrenStopped;
  UINTN                       Index;
  EFI_SCSI_IO_PROTOCOL        *ScsiIo;
  SCSI_IO_DEV                 *ScsiIoDevice;
  VOID                        *ScsiPassThru;
  EFI_SCSI_BUS_PROTOCOL       *Scsidentifier;
  SCSI_BUS_DEVICE             *ScsiBusDev;

  if (NumberOfChildren == 0) {
    //
    // Get the SCSI_BUS_DEVICE
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &mScsiBusProtocolGuid,
                    (VOID **) &Scsidentifier,
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
           &mScsiBusProtocolGuid,
           &ScsiBusDev->BusIdentify
           );

    //
    // Close the bus driver
    //
    if (ScsiBusDev->ExtScsiSupport) {
      gBS->CloseProtocol (
             Controller,
             &gEfiExtScsiPassThruProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    } else {
      gBS->CloseProtocol (
             Controller,
             &gEfiScsiPassThruProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

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
                    (VOID **) &ScsiIo,
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
    if (ScsiIoDevice->ExtScsiSupport) {
      Status = gBS->CloseProtocol (
                      Controller,
                      &gEfiExtScsiPassThruProtocolGuid,
                      This->DriverBindingHandle,
                      ChildHandleBuffer[Index]
                      );

    } else {
      Status = gBS->CloseProtocol (
                      Controller,
                      &gEfiScsiPassThruProtocolGuid,
                      This->DriverBindingHandle,
                      ChildHandleBuffer[Index]
                      );
    }

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
      if (ScsiIoDevice->ExtScsiSupport) {
        gBS->OpenProtocol (
               Controller,
               &gEfiExtScsiPassThruProtocolGuid,
               &ScsiPassThru,
               This->DriverBindingHandle,
               ChildHandleBuffer[Index],
               EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
               );
      } else {
        gBS->OpenProtocol (
               Controller,
               &gEfiScsiPassThruProtocolGuid,
               &ScsiPassThru,
               This->DriverBindingHandle,
               ChildHandleBuffer[Index],
               EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
               );
      }
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
  IN  EFI_SCSI_IO_PROTOCOL     *This,
  OUT UINT8                    *DeviceType
  )
{
  SCSI_IO_DEV *ScsiIoDevice;

  if (DeviceType == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ScsiIoDevice  = SCSI_IO_DEV_FROM_THIS (This);
  *DeviceType   = ScsiIoDevice->ScsiDeviceType;
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
  IN  EFI_SCSI_IO_PROTOCOL    *This,
  IN OUT UINT8                **Target,
  OUT UINT64                  *Lun
  )
{
  SCSI_IO_DEV *ScsiIoDevice;

  if (Target == NULL || Lun == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ScsiIoDevice = SCSI_IO_DEV_FROM_THIS (This);

  CopyMem (*Target,&ScsiIoDevice->Pun, TARGET_MAX_BYTES);

  *Lun         = ScsiIoDevice->Lun;

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
  IN  EFI_SCSI_IO_PROTOCOL     *This
  )
{
  SCSI_IO_DEV *ScsiIoDevice;

  ScsiIoDevice = SCSI_IO_DEV_FROM_THIS (This);

  if (ScsiIoDevice->ExtScsiSupport){
    return ScsiIoDevice->ExtScsiPassThru->ResetChannel (ScsiIoDevice->ExtScsiPassThru);
  } else {
    return ScsiIoDevice->ScsiPassThru->ResetChannel (ScsiIoDevice->ScsiPassThru);
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
  IN  EFI_SCSI_IO_PROTOCOL     *This
  )
{
  SCSI_IO_DEV  *ScsiIoDevice;
  UINT8        Target[TARGET_MAX_BYTES];

  ScsiIoDevice = SCSI_IO_DEV_FROM_THIS (This);
  CopyMem (Target,&ScsiIoDevice->Pun, TARGET_MAX_BYTES);


  if (ScsiIoDevice->ExtScsiSupport) {
    return ScsiIoDevice->ExtScsiPassThru->ResetTargetLun (
                                        ScsiIoDevice->ExtScsiPassThru,
                                        Target,
                                        ScsiIoDevice->Lun
                                          );
  } else {
    return ScsiIoDevice->ScsiPassThru->ResetTarget (
                                          ScsiIoDevice->ScsiPassThru,
                                          ScsiIoDevice->Pun.ScsiId.Scsi,
                                          ScsiIoDevice->Lun
                                            );
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
  IN     EFI_SCSI_IO_PROTOCOL                     *This,
  IN OUT EFI_SCSI_IO_SCSI_REQUEST_PACKET          *Packet,
  IN     EFI_EVENT                                Event  OPTIONAL
  )
{
  SCSI_IO_DEV                                 *ScsiIoDevice;
  EFI_STATUS                                  Status;
  UINT8                                       Target[TARGET_MAX_BYTES];
  EFI_EVENT                                   PacketEvent;
  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *ExtRequestPacket;
  SCSI_EVENT_DATA                             EventData;                                     

  PacketEvent = NULL;
  
  if (Packet == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ScsiIoDevice  = SCSI_IO_DEV_FROM_THIS (This);
  CopyMem (Target,&ScsiIoDevice->Pun, TARGET_MAX_BYTES);

  if (ScsiIoDevice->ExtScsiSupport) {
    ExtRequestPacket = (EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *) Packet;
    Status = ScsiIoDevice->ExtScsiPassThru->PassThru (
                                          ScsiIoDevice->ExtScsiPassThru,
                                          Target,
                                          ScsiIoDevice->Lun,
                                          ExtRequestPacket,
                                          Event
                                          );
  } else {

    mWorkingBuffer = AllocatePool (sizeof(EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET));

    if (mWorkingBuffer == NULL) {
      return EFI_DEVICE_ERROR;
    }

    //
    // Convert package into EFI1.0, EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET.
    //
    Status = ScsiioToPassThruPacket(Packet, (EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET*)mWorkingBuffer);
    if (EFI_ERROR(Status)) {
      FreePool(mWorkingBuffer);
      return Status;
    }

    if (((ScsiIoDevice->ScsiPassThru->Mode->Attributes & EFI_SCSI_PASS_THRU_ATTRIBUTES_NONBLOCKIO) != 0) && (Event !=  NULL)) {
      EventData.Data1 = (VOID*)Packet;
      EventData.Data2 = Event;
      //
      // Create Event
      //
      Status = gBS->CreateEvent (
                       EVT_NOTIFY_SIGNAL,
                       TPL_CALLBACK,
                       NotifyFunction,
                       &EventData,
                       &PacketEvent
                       );
      if (EFI_ERROR(Status)) {
        FreePool(mWorkingBuffer);
        return Status;
      }

      Status = ScsiIoDevice->ScsiPassThru->PassThru (
                                          ScsiIoDevice->ScsiPassThru,
                                          ScsiIoDevice->Pun.ScsiId.Scsi,
                                          ScsiIoDevice->Lun,
                                          mWorkingBuffer,
                                          PacketEvent
                                          );

      if (EFI_ERROR(Status)) {
        FreePool(mWorkingBuffer);
        gBS->CloseEvent(PacketEvent);
        return Status;
      }

    } else {
      //
      // If there's no event or SCSI Device doesn't support NON-BLOCKING, just convert
      // EFI1.0 PassThru packet back to UEFI2.0 SCSI IO Packet.
      //
      Status = ScsiIoDevice->ScsiPassThru->PassThru (
                                          ScsiIoDevice->ScsiPassThru,
                                          ScsiIoDevice->Pun.ScsiId.Scsi,
                                          ScsiIoDevice->Lun,
                                          mWorkingBuffer,
                                          Event
                                          );
      if (EFI_ERROR(Status)) {
        FreePool(mWorkingBuffer);
        return Status;
      }

      PassThruToScsiioPacket((EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET*)mWorkingBuffer,Packet);
      //
      // After converting EFI1.0 PassThru Packet back to UEFI2.0 SCSI IO Packet,
      // free mWorkingBuffer.
      //
      FreePool(mWorkingBuffer);
    }
  }
  return Status;
}


/**
  Scan SCSI Bus to discover the device, and attach ScsiIoProtocol to it.

  @param  This           Protocol instance pointer
  @param  Controller     Controller handle
  @param  TargetId       Tartget to be scanned
  @param  Lun            The Lun of the SCSI device on the SCSI channel.
  @param  ScsiBusDev     The pointer of SCSI_BUS_DEVICE

  @retval EFI_SUCCESS           Successfully to discover the device and attach
                                ScsiIoProtocol to it.
  @retval EFI_OUT_OF_RESOURCES  Fail to discover the device.

**/
EFI_STATUS
EFIAPI
ScsiScanCreateDevice (
  IN     EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN     EFI_HANDLE                    Controller,
  IN     SCSI_TARGET_ID                *TargetId,
  IN     UINT64                        Lun,
  IN OUT SCSI_BUS_DEVICE               *ScsiBusDev
  )
{
  EFI_STATUS                Status;
  SCSI_IO_DEV               *ScsiIoDevice;
  EFI_DEVICE_PATH_PROTOCOL  *ScsiDevicePath;

  ScsiIoDevice = AllocateZeroPool (sizeof (SCSI_IO_DEV));
  if (ScsiIoDevice == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ScsiIoDevice->Signature                 = SCSI_IO_DEV_SIGNATURE;
  CopyMem(&ScsiIoDevice->Pun, TargetId, TARGET_MAX_BYTES);
  ScsiIoDevice->Lun                       = Lun;

  if (ScsiBusDev->ExtScsiSupport) {
    ScsiIoDevice->ExtScsiPassThru         = ScsiBusDev->ExtScsiInterface;
    ScsiIoDevice->ExtScsiSupport          = TRUE;
    ScsiIoDevice->ScsiIo.IoAlign          = ScsiIoDevice->ExtScsiPassThru->Mode->IoAlign;

  } else {
    ScsiIoDevice->ScsiPassThru            = ScsiBusDev->ScsiInterface;
    ScsiIoDevice->ExtScsiSupport          = FALSE;
    ScsiIoDevice->ScsiIo.IoAlign          = ScsiIoDevice->ScsiPassThru->Mode->IoAlign;
  }

  ScsiIoDevice->ScsiIo.GetDeviceType      = ScsiGetDeviceType;
  ScsiIoDevice->ScsiIo.GetDeviceLocation  = ScsiGetDeviceLocation;
  ScsiIoDevice->ScsiIo.ResetBus           = ScsiResetBus;
  ScsiIoDevice->ScsiIo.ResetDevice        = ScsiResetDevice;
  ScsiIoDevice->ScsiIo.ExecuteScsiCommand = ScsiExecuteSCSICommand;


  if (!DiscoverScsiDevice (ScsiIoDevice)) {
    FreePool (ScsiIoDevice);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set Device Path
  //
  ScsiDevicePath = NULL;
  if (ScsiIoDevice->ExtScsiSupport){
    Status = ScsiIoDevice->ExtScsiPassThru->BuildDevicePath (
                                          ScsiIoDevice->ExtScsiPassThru,
                                          &ScsiIoDevice->Pun.ScsiId.ExtScsi[0],
                                          ScsiIoDevice->Lun,
                                          &ScsiDevicePath
                                          );
  } else {
    Status = ScsiIoDevice->ScsiPassThru->BuildDevicePath (
                                          ScsiIoDevice->ScsiPassThru,
                                          ScsiIoDevice->Pun.ScsiId.Scsi,
                                          ScsiIoDevice->Lun,
                                          &ScsiDevicePath
                                          );
  }

  if (Status == EFI_OUT_OF_RESOURCES) {
    FreePool (ScsiIoDevice);
    return Status;
  }

  ScsiIoDevice->DevicePath = AppendDevicePathNode (
                              ScsiBusDev->DevicePath,
                              ScsiDevicePath
                              );
  //
  // The memory space for ScsiDevicePath is allocated in
  // ScsiPassThru->BuildDevicePath() function; It is no longer used
  // after EfiAppendDevicePathNode,so free the memory it occupies.
  //
  FreePool (ScsiDevicePath);

  if (ScsiIoDevice->DevicePath == NULL) {
    FreePool (ScsiIoDevice);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ScsiIoDevice->Handle,
                  &gEfiDevicePathProtocolGuid,
                  ScsiIoDevice->DevicePath,
                  &gEfiScsiIoProtocolGuid,
                  &ScsiIoDevice->ScsiIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (ScsiIoDevice->DevicePath);
    FreePool (ScsiIoDevice);
    return EFI_OUT_OF_RESOURCES;
  } else {
    if (ScsiBusDev->ExtScsiSupport) {
      gBS->OpenProtocol (
            Controller,
            &gEfiExtScsiPassThruProtocolGuid,
            (VOID **) &(ScsiBusDev->ExtScsiInterface),
            This->DriverBindingHandle,
            ScsiIoDevice->Handle,
            EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
            );
     } else {
      gBS->OpenProtocol (
            Controller,
            &gEfiScsiPassThruProtocolGuid,
            (VOID **) &(ScsiBusDev->ScsiInterface),
            This->DriverBindingHandle,
            ScsiIoDevice->Handle,
            EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
            );
     }
  }
  return EFI_SUCCESS;
}


/**
  Discovery SCSI Device

  @param  ScsiIoDevice    The pointer of SCSI_IO_DEV

  @retval  TRUE   Find SCSI Device and verify it.
  @retval  FALSE  Unable to find SCSI Device.

**/
BOOLEAN
DiscoverScsiDevice (
  IN OUT  SCSI_IO_DEV   *ScsiIoDevice
  )
{
  EFI_STATUS            Status;
  UINT32                InquiryDataLength;
  UINT8                 SenseDataLength;
  UINT8                 HostAdapterStatus;
  UINT8                 TargetStatus;
  EFI_SCSI_SENSE_DATA   SenseData;
  EFI_SCSI_INQUIRY_DATA InquiryData;

  HostAdapterStatus = 0;
  TargetStatus      = 0;
  //
  // Using Inquiry command to scan for the device
  //
  InquiryDataLength = sizeof (EFI_SCSI_INQUIRY_DATA);
  SenseDataLength   = sizeof (EFI_SCSI_SENSE_DATA);

  Status = ScsiInquiryCommand (
            &ScsiIoDevice->ScsiIo,
            EFI_TIMER_PERIOD_SECONDS (1),
            (VOID *) &SenseData,
            &SenseDataLength,
            &HostAdapterStatus,
            &TargetStatus,
            (VOID *) &InquiryData,
            &InquiryDataLength,
            FALSE
            );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  //
  // Retrieved inquiry data successfully
  //
  if ((InquiryData.Peripheral_Qualifier != 0) &&
      (InquiryData.Peripheral_Qualifier != 3)) {
    return FALSE;
  }

  if (InquiryData.Peripheral_Qualifier == 3) {
    if (InquiryData.Peripheral_Type != 0x1f) {
      return FALSE;
    }
  }

  if (0x1e >= InquiryData.Peripheral_Type && InquiryData.Peripheral_Type >= 0xa) {
    return FALSE;
  }

  //
  // valid device type and peripheral qualifier combination.
  //
  ScsiIoDevice->ScsiDeviceType  = InquiryData.Peripheral_Type;
  ScsiIoDevice->RemovableDevice = InquiryData.RMB;
  if (InquiryData.Version == 0) {
    ScsiIoDevice->ScsiVersion = 0;
  } else {
    //
    // ANSI-approved version
    //
    ScsiIoDevice->ScsiVersion = (UINT8) (InquiryData.Version & 0x03);
  }

  return TRUE;
}


/**
  Convert EFI_SCSI_IO_SCSI_REQUEST_PACKET packet to EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET packet.

  @param  Packet         The pointer of EFI_SCSI_IO_SCSI_REQUEST_PACKET
  @param  CommandPacket  The pointer of EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET

**/
EFI_STATUS
EFIAPI
ScsiioToPassThruPacket (
  IN      EFI_SCSI_IO_SCSI_REQUEST_PACKET         *Packet,
  OUT     EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *CommandPacket
  )
{
  //
  //EFI 1.10 doesn't support Bi-Direction Command.
  //
  if (Packet->DataDirection == EFI_SCSI_IO_DATA_DIRECTION_BIDIRECTIONAL) {
    return EFI_UNSUPPORTED;
  }

  ZeroMem (CommandPacket, sizeof (EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET));

  CommandPacket->Timeout           = Packet->Timeout;
  CommandPacket->Cdb               = Packet->Cdb;
  CommandPacket->CdbLength         = Packet->CdbLength;
  CommandPacket->DataDirection     = Packet->DataDirection;
  CommandPacket->HostAdapterStatus = Packet->HostAdapterStatus;
  CommandPacket->TargetStatus      = Packet->TargetStatus;
  CommandPacket->SenseData         = Packet->SenseData;
  CommandPacket->SenseDataLength   = Packet->SenseDataLength;

  if (Packet->DataDirection == EFI_SCSI_IO_DATA_DIRECTION_READ) {
    CommandPacket->DataBuffer = Packet->InDataBuffer;
    CommandPacket->TransferLength = Packet->InTransferLength;
  } else if (Packet->DataDirection == EFI_SCSI_IO_DATA_DIRECTION_WRITE) {
    CommandPacket->DataBuffer = Packet->OutDataBuffer;
    CommandPacket->TransferLength = Packet->OutTransferLength;
  }
  return EFI_SUCCESS;
}


/**
  Convert EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET packet to EFI_SCSI_IO_SCSI_REQUEST_PACKET packet.

  @param  ScsiPacket  The pointer of EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET
  @param  Packet      The pointer of EFI_SCSI_IO_SCSI_REQUEST_PACKET

**/
EFI_STATUS
EFIAPI
PassThruToScsiioPacket (
  IN     EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *ScsiPacket,
  OUT    EFI_SCSI_IO_SCSI_REQUEST_PACKET         *Packet
  )
{
  Packet->Timeout           = ScsiPacket->Timeout;
  Packet->Cdb               = ScsiPacket->Cdb;
  Packet->CdbLength         = ScsiPacket->CdbLength;
  Packet->DataDirection     = ScsiPacket->DataDirection;
  Packet->HostAdapterStatus = ScsiPacket->HostAdapterStatus;
  Packet->TargetStatus      = ScsiPacket->TargetStatus;
  Packet->SenseData         = ScsiPacket->SenseData;
  Packet->SenseDataLength   = ScsiPacket->SenseDataLength;

  if (ScsiPacket->DataDirection == EFI_SCSI_IO_DATA_DIRECTION_READ) {
    Packet->InDataBuffer = ScsiPacket->DataBuffer;
    Packet->InTransferLength = ScsiPacket->TransferLength;
  } else if (Packet->DataDirection == EFI_SCSI_IO_DATA_DIRECTION_WRITE) {
    Packet->OutDataBuffer = ScsiPacket->DataBuffer;
    Packet->OutTransferLength = ScsiPacket->TransferLength;
  }

  return EFI_SUCCESS;
}

/**
  Notify Function in which convert EFI1.0 PassThru Packet back to UEF2.0
  SCSI IO Packet.

  @param  Event    The instance of EFI_EVENT.
  @param  Context  The parameter passed in.

**/
VOID
EFIAPI
NotifyFunction (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_SCSI_IO_SCSI_REQUEST_PACKET          *Packet;
  EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET   *ScsiPacket;
  EFI_EVENT                                CallerEvent;
  SCSI_EVENT_DATA                          *PassData;

  PassData = (SCSI_EVENT_DATA*)Context;
  Packet  = (EFI_SCSI_IO_SCSI_REQUEST_PACKET *)PassData->Data1;
  ScsiPacket =  (EFI_SCSI_PASS_THRU_SCSI_REQUEST_PACKET*)mWorkingBuffer;

  //
  // Convert EFI1.0 PassThru packet to UEFI2.0 SCSI IO Packet.
  //
  PassThruToScsiioPacket(ScsiPacket, Packet);

  //
  // After converting EFI1.0 PassThru Packet back to UEFI2.0 SCSI IO Packet,
  // free mWorkingBuffer.
  //
  gBS->FreePool(mWorkingBuffer);

  //
  // Signal Event to tell caller to pick up UEFI2.0 SCSI IO Packet.
  //
  CallerEvent = PassData->Data2;
  gBS->CloseEvent(Event);
  gBS->SignalEvent(CallerEvent);
}

