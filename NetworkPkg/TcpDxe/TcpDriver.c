/** @file
  The driver binding and service binding protocol for the TCP driver.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "TcpMain.h"

UINT16                        mTcp4RandomPort;
UINT16                        mTcp6RandomPort;

TCP_HEARTBEAT_TIMER           mTcpTimer = {
  NULL,
  0
};

EFI_TCP4_PROTOCOL             gTcp4ProtocolTemplate = {
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

EFI_TCP6_PROTOCOL             gTcp6ProtocolTemplate = {
  Tcp6GetModeData,
  Tcp6Configure,
  Tcp6Connect,
  Tcp6Accept,
  Tcp6Transmit,
  Tcp6Receive,
  Tcp6Close,
  Tcp6Cancel,
  Tcp6Poll
};

SOCK_INIT_DATA                mTcpDefaultSockData = {
  SockStream,
  SO_CLOSED,
  NULL,
  TCP_BACKLOG,
  TCP_SND_BUF_SIZE,
  TCP_RCV_BUF_SIZE,
  IP_VERSION_4,
  NULL,
  TcpCreateSocketCallback,
  TcpDestroySocketCallback,
  NULL,
  NULL,
  0,
  TcpDispatcher,
  NULL,
};

EFI_DRIVER_BINDING_PROTOCOL   gTcp4DriverBinding = {
  Tcp4DriverBindingSupported,
  Tcp4DriverBindingStart,
  Tcp4DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_DRIVER_BINDING_PROTOCOL   gTcp6DriverBinding = {
  Tcp6DriverBindingSupported,
  Tcp6DriverBindingStart,
  Tcp6DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL  gTcpServiceBinding = {
  TcpServiceBindingCreateChild,
  TcpServiceBindingDestroyChild
};


/**
  Create and start the heartbeat timer for the TCP driver.

  @retval EFI_SUCCESS            The timer was successfully created and started.
  @retval other                  The timer was not created.

**/
EFI_STATUS
TcpCreateTimer (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (mTcpTimer.RefCnt == 0) {

    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    TcpTicking,
                    NULL,
                    &mTcpTimer.TimerEvent
                    );
    if (!EFI_ERROR (Status)) {

      Status = gBS->SetTimer (
                      mTcpTimer.TimerEvent,
                      TimerPeriodic,
                      (UINT64) (TICKS_PER_SECOND / TCP_TICK_HZ)
                      );
    }
  }

  if (!EFI_ERROR (Status)) {

    mTcpTimer.RefCnt++;
  }

  return Status;
}

/**
  Stop and destroy the heartbeat timer for TCP driver.

**/
VOID
TcpDestroyTimer (
  VOID
  )
{
  ASSERT (mTcpTimer.RefCnt > 0);

  mTcpTimer.RefCnt--;

  if (mTcpTimer.RefCnt > 0) {
    return;
  }

  gBS->SetTimer (mTcpTimer.TimerEvent, TimerCancel, 0);
  gBS->CloseEvent (mTcpTimer.TimerEvent);
  mTcpTimer.TimerEvent = NULL;
}

/**
  The entry point for Tcp driver, which is used to install Tcp driver on the ImageHandle.

  @param[in]  ImageHandle   The firmware allocated handle for this driver image.
  @param[in]  SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS   The driver loaded.
  @retval other         The driver did not load.

**/
EFI_STATUS
EFIAPI
TcpDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT32      Seed;

  //
  // Install the TCP Driver Binding Protocol
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gTcp4DriverBinding,
             ImageHandle,
             &gTcpComponentName,
             &gTcpComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install the TCP Driver Binding Protocol
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gTcp6DriverBinding,
             NULL,
             &gTcpComponentName,
             &gTcpComponentName2
             );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           ImageHandle,
           &gEfiDriverBindingProtocolGuid,
           &gTcp4DriverBinding,
           &gEfiComponentName2ProtocolGuid,
           &gTcpComponentName2,
           &gEfiComponentNameProtocolGuid,
           &gTcpComponentName,
           NULL
           );
    return Status;
  }

  //
  // Initialize ISS and random port.
  //
  Seed            = NetRandomInitSeed ();
  mTcpGlobalIss   = NET_RANDOM (Seed) % mTcpGlobalIss;
  mTcp4RandomPort = (UINT16) (TCP_PORT_KNOWN + (NET_RANDOM (Seed) % TCP_PORT_KNOWN));
  mTcp6RandomPort = mTcp4RandomPort;

  return EFI_SUCCESS;
}

/**
  Create a new TCP4 or TCP6 driver service binding protocol

  @param[in]  Controller         Controller handle of device to bind driver to.
  @param[in]  Image              The TCP driver's image handle.
  @param[in]  IpVersion          IP_VERSION_4 or IP_VERSION_6.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate some resources.
  @retval EFI_SUCCESS            A new IP6 service binding private was created.

**/
EFI_STATUS
TcpCreateService (
  IN EFI_HANDLE  Controller,
  IN EFI_HANDLE  Image,
  IN UINT8       IpVersion
  )
{
  EFI_STATUS         Status;
  EFI_GUID           *IpServiceBindingGuid;
  EFI_GUID           *TcpServiceBindingGuid;
  TCP_SERVICE_DATA   *TcpServiceData;
  IP_IO_OPEN_DATA    OpenData;

  if (IpVersion == IP_VERSION_4) {
    IpServiceBindingGuid  = &gEfiIp4ServiceBindingProtocolGuid;
    TcpServiceBindingGuid = &gEfiTcp4ServiceBindingProtocolGuid;
  } else {
    IpServiceBindingGuid  = &gEfiIp6ServiceBindingProtocolGuid;
    TcpServiceBindingGuid = &gEfiTcp6ServiceBindingProtocolGuid;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  TcpServiceBindingGuid,
                  NULL,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  IpServiceBindingGuid,
                  NULL,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Create the TCP service data.
  //
  TcpServiceData = AllocateZeroPool (sizeof (TCP_SERVICE_DATA));
  if (TcpServiceData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TcpServiceData->Signature            = TCP_DRIVER_SIGNATURE;
  TcpServiceData->ControllerHandle     = Controller;
  TcpServiceData->DriverBindingHandle  = Image;
  TcpServiceData->IpVersion            = IpVersion;
  CopyMem (
    &TcpServiceData->ServiceBinding,
    &gTcpServiceBinding,
    sizeof (EFI_SERVICE_BINDING_PROTOCOL)
    );

  TcpServiceData->IpIo = IpIoCreate (Image, Controller, IpVersion);
  if (TcpServiceData->IpIo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }


  InitializeListHead (&TcpServiceData->SocketList);
  ZeroMem (&OpenData, sizeof (IP_IO_OPEN_DATA));

  if (IpVersion == IP_VERSION_4) {
    CopyMem (
      &OpenData.IpConfigData.Ip4CfgData,
      &mIp4IoDefaultIpConfigData,
      sizeof (EFI_IP4_CONFIG_DATA)
      );
    OpenData.IpConfigData.Ip4CfgData.DefaultProtocol = EFI_IP_PROTO_TCP;
  } else {
    CopyMem (
      &OpenData.IpConfigData.Ip6CfgData,
      &mIp6IoDefaultIpConfigData,
      sizeof (EFI_IP6_CONFIG_DATA)
      );
    OpenData.IpConfigData.Ip6CfgData.DefaultProtocol = EFI_IP_PROTO_TCP;
  }

  OpenData.PktRcvdNotify  = TcpRxCallback;
  Status                  = IpIoOpen (TcpServiceData->IpIo, &OpenData);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = TcpCreateTimer ();
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  TcpServiceBindingGuid,
                  &TcpServiceData->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    TcpDestroyTimer ();

    goto ON_ERROR;
  }

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
  Callback function which provided by user to remove one node in NetDestroyLinkList process.
  
  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
TcpDestroyChildEntryInHandleBuffer (
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
  ServiceBinding    = ((TCP_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->ServiceBinding;
  NumberOfChildren  = ((TCP_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->NumberOfChildren;
  ChildHandleBuffer = ((TCP_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->ChildHandleBuffer;

  if (!NetIsInHandleBuffer (Sock->SockHandle, NumberOfChildren, ChildHandleBuffer)) {
    return EFI_SUCCESS;
  }

  return ServiceBinding->DestroyChild (ServiceBinding, Sock->SockHandle);
}

/**
  Destroy a TCP6 or TCP4 service binding instance. It will release all
  the resources allocated by the instance.

  @param[in]  Controller         Controller handle of device to bind driver to.
  @param[in]  ImageHandle        The TCP driver's image handle.
  @param[in]  NumberOfChildren   Number of Handles in ChildHandleBuffer. If number
                                 of children is zero stop the entire bus driver.
  @param[in]  ChildHandleBuffer  An array of child handles to be freed. May be NULL
                                 if NumberOfChildren is 0.  
  @param[in]  IpVersion          IP_VERSION_4 or IP_VERSION_6

  @retval EFI_SUCCESS            The resources used by the instance were cleaned up.
  @retval Others                 Failed to clean up some of the resources.

**/
EFI_STATUS
TcpDestroyService (
  IN EFI_HANDLE  Controller,
  IN EFI_HANDLE  ImageHandle,
  IN UINTN       NumberOfChildren,
  IN EFI_HANDLE  *ChildHandleBuffer, OPTIONAL
  IN UINT8       IpVersion
  )
{
  EFI_HANDLE                    NicHandle;
  EFI_GUID                      *IpProtocolGuid;
  EFI_GUID                      *ServiceBindingGuid;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  TCP_SERVICE_DATA              *TcpServiceData;
  EFI_STATUS                    Status;
  LIST_ENTRY                    *List;
  TCP_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT  Context;

  ASSERT ((IpVersion == IP_VERSION_4) || (IpVersion == IP_VERSION_6));

  if (IpVersion == IP_VERSION_4) {
    IpProtocolGuid     = &gEfiIp4ProtocolGuid;
    ServiceBindingGuid = &gEfiTcp4ServiceBindingProtocolGuid;
  } else {
    IpProtocolGuid     = &gEfiIp6ProtocolGuid;
    ServiceBindingGuid = &gEfiTcp6ServiceBindingProtocolGuid;
  }

  NicHandle = NetLibGetNicHandle (Controller, IpProtocolGuid);
  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  ServiceBindingGuid,
                  (VOID **) &ServiceBinding,
                  ImageHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  TcpServiceData = TCP_SERVICE_FROM_THIS (ServiceBinding);

  if (NumberOfChildren != 0) {
    List = &TcpServiceData->SocketList;
    Context.ServiceBinding = ServiceBinding;
    Context.NumberOfChildren = NumberOfChildren;
    Context.ChildHandleBuffer = ChildHandleBuffer;
    Status = NetDestroyLinkList (
               List,
               TcpDestroyChildEntryInHandleBuffer,
               &Context,
               NULL
               );
  } else if (IsListEmpty (&TcpServiceData->SocketList)) {
    //
    // Uninstall TCP servicebinding protocol
    //
    gBS->UninstallMultipleProtocolInterfaces (
           NicHandle,
           ServiceBindingGuid,
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
    TcpDestroyTimer ();

    //
    // Release the TCP service data
    //
    FreePool (TcpServiceData);

    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Test to see if this driver supports ControllerHandle.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of device to test.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific
                                  child device to start.

  @retval EFI_SUCCESS             This driver supports this device.
  @retval EFI_ALREADY_STARTED     This driver is already running on this device.
  @retval other                   This driver does not support this device.

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
  // Test for the Ip4ServiceBinding Protocol
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

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to bind driver to.
  @param[in]  RemainingDevicePath    Optional parameter use to pick a specific child
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
  EFI_STATUS  Status;

  Status = TcpCreateService (ControllerHandle, This->DriverBindingHandle, IP_VERSION_4);
  if ((Status == EFI_ALREADY_STARTED) || (Status == EFI_UNSUPPORTED)) {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Stop this driver on ControllerHandle.

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
Tcp4DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  return TcpDestroyService (
           ControllerHandle,
           This->DriverBindingHandle,
           NumberOfChildren,
           ChildHandleBuffer,
           IP_VERSION_4
           );
}

/**
  Test to see if this driver supports ControllerHandle.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of device to test.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific
                                  child device to start.

  @retval EFI_SUCCESS             This driver supports this device.
  @retval EFI_ALREADY_STARTED     This driver is already running on this device.
  @retval other                   This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Tcp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // Test for the Tcp6ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiTcp6ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }
  
  //
  // Test for the Ip6ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiIp6ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  return Status;
}

/**
  Start this driver on ControllerHandle.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to bind driver to.
  @param[in]  RemainingDevicePath    Optional parameter use to pick a specific child
                                     device to start.

  @retval EFI_SUCCESS            The driver is added to ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources to start the
                                 driver.
  @retval other                  The driver cannot be added to ControllerHandle.

**/
EFI_STATUS
EFIAPI
Tcp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = TcpCreateService (ControllerHandle, This->DriverBindingHandle, IP_VERSION_6);
  if ((Status == EFI_ALREADY_STARTED) || (Status == EFI_UNSUPPORTED)) {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Stop this driver on ControllerHandle.

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
Tcp6DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  return TcpDestroyService (
           ControllerHandle,
           This->DriverBindingHandle,
           NumberOfChildren,
           ChildHandleBuffer,
           IP_VERSION_6
           );
}

/**
  The Callback funtion called after the TCP socket was created.

  @param[in]  This            Pointer to the socket just created
  @param[in]  Context         Context of the socket

  @retval EFI_SUCCESS         This protocol installed successfully.
  @retval other               An error occured.

**/
EFI_STATUS
TcpCreateSocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  )
{
  EFI_STATUS        Status;
  TCP_SERVICE_DATA  *TcpServiceData;
  EFI_GUID          *IpProtocolGuid;
  VOID              *Ip;

  if (This->IpVersion == IP_VERSION_4) {
    IpProtocolGuid = &gEfiIp4ProtocolGuid;
  } else {
    IpProtocolGuid = &gEfiIp6ProtocolGuid;
  }

  TcpServiceData = ((TCP_PROTO_DATA *) This->ProtoReserved)->TcpService;

  //
  // Open the default IP protocol of IP_IO BY_DRIVER.
  //
  Status = gBS->OpenProtocol (
                  TcpServiceData->IpIo->ChildHandle,
                  IpProtocolGuid,
                  &Ip,
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
           IpProtocolGuid,
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
  The callback function called before the TCP socket was to be destroyed.

  @param[in]  This                   The TCP socket to be destroyed.
  @param[in]  Context                The context of the socket.

**/
VOID
TcpDestroySocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  )
{
  TCP_SERVICE_DATA  *TcpServiceData;
  EFI_GUID          *IpProtocolGuid;

  if (This->IpVersion == IP_VERSION_4) {
    IpProtocolGuid = &gEfiIp4ProtocolGuid;
  } else {
    IpProtocolGuid = &gEfiIp6ProtocolGuid;
  }

  TcpServiceData = ((TCP_PROTO_DATA *) This->ProtoReserved)->TcpService;

  //
  // Remove this node from the list.
  //
  RemoveEntryList (&This->Link);

  //
  // Close the IP protocol.
  //
  gBS->CloseProtocol (
         TcpServiceData->IpIo->ChildHandle,
         IpProtocolGuid,
         TcpServiceData->DriverBindingHandle,
         This->SockHandle
         );
}

/**
  Creates a child handle with a set of TCP services.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in]      This          Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in, out] ChildHandle   Pointer to the handle of the child to create.
                                If it is NULL, then a new handle is created.
                                If it is a pointer to an existing UEFI handle,
                                then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources availabe to create
                                the child.
  @retval other                 The child handle was not created.

**/
EFI_STATUS
EFIAPI
TcpServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  )
{
  SOCKET            *Sock;
  TCP_SERVICE_DATA  *TcpServiceData;
  TCP_PROTO_DATA    TcpProto;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  if (NULL == This || NULL == ChildHandle) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status              = EFI_SUCCESS;
  TcpServiceData      = TCP_SERVICE_FROM_THIS (This);
  TcpProto.TcpService = TcpServiceData;
  TcpProto.TcpPcb     = NULL;

  //
  // Create a tcp instance with defualt Tcp default
  // sock init data and TcpProto
  //
  mTcpDefaultSockData.ProtoData     = &TcpProto;
  mTcpDefaultSockData.DataSize      = sizeof (TCP_PROTO_DATA);
  mTcpDefaultSockData.DriverBinding = TcpServiceData->DriverBindingHandle;
  mTcpDefaultSockData.IpVersion     = TcpServiceData->IpVersion;

  if (TcpServiceData->IpVersion == IP_VERSION_4) {
    mTcpDefaultSockData.Protocol = &gTcp4ProtocolTemplate;
  } else {
    mTcpDefaultSockData.Protocol = &gTcp6ProtocolTemplate;
  }

  Sock = SockCreateChild (&mTcpDefaultSockData);
  if (NULL == Sock) {
    DEBUG (
      (EFI_D_ERROR,
      "TcpDriverBindingCreateChild: No resource to create a Tcp Child\n")
      );

    Status = EFI_OUT_OF_RESOURCES;
  } else {
    *ChildHandle = Sock->SockHandle;
  }

  mTcpDefaultSockData.ProtoData  = NULL;

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Destroys a child handle with a set of TCP services.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Handle of the child to be destroyed.

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
TcpServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS  Status;
  VOID        *Tcp;
  SOCKET      *Sock;

  if (NULL == This || NULL == ChildHandle) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // retrieve the Tcp4 protocol from ChildHandle
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiTcp4ProtocolGuid,
                  &Tcp,
                  gTcp4DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    //
    // No Tcp4, try the Tcp6 protocol
    //
    Status = gBS->OpenProtocol (
                    ChildHandle,
                    &gEfiTcp6ProtocolGuid,
                    &Tcp,
                    gTcp6DriverBinding.DriverBindingHandle,
                    ChildHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      Status = EFI_UNSUPPORTED;
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // destroy this sock and related Tcp protocol control
    // block
    //
    Sock = SOCK_FROM_THIS (Tcp);

    SockDestroyChild (Sock);
  }

  return Status;
}
