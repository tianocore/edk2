/** @file
  Implementation of Mtftp drivers.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Mtftp4Impl.h"

EFI_DRIVER_BINDING_PROTOCOL  gMtftp4DriverBinding = {
  Mtftp4DriverBindingSupported,
  Mtftp4DriverBindingStart,
  Mtftp4DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL  gMtftp4ServiceBindingTemplete = {
  Mtftp4ServiceBindingCreateChild,
  Mtftp4ServiceBindingDestroyChild
};

/**
  The driver entry point which installs multiple protocols to the ImageHandle.

  @param ImageHandle    The MTFTP's image handle.
  @param SystemTable    The system table.

  @retval EFI_SUCCESS  The handles are successfully installed on the image.
  @retval others       some EFI_ERROR occurred.

**/
EFI_STATUS
EFIAPI
Mtftp4DriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gMtftp4DriverBinding,
           ImageHandle,
           &gMtftp4ComponentName,
           &gMtftp4ComponentName2
           );
}

/**
  Test whether MTFTP driver support this controller.

  @param  This                   The MTFTP driver binding instance
  @param  Controller             The controller to test
  @param  RemainingDevicePath    The remaining device path

  @retval EFI_SUCCESS            The controller has UDP service binding protocol
                                 installed, MTFTP can support it.
  @retval EFI_ALREADY_STARTED    The device specified by ControllerHandle and
                                 RemainingDevicePath is already being managed by
                                 the driver specified by This.
  @retval EFI_ACCESS_DENIED      The device specified by ControllerHandle and
                                 RemainingDevicePath is already being managed by a
                                 different driver or an application that requires
                                 exclusive access.
  @retval EFI_UNSUPPORTED        The device specified by ControllerHandle and
                                 RemainingDevicePath is not supported by the driver
                                 specified by This.

**/
EFI_STATUS
EFIAPI
Mtftp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS  Status;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUdp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  return Status;
}

/**
  Config a NULL UDP that is used to keep the connection between UDP and MTFTP.

  Just leave the Udp child unconfigured. When UDP is unloaded,
    MTFTP will be informed with DriverBinding Stop.

  @param  UdpIo                  The UDP_IO to configure
  @param  Context                The opaque parameter to the callback

  @retval EFI_SUCCESS            It always return EFI_SUCCESS directly.

**/
EFI_STATUS
EFIAPI
Mtftp4ConfigNullUdp (
  IN UDP_IO  *UdpIo,
  IN VOID    *Context
  )
{
  return EFI_SUCCESS;
}

/**
  Create then initialize a MTFTP service binding instance.

  @param  Controller             The controller to install the MTFTP service
                                 binding on
  @param  Image                  The driver binding image of the MTFTP driver
  @param  Service                The variable to receive the created service
                                 binding instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource to create the instance
  @retval EFI_DEVICE_ERROR       Failed to create a NULL UDP port to keep
                                 connection  with UDP.
  @retval EFI_SUCCESS            The service instance is created for the
                                 controller.

**/
EFI_STATUS
Mtftp4CreateService (
  IN     EFI_HANDLE   Controller,
  IN     EFI_HANDLE   Image,
  OUT MTFTP4_SERVICE  **Service
  )
{
  MTFTP4_SERVICE  *MtftpSb;
  EFI_STATUS      Status;

  *Service = NULL;
  MtftpSb  = AllocatePool (sizeof (MTFTP4_SERVICE));

  if (MtftpSb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MtftpSb->Signature      = MTFTP4_SERVICE_SIGNATURE;
  MtftpSb->ServiceBinding = gMtftp4ServiceBindingTemplete;
  MtftpSb->ChildrenNum    = 0;
  InitializeListHead (&MtftpSb->Children);

  MtftpSb->Timer            = NULL;
  MtftpSb->TimerNotifyLevel = NULL;
  MtftpSb->TimerToGetMap    = NULL;
  MtftpSb->Controller       = Controller;
  MtftpSb->Image            = Image;
  MtftpSb->ConnectUdp       = NULL;

  //
  // Create the timer and a udp to be notified when UDP is uninstalled
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  Mtftp4OnTimerTick,
                  MtftpSb,
                  &MtftpSb->Timer
                  );
  if (EFI_ERROR (Status)) {
    FreePool (MtftpSb);
    return Status;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_NOTIFY,
                  Mtftp4OnTimerTickNotifyLevel,
                  MtftpSb,
                  &MtftpSb->TimerNotifyLevel
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (MtftpSb->Timer);
    FreePool (MtftpSb);
    return Status;
  }

  //
  // Create the timer used to time out the procedure which is used to
  // get the default IP address.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &MtftpSb->TimerToGetMap
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (MtftpSb->TimerNotifyLevel);
    gBS->CloseEvent (MtftpSb->Timer);
    FreePool (MtftpSb);
    return Status;
  }

  MtftpSb->ConnectUdp = UdpIoCreateIo (
                          Controller,
                          Image,
                          Mtftp4ConfigNullUdp,
                          UDP_IO_UDP4_VERSION,
                          NULL
                          );

  if (MtftpSb->ConnectUdp == NULL) {
    gBS->CloseEvent (MtftpSb->TimerToGetMap);
    gBS->CloseEvent (MtftpSb->TimerNotifyLevel);
    gBS->CloseEvent (MtftpSb->Timer);
    FreePool (MtftpSb);
    return EFI_DEVICE_ERROR;
  }

  *Service = MtftpSb;
  return EFI_SUCCESS;
}

/**
  Release all the resource used the MTFTP service binding instance.

  @param  MtftpSb                The MTFTP service binding instance.

**/
VOID
Mtftp4CleanService (
  IN MTFTP4_SERVICE  *MtftpSb
  )
{
  UdpIoFreeIo (MtftpSb->ConnectUdp);
  gBS->CloseEvent (MtftpSb->TimerToGetMap);
  gBS->CloseEvent (MtftpSb->TimerNotifyLevel);
  gBS->CloseEvent (MtftpSb->Timer);
}

/**
  Start the MTFTP driver on this controller.

  MTFTP driver will install a MTFTP SERVICE BINDING protocol on the supported
  controller, which can be used to create/destroy MTFTP children.

  @param  This                   The MTFTP driver binding protocol.
  @param  Controller             The controller to manage.
  @param  RemainingDevicePath    Remaining device path.

  @retval EFI_ALREADY_STARTED    The MTFTP service binding protocol has been
                                 started  on the controller.
  @retval EFI_SUCCESS            The MTFTP service binding is installed on the
                                 controller.

**/
EFI_STATUS
EFIAPI
Mtftp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  MTFTP4_SERVICE  *MtftpSb;
  EFI_STATUS      Status;

  //
  // Directly return if driver is already running.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiMtftp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (Status == EFI_SUCCESS) {
    return EFI_ALREADY_STARTED;
  }

  Status = Mtftp4CreateService (Controller, This->DriverBindingHandle, &MtftpSb);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (MtftpSb != NULL);

  Status = gBS->SetTimer (MtftpSb->Timer, TimerPeriodic, TICKS_PER_SECOND);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->SetTimer (MtftpSb->TimerNotifyLevel, TimerPeriodic, TICKS_PER_SECOND);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Install the Mtftp4ServiceBinding Protocol onto Controller
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiMtftp4ServiceBindingProtocolGuid,
                  &MtftpSb->ServiceBinding,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  Mtftp4CleanService (MtftpSb);
  FreePool (MtftpSb);

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
Mtftp4DestroyChildEntryInHandleBuffer (
  IN LIST_ENTRY  *Entry,
  IN VOID        *Context
  )
{
  MTFTP4_PROTOCOL               *Instance;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;

  if ((Entry == NULL) || (Context == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance          = NET_LIST_USER_STRUCT_S (Entry, MTFTP4_PROTOCOL, Link, MTFTP4_PROTOCOL_SIGNATURE);
  ServiceBinding    = ((MTFTP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ServiceBinding;
  NumberOfChildren  = ((MTFTP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->NumberOfChildren;
  ChildHandleBuffer = ((MTFTP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ChildHandleBuffer;

  if (!NetIsInHandleBuffer (Instance->Handle, NumberOfChildren, ChildHandleBuffer)) {
    return EFI_SUCCESS;
  }

  return ServiceBinding->DestroyChild (ServiceBinding, Instance->Handle);
}

/**
  Stop the MTFTP driver on controller. The controller is a UDP
  child handle.

  @param  This                   The MTFTP driver binding protocol
  @param  Controller             The controller to stop
  @param  NumberOfChildren       The number of children
  @param  ChildHandleBuffer      The array of the child handle.

  @retval EFI_SUCCESS            The driver is stopped on the controller.
  @retval EFI_DEVICE_ERROR       Failed to stop the driver on the controller.

**/
EFI_STATUS
EFIAPI
Mtftp4DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_SERVICE_BINDING_PROTOCOL                *ServiceBinding;
  MTFTP4_SERVICE                              *MtftpSb;
  EFI_HANDLE                                  NicHandle;
  EFI_STATUS                                  Status;
  LIST_ENTRY                                  *List;
  MTFTP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT  Context;

  //
  // MTFTP driver opens UDP child, So, Controller is a UDP
  // child handle. Locate the Nic handle first. Then get the
  // MTFTP private data back.
  //
  NicHandle = NetLibGetNicHandle (Controller, &gEfiUdp4ProtocolGuid);

  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiMtftp4ServiceBindingProtocolGuid,
                  (VOID **)&ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  MtftpSb = MTFTP4_SERVICE_FROM_THIS (ServiceBinding);

  if (!IsListEmpty (&MtftpSb->Children)) {
    //
    // Destroy the Mtftp4 child instance in ChildHandleBuffer.
    //
    List                      = &MtftpSb->Children;
    Context.ServiceBinding    = ServiceBinding;
    Context.NumberOfChildren  = NumberOfChildren;
    Context.ChildHandleBuffer = ChildHandleBuffer;
    Status                    = NetDestroyLinkList (
                                  List,
                                  Mtftp4DestroyChildEntryInHandleBuffer,
                                  &Context,
                                  NULL
                                  );
  }

  if ((NumberOfChildren == 0) && IsListEmpty (&MtftpSb->Children)) {
    gBS->UninstallProtocolInterface (
           NicHandle,
           &gEfiMtftp4ServiceBindingProtocolGuid,
           ServiceBinding
           );

    Mtftp4CleanService (MtftpSb);
    if (gMtftp4ControllerNameTable != NULL) {
      FreeUnicodeStringTable (gMtftp4ControllerNameTable);
      gMtftp4ControllerNameTable = NULL;
    }

    FreePool (MtftpSb);

    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Initialize a MTFTP protocol instance which is the child of MtftpSb.

  @param  MtftpSb                The MTFTP service binding protocol.
  @param  Instance               The MTFTP instance to initialize.

**/
VOID
Mtftp4InitProtocol (
  IN     MTFTP4_SERVICE  *MtftpSb,
  OUT MTFTP4_PROTOCOL    *Instance
  )
{
  ZeroMem (Instance, sizeof (MTFTP4_PROTOCOL));

  Instance->Signature = MTFTP4_PROTOCOL_SIGNATURE;
  InitializeListHead (&Instance->Link);
  CopyMem (&Instance->Mtftp4, &gMtftp4ProtocolTemplate, sizeof (Instance->Mtftp4));
  Instance->State   = MTFTP4_STATE_UNCONFIGED;
  Instance->Service = MtftpSb;

  InitializeListHead (&Instance->Blocks);
}

/**
  Create a MTFTP child for the service binding instance, then
  install the MTFTP protocol to the ChildHandle.

  @param  This                   The MTFTP service binding instance.
  @param  ChildHandle            The Child handle to install the MTFTP protocol.

  @retval EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource for the new child.
  @retval EFI_SUCCESS            The child is successfully create.

**/
EFI_STATUS
EFIAPI
Mtftp4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  MTFTP4_SERVICE   *MtftpSb;
  MTFTP4_PROTOCOL  *Instance;
  EFI_STATUS       Status;
  EFI_TPL          OldTpl;
  VOID             *Udp4;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = AllocatePool (sizeof (*Instance));

  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MtftpSb = MTFTP4_SERVICE_FROM_THIS (This);

  Mtftp4InitProtocol (MtftpSb, Instance);

  Instance->UnicastPort = UdpIoCreateIo (
                            MtftpSb->Controller,
                            MtftpSb->Image,
                            Mtftp4ConfigNullUdp,
                            UDP_IO_UDP4_VERSION,
                            Instance
                            );

  if (Instance->UnicastPort == NULL) {
    FreePool (Instance);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Install the MTFTP protocol onto ChildHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiMtftp4ProtocolGuid,
                  &Instance->Mtftp4,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    UdpIoFreeIo (Instance->UnicastPort);
    FreePool (Instance);
    return Status;
  }

  Instance->Handle = *ChildHandle;

  //
  // Open the Udp4 protocol BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  MtftpSb->ConnectUdp->UdpHandle,
                  &gEfiUdp4ProtocolGuid,
                  (VOID **)&Udp4,
                  gMtftp4DriverBinding.DriverBindingHandle,
                  Instance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Open the Udp4 protocol by child.
  //
  Status = gBS->OpenProtocol (
                  Instance->UnicastPort->UdpHandle,
                  &gEfiUdp4ProtocolGuid,
                  (VOID **)&Udp4,
                  gMtftp4DriverBinding.DriverBindingHandle,
                  Instance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    //
    // Close the Udp4 protocol.
    //
    gBS->CloseProtocol (
           MtftpSb->ConnectUdp->UdpHandle,
           &gEfiUdp4ProtocolGuid,
           gMtftp4DriverBinding.DriverBindingHandle,
           *ChildHandle
           );
    goto ON_ERROR;
  }

  //
  // Add it to the parent's child list.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&MtftpSb->Children, &Instance->Link);
  MtftpSb->ChildrenNum++;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

ON_ERROR:
  if (Instance->Handle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           Instance->Handle,
           &gEfiMtftp4ProtocolGuid,
           &Instance->Mtftp4,
           NULL
           );
  }

  UdpIoFreeIo (Instance->UnicastPort);
  FreePool (Instance);

  return Status;
}

/**
  Destroy one of the service binding's child.

  @param  This                   The service binding instance
  @param  ChildHandle            The child handle to destroy

  @retval EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval EFI_UNSUPPORTED        The child may have already been destroyed.
  @retval EFI_SUCCESS            The child is destroyed and removed from the
                                 parent's child list.

**/
EFI_STATUS
EFIAPI
Mtftp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  MTFTP4_SERVICE       *MtftpSb;
  MTFTP4_PROTOCOL      *Instance;
  EFI_MTFTP4_PROTOCOL  *Mtftp4;
  EFI_STATUS           Status;
  EFI_TPL              OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the private context data structures
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiMtftp4ProtocolGuid,
                  (VOID **)&Mtftp4,
                  gMtftp4DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance = MTFTP4_PROTOCOL_FROM_THIS (Mtftp4);
  MtftpSb  = MTFTP4_SERVICE_FROM_THIS (This);

  if (Instance->Service != MtftpSb) {
    return EFI_INVALID_PARAMETER;
  }

  if (Instance->InDestroy) {
    return EFI_SUCCESS;
  }

  Instance->InDestroy = TRUE;

  //
  // Close the Udp4 protocol.
  //
  gBS->CloseProtocol (
         MtftpSb->ConnectUdp->UdpHandle,
         &gEfiUdp4ProtocolGuid,
         gMtftp4DriverBinding.DriverBindingHandle,
         ChildHandle
         );

  gBS->CloseProtocol (
         Instance->UnicastPort->UdpHandle,
         &gEfiUdp4ProtocolGuid,
         gMtftp4DriverBinding.DriverBindingHandle,
         ChildHandle
         );

  if (Instance->McastUdpPort != NULL) {
    gBS->CloseProtocol (
           Instance->McastUdpPort->UdpHandle,
           &gEfiUdp4ProtocolGuid,
           gMtftp4DriverBinding.DriverBindingHandle,
           ChildHandle
           );
  }

  //
  // Uninstall the MTFTP4 protocol first to enable a top down destruction.
  //
  Status = gBS->UninstallProtocolInterface (
                  ChildHandle,
                  &gEfiMtftp4ProtocolGuid,
                  Mtftp4
                  );

  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    return Status;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Mtftp4CleanOperation (Instance, EFI_DEVICE_ERROR);
  UdpIoFreeIo (Instance->UnicastPort);

  RemoveEntryList (&Instance->Link);
  MtftpSb->ChildrenNum--;

  gBS->RestoreTPL (OldTpl);

  FreePool (Instance);
  return EFI_SUCCESS;
}
