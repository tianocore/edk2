/** @file

Copyright (c) 2005 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Tcp4Driver.c

Abstract:


**/

#include "Tcp4Main.h"


UINT16                                mTcp4RandomPort;
extern EFI_COMPONENT_NAME_PROTOCOL    gTcp4ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL   gTcp4ComponentName2;

TCP4_HEARTBEAT_TIMER  mTcp4Timer = {
  NULL,
  0
};

EFI_TCP4_PROTOCOL mTcp4ProtocolTemplate = {
  Tcp4GetModeData,
  Tcp4Configure,
  Tcp4Routes,
  Tcp4Connect,
  Tcp4Accept,
  Tcp4Transmit,
  Tcp4Receive,
  Tcp4Close,
  Tcp4Cancel,
  Tcp4Poll
};

SOCK_INIT_DATA mTcp4DefaultSockData = {
  SOCK_STREAM,
  (SOCK_STATE) 0,
  NULL,
  TCP_BACKLOG,
  TCP_SND_BUF_SIZE,
  TCP_RCV_BUF_SIZE,
  &mTcp4ProtocolTemplate,
  Tcp4CreateSocketCallback,
  Tcp4DestroySocketCallback,
  NULL,
  NULL,
  0,
  Tcp4Dispatcher,
  NULL,
};

EFI_DRIVER_BINDING_PROTOCOL mTcp4DriverBinding = {
  Tcp4DriverBindingSupported,
  Tcp4DriverBindingStart,
  Tcp4DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL mTcp4ServiceBinding = {
  Tcp4ServiceBindingCreateChild,
  Tcp4ServiceBindingDestroyChild
};


/**
  Create and start the heartbeat timer for TCP driver.

  @retval EFI_SUCCESS            The timer is successfully created and started.
  @retval other                  The timer is not created.

**/
EFI_STATUS
Tcp4CreateTimer (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (mTcp4Timer.RefCnt == 0) {

    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    TcpTicking,
                    NULL,
                    &mTcp4Timer.TimerEvent
                    );
    if (!EFI_ERROR (Status)) {

      Status = gBS->SetTimer (
                      mTcp4Timer.TimerEvent,
                      TimerPeriodic,
                      (UINT64) (TICKS_PER_SECOND / TCP_TICK_HZ)
                      );
    }
  }

  if (!EFI_ERROR (Status)) {

    mTcp4Timer.RefCnt++;
  }

  return Status;
}


/**
  Stop and destroy the heartbeat timer for TCP driver.
  
**/
VOID
Tcp4DestroyTimer (
  VOID
  )
{
  ASSERT (mTcp4Timer.RefCnt > 0);

  mTcp4Timer.RefCnt--;

  if (mTcp4Timer.RefCnt > 0) {
    return;
  }

  gBS->SetTimer (mTcp4Timer.TimerEvent, TimerCancel, 0);
  gBS->CloseEvent (mTcp4Timer.TimerEvent);
  mTcp4Timer.TimerEvent = NULL;
}

/**
  The entry point for Tcp4 driver, used to install Tcp4 driver on the ImageHandle.

  @param  ImageHandle   The firmware allocated handle for this
                        driver image.
  @param  SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS   Driver loaded.
  @retval other         Driver not loaded.

**/
EFI_STATUS
EFIAPI
Tcp4DriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT32      Seed;

  //
  // Install the TCP4 Driver Binding Protocol
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &mTcp4DriverBinding,
             ImageHandle,
             &gTcp4ComponentName,
             &gTcp4ComponentName2
             );
  ASSERT_EFI_ERROR (Status);
  //
  // Initialize ISS and random port.
  //
  Seed            = NetRandomInitSeed ();
  mTcpGlobalIss   = NET_RANDOM (Seed) % mTcpGlobalIss;
  mTcp4RandomPort = (UINT16) (TCP4_PORT_KNOWN +
                    (UINT16) (NET_RANDOM(Seed) % TCP4_PORT_KNOWN));

  return Status;
}


/**
  Test to see if this driver supports ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to test.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            This driver supports this device.
  @retval EFI_ALREADY_STARTED    This driver is already running on this device.
  @retval other                  This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Tcp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // Test for the Tcp4ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiTcp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test for the Ip4 Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiIp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  return Status;
}


/**
  Start this driver on ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The driver is added to ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources to start the
                                 driver.
  @retval other                  The driver cannot be added to ControllerHandle.

**/
EFI_STATUS
EFIAPI
Tcp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS               Status;
  TCP4_SERVICE_DATA        *TcpServiceData;
  IP_IO_OPEN_DATA          OpenData;

  TcpServiceData = AllocateZeroPool (sizeof (TCP4_SERVICE_DATA));

  if (NULL == TcpServiceData) {
    DEBUG ((EFI_D_ERROR, "Tcp4DriverBindingStart: Have no enough"
      " resource to create a Tcp Servcie Data!\n"));

    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create a new IP IO to Consume it
  //
  TcpServiceData->IpIo = IpIoCreate (This->DriverBindingHandle, ControllerHandle);
  if (NULL == TcpServiceData->IpIo) {

    DEBUG ((EFI_D_ERROR, "Tcp4DriverBindingStart: Have no enough"
      " resource to create an Ip Io!\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Configure and start IpIo.
  //
  ZeroMem (&OpenData, sizeof (IP_IO_OPEN_DATA));

  CopyMem (&OpenData.IpConfigData, &mIpIoDefaultIpConfigData, sizeof (OpenData.IpConfigData));
  OpenData.IpConfigData.DefaultProtocol = EFI_IP_PROTO_TCP;

  OpenData.PktRcvdNotify = Tcp4RxCallback;
  Status                 = IpIoOpen (TcpServiceData->IpIo, &OpenData);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Create the timer event used by TCP driver
  //
  Status = Tcp4CreateTimer ();
  if (EFI_ERROR (Status)) {

    DEBUG ((EFI_D_ERROR, "Tcp4DriverBindingStart: Create TcpTimer"
      " Event failed with %r\n", Status));

    goto ON_ERROR;
  }

  //
  // Install the Tcp4ServiceBinding Protocol on the
  // controller handle
  //
  TcpServiceData->Tcp4ServiceBinding = mTcp4ServiceBinding;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiTcp4ServiceBindingProtocolGuid,
                  &TcpServiceData->Tcp4ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {

    DEBUG ((EFI_D_ERROR, "Tcp4DriverBindingStart: Install Tcp4 Service Binding"
      " Protocol failed for %r\n", Status));

    Tcp4DestroyTimer ();
    goto ON_ERROR;
  }

  //
  // Initialize member in TcpServiceData
  //
  TcpServiceData->ControllerHandle    = ControllerHandle;
  TcpServiceData->Signature           = TCP4_DRIVER_SIGNATURE;
  TcpServiceData->DriverBindingHandle = This->DriverBindingHandle;

  InitializeListHead (&TcpServiceData->SocketList);

  TcpSetVariableData (TcpServiceData);

  return EFI_SUCCESS;

ON_ERROR:

  if (TcpServiceData->IpIo != NULL) {
    IpIoDestroy (TcpServiceData->IpIo);
  }

  gBS->FreePool (TcpServiceData);

  return Status;
}


/**
  Stop this driver on ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to stop driver on.
  @param  NumberOfChildren       Number of Handles in ChildHandleBuffer. If number
                                 of children is zero stop the entire bus driver.
  @param  ChildHandleBuffer      List of Child Handles to Stop.

  @retval EFI_SUCCESS            This driver is removed from ControllerHandle.
  @retval other                  This driver is not removed from ControllerHandle.

**/
EFI_STATUS
EFIAPI
Tcp4DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          NicHandle;
  EFI_SERVICE_BINDING_PROTOCOL        *ServiceBinding;
  TCP4_SERVICE_DATA                   *TcpServiceData;
  SOCKET                              *Sock;

  // Find the NicHandle where Tcp4 ServiceBinding Protocol is installed.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiIp4ProtocolGuid);
  if (NicHandle == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Retrieve the TCP driver Data Structure
  //
  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiTcp4ServiceBindingProtocolGuid,
                  (VOID **) &ServiceBinding,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {

    DEBUG ((EFI_D_ERROR, "Tcp4DriverBindingStop: Locate Tcp4 Service "
      " Binding Protocol failed with %r\n", Status));

    return EFI_DEVICE_ERROR;
  }

  TcpServiceData = TCP4_FROM_THIS (ServiceBinding);

  if (NumberOfChildren == 0) {
    //
    // Uninstall TCP servicebinding protocol
    //
    gBS->UninstallMultipleProtocolInterfaces (
           NicHandle,
           &gEfiTcp4ServiceBindingProtocolGuid,
           ServiceBinding,
           NULL
           );

    //
    // Destroy the IpIO consumed by TCP driver
    //
    IpIoDestroy (TcpServiceData->IpIo);

    //
    // Destroy the heartbeat timer.
    //
    Tcp4DestroyTimer ();

    //
    // Clear the variable.
    //
    TcpClearVariableData (TcpServiceData);

    //
    // Release the TCP service data
    //
    gBS->FreePool (TcpServiceData);
  } else {

    while (!IsListEmpty (&TcpServiceData->SocketList)) {
      Sock = NET_LIST_HEAD (&TcpServiceData->SocketList, SOCKET, Link);

      ServiceBinding->DestroyChild (ServiceBinding, Sock->SockHandle);
    }
  }

  return Status;
}

/**
  Open Ip4 and device path protocols for a created socket, and insert it in 
  socket list.
  
  @param  This                Pointer to the socket just created
  @param  Context             Context of the socket
  
  @retval EFI_SUCCESS         This protocol is installed successfully.
  @retval other               Some error occured.
  
**/
EFI_STATUS
Tcp4CreateSocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  )
{
  EFI_STATUS         Status;
  TCP4_SERVICE_DATA  *TcpServiceData;
  EFI_IP4_PROTOCOL   *Ip4;

  TcpServiceData = ((TCP4_PROTO_DATA *) This->ProtoReserved)->TcpService;

  //
  // Open the default Ip4 protocol of IP_IO BY_DRIVER.
  //
  Status = gBS->OpenProtocol (
                  TcpServiceData->IpIo->ChildHandle,
                  &gEfiIp4ProtocolGuid,
                  (VOID **) &Ip4,
                  TcpServiceData->DriverBindingHandle,
                  This->SockHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the device path on the handle where service binding resides on.
  //
  Status = gBS->OpenProtocol (
                  TcpServiceData->ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &This->ParentDevicePath,
                  TcpServiceData->DriverBindingHandle,
                  This->SockHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           TcpServiceData->IpIo->ChildHandle,
           &gEfiIp4ProtocolGuid,
           TcpServiceData->DriverBindingHandle,
           This->SockHandle
           );
  } else {
    //
    // Insert this socket into the SocketList.
    //
    InsertTailList (&TcpServiceData->SocketList, &This->Link);
  }

  return Status;
}

/**
  Close Ip4 and device path protocols for a socket, and remove it from socket list. 
    
  @param  This                Pointer to the socket to be removed
  @param  Context             Context of the socket
  
**/
VOID
Tcp4DestroySocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  )
{
  TCP4_SERVICE_DATA  *TcpServiceData;

  TcpServiceData = ((TCP4_PROTO_DATA *) This->ProtoReserved)->TcpService;

  //
  // Remove this node from the list.
  //
  RemoveEntryList (&This->Link);

  //
  // Close the device path protocol
  //
  gBS->CloseProtocol (
         TcpServiceData->ControllerHandle,
         &gEfiDevicePathProtocolGuid,
         TcpServiceData->DriverBindingHandle,
         This->SockHandle
         );

  //
  // Close the Ip4 protocol.
  //
  gBS->CloseProtocol (
         TcpServiceData->IpIo->ChildHandle,
         &gEfiIp4ProtocolGuid,
         TcpServiceData->DriverBindingHandle,
         This->SockHandle
         );
}

/**
  Creates a child handle with a set of TCP4 services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Pointer to the handle of the child to create.  If
                                 it is NULL, then a new handle is created. If it is
                                 not NULL, then the I/O services are added to the
                                 existing child handle.

  @retval EFI_SUCCESS            The child handle is created.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources to create the
                                 child.

**/
EFI_STATUS
EFIAPI
Tcp4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  SOCKET            *Sock;
  TCP4_SERVICE_DATA *TcpServiceData;
  TCP4_PROTO_DATA   TcpProto;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  if (NULL == This || NULL == ChildHandle) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl              = gBS->RaiseTPL (TPL_CALLBACK);
  Status              = EFI_SUCCESS;
  TcpServiceData      = TCP4_FROM_THIS (This);
  TcpProto.TcpService = TcpServiceData;
  TcpProto.TcpPcb     = NULL;

  //
  // Create a tcp instance with defualt Tcp default
  // sock init data and TcpProto
  //
  mTcp4DefaultSockData.ProtoData     = &TcpProto;
  mTcp4DefaultSockData.DataSize      = sizeof (TCP4_PROTO_DATA);
  mTcp4DefaultSockData.DriverBinding = TcpServiceData->DriverBindingHandle;

  Sock = SockCreateChild (&mTcp4DefaultSockData);
  if (NULL == Sock) {
    DEBUG ((EFI_D_ERROR, "Tcp4DriverBindingCreateChild: "
      "No resource to create a Tcp Child\n"));

    Status = EFI_OUT_OF_RESOURCES;
  } else {
    *ChildHandle = Sock->SockHandle;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Destroys a child handle with a set of UDP4 services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Handle of the child to be destroyed.

  @retval EFI_SUCCESS            The TCP4 services are removed from  the child
                                 handle.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval other                  The child handle is not destroyed.

**/
EFI_STATUS
EFIAPI
Tcp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS         Status;
  EFI_TCP4_PROTOCOL  *Tcp4;
  SOCKET             *Sock;
  EFI_TPL            OldTpl;

  if (NULL == This || NULL == ChildHandle) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // retrieve the Tcp4 protocol from ChildHandle
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiTcp4ProtocolGuid,
                  (VOID **) &Tcp4,
                  mTcp4DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
  } else {
    //
    // destroy this sock and related Tcp protocol control
    // block
    //
    Sock = SOCK_FROM_THIS (Tcp4);

    SockDestroyChild (Sock);
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

