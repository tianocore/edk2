/** @file
  The wrap of TCP/IP Socket interface.

Copyright (c) 2004 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

/**
  The common notify function associated with various Tcp4Io events. 

  @param[in]  Event   The event signaled.
  @param[in]  Context The context.
**/
VOID
EFIAPI
Tcp4IoCommonNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  *((BOOLEAN *) Context) = TRUE;
}

/**
  Create a TCP socket with the specified configuration data. 

  @param[in]  Image      The handle of the driver image.
  @param[in]  Controller The handle of the controller.
  @param[in]  ConfigData The Tcp4 configuration data.
  @param[in]  Tcp4Io     The Tcp4Io.
  
  @retval EFI_SUCCESS    The TCP socket is created and configured.
  @retval Others         Failed to create the TCP socket or configure it.
**/
EFI_STATUS
Tcp4IoCreateSocket (
  IN EFI_HANDLE           Image,
  IN EFI_HANDLE           Controller,
  IN TCP4_IO_CONFIG_DATA  *ConfigData,
  IN TCP4_IO              *Tcp4Io
  )
{
  EFI_STATUS            Status;
  EFI_TCP4_PROTOCOL     *Tcp4;
  EFI_TCP4_CONFIG_DATA  Tcp4ConfigData;
  EFI_TCP4_OPTION       ControlOption;
  EFI_TCP4_ACCESS_POINT *AccessPoint;

  Tcp4Io->Handle = NULL;
  Tcp4Io->ConnToken.CompletionToken.Event = NULL;
  Tcp4Io->TxToken.CompletionToken.Event = NULL;
  Tcp4Io->RxToken.CompletionToken.Event = NULL;
  Tcp4Io->CloseToken.CompletionToken.Event = NULL;
  Tcp4 = NULL;

  //
  // Create the TCP4 child instance and get the TCP4 protocol.
  //
  Status = NetLibCreateServiceChild (
            Controller,
            Image,
            &gEfiTcp4ServiceBindingProtocolGuid,
            &Tcp4Io->Handle
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Tcp4Io->Handle,
                  &gEfiTcp4ProtocolGuid,
                  (VOID **)&Tcp4Io->Tcp4,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Tcp4Io->Image       = Image;
  Tcp4Io->Controller  = Controller;
  Tcp4                = Tcp4Io->Tcp4;

  //
  // Set the configuration parameters.
  //
  ControlOption.ReceiveBufferSize       = 0x200000;
  ControlOption.SendBufferSize          = 0x200000;
  ControlOption.MaxSynBackLog           = 0;
  ControlOption.ConnectionTimeout       = 0;
  ControlOption.DataRetries             = 6;
  ControlOption.FinTimeout              = 0;
  ControlOption.TimeWaitTimeout         = 0;
  ControlOption.KeepAliveProbes         = 4;
  ControlOption.KeepAliveTime           = 0;
  ControlOption.KeepAliveInterval       = 0;
  ControlOption.EnableNagle             = FALSE;
  ControlOption.EnableTimeStamp         = FALSE;
  ControlOption.EnableWindowScaling     = TRUE;
  ControlOption.EnableSelectiveAck      = FALSE;
  ControlOption.EnablePathMtuDiscovery  = FALSE;

  Tcp4ConfigData.TypeOfService          = 8;
  Tcp4ConfigData.TimeToLive             = 255;
  Tcp4ConfigData.ControlOption          = &ControlOption;

  AccessPoint = &Tcp4ConfigData.AccessPoint;

  AccessPoint->UseDefaultAddress = FALSE;
  AccessPoint->StationPort = 0;
  AccessPoint->RemotePort = ConfigData->RemotePort;
  AccessPoint->ActiveFlag = TRUE;

  CopyMem (&AccessPoint->StationAddress, &ConfigData->LocalIp, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&AccessPoint->SubnetMask, &ConfigData->SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&AccessPoint->RemoteAddress, &ConfigData->RemoteIp, sizeof (EFI_IPv4_ADDRESS));

  //
  // Configure the TCP4 protocol.
  //
  Status = Tcp4->Configure (Tcp4, &Tcp4ConfigData);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  if (!EFI_IP4_EQUAL (&ConfigData->Gateway, &mZeroIp4Addr)) {
    //
    // the gateway is not zero, add the default route by hand
    //
    Status = Tcp4->Routes (Tcp4, FALSE, &mZeroIp4Addr, &mZeroIp4Addr, &ConfigData->Gateway);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }
  //
  // Create events for variuos asynchronous operations.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Tcp4IoCommonNotify,
                  &Tcp4Io->IsConnDone,
                  &Tcp4Io->ConnToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Tcp4IoCommonNotify,
                  &Tcp4Io->IsTxDone,
                  &Tcp4Io->TxToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Tcp4IoCommonNotify,
                  &Tcp4Io->IsRxDone,
                  &Tcp4Io->RxToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Tcp4IoCommonNotify,
                  &Tcp4Io->IsCloseDone,
                  &Tcp4Io->CloseToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Tcp4Io->IsTxDone  = FALSE;
  Tcp4Io->IsRxDone  = FALSE;

  return EFI_SUCCESS;

ON_ERROR:

  if (Tcp4Io->RxToken.CompletionToken.Event != NULL) {
    gBS->CloseEvent (Tcp4Io->RxToken.CompletionToken.Event);
  }

  if (Tcp4Io->TxToken.CompletionToken.Event != NULL) {
    gBS->CloseEvent (Tcp4Io->TxToken.CompletionToken.Event);
  }

  if (Tcp4Io->ConnToken.CompletionToken.Event != NULL) {
    gBS->CloseEvent (Tcp4Io->ConnToken.CompletionToken.Event);
  }

  if (Tcp4 != NULL) {
    Tcp4->Configure (Tcp4, NULL);

    gBS->CloseProtocol (
          Tcp4Io->Handle,
          &gEfiTcp4ProtocolGuid,
          Image,
          Controller
          );
  }

  NetLibDestroyServiceChild (
    Controller,
    Image,
    &gEfiTcp4ServiceBindingProtocolGuid,
    Tcp4Io->Handle
    );

  return Status;
}

/**
  Destroy the socket. 

  @param[in]  Tcp4Io The Tcp4Io which wraps the socket to be destroyeds.
**/
VOID
Tcp4IoDestroySocket (
  IN TCP4_IO  *Tcp4Io
  )
{
  EFI_TCP4_PROTOCOL *Tcp4;

  Tcp4 = Tcp4Io->Tcp4;

  Tcp4->Configure (Tcp4, NULL);

  gBS->CloseEvent (Tcp4Io->TxToken.CompletionToken.Event);
  gBS->CloseEvent (Tcp4Io->RxToken.CompletionToken.Event);
  gBS->CloseEvent (Tcp4Io->ConnToken.CompletionToken.Event);

  gBS->CloseProtocol (
        Tcp4Io->Handle,
        &gEfiTcp4ProtocolGuid,
        Tcp4Io->Image,
        Tcp4Io->Controller
        );

  NetLibDestroyServiceChild (
    Tcp4Io->Controller,
    Tcp4Io->Image,
    &gEfiTcp4ServiceBindingProtocolGuid,
    Tcp4Io->Handle
    );
}

/**
  Connect to the other endpoint of the TCP socket.

  @param[in, out]  Tcp4Io    The Tcp4Io wrapping the TCP socket.
  @param[in]       Timeout   The time to wait for connection done.
  
  @retval EFI_SUCCESS          Connect to the other endpoint of the TCP socket successfully.
  @retval EFI_TIMEOUT          Failed to connect to the other endpoint of the TCP socket in the                               specified time period.
  @retval Others               Other errors as indicated.
**/
EFI_STATUS
Tcp4IoConnect (
  IN OUT TCP4_IO    *Tcp4Io,
  IN EFI_EVENT      Timeout
  )
{
  EFI_TCP4_PROTOCOL *Tcp4;
  EFI_STATUS        Status;

  Tcp4Io->IsConnDone  = FALSE;
  Tcp4                = Tcp4Io->Tcp4;
  Status              = Tcp4->Connect (Tcp4, &Tcp4Io->ConnToken);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (!Tcp4Io->IsConnDone && EFI_ERROR (gBS->CheckEvent (Timeout))) {
    Tcp4->Poll (Tcp4);
  }

  if (!Tcp4Io->IsConnDone) {
    Status = EFI_TIMEOUT;
  } else {
    Status = Tcp4Io->ConnToken.CompletionToken.Status;
  }

  return Status;
}

/**
  Reset the socket.

  @param[in, out]  Tcp4Io The Tcp4Io wrapping the TCP socket.
**/
VOID
Tcp4IoReset (
  IN OUT TCP4_IO  *Tcp4Io
  )
{
  EFI_STATUS        Status;
  EFI_TCP4_PROTOCOL *Tcp4;

  Tcp4Io->CloseToken.AbortOnClose = TRUE;
  Tcp4Io->IsCloseDone             = FALSE;

  Tcp4 = Tcp4Io->Tcp4;
  Status = Tcp4->Close (Tcp4, &Tcp4Io->CloseToken);
  if (EFI_ERROR (Status)) {
    return ;
  }

  while (!Tcp4Io->IsCloseDone) {
    Tcp4->Poll (Tcp4);
  }
}

/**
  Transmit the Packet to the other endpoint of the socket.

  @param[in]   Tcp4Io          The Tcp4Io wrapping the TCP socket.
  @param[in]   Packet          The packet to transmit.
  
  @retval EFI_SUCCESS          The packet is trasmitted.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval Others               Other errors as indicated.
**/
EFI_STATUS
Tcp4IoTransmit (
  IN TCP4_IO  *Tcp4Io,
  IN NET_BUF  *Packet
  )
{
  EFI_TCP4_TRANSMIT_DATA  *TxData;
  EFI_TCP4_PROTOCOL       *Tcp4;
  EFI_STATUS              Status;

  TxData = AllocatePool (sizeof (EFI_TCP4_TRANSMIT_DATA) + (Packet->BlockOpNum - 1) * sizeof (EFI_TCP4_FRAGMENT_DATA));
  if (TxData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TxData->Push        = TRUE;
  TxData->Urgent      = FALSE;
  TxData->DataLength  = Packet->TotalSize;

  //
  // Build the fragment table.
  //
  TxData->FragmentCount = Packet->BlockOpNum;
  NetbufBuildExt (Packet, (NET_FRAGMENT *) &TxData->FragmentTable[0], &TxData->FragmentCount);

  Tcp4Io->TxToken.Packet.TxData = TxData;

  //
  // Trasnmit the packet.
  //
  Tcp4    = Tcp4Io->Tcp4;
  Status  = Tcp4->Transmit (Tcp4, &Tcp4Io->TxToken);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  while (!Tcp4Io->IsTxDone) {
    Tcp4->Poll (Tcp4);
  }

  Tcp4Io->IsTxDone  = FALSE;

  Status            = Tcp4Io->TxToken.CompletionToken.Status;

ON_EXIT:

  FreePool (TxData);

  return Status;
}

/**
  Receive data from the socket.

  @param[in]  Tcp4Io           The Tcp4Io which wraps the socket to be destroyed.
  @param[in]  Packet           The buffer to hold the data copy from the soket rx buffer.
  @param[in]  AsyncMode        Is this receive asyncronous or not.
  @param[in]  Timeout          The time to wait for receiving the amount of data the Packet
                               can hold.

  @retval EFI_SUCCESS          The required amount of data is received from the socket.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate momery.
  @retval EFI_TIMEOUT          Failed to receive the required amount of data in the
                               specified time period.
  @retval Others               Other errors as indicated.
**/
EFI_STATUS
Tcp4IoReceive (
  IN TCP4_IO    *Tcp4Io,
  IN NET_BUF    *Packet,
  IN BOOLEAN    AsyncMode,
  IN EFI_EVENT  Timeout
  )
{
  EFI_TCP4_PROTOCOL     *Tcp4;
  EFI_TCP4_RECEIVE_DATA RxData;
  EFI_STATUS            Status;
  NET_FRAGMENT          *Fragment;
  UINT32                FragmentCount;
  UINT32                CurrentFragment;

  FragmentCount = Packet->BlockOpNum;
  Fragment      = AllocatePool (FragmentCount * sizeof (NET_FRAGMENT));
  if (Fragment == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Build the fragment table.
  //
  NetbufBuildExt (Packet, Fragment, &FragmentCount);

  RxData.FragmentCount          = 1;
  Tcp4Io->RxToken.Packet.RxData = &RxData;
  CurrentFragment               = 0;
  Tcp4                          = Tcp4Io->Tcp4;
  Status                        = EFI_SUCCESS;

  while (CurrentFragment < FragmentCount) {
    RxData.DataLength                       = Fragment[CurrentFragment].Len;
    RxData.FragmentTable[0].FragmentLength  = Fragment[CurrentFragment].Len;
    RxData.FragmentTable[0].FragmentBuffer  = Fragment[CurrentFragment].Bulk;

    Status = Tcp4->Receive (Tcp4, &Tcp4Io->RxToken);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    while (!Tcp4Io->IsRxDone && ((Timeout == NULL) || EFI_ERROR (gBS->CheckEvent (Timeout)))) {
      //
      // Poll until some data is received or something error happens.
      //
      Tcp4->Poll (Tcp4);
    }

    if (!Tcp4Io->IsRxDone) {
      //
      // Timeout occurs, cancel the receive request.
      //
      Tcp4->Cancel (Tcp4, &Tcp4Io->RxToken.CompletionToken);

      Status = EFI_TIMEOUT;
      goto ON_EXIT;
    } else {
      Tcp4Io->IsRxDone = FALSE;
    }

    if (EFI_ERROR (Tcp4Io->RxToken.CompletionToken.Status)) {
      Status = Tcp4Io->RxToken.CompletionToken.Status;
      goto ON_EXIT;
    }

    Fragment[CurrentFragment].Len -= RxData.FragmentTable[0].FragmentLength;
    if (Fragment[CurrentFragment].Len == 0) {
      CurrentFragment++;
    } else {
      Fragment[CurrentFragment].Bulk += RxData.FragmentTable[0].FragmentLength;
    }
  }

ON_EXIT:
  Tcp4Io->RxToken.Packet.RxData = NULL;
  FreePool (Fragment);

  return Status;
}
