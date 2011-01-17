/** @file
  This library is used to share code between UEFI network stack modules.
  It provides the helper routines to access TCP service.

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/TcpIoLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

/**
  The common notify function associated with various TcpIo events. 

  @param[in]  Event   The event signaled.
  @param[in]  Context The context.

**/
VOID
EFIAPI
TcpIoCommonNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if ((Event == NULL) || (Context == NULL)) {
    return ;
  }

  *((BOOLEAN *) Context) = TRUE;
}

/**
  The internal function for delay configuring TCP6 when IP6 driver is still in DAD.

  @param[in]  Tcp6               The EFI_TCP6_PROTOCOL protocol instance.
  @param[in]  Tcp6ConfigData     The Tcp6 configuration data.

  @retval EFI_SUCCESS            The operational settings successfully
                                 completed.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval Others                 Failed to finish the operation.

**/
EFI_STATUS
TcpIoGetMapping (
  IN EFI_TCP6_PROTOCOL    *Tcp6,
  IN EFI_TCP6_CONFIG_DATA *Tcp6ConfigData
  )
{
  EFI_STATUS              Status;
  EFI_EVENT               Event;

  if ((Tcp6 == NULL) || (Tcp6ConfigData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Event  = NULL;
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = gBS->SetTimer (
                  Event,
                  TimerRelative,
                  TCP_GET_MAPPING_TIMEOUT
                  );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  while (EFI_ERROR (gBS->CheckEvent (Event))) {

    Tcp6->Poll (Tcp6);

    Status = Tcp6->Configure (Tcp6, Tcp6ConfigData);

    if (!EFI_ERROR (Status)) {
      break;
    }
  }

ON_EXIT:

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  return Status;
}

/**
  Create a TCP socket with the specified configuration data. 

  @param[in]  Image      The handle of the driver image.
  @param[in]  Controller The handle of the controller.
  @param[in]  TcpVersion The version of Tcp, TCP_VERSION_4 or TCP_VERSION_6.
  @param[in]  ConfigData The Tcp configuration data.
  @param[out] TcpIo      The TcpIo.
  
  @retval EFI_SUCCESS            The TCP socket is created and configured.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Others                 Failed to create the TCP socket or configure it.

**/
EFI_STATUS
EFIAPI
TcpIoCreateSocket (
  IN EFI_HANDLE             Image,
  IN EFI_HANDLE             Controller,
  IN UINT8                  TcpVersion,
  IN TCP_IO_CONFIG_DATA     *ConfigData,
  OUT TCP_IO                *TcpIo
  )
{
  EFI_STATUS                Status;
  EFI_EVENT                 Event;
  EFI_GUID                  *ServiceBindingGuid;
  EFI_GUID                  *ProtocolGuid;
  VOID                      **Interface;
  EFI_TCP4_OPTION           ControlOption;
  EFI_TCP4_CONFIG_DATA      Tcp4ConfigData;
  EFI_TCP4_ACCESS_POINT     *AccessPoint4;
  EFI_TCP4_PROTOCOL         *Tcp4;
  EFI_TCP6_CONFIG_DATA      Tcp6ConfigData;
  EFI_TCP6_ACCESS_POINT     *AccessPoint6;
  EFI_TCP6_PROTOCOL         *Tcp6;
  EFI_TCP4_RECEIVE_DATA     *RxData;

  if ((Image == NULL) || (Controller == NULL) || (ConfigData == NULL) || (TcpIo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Tcp4 = NULL;
  Tcp6 = NULL;

  ZeroMem (TcpIo, sizeof (TCP_IO));

  if (TcpVersion == TCP_VERSION_4) {
    ServiceBindingGuid = &gEfiTcp4ServiceBindingProtocolGuid;
    ProtocolGuid       = &gEfiTcp4ProtocolGuid;
    Interface          = (VOID **) (&TcpIo->Tcp.Tcp4);
  } else if (TcpVersion == TCP_VERSION_6) {
    ServiceBindingGuid = &gEfiTcp6ServiceBindingProtocolGuid;
    ProtocolGuid       = &gEfiTcp6ProtocolGuid;
    Interface          = (VOID **) (&TcpIo->Tcp.Tcp6);
  } else {
    return EFI_UNSUPPORTED;
  }

  TcpIo->TcpVersion = TcpVersion;

  //
  // Create the TCP child instance and get the TCP protocol.
  //  
  Status = NetLibCreateServiceChild (
             Controller,
             Image,
             ServiceBindingGuid,
             &TcpIo->Handle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  TcpIo->Handle,
                  ProtocolGuid,
                  Interface,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) || (*Interface == NULL)) {
    goto ON_ERROR;
  }

  if (TcpVersion == TCP_VERSION_4) {
    Tcp4             = TcpIo->Tcp.Tcp4;
  } else {
    Tcp6             = TcpIo->Tcp.Tcp6;
  }

  TcpIo->Image       = Image;
  TcpIo->Controller  = Controller;

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

  if (TcpVersion == TCP_VERSION_4) {
    Tcp4ConfigData.TypeOfService        = 8;
    Tcp4ConfigData.TimeToLive           = 255;
    Tcp4ConfigData.ControlOption        = &ControlOption;

    AccessPoint4                        = &Tcp4ConfigData.AccessPoint;

    ZeroMem (AccessPoint4, sizeof (EFI_TCP4_ACCESS_POINT));
    AccessPoint4->StationPort           = ConfigData->Tcp4IoConfigData.StationPort;
    AccessPoint4->RemotePort            = ConfigData->Tcp4IoConfigData.RemotePort;
    AccessPoint4->ActiveFlag            = ConfigData->Tcp4IoConfigData.ActiveFlag;

    CopyMem (
      &AccessPoint4->StationAddress,
      &ConfigData->Tcp4IoConfigData.LocalIp,
      sizeof (EFI_IPv4_ADDRESS)
      );
    CopyMem (
      &AccessPoint4->SubnetMask,
      &ConfigData->Tcp4IoConfigData.SubnetMask,
      sizeof (EFI_IPv4_ADDRESS)
      );
    CopyMem (
      &AccessPoint4->RemoteAddress,
      &ConfigData->Tcp4IoConfigData.RemoteIp,
      sizeof (EFI_IPv4_ADDRESS)
      );

    ASSERT (Tcp4 != NULL);

    //
    // Configure the TCP4 protocol.
    //
    Status = Tcp4->Configure (Tcp4, &Tcp4ConfigData);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    if (!EFI_IP4_EQUAL (&ConfigData->Tcp4IoConfigData.Gateway, &mZeroIp4Addr)) {
      //
      // The gateway is not zero. Add the default route manually.
      //
      Status = Tcp4->Routes (
                       Tcp4,
                       FALSE,
                       &mZeroIp4Addr,
                       &mZeroIp4Addr,
                       &ConfigData->Tcp4IoConfigData.Gateway
                       );
      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
    }
  } else {
    Tcp6ConfigData.TrafficClass         = 0;
    Tcp6ConfigData.HopLimit             = 255;
    Tcp6ConfigData.ControlOption        = (EFI_TCP6_OPTION *) &ControlOption;

    AccessPoint6                        = &Tcp6ConfigData.AccessPoint;

    ZeroMem (AccessPoint6, sizeof (EFI_TCP6_ACCESS_POINT));
    AccessPoint6->StationPort           = ConfigData->Tcp6IoConfigData.StationPort;
    AccessPoint6->RemotePort            = ConfigData->Tcp6IoConfigData.RemotePort;
    AccessPoint6->ActiveFlag            = ConfigData->Tcp6IoConfigData.ActiveFlag;

    IP6_COPY_ADDRESS (&AccessPoint6->RemoteAddress, &ConfigData->Tcp6IoConfigData.RemoteIp);


    ASSERT (Tcp6 != NULL);
    //
    // Configure the TCP6 protocol.
    //
    Status = Tcp6->Configure (Tcp6, &Tcp6ConfigData);
    if (Status == EFI_NO_MAPPING) {
      Status = TcpIoGetMapping (Tcp6, &Tcp6ConfigData);
    }

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
                  TcpIoCommonNotify,
                  &TcpIo->IsConnDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  TcpIo->ConnToken.Tcp4Token.CompletionToken.Event = Event;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  TcpIoCommonNotify,
                  &TcpIo->IsListenDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  TcpIo->ListenToken.Tcp4Token.CompletionToken.Event = Event;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  TcpIoCommonNotify,
                  &TcpIo->IsTxDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  TcpIo->TxToken.Tcp4Token.CompletionToken.Event = Event;


  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  TcpIoCommonNotify,
                  &TcpIo->IsRxDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  TcpIo->RxToken.Tcp4Token.CompletionToken.Event = Event;

  RxData = (EFI_TCP4_RECEIVE_DATA *) AllocateZeroPool (sizeof (EFI_TCP4_RECEIVE_DATA));
  if (RxData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  TcpIo->RxToken.Tcp4Token.Packet.RxData = RxData;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  TcpIoCommonNotify,
                  &TcpIo->IsCloseDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  TcpIo->CloseToken.Tcp4Token.CompletionToken.Event = Event;


  return EFI_SUCCESS;

ON_ERROR:

  TcpIoDestroySocket (TcpIo);

  return Status;
}
  
/**
  Destroy the socket. 

  @param[in]  TcpIo The TcpIo which wraps the socket to be destroyed.

**/
VOID
EFIAPI
TcpIoDestroySocket (
  IN TCP_IO                 *TcpIo
  )
{
  EFI_EVENT                 Event;
  EFI_TCP4_PROTOCOL         *Tcp4;
  EFI_TCP6_PROTOCOL         *Tcp6;
  UINT8                     TcpVersion;
  EFI_GUID                  *ServiceBindingGuid;
  EFI_GUID                  *ProtocolGuid;
  EFI_HANDLE                ChildHandle;

  if (TcpIo == NULL) {
    return ;
  }

  TcpVersion = TcpIo->TcpVersion;

  if ((TcpVersion != TCP_VERSION_4) && (TcpVersion != TCP_VERSION_6)) {
    return ;
  }

  Event = TcpIo->ConnToken.Tcp4Token.CompletionToken.Event;

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  Event = TcpIo->ListenToken.Tcp4Token.CompletionToken.Event;

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  Event = TcpIo->TxToken.Tcp4Token.CompletionToken.Event;

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  Event = TcpIo->RxToken.Tcp4Token.CompletionToken.Event;

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  Event = TcpIo->CloseToken.Tcp4Token.CompletionToken.Event;

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  if (TcpIo->RxToken.Tcp4Token.Packet.RxData != NULL) {
    FreePool (TcpIo->RxToken.Tcp4Token.Packet.RxData);
  }

  Tcp4 = NULL;
  Tcp6 = NULL;


  if (TcpVersion == TCP_VERSION_4) {
    ServiceBindingGuid = &gEfiTcp4ServiceBindingProtocolGuid;
    ProtocolGuid       = &gEfiTcp4ProtocolGuid;
    Tcp4 = TcpIo->Tcp.Tcp4;
    if (Tcp4 != NULL) {
      Tcp4->Configure (Tcp4, NULL);
    }
  } else {
    ServiceBindingGuid = &gEfiTcp6ServiceBindingProtocolGuid;
    ProtocolGuid       = &gEfiTcp6ProtocolGuid;
    Tcp6 = TcpIo->Tcp.Tcp6;
    if (Tcp6 != NULL) {
      Tcp6->Configure (Tcp6, NULL);
    }
  }

  if ((Tcp4 != NULL) || (Tcp6 != NULL)) {

    gBS->CloseProtocol (
           TcpIo->Handle,
           ProtocolGuid,
           TcpIo->Image,
           TcpIo->Controller
           );
  }

  ChildHandle = NULL;

  if (TcpIo->IsListenDone) {
    if (TcpVersion == TCP_VERSION_4) {
      Tcp4 = TcpIo->NewTcp.Tcp4;
      if (Tcp4 != NULL) {
        Tcp4->Configure (Tcp4, NULL);
        ChildHandle = TcpIo->ListenToken.Tcp4Token.NewChildHandle;
      }
    } else {
      Tcp6 = TcpIo->NewTcp.Tcp6;
      if (Tcp6 != NULL) {
        Tcp6->Configure (Tcp6, NULL);
        ChildHandle = TcpIo->ListenToken.Tcp6Token.NewChildHandle;
      }
    }

    if (ChildHandle != NULL) {

      gBS->CloseProtocol (
             ChildHandle,
             ProtocolGuid,
             TcpIo->Image,
             TcpIo->Controller
             );
    }
  }

  NetLibDestroyServiceChild (
    TcpIo->Controller,
    TcpIo->Image,
    ServiceBindingGuid,
    TcpIo->Handle
    );
}

/**
  Connect to the other endpoint of the TCP socket.

  @param[in, out]  TcpIo     The TcpIo wrapping the TCP socket.
  @param[in]       Timeout   The time to wait for connection done.
  
  @retval EFI_SUCCESS            Connect to the other endpoint of the TCP socket
                                 successfully.
  @retval EFI_TIMEOUT            Failed to connect to the other endpoint of the
                                 TCP socket in the specified time period.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TcpIoConnect (
  IN OUT TCP_IO             *TcpIo,
  IN     EFI_EVENT          Timeout
  )
{
  EFI_TCP4_PROTOCOL         *Tcp4;
  EFI_TCP6_PROTOCOL         *Tcp6;
  EFI_STATUS                Status;

  if ((TcpIo == NULL) || (TcpIo->Tcp.Tcp4 == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TcpIo->IsConnDone = FALSE;

  Tcp4 = NULL;
  Tcp6 = NULL;

  if (TcpIo->TcpVersion == TCP_VERSION_4) {
    Tcp4   = TcpIo->Tcp.Tcp4;
    Status = Tcp4->Connect (Tcp4, &TcpIo->ConnToken.Tcp4Token);
  } else if (TcpIo->TcpVersion == TCP_VERSION_6) {
    Tcp6   = TcpIo->Tcp.Tcp6;
    Status = Tcp6->Connect (Tcp6, &TcpIo->ConnToken.Tcp6Token);
  } else {
    return EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (!TcpIo->IsConnDone && EFI_ERROR (gBS->CheckEvent (Timeout))) {
    if (TcpIo->TcpVersion == TCP_VERSION_4) {
      Tcp4->Poll (Tcp4);
    } else {
      Tcp6->Poll (Tcp6);
    }
  }

  if (!TcpIo->IsConnDone) {
    Status = EFI_TIMEOUT;
  } else {
    Status = TcpIo->ConnToken.Tcp4Token.CompletionToken.Status;
  }

  return Status;
}

/**
  Accept the incomding request from the other endpoint of the TCP socket.

  @param[in, out]  TcpIo     The TcpIo wrapping the TCP socket.
  @param[in]       Timeout   The time to wait for connection done.

  
  @retval EFI_SUCCESS            Connect to the other endpoint of the TCP socket
                                 successfully.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.

  @retval EFI_TIMEOUT            Failed to connect to the other endpoint of the
                                 TCP socket in the specified time period.                      
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TcpIoAccept (
  IN OUT TCP_IO             *TcpIo,
  IN     EFI_EVENT          Timeout
  )
{
  EFI_STATUS                Status;
  EFI_GUID                  *ProtocolGuid;
  EFI_TCP4_PROTOCOL         *Tcp4;
  EFI_TCP6_PROTOCOL         *Tcp6;

  if ((TcpIo == NULL) || (TcpIo->Tcp.Tcp4 == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TcpIo->IsListenDone = FALSE;

  Tcp4 = NULL;
  Tcp6 = NULL;

  if (TcpIo->TcpVersion == TCP_VERSION_4) {
    Tcp4   = TcpIo->Tcp.Tcp4;
    Status = Tcp4->Accept (Tcp4, &TcpIo->ListenToken.Tcp4Token);
  } else if (TcpIo->TcpVersion == TCP_VERSION_6) {
    Tcp6   = TcpIo->Tcp.Tcp6;
    Status = Tcp6->Accept (Tcp6, &TcpIo->ListenToken.Tcp6Token);
  } else {
    return EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (!TcpIo->IsListenDone && EFI_ERROR (gBS->CheckEvent (Timeout))) {
    if (TcpIo->TcpVersion == TCP_VERSION_4) {
      Tcp4->Poll (Tcp4);
    } else {
      Tcp6->Poll (Tcp6);
    }
  }

  if (!TcpIo->IsListenDone) {
    Status = EFI_TIMEOUT;
  } else {
    Status = TcpIo->ListenToken.Tcp4Token.CompletionToken.Status;
  }

  //
  // The new TCP instance handle created for the established connection is 
  // in ListenToken.
  //
  if (!EFI_ERROR (Status)) {
    if (TcpIo->TcpVersion == TCP_VERSION_4) {
      ProtocolGuid = &gEfiTcp4ProtocolGuid;
    } else {
      ProtocolGuid = &gEfiTcp6ProtocolGuid;
    }
    
    Status = gBS->OpenProtocol (
                    TcpIo->ListenToken.Tcp4Token.NewChildHandle,
                    ProtocolGuid,
                    (VOID **) (&TcpIo->NewTcp.Tcp4),
                    TcpIo->Image,
                    TcpIo->Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );

  }

  return Status;
}

/**
  Reset the socket.

  @param[in, out]  TcpIo The TcpIo wrapping the TCP socket.

**/
VOID
EFIAPI
TcpIoReset (
  IN OUT TCP_IO             *TcpIo
  )
{
  EFI_TCP4_PROTOCOL         *Tcp4;
  EFI_TCP6_PROTOCOL         *Tcp6;
  EFI_STATUS                Status;

  if ((TcpIo == NULL) || (TcpIo->Tcp.Tcp4 == NULL)) {
    return ;
  }

  TcpIo->IsCloseDone = FALSE;
  Tcp4               = NULL;
  Tcp6               = NULL;

  if (TcpIo->TcpVersion == TCP_VERSION_4) { 
    TcpIo->CloseToken.Tcp4Token.AbortOnClose = TRUE;
    Tcp4 = TcpIo->Tcp.Tcp4;
    Status = Tcp4->Close (Tcp4, &TcpIo->CloseToken.Tcp4Token);
  } else if (TcpIo->TcpVersion == TCP_VERSION_6) {
    TcpIo->CloseToken.Tcp6Token.AbortOnClose = TRUE;
    Tcp6 = TcpIo->Tcp.Tcp6;
    Status = Tcp6->Close (Tcp6, &TcpIo->CloseToken.Tcp6Token);
  } else {
    return ;
  }

  if (EFI_ERROR (Status)) {
    return ;
  }

  while (!TcpIo->IsCloseDone) {
    if (TcpIo->TcpVersion == TCP_VERSION_4) {
      Tcp4->Poll (Tcp4);
    } else {
      Tcp6->Poll (Tcp6);
    }
  }
}

  
/**
  Transmit the Packet to the other endpoint of the socket.

  @param[in]   TcpIo           The TcpIo wrapping the TCP socket.
  @param[in]   Packet          The packet to transmit.
  
  @retval EFI_SUCCESS            The packet is trasmitted.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TcpIoTransmit (
  IN TCP_IO                 *TcpIo,
  IN NET_BUF                *Packet
  )
{
  EFI_STATUS                Status;
  VOID                      *Data;
  EFI_TCP4_PROTOCOL         *Tcp4;
  EFI_TCP6_PROTOCOL         *Tcp6;
  UINTN                     Size;

  if ((TcpIo == NULL) || (TcpIo->Tcp.Tcp4 == NULL)|| (Packet == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (TcpIo->TcpVersion == TCP_VERSION_4) {

    Size = sizeof (EFI_TCP4_TRANSMIT_DATA) + 
           (Packet->BlockOpNum - 1) * sizeof (EFI_TCP4_FRAGMENT_DATA);
  } else if (TcpIo->TcpVersion == TCP_VERSION_6) {
    Size = sizeof (EFI_TCP6_TRANSMIT_DATA) +
           (Packet->BlockOpNum - 1) * sizeof (EFI_TCP6_FRAGMENT_DATA);
  } else {
    return EFI_UNSUPPORTED;
  }

  Data = AllocatePool (Size);
  if (Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((EFI_TCP4_TRANSMIT_DATA *) Data)->Push        = TRUE;
  ((EFI_TCP4_TRANSMIT_DATA *) Data)->Urgent      = FALSE;
  ((EFI_TCP4_TRANSMIT_DATA *) Data)->DataLength  = Packet->TotalSize;

  //
  // Build the fragment table.
  //
  ((EFI_TCP4_TRANSMIT_DATA *) Data)->FragmentCount = Packet->BlockOpNum;

  NetbufBuildExt (
    Packet,
    (NET_FRAGMENT *) &((EFI_TCP4_TRANSMIT_DATA *) Data)->FragmentTable[0],
    &((EFI_TCP4_TRANSMIT_DATA *) Data)->FragmentCount
    );

  Tcp4   = NULL;
  Tcp6   = NULL;
  Status = EFI_DEVICE_ERROR;

  //
  // Trasnmit the packet.
  //
  if (TcpIo->TcpVersion == TCP_VERSION_4) {
    TcpIo->TxToken.Tcp4Token.Packet.TxData = (EFI_TCP4_TRANSMIT_DATA *) Data;
    Tcp4    = TcpIo->Tcp.Tcp4;
    if (TcpIo->IsListenDone) {
      Tcp4 = TcpIo->NewTcp.Tcp4;
    }

    if (Tcp4 == NULL) {
      goto ON_EXIT;
    }
    
    Status  = Tcp4->Transmit (Tcp4, &TcpIo->TxToken.Tcp4Token);
  } else {
    TcpIo->TxToken.Tcp6Token.Packet.TxData = (EFI_TCP6_TRANSMIT_DATA *) Data;
    Tcp6    = TcpIo->Tcp.Tcp6;
    if (TcpIo->IsListenDone) {
      Tcp6 = TcpIo->NewTcp.Tcp6;
    }

    if (Tcp6 == NULL) {
      goto ON_EXIT;
    }

    Status  = Tcp6->Transmit (Tcp6, &TcpIo->TxToken.Tcp6Token);
  }

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  while (!TcpIo->IsTxDone) {
    if (TcpIo->TcpVersion == TCP_VERSION_4) {
      Tcp4->Poll (Tcp4);
    } else {
      Tcp6->Poll (Tcp6);
    }
  }

  TcpIo->IsTxDone  = FALSE;
  Status           = TcpIo->TxToken.Tcp4Token.CompletionToken.Status;

ON_EXIT:

  FreePool (Data);

  return Status;
}

/**
  Receive data from the socket.

  @param[in, out]  TcpIo       The TcpIo which wraps the socket to be destroyed.
  @param[in]       Packet      The buffer to hold the data copy from the socket rx buffer.
  @param[in]       AsyncMode   Is this receive asyncronous or not.
  @param[in]       Timeout     The time to wait for receiving the amount of data the Packet
                               can hold.

  @retval EFI_SUCCESS            The required amount of data is received from the socket.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate momery.
  @retval EFI_TIMEOUT            Failed to receive the required amount of data in the
                                 specified time period.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TcpIoReceive (
  IN OUT TCP_IO             *TcpIo,
  IN     NET_BUF            *Packet,
  IN     BOOLEAN            AsyncMode,
  IN     EFI_EVENT          Timeout
  )
{
  EFI_TCP4_PROTOCOL         *Tcp4;
  EFI_TCP6_PROTOCOL         *Tcp6;
  EFI_TCP4_RECEIVE_DATA     *RxData;
  EFI_STATUS                Status;
  NET_FRAGMENT              *Fragment;
  UINT32                    FragmentCount;
  UINT32                    CurrentFragment;

  if ((TcpIo == NULL) || (TcpIo->Tcp.Tcp4 == NULL)|| (Packet == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  RxData = TcpIo->RxToken.Tcp4Token.Packet.RxData;
  if (RxData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Tcp4 = NULL;
  Tcp6 = NULL;

  if (TcpIo->TcpVersion == TCP_VERSION_4) {
    Tcp4 = TcpIo->Tcp.Tcp4;

    if (TcpIo->IsListenDone) {
      Tcp4 = TcpIo->NewTcp.Tcp4;
    }

    if (Tcp4 == NULL) {
      return EFI_DEVICE_ERROR;
    }

  } else if (TcpIo->TcpVersion == TCP_VERSION_6) {
    Tcp6 = TcpIo->Tcp.Tcp6;

    if (TcpIo->IsListenDone) {
      Tcp6 = TcpIo->NewTcp.Tcp6;
    }

    if (Tcp6 == NULL) {
      return EFI_DEVICE_ERROR; 
    }

  } else {
    return EFI_UNSUPPORTED;
  }

  FragmentCount = Packet->BlockOpNum;
  Fragment      = AllocatePool (FragmentCount * sizeof (NET_FRAGMENT));
  if (Fragment == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  //
  // Build the fragment table.
  //
  NetbufBuildExt (Packet, Fragment, &FragmentCount);

  RxData->FragmentCount         = 1;
  CurrentFragment               = 0;
  Status                        = EFI_SUCCESS;

  while (CurrentFragment < FragmentCount) {
    RxData->DataLength                       = Fragment[CurrentFragment].Len;
    RxData->FragmentTable[0].FragmentLength  = Fragment[CurrentFragment].Len;
    RxData->FragmentTable[0].FragmentBuffer  = Fragment[CurrentFragment].Bulk;

    if (TcpIo->TcpVersion == TCP_VERSION_4) {
      Status = Tcp4->Receive (Tcp4, &TcpIo->RxToken.Tcp4Token);
    } else {
      Status = Tcp6->Receive (Tcp6, &TcpIo->RxToken.Tcp6Token);
    }
    
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
    
    while (!TcpIo->IsRxDone && ((Timeout == NULL) || EFI_ERROR (gBS->CheckEvent (Timeout)))) {
      //
      // Poll until some data is received or an error occurs.
      //
      if (TcpIo->TcpVersion == TCP_VERSION_4) {
        Tcp4->Poll (Tcp4);
      } else {
        Tcp6->Poll (Tcp6);
      }
    }

    if (!TcpIo->IsRxDone) {
      //
      // Timeout occurs, cancel the receive request.
      //
      if (TcpIo->TcpVersion == TCP_VERSION_4) {
        Tcp4->Cancel (Tcp4, &TcpIo->RxToken.Tcp4Token.CompletionToken);
      } else {
        Tcp6->Cancel (Tcp6, &TcpIo->RxToken.Tcp6Token.CompletionToken);
      }

      Status = EFI_TIMEOUT;
      goto ON_EXIT;
    } else {
      TcpIo->IsRxDone = FALSE;
    }

    Status = TcpIo->RxToken.Tcp4Token.CompletionToken.Status;

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    Fragment[CurrentFragment].Len -= RxData->FragmentTable[0].FragmentLength;
    if (Fragment[CurrentFragment].Len == 0) {
      CurrentFragment++;
    } else {
      Fragment[CurrentFragment].Bulk += RxData->FragmentTable[0].FragmentLength;
    }
  }

ON_EXIT:

  if (Fragment != NULL) {
    FreePool (Fragment);
  }

  return Status;
}
