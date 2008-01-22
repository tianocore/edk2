/*++

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IScsiTcp4Io.c

Abstract:

--*/

#include "IScsiImpl.h"

VOID
EFIAPI
Tcp4IoCommonNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
/*++

Routine Description:

  The common notify function associated with various Tcp4Io events. 

Arguments:

  Event   - The event signaled.
  Contect - The context.

Returns:

  None.

--*/
{
  *((BOOLEAN *) Context) = TRUE;
}

EFI_STATUS
Tcp4IoCreateSocket (
  IN EFI_HANDLE           Image,
  IN EFI_HANDLE           Controller,
  IN TCP4_IO_CONFIG_DATA  *ConfigData,
  IN TCP4_IO              *Tcp4Io
  )
/*++

Routine Description:

  Create a TCP socket with the specified configuration data. 

Arguments:

  Image      - The handle of the driver image.
  Controller - The handle of the controller.
  ConfigData - The Tcp4 configuration data.
  Tcp4Io     - The Tcp4Io.

Returns:

  EFI_SUCCESS - The TCP socket is created and configured.
  other       - Failed to create the TCP socket or configure it.

--*/
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

  NetCopyMem (&AccessPoint->StationAddress, &ConfigData->LocalIp, sizeof (EFI_IPv4_ADDRESS));
  NetCopyMem (&AccessPoint->SubnetMask, &ConfigData->SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  NetCopyMem (&AccessPoint->RemoteAddress, &ConfigData->RemoteIp, sizeof (EFI_IPv4_ADDRESS));

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
                  EFI_EVENT_NOTIFY_SIGNAL,
                  NET_TPL_EVENT,
                  Tcp4IoCommonNotify,
                  &Tcp4Io->IsConnDone,
                  &Tcp4Io->ConnToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->CreateEvent (
                  EFI_EVENT_NOTIFY_SIGNAL,
                  NET_TPL_EVENT,
                  Tcp4IoCommonNotify,
                  &Tcp4Io->IsTxDone,
                  &Tcp4Io->TxToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->CreateEvent (
                  EFI_EVENT_NOTIFY_SIGNAL,
                  NET_TPL_EVENT,
                  Tcp4IoCommonNotify,
                  &Tcp4Io->IsRxDone,
                  &Tcp4Io->RxToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->CreateEvent (
                  EFI_EVENT_NOTIFY_SIGNAL,
                  NET_TPL_EVENT,
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

VOID
Tcp4IoDestroySocket (
  IN TCP4_IO  *Tcp4Io
  )
/*++

Routine Description:

  Destroy the socket. 

Arguments:

  Tcp4Io - The Tcp4Io which wraps the socket to be destroyeds.

Returns:

  None.

--*/
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

EFI_STATUS
Tcp4IoConnect (
  IN TCP4_IO    *Tcp4Io,
  IN EFI_EVENT  Timeout
  )
/*++

Routine Description:

  Connect to the other endpoint of the TCP socket.

Arguments:

  Tcp4Io  - The Tcp4Io wrapping the TCP socket.
  Timeout - The time to wait for connection done.

Returns:

  None.

--*/
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

VOID
Tcp4IoReset (
  IN TCP4_IO  *Tcp4Io
  )
/*++

Routine Description:

  Reset the socket.

Arguments:

  Tcp4Io - The Tcp4Io wrapping the TCP socket.

Returns:

  None.

--*/
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

EFI_STATUS
Tcp4IoTransmit (
  IN TCP4_IO  *Tcp4Io,
  IN NET_BUF  *Packet
  )
/*++

Routine Description:

  Transmit the Packet to the other endpoint of the socket.

Arguments:

  Tcp4Io - The Tcp4Io wrapping the TCP socket.
  Packet - The packet to transmit

Returns:

  EFI_SUCCESS          - The packet is trasmitted.
  EFI_OUT_OF_RESOURCES - Failed to allocate memory.

--*/
{
  EFI_TCP4_TRANSMIT_DATA  *TxData;
  EFI_TCP4_PROTOCOL       *Tcp4;
  EFI_STATUS              Status;

  TxData = NetAllocatePool (sizeof (EFI_TCP4_TRANSMIT_DATA) + (Packet->BlockOpNum - 1) * sizeof (EFI_TCP4_FRAGMENT_DATA));
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

  NetFreePool (TxData);

  return Status;
}

EFI_STATUS
Tcp4IoReceive (
  IN TCP4_IO    *Tcp4Io,
  IN NET_BUF    *Packet,
  IN BOOLEAN    AsyncMode,
  IN EFI_EVENT  Timeout
  )
/*++

Routine Description:

  Receive data from the socket.

Arguments:

  Tcp4Io    - The Tcp4Io which wraps the socket to be destroyeds.
  Packet    - The buffer to hold the data copy from the soket rx buffer.
  AsyncMode - Is this receive asyncronous or not.
  Timeout   - The time to wait for receiving the amount of data the Packet
              can hold.

Returns:

  EFI_SUCCESS          - The required amount of data is received from the socket.
  EFI_OUT_OF_RESOURCES - Failed to allocate momery.
  EFI_TIMEOUT          - Failed to receive the required amount of data in the
                         specified time period.

--*/
{
  EFI_TCP4_PROTOCOL     *Tcp4;
  EFI_TCP4_RECEIVE_DATA RxData;
  EFI_STATUS            Status;
  NET_FRAGMENT          *Fragment;
  UINT32                FragmentCount;
  UINT32                CurrentFragment;

  FragmentCount = Packet->BlockOpNum;
  Fragment      = NetAllocatePool (FragmentCount * sizeof (NET_FRAGMENT));
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

  NetFreePool (Fragment);

  return Status;
}
