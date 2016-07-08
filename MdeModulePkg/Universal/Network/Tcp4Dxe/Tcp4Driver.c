/** @file
  Tcp driver function.

Copyright (c) 2005 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Tcp4Main.h"


UINT16                                mTcp4RandomPort;
extern EFI_COMPONENT_NAME_PROTOCOL    gTcp4ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL   gTcp4ComponentName2;
extern EFI_UNICODE_STRING_TABLE       *gTcpControllerNameTable;

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
  SockStream,
  0,
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
  Callback function which provided by user to remove one node in NetDestroyLinkList process.
  
  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
Tcp4DestroyChildEntryInHandleBuffer (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context
  )
{
  SOCKET                        *Sock;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;

  if (Entry == NULL || Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Sock = NET_LIST_USER_STRUCT_S (Entry, SOCKET, Link, SOCK_SIGNATURE);
  ServiceBinding    = ((TCP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->ServiceBinding;
  NumberOfChildren  = ((TCP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->NumberOfChildren;
  ChildHandleBuffer = ((TCP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->ChildHandleBuffer;

  if (!NetIsInHandleBuffer (Sock->SockHandle, NumberOfChildren, ChildHandleBuffer)) {
    return EFI_SUCCESS;
  }

  return ServiceBinding->DestroyChild (ServiceBinding, Sock->SockHandle);
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
  Tests to see if this driver supports a given controller.
  
  If a child device is provided, it further tests to see if this driver supports 
  creating a handle for the specified child device.

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle     The handle of the controller to test. This handle 
                               must support a protocol interface that supplies 
                               an I/O abstraction to the driver.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                               This parameter is ignored by device drivers, and is optional for bus drivers.


  @retval EFI_SUCCESS          The device specified by ControllerHandle and
                               RemainingDevicePath is supported by the driver 
                               specified by This.
  @retval EFI_ALREADY_STARTED  The device specified by ControllerHandle and
                               RemainingDevicePath is already being managed by 
                               the driver specified by This.
  @retval EFI_ACCESS_DENIED    The device specified by ControllerHandle and
                               RemainingDevicePath is already being managed by a 
                               different driver or an application that requires 
                               exclusive access.
  @retval EFI_UNSUPPORTED      The device specified by ControllerHandle and
                               RemainingDevicePath is not supported by the driver 
                               specified by This.
                               
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
  
  The Start() function is designed to be invoked from the EFI boot service 
  ConnectController(). As a result, much of the error checking on the parameters 
  to Start() has been moved into this common boot service. It is legal to call 
  Start() from other locations, but the following calling restrictions must be 
  followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally 
     aligned EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified 
     by This must have been called with the same calling parameters, and Supported() 
     must have returned EFI_SUCCESS.

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle     The handle of the controller to start. This handle 
                               must support a protocol interface that supplies 
                               an I/O abstraction to the driver.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                               This parameter is ignored by device drivers, and is 
                               optional for bus drivers.

  @retval EFI_SUCCESS          The device was started.
  @retval EFI_ALREADY_STARTED  The device could not be started due to a device error.
  @retval EFI_OUT_OF_RESOURCES The request could not be completed due to a lack 
                               of resources.

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
      " resource to create a Tcp Servcie Data\n"));

    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create a new IP IO to Consume it
  //
  TcpServiceData->IpIo = IpIoCreate (
                           This->DriverBindingHandle,
                           ControllerHandle,
                           IP_VERSION_4
                           );
  if (NULL == TcpServiceData->IpIo) {

    DEBUG ((EFI_D_ERROR, "Tcp4DriverBindingStart: Have no enough"
      " resource to create an Ip Io\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Configure and start IpIo.
  //
  ZeroMem (&OpenData, sizeof (IP_IO_OPEN_DATA));

  CopyMem (
    &OpenData.IpConfigData.Ip4CfgData,
    &mIp4IoDefaultIpConfigData,
    sizeof (EFI_IP4_CONFIG_DATA)
    );

  OpenData.IpConfigData.Ip4CfgData.DefaultProtocol = EFI_IP_PROTO_TCP;

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

  return EFI_SUCCESS;

ON_ERROR:

  if (TcpServiceData->IpIo != NULL) {
    IpIoDestroy (TcpServiceData->IpIo);
    TcpServiceData->IpIo = NULL;
  }

  FreePool (TcpServiceData);

  return Status;
}


/**
  Stop this driver on ControllerHandle.
  
  The Stop() function is designed to be invoked from the EFI boot service 
  DisconnectController(). As a result, much of the error checking on the parameters 
  to Stop() has been moved into this common boot service. It is legal to call Stop() 
  from other locations, but the following calling restrictions must be followed 
  or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call 
     to this same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this 
     driver's Start() function, and the Start() function must have called OpenProtocol() 
     on ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
  
  @param  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle  A handle to the device being stopped. The handle must 
                            support a bus specific I/O protocol for the driver 
                            to use to stop the device.
  @param  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param  ChildHandleBuffer An array of child handles to be freed. May be NULL if 
                            NumberOfChildren is 0.

  @retval EFI_SUCCESS       The device was stopped.
  @retval EFI_DEVICE_ERROR  The device could not be stopped due to a device error.

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
  EFI_STATUS                                Status;
  EFI_HANDLE                                NicHandle;
  EFI_SERVICE_BINDING_PROTOCOL              *ServiceBinding;
  TCP4_SERVICE_DATA                         *TcpServiceData;
  LIST_ENTRY                                *List;
  TCP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT  Context;

  // Find the NicHandle where Tcp4 ServiceBinding Protocol is installed.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiIp4ProtocolGuid);
  if (NicHandle == NULL) {
    return EFI_SUCCESS;
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

  if (NumberOfChildren != 0) {
    List = &TcpServiceData->SocketList; 
    Context.ServiceBinding = ServiceBinding;
    Context.NumberOfChildren = NumberOfChildren;
    Context.ChildHandleBuffer = ChildHandleBuffer;
    Status = NetDestroyLinkList (
               List,
               Tcp4DestroyChildEntryInHandleBuffer,
               &Context,
               NULL
               );
  } else if (IsListEmpty (&TcpServiceData->SocketList)) {
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
    TcpServiceData->IpIo = NULL;

    //
    // Destroy the heartbeat timer.
    //
    Tcp4DestroyTimer ();

    if (gTcpControllerNameTable != NULL) {
      FreeUnicodeStringTable (gTcpControllerNameTable);
      gTcpControllerNameTable = NULL;
    }
    
    //
    // Release the TCP service data
    //
    FreePool (TcpServiceData);

    Status = EFI_SUCCESS;
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
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
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
  Creates a child handle and installs a protocol.
  
  The CreateChild() function installs a protocol on ChildHandle. If ChildHandle 
  is a pointer to NULL, then a new handle is created and returned in ChildHandle. 
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing 
  ChildHandle.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Pointer to the handle of the child to create. If it is NULL, then 
                      a new handle is created. If it is a pointer to an existing UEFI 
                      handle, then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child.
  @retval other                 The child handle was not created.

**/
EFI_STATUS
EFIAPI
Tcp4ServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
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

  mTcp4DefaultSockData.ProtoData = NULL;

  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Destroys a child handle with a protocol installed on it.
  
  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol 
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the 
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This         Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle  Handle of the child to destroy

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is 
                                being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed.
  
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

  if (NULL == This || NULL == ChildHandle) {
    return EFI_INVALID_PARAMETER;
  }

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

  return Status;
}

