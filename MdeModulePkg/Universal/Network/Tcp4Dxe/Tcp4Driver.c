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


UINT16                               mTcp4RandomPort;
extern EFI_COMPONENT_NAME_PROTOCOL   gTcp4ComponentName;

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

  None.

  @retval EFI_SUCCESS            The timer is successfully created and started.
  @retval other                  The timer is not created.

**/
STATIC
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
                    NET_TPL_TIMER,
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

  None.

  @return None.

**/
STATIC
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

//@MT: EFI_DRIVER_ENTRY_POINT (Tcp4DriverEntryPoint)

EFI_STATUS
EFIAPI
Tcp4DriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  The entry point for Tcp4 driver. used to install
  Tcp4 driver on the ImageHandle.

Arguments:

  ImageHandle - The firmware allocated handle for this
                driver image.
  SystemTable - Pointer to the EFI system table.

Returns:

  EFI_SUCCESS - Driver loaded.
  other       - Driver not loaded.

--*/
{
  EFI_STATUS  Status;
  UINT32      Seed;

  //
  // Install the TCP4 Driver Binding Protocol
  //
  Status = NetLibInstallAllDriverProtocols (
             ImageHandle,
             SystemTable,
             &mTcp4DriverBinding,
             ImageHandle,
             &gTcp4ComponentName,
             NULL,
             NULL
             );

  //
  // Initialize ISS and random port.
  //
  Seed            = NetRandomInitSeed ();
  mTcpGlobalIss   = NET_RANDOM (Seed) % mTcpGlobalIss;
  mTcp4RandomPort = (UINT16) ( TCP4_PORT_KNOWN +
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
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
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
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS               Status;
  TCP4_SERVICE_DATA        *TcpServiceData;
  IP_IO_OPEN_DATA          OpenData;

  TcpServiceData = NetAllocateZeroPool (sizeof (TCP4_SERVICE_DATA));

  if (NULL == TcpServiceData) {
    TCP4_DEBUG_ERROR (("Tcp4DriverBindingStart: Have no enough"
      " resource to create a Tcp Servcie Data!\n"));

    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create a new IP IO to Consume it
  //
  TcpServiceData->IpIo = IpIoCreate (This->DriverBindingHandle, ControllerHandle);
  if (NULL == TcpServiceData->IpIo) {

    TCP4_DEBUG_ERROR (("Tcp4DriverBindingStart: Have no enough"
      " resource to create an Ip Io!\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ReleaseServiceData;
  }

  //
  // Configure and start IpIo.
  //
  NetZeroMem (&OpenData, sizeof (IP_IO_OPEN_DATA));

  CopyMem (&OpenData.IpConfigData, &mIpIoDefaultIpConfigData, sizeof (EFI_IP4_CONFIG_DATA));
  OpenData.IpConfigData.DefaultProtocol = EFI_IP_PROTO_TCP;

  OpenData.PktRcvdNotify = Tcp4RxCallback;
  Status                 = IpIoOpen (TcpServiceData->IpIo, &OpenData);

  if (EFI_ERROR (Status)) {
      goto ReleaseServiceData;
  }

  //
  // Create the timer event used by TCP driver
  //
  Status = Tcp4CreateTimer ();
  if (EFI_ERROR (Status)) {

    TCP4_DEBUG_ERROR (("Tcp4DriverBindingStart: Create TcpTimer"
      " Event failed with %r\n", Status));

    goto ReleaseIpIo;
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

    TCP4_DEBUG_ERROR (("Tcp4DriverBindingStart: Install Tcp4 Service Binding"
      " Protocol failed for %r\n", Status));

    goto ReleaseTimer;
  }

  //
  // Initialize member in TcpServiceData
  //
  TcpServiceData->ControllerHandle    = ControllerHandle;
  TcpServiceData->Signature           = TCP4_DRIVER_SIGNATURE;
  TcpServiceData->DriverBindingHandle = This->DriverBindingHandle;

  TcpSetVariableData (TcpServiceData);

  return EFI_SUCCESS;

ReleaseTimer:

  Tcp4DestroyTimer ();

ReleaseIpIo:

  IpIoDestroy (TcpServiceData->IpIo);

ReleaseServiceData:

  NetFreePool (TcpServiceData);

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
  EFI_SERVICE_BINDING_PROTOCOL        *Tcp4ServiceBinding;
  TCP4_SERVICE_DATA                   *TcpServiceData;
  TCP_CB                              *TcpPcb;
  SOCKET                              *Sock;
  TCP4_PROTO_DATA                     *TcpProto;
  NET_LIST_ENTRY                      *Entry;
  NET_LIST_ENTRY                      *NextEntry;

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
                  (VOID **) &Tcp4ServiceBinding,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {

    TCP4_DEBUG_ERROR (("Tcp4DriverBindingStop: Locate Tcp4 Service "
      " Binding Protocol failed with %r\n", Status));

    return Status;
  }

  TcpServiceData = TCP4_FROM_THIS (Tcp4ServiceBinding);

  //
  // Kill TCP driver
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &mTcpRunQue) {
    TcpPcb = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    //
    // Try to destroy this child
    //
    Sock     = TcpPcb->Sk;
    TcpProto = (TCP4_PROTO_DATA *) Sock->ProtoReserved;

    if (TcpProto->TcpService == TcpServiceData) {
      Status = SockDestroyChild (Sock);

      if (EFI_ERROR (Status)) {

        TCP4_DEBUG_ERROR (("Tcp4DriverBindingStop: Destroy Tcp "
          "instance failed with %r\n", Status));
        return Status;
      }
    }
  }

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &mTcpListenQue) {
    TcpPcb = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    //
    // Try to destroy this child
    //
    Sock     = TcpPcb->Sk;
    TcpProto = (TCP4_PROTO_DATA *) Sock->ProtoReserved;

    if (TcpProto->TcpService == TcpServiceData) {
      Status = SockDestroyChild (TcpPcb->Sk);
      if (EFI_ERROR (Status)) {

        TCP4_DEBUG_ERROR (("Tcp4DriverBindingStop: Destroy Tcp "
          "instance failed with %r\n", Status));
        return Status;
      }
    }
  }

  //
  // Uninstall TCP servicebinding protocol
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  NicHandle,
                  &gEfiTcp4ServiceBindingProtocolGuid,
                  Tcp4ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {

    TCP4_DEBUG_ERROR (("Tcp4DriverBindingStop: Uninstall TCP service "
      "binding protocol failed with %r\n", Status));
    return Status;
  }

  //
  // Destroy the IpIO consumed by TCP driver
  //
  Status = IpIoDestroy (TcpServiceData->IpIo);

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
  NetFreePool (TcpServiceData);

  return Status;
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
  VOID              *Ip4;
  EFI_TPL           OldTpl;

  if (NULL == This || NULL == ChildHandle) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl              = NET_RAISE_TPL (NET_TPL_LOCK);
  TcpServiceData      = TCP4_FROM_THIS (This);
  TcpProto.TcpService = TcpServiceData;
  TcpProto.TcpPcb     = NULL;

  //
  // Create a tcp instance with defualt Tcp default
  // sock init data and TcpProto
  //
  mTcp4DefaultSockData.DriverBinding = TcpServiceData->DriverBindingHandle;

  Sock = SockCreateChild (&mTcp4DefaultSockData, &TcpProto, sizeof (TCP4_PROTO_DATA));
  if (NULL == Sock) {
    TCP4_DEBUG_ERROR (("Tcp4DriverBindingCreateChild: "
      "No resource to create a Tcp Child\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  *ChildHandle = Sock->SockHandle;

  //
  // Open the default Ip4 protocol of IP_IO BY_DRIVER.
  //
  Status = gBS->OpenProtocol (
                  TcpServiceData->IpIo->ChildHandle,
                  &gEfiIp4ProtocolGuid,
                  (VOID **) &Ip4,
                  TcpServiceData->DriverBindingHandle,
                  Sock->SockHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    SockDestroyChild (Sock);
  }

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
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
  TCP4_PROTO_DATA    *TcpProtoData;
  TCP4_SERVICE_DATA  *TcpServiceData;
  EFI_TPL            OldTpl;

  if (NULL == This || NULL == ChildHandle) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = NET_RAISE_TPL (NET_TPL_LOCK);

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
    goto ON_EXIT;
  }

  //
  // destroy this sock and related Tcp protocol control
  // block
  //
  Sock           = SOCK_FROM_THIS (Tcp4);
  TcpProtoData   = (TCP4_PROTO_DATA *) Sock->ProtoReserved;
  TcpServiceData = TcpProtoData->TcpService;

  Status = SockDestroyChild (Sock);

  //
  // Close the Ip4 protocol.
  //
  gBS->CloseProtocol (
         TcpServiceData->IpIo->ChildHandle,
         &gEfiIp4ProtocolGuid,
         TcpServiceData->DriverBindingHandle,
         ChildHandle
         );

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}
