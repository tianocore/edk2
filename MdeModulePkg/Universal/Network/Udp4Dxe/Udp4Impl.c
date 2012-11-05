/** @file
  The implementation of the Udp4 protocol.
  
Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Udp4Impl.h"

UINT16  mUdp4RandomPort;

/**
  This function checks and timeouts the I/O datagrams holding by the corresponding
  service context.

  @param[in]  Event                  The event this function registered to.
  @param[in]  Context                The context data registered during the creation of
                                     the Event.

**/
VOID
EFIAPI
Udp4CheckTimeout (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  This function finds the udp instance by the specified <Address, Port> pair.

  @param[in]  InstanceList           Pointer to the head of the list linking the udp
                                     instances.
  @param[in]  Address                Pointer to the specified IPv4 address.
  @param[in]  Port                   The udp port number.

  @retval TRUE     The specified <Address, Port> pair is found.
  @retval FALSE    Otherwise.

**/
BOOLEAN
Udp4FindInstanceByPort (
  IN LIST_ENTRY        *InstanceList,
  IN EFI_IPv4_ADDRESS  *Address,
  IN UINT16            Port
  );

/**
  This function is the packet transmitting notify function registered to the IpIo
  interface. It's called to signal the udp TxToken when IpIo layer completes the
  transmitting of the udp datagram.

  @param[in]  Status                 The completion status of the output udp datagram.
  @param[in]  Context                Pointer to the context data.
  @param[in]  Sender                 Specify a pointer of EFI_IP4_PROTOCOL for sending.
  @param[in]  NotifyData             Pointer to the notify data.

**/
VOID
EFIAPI
Udp4DgramSent (
  IN EFI_STATUS        Status,
  IN VOID              *Context,
  IN IP_IO_IP_PROTOCOL Sender,
  IN VOID              *NotifyData
  );

/**
  This function processes the received datagram passed up by the IpIo layer.

  @param[in]  Status             The status of this udp datagram.
  @param[in]  IcmpError          The IcmpError code, only available when Status is
                                 EFI_ICMP_ERROR.
  @param[in]  NetSession         Pointer to the EFI_NET_SESSION_DATA.
  @param[in]  Packet             Pointer to the NET_BUF containing the received udp
                                 datagram.
  @param[in]  Context            Pointer to the context data.

**/
VOID
EFIAPI
Udp4DgramRcvd (
  IN EFI_STATUS            Status,
  IN UINT8                 IcmpError,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Packet,
  IN VOID                  *Context
  );

/**
  This function cancels the token specified by Arg in the Map. This is a callback
  used by Udp4InstanceCancelToken().

  @param[in]  Map                Pointer to the NET_MAP.
  @param[in]  Item               Pointer to the NET_MAP_ITEM.
  @param[in]  Arg                Pointer to the token to be cancelled, if NULL,
                                 the token specified by Item is cancelled.

  @retval EFI_SUCCESS            The token is cancelled if Arg is NULL or the token
                                 is not the same as that in the Item if Arg is not
                                 NULL.
  @retval EFI_ABORTED            Arg is not NULL, and the token specified by Arg is
                                 cancelled.

**/
EFI_STATUS
EFIAPI
Udp4CancelTokens (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg OPTIONAL
  );

/**
  This function matches the received udp datagram with the Instance.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  Udp4Session        Pointer to the EFI_UDP4_SESSION_DATA abstracted
                                 from the received udp datagram.

  @retval TRUE       The udp datagram matches the receiving requirments of the
                     udp Instance.
  @retval FALSE      Otherwise.

**/
BOOLEAN
Udp4MatchDgram (
  IN UDP4_INSTANCE_DATA     *Instance,
  IN EFI_UDP4_SESSION_DATA  *Udp4Session
  );

/**
  This function removes the Wrap specified by Context and release relevant resources.

  @param[in]  Event              The Event this notify function registered to.
  @param[in]  Context            Pointer to the context data.

**/
VOID
EFIAPI
Udp4RecycleRxDataWrap (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  This function wraps the Packet and the RxData.

  @param[in]  Instance               Pointer to the instance context data.
  @param[in]  Packet                 Pointer to the buffer containing the received
                                     datagram.
  @param[in]  RxData                 Pointer to the EFI_UDP4_RECEIVE_DATA of this
                                     datagram.

  @return Pointer to the structure wrapping the RxData and the Packet.

**/
UDP4_RXDATA_WRAP *
Udp4WrapRxData (
  IN UDP4_INSTANCE_DATA     *Instance,
  IN NET_BUF                *Packet,
  IN EFI_UDP4_RECEIVE_DATA  *RxData
  );

/**
  This function enqueues the received datagram into the instances' receiving queues.

  @param[in]  Udp4Service            Pointer to the udp service context data.
  @param[in]  Packet                 Pointer to the buffer containing the received
                                     datagram.
  @param[in]  RxData                 Pointer to the EFI_UDP4_RECEIVE_DATA of this
                                     datagram.

  @return The times this datagram is enqueued.

**/
UINTN
Udp4EnqueueDgram (
  IN UDP4_SERVICE_DATA      *Udp4Service,
  IN NET_BUF                *Packet,
  IN EFI_UDP4_RECEIVE_DATA  *RxData
  );

/**
  This function delivers the datagrams enqueued in the instances.

  @param[in]  Udp4Service            Pointer to the udp service context data.

**/
VOID
Udp4DeliverDgram (
  IN UDP4_SERVICE_DATA  *Udp4Service
  );

/**
  This function demultiplexes the received udp datagram to the apropriate instances.

  @param[in]  Udp4Service            Pointer to the udp service context data.
  @param[in]  NetSession             Pointer to the EFI_NET_SESSION_DATA abstrated from
                                     the received datagram.
  @param[in]  Packet                 Pointer to the buffer containing the received udp
                                     datagram.

**/
VOID
Udp4Demultiplex (
  IN UDP4_SERVICE_DATA     *Udp4Service,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Packet
  );

/**
  This function handles the received Icmp Error message and demultiplexes it to the
  instance.

  @param[in]  Udp4Service            Pointer to the udp service context data.
  @param[in]  IcmpError              The icmp error code.
  @param[in]  NetSession             Pointer to the EFI_NET_SESSION_DATA abstracted
                                 from the received Icmp Error packet.
  @param[in]  Packet                 Pointer to the Icmp Error packet.

**/
VOID
Udp4IcmpHandler (
  IN UDP4_SERVICE_DATA     *Udp4Service,
  IN UINT8                 IcmpError,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Packet
  );

/**
  This function builds and sends out a icmp port unreachable message.

  @param[in]  IpIo                   Pointer to the IP_IO instance.
  @param[in]  NetSession             Pointer to the EFI_NET_SESSION_DATA of the packet
                                     causes this icmp error message.
  @param[in]  Udp4Header             Pointer to the udp header of the datagram causes
                                     this icmp error message.

**/
VOID
Udp4SendPortUnreach (
  IN IP_IO                 *IpIo,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN VOID                  *Udp4Header
  );


/**
  Create the Udp service context data.

  @param[in, out] Udp4Service      Pointer to the UDP4_SERVICE_DATA.
  @param[in]      ImageHandle      The image handle of this udp4 driver.
  @param[in]      ControllerHandle The controller handle this udp4 driver binds on.

  @retval EFI_SUCCESS              The udp4 service context data is created and
                                   initialized.
  @retval EFI_OUT_OF_RESOURCES     Cannot allocate memory.
  @retval other                    Other error occurs.

**/
EFI_STATUS
Udp4CreateService (
  IN OUT UDP4_SERVICE_DATA  *Udp4Service,
  IN     EFI_HANDLE         ImageHandle,
  IN     EFI_HANDLE         ControllerHandle
  )
{
  EFI_STATUS          Status;
  IP_IO_OPEN_DATA     OpenData;
  EFI_IP4_CONFIG_DATA *Ip4ConfigData;

  ZeroMem (Udp4Service, sizeof (UDP4_SERVICE_DATA));

  Udp4Service->Signature        = UDP4_SERVICE_DATA_SIGNATURE;
  Udp4Service->ServiceBinding   = mUdp4ServiceBinding;
  Udp4Service->ImageHandle      = ImageHandle;
  Udp4Service->ControllerHandle = ControllerHandle;
  Udp4Service->ChildrenNumber   = 0;

  InitializeListHead (&Udp4Service->ChildrenList);

  //
  // Create the IpIo for this service context.
  //
  Udp4Service->IpIo = IpIoCreate (ImageHandle, ControllerHandle, IP_VERSION_4);
  if (Udp4Service->IpIo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set the OpenData used to open the IpIo.
  //
  Ip4ConfigData = &OpenData.IpConfigData.Ip4CfgData;
  CopyMem (Ip4ConfigData, &mIp4IoDefaultIpConfigData, sizeof (EFI_IP4_CONFIG_DATA));
  Ip4ConfigData->AcceptBroadcast = TRUE;
  OpenData.RcvdContext           = (VOID *) Udp4Service;
  OpenData.SndContext            = NULL;
  OpenData.PktRcvdNotify         = Udp4DgramRcvd;
  OpenData.PktSentNotify         = Udp4DgramSent;

  //
  // Configure and start the IpIo.
  //
  Status = IpIoOpen (Udp4Service->IpIo, &OpenData);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Create the event for Udp timeout checking.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Udp4CheckTimeout,
                  Udp4Service,
                  &Udp4Service->TimeoutEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Start the timeout timer event.
  //
  Status = gBS->SetTimer (
                  Udp4Service->TimeoutEvent,
                  TimerPeriodic,
                  UDP4_TIMEOUT_INTERVAL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (Udp4Service->TimeoutEvent != NULL) {
    gBS->CloseEvent (Udp4Service->TimeoutEvent);
  }

  IpIoDestroy (Udp4Service->IpIo);

  return Status;
}


/**
  Clean the Udp service context data.

  @param[in]  Udp4Service            Pointer to the UDP4_SERVICE_DATA.

**/
VOID
Udp4CleanService (
  IN UDP4_SERVICE_DATA  *Udp4Service
  )
{
  //
  // Cancel the TimeoutEvent timer.
  //
  gBS->SetTimer (Udp4Service->TimeoutEvent, TimerCancel, 0);

  //
  // Close the TimeoutEvent timer.
  //
  gBS->CloseEvent (Udp4Service->TimeoutEvent);

  //
  // Destroy the IpIo.
  //
  IpIoDestroy (Udp4Service->IpIo);
}


/**
  This function checks and timeouts the I/O datagrams holding by the corresponding
  service context.

  @param[in]  Event                  The event this function registered to.
  @param[in]  Context                The context data registered during the creation of
                                     the Event.

**/
VOID
EFIAPI
Udp4CheckTimeout (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UDP4_SERVICE_DATA   *Udp4Service;
  LIST_ENTRY          *Entry;
  UDP4_INSTANCE_DATA  *Instance;
  LIST_ENTRY          *WrapEntry;
  LIST_ENTRY          *NextEntry;
  UDP4_RXDATA_WRAP    *Wrap;

  Udp4Service = (UDP4_SERVICE_DATA *) Context;
  NET_CHECK_SIGNATURE (Udp4Service, UDP4_SERVICE_DATA_SIGNATURE);

  NET_LIST_FOR_EACH (Entry, &Udp4Service->ChildrenList) {
    //
    // Iterate all the instances belonging to this service context.
    //
    Instance = NET_LIST_USER_STRUCT (Entry, UDP4_INSTANCE_DATA, Link);
    NET_CHECK_SIGNATURE (Instance, UDP4_INSTANCE_DATA_SIGNATURE);

    if (!Instance->Configured || (Instance->ConfigData.ReceiveTimeout == 0)) {
      //
      // Skip this instance if it's not configured or no receive timeout.
      //
      continue;
    }

    NET_LIST_FOR_EACH_SAFE (WrapEntry, NextEntry, &Instance->RcvdDgramQue) {
      //
      // Iterate all the rxdatas belonging to this udp instance.
      //
      Wrap = NET_LIST_USER_STRUCT (WrapEntry, UDP4_RXDATA_WRAP, Link);

      //
      // TimeoutTick unit is microsecond, MNP_TIMEOUT_CHECK_INTERVAL unit is 100ns.
      //
      if (Wrap->TimeoutTick < (UDP4_TIMEOUT_INTERVAL / 10)) {
        //
        // Remove this RxData if it timeouts.
        //
        Udp4RecycleRxDataWrap (NULL, (VOID *) Wrap);
      } else {
        Wrap->TimeoutTick -= (UDP4_TIMEOUT_INTERVAL / 10);
      }
    }
  }
}


/**
  This function intializes the new created udp instance.

  @param[in]      Udp4Service       Pointer to the UDP4_SERVICE_DATA.
  @param[in, out] Instance          Pointer to the un-initialized UDP4_INSTANCE_DATA.

**/
VOID
Udp4InitInstance (
  IN     UDP4_SERVICE_DATA   *Udp4Service,
  IN OUT UDP4_INSTANCE_DATA  *Instance
  )
{
  //
  // Set the signature.
  //
  Instance->Signature = UDP4_INSTANCE_DATA_SIGNATURE;

  //
  // Init the lists.
  //
  InitializeListHead (&Instance->Link);
  InitializeListHead (&Instance->RcvdDgramQue);
  InitializeListHead (&Instance->DeliveredDgramQue);

  //
  // Init the NET_MAPs.
  //
  NetMapInit (&Instance->TxTokens);
  NetMapInit (&Instance->RxTokens);
  NetMapInit (&Instance->McastIps);

  //
  // Save the pointer to the UDP4_SERVICE_DATA, and initialize other members.
  //
  Instance->Udp4Service = Udp4Service;
  CopyMem (&Instance->Udp4Proto, &mUdp4Protocol, sizeof (Instance->Udp4Proto));
  Instance->IcmpError   = EFI_SUCCESS;
  Instance->Configured  = FALSE;
  Instance->IsNoMapping = FALSE;
  Instance->Destroyed   = FALSE;
}


/**
  This function cleans the udp instance.

  @param[in]  Instance               Pointer to the UDP4_INSTANCE_DATA to clean.

**/
VOID
Udp4CleanInstance (
  IN UDP4_INSTANCE_DATA  *Instance
  )
{
  NetMapClean (&Instance->McastIps);
  NetMapClean (&Instance->RxTokens);
  NetMapClean (&Instance->TxTokens);
}


/**
  This function finds the udp instance by the specified <Address, Port> pair.

  @param[in]  InstanceList           Pointer to the head of the list linking the udp
                                     instances.
  @param[in]  Address                Pointer to the specified IPv4 address.
  @param[in]  Port                   The udp port number.

  @retval TRUE     The specified <Address, Port> pair is found.
  @retval FALSE    Otherwise.

**/
BOOLEAN
Udp4FindInstanceByPort (
  IN LIST_ENTRY        *InstanceList,
  IN EFI_IPv4_ADDRESS  *Address,
  IN UINT16            Port
  )
{
  LIST_ENTRY            *Entry;
  UDP4_INSTANCE_DATA    *Instance;
  EFI_UDP4_CONFIG_DATA  *ConfigData;

  NET_LIST_FOR_EACH (Entry, InstanceList) {
    //
    // Iterate all the udp instances.
    //
    Instance   = NET_LIST_USER_STRUCT (Entry, UDP4_INSTANCE_DATA, Link);
    ConfigData = &Instance->ConfigData;

    if (!Instance->Configured || ConfigData->AcceptAnyPort) {
      //
      // If the instance is not configured or the configdata of the instance indicates
      // this instance accepts any port, skip it.
      //
      continue;
    }

    if (EFI_IP4_EQUAL (&ConfigData->StationAddress, Address) &&
      (ConfigData->StationPort == Port)) {
      //
      // if both the address and the port are the same, return TRUE.
      //
      return TRUE;
    }
  }

  //
  // return FALSE when matching fails.
  //
  return FALSE;
}


/**
  This function tries to bind the udp instance according to the configured port
  allocation strategy.

  @param[in]      InstanceList   Pointer to the head of the list linking the udp
                                 instances.
  @param[in, out] ConfigData     Pointer to the ConfigData of the instance to be
                                 bound. ConfigData->StationPort will be assigned
                                 with an available port value on success.

  @retval EFI_SUCCESS            The bound operation is completed successfully.
  @retval EFI_ACCESS_DENIED      The <Address, Port> specified by the ConfigData is
                                 already used by other instance.
  @retval EFI_OUT_OF_RESOURCES   No available port resources.

**/
EFI_STATUS
Udp4Bind (
  IN     LIST_ENTRY            *InstanceList,
  IN OUT EFI_UDP4_CONFIG_DATA  *ConfigData
  )
{
  EFI_IPv4_ADDRESS  *StationAddress;
  UINT16            StartPort;

  if (ConfigData->AcceptAnyPort) {
    return EFI_SUCCESS;
  }

  StationAddress = &ConfigData->StationAddress;

  if (ConfigData->StationPort != 0) {

    if (!ConfigData->AllowDuplicatePort &&
      Udp4FindInstanceByPort (InstanceList, StationAddress, ConfigData->StationPort)) {
      //
      // Do not allow duplicate port and the port is already used by other instance.
      //
      return EFI_ACCESS_DENIED;
    }
  } else {
    //
    // select a random port for this instance;
    //

    if (ConfigData->AllowDuplicatePort) {
      //
      // Just pick up the random port if the instance allows duplicate port.
      //
      ConfigData->StationPort = mUdp4RandomPort;
    } else {

      StartPort = mUdp4RandomPort;

      while (Udp4FindInstanceByPort(InstanceList, StationAddress, mUdp4RandomPort)) {

        mUdp4RandomPort++;
        if (mUdp4RandomPort == 0) {
          mUdp4RandomPort = UDP4_PORT_KNOWN;
        }

        if (mUdp4RandomPort == StartPort) {
          //
          // No available port.
          //
          return EFI_OUT_OF_RESOURCES;
        }
      }

      ConfigData->StationPort = mUdp4RandomPort;
    }

    mUdp4RandomPort++;
    if (mUdp4RandomPort == 0) {
      mUdp4RandomPort = UDP4_PORT_KNOWN;
    }
  }

  return EFI_SUCCESS;
}


/**
  This function is used to check whether the NewConfigData has any un-reconfigurable
  parameters changed compared to the OldConfigData.

  @param[in]  OldConfigData      Pointer to the current ConfigData the udp instance
                                 uses.
  @param[in]  NewConfigData      Pointer to the new ConfigData.

  @retval TRUE     The instance is reconfigurable.
  @retval FALSE    Otherwise.

**/
BOOLEAN
Udp4IsReconfigurable (
  IN EFI_UDP4_CONFIG_DATA  *OldConfigData,
  IN EFI_UDP4_CONFIG_DATA  *NewConfigData
  )
{
  if ((NewConfigData->AcceptAnyPort      != OldConfigData->AcceptAnyPort)     ||
      (NewConfigData->AcceptBroadcast    != OldConfigData->AcceptBroadcast)   ||
      (NewConfigData->AcceptPromiscuous  != OldConfigData->AcceptPromiscuous) ||
      (NewConfigData->AllowDuplicatePort != OldConfigData->AllowDuplicatePort)
      ) {
    //
    // The receiving filter parameters cannot be changed.
    //
    return FALSE;
  }

  if ((!NewConfigData->AcceptAnyPort) &&
      (NewConfigData->StationPort != OldConfigData->StationPort)
      ) {
    //
    // The port is not changeable.
    //
    return FALSE;
  }

  if (!NewConfigData->AcceptPromiscuous) {

    if (NewConfigData->UseDefaultAddress != OldConfigData->UseDefaultAddress) {
      //
      // The NewConfigData differs to the old one on the UseDefaultAddress.
      //
      return FALSE;
    }

    if (!NewConfigData->UseDefaultAddress &&
        (!EFI_IP4_EQUAL (&NewConfigData->StationAddress, &OldConfigData->StationAddress) ||
         !EFI_IP4_EQUAL (&NewConfigData->SubnetMask, &OldConfigData->SubnetMask))
        ) {
      //
      // If the instance doesn't use the default address, and the new address or
      // new subnet mask is different from the old values.
      //
      return FALSE;
    }
  }

  if (!EFI_IP4_EQUAL (&NewConfigData->RemoteAddress, &OldConfigData->RemoteAddress)) {
    //
    // The remoteaddress is not the same.
    //
    return FALSE;
  }

  if (!EFI_IP4_EQUAL (&NewConfigData->RemoteAddress, &mZeroIp4Addr) &&
      NewConfigData->RemotePort != OldConfigData->RemotePort
      ) {
    //
    // The RemotePort differs if it's designated in the configdata.
    //
    return FALSE;
  }

  //
  // All checks pass, return TRUE.
  //
  return TRUE;
}


/**
  This function builds the Ip4 configdata from the Udp4ConfigData.

  @param[in]       Udp4ConfigData    Pointer to the EFI_UDP4_CONFIG_DATA.
  @param[in, out]  Ip4ConfigData     Pointer to the EFI_IP4_CONFIG_DATA.

**/
VOID
Udp4BuildIp4ConfigData (
  IN     EFI_UDP4_CONFIG_DATA  *Udp4ConfigData,
  IN OUT EFI_IP4_CONFIG_DATA   *Ip4ConfigData
  )
{
  CopyMem (Ip4ConfigData, &mIp4IoDefaultIpConfigData, sizeof (*Ip4ConfigData));

  Ip4ConfigData->DefaultProtocol   = EFI_IP_PROTO_UDP;
  Ip4ConfigData->AcceptBroadcast   = Udp4ConfigData->AcceptBroadcast;
  Ip4ConfigData->AcceptPromiscuous = Udp4ConfigData->AcceptPromiscuous;
  Ip4ConfigData->UseDefaultAddress = Udp4ConfigData->UseDefaultAddress;
  CopyMem (&Ip4ConfigData->StationAddress, &Udp4ConfigData->StationAddress, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Ip4ConfigData->SubnetMask, &Udp4ConfigData->SubnetMask, sizeof (EFI_IPv4_ADDRESS));

  //
  // use the -1 magic number to disable the receiving process of the ip instance.
  //
  Ip4ConfigData->ReceiveTimeout    = (UINT32) (-1);
}


/**
  This function validates the TxToken, it returns the error code according to the spec.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  TxToken            Pointer to the token to be checked.

  @retval EFI_SUCCESS            The TxToken is valid.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE: This is
                                 NULL. Token is NULL. Token.Event is NULL.
                                 Token.Packet.TxData is NULL.
                                 Token.Packet.TxData.FragmentCount is zero.
                                 Token.Packet.TxData.DataLength is not equal to the
                                 sum of fragment lengths. One or more of the
                                 Token.Packet.TxData.FragmentTable[].
                                 FragmentLength fields is zero. One or more of the
                                 Token.Packet.TxData.FragmentTable[].
                                 FragmentBuffer fields is NULL.
                                 Token.Packet.TxData. GatewayAddress is not a
                                 unicast IPv4 address if it is not NULL. One or
                                 more IPv4 addresses in Token.Packet.TxData.
                                 UdpSessionData are not valid unicast IPv4
                                 addresses if the UdpSessionData is not NULL.
  @retval EFI_BAD_BUFFER_SIZE    The data length is greater than the maximum UDP
                                 packet size.

**/
EFI_STATUS
Udp4ValidateTxToken (
  IN UDP4_INSTANCE_DATA         *Instance,
  IN EFI_UDP4_COMPLETION_TOKEN  *TxToken
  )
{
  EFI_UDP4_TRANSMIT_DATA  *TxData;
  UINT32                  Index;
  UINT32                  TotalLen;
  EFI_UDP4_CONFIG_DATA    *ConfigData;
  EFI_UDP4_SESSION_DATA   *UdpSessionData;
  IP4_ADDR                SourceAddress;
  IP4_ADDR                GatewayAddress;

  if (TxToken->Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TxData = TxToken->Packet.TxData;

  if ((TxData == NULL) || (TxData->FragmentCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  TotalLen = 0;
  for (Index = 0; Index < TxData->FragmentCount; Index++) {

    if ((TxData->FragmentTable[Index].FragmentBuffer == NULL) ||
      (TxData->FragmentTable[Index].FragmentLength == 0)) {
      //
      // if the FragmentBuffer is NULL or the FragmentLeng is zero.
      //
      return EFI_INVALID_PARAMETER;
    }

    TotalLen += TxData->FragmentTable[Index].FragmentLength;
  }

  if (TotalLen != TxData->DataLength) {
    //
    // The TotalLen calculated by adding all the FragmentLeng doesn't equal to the
    // DataLength.
    //
    return EFI_INVALID_PARAMETER;
  }

  if (TxData->GatewayAddress != NULL) {
    CopyMem (&GatewayAddress, TxData->GatewayAddress, sizeof (IP4_ADDR));

    if (!NetIp4IsUnicast (NTOHL (GatewayAddress), 0)) {
      //
      // The specified GatewayAddress is not a unicast IPv4 address while it's not 0.
      //
      return EFI_INVALID_PARAMETER;
    }
  }

  ConfigData     = &Instance->ConfigData;
  UdpSessionData = TxData->UdpSessionData;

  if (UdpSessionData != NULL) {

    CopyMem (&SourceAddress, &UdpSessionData->SourceAddress, sizeof (IP4_ADDR));

    if ((SourceAddress != 0) && !NetIp4IsUnicast (HTONL (SourceAddress), 0)) {
      //
      // Check whether SourceAddress is a valid IPv4 address in case it's not zero.
      // The configured station address is used if SourceAddress is zero.
      //
      return EFI_INVALID_PARAMETER;
    }

    if ((UdpSessionData->DestinationPort == 0) && (ConfigData->RemotePort == 0)) {
      //
      // Ambiguous, no avalaible DestinationPort for this token.
      //
      return EFI_INVALID_PARAMETER;
    }

    if (EFI_IP4_EQUAL (&UdpSessionData->DestinationAddress, &mZeroIp4Addr)) {
      //
      // The DestinationAddress specified in the UdpSessionData is 0.
      //
      return EFI_INVALID_PARAMETER;
    }
  } else if (EFI_IP4_EQUAL (&ConfigData->RemoteAddress, &mZeroIp4Addr)) {
    //
    // the configured RemoteAddress is all zero, and the user doens't override the
    // destination address.
    //
    return EFI_INVALID_PARAMETER;
  }

  if (TxData->DataLength > UDP4_MAX_DATA_SIZE) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}


/**
  This function checks whether the specified Token duplicates with the one in the Map.

  @param[in]  Map                Pointer to the NET_MAP.
  @param[in]  Item               Pointer to the NET_MAP_ITEM contain the pointer to
                                 the Token.
  @param[in]  Context            Pointer to the Token to be checked.

  @retval EFI_SUCCESS            The Token specified by Context differs from the
                                 one in the Item.
  @retval EFI_ACCESS_DENIED      The Token duplicates with the one in the Item.

**/
EFI_STATUS
EFIAPI
Udp4TokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  EFI_UDP4_COMPLETION_TOKEN  *Token;
  EFI_UDP4_COMPLETION_TOKEN  *TokenInItem;

  Token       = (EFI_UDP4_COMPLETION_TOKEN*) Context;
  TokenInItem = (EFI_UDP4_COMPLETION_TOKEN*) Item->Key;

  if ((Token == TokenInItem) || (Token->Event == TokenInItem->Event)) {
    //
    // The Token duplicates with the TokenInItem in case either the two pointers are the
    // same or the Events of these two tokens are the same.
    //
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}


/**
  This function calculates the checksum for the Packet, utilizing the pre-calculated
  pseudo HeadSum to reduce some overhead.

  @param[in]  Packet             Pointer to the NET_BUF contains the udp datagram.
  @param[in]  HeadSum            Checksum of the pseudo header execpt the length
                                 field.

  @retval The 16-bit checksum of this udp datagram.

**/
UINT16
Udp4Checksum (
  IN NET_BUF *Packet,
  IN UINT16  HeadSum
  )
{
  UINT16  Checksum;

  Checksum  = NetbufChecksum (Packet);
  Checksum  = NetAddChecksum (Checksum, HeadSum);

  Checksum  = NetAddChecksum (Checksum, HTONS ((UINT16) Packet->TotalSize));

  return (UINT16) ~Checksum;
}


/**
  This function removes the specified Token from the TokenMap.

  @param[in, out] TokenMap       Pointer to the NET_MAP containing the tokens.
  @param[in]      Token          Pointer to the Token to be removed.

  @retval EFI_SUCCESS            The specified Token is removed from the TokenMap.
  @retval EFI_NOT_FOUND          The specified Token is not found in the TokenMap.

**/
EFI_STATUS
Udp4RemoveToken (
  IN OUT NET_MAP                    *TokenMap,
  IN     EFI_UDP4_COMPLETION_TOKEN  *Token
  )
{
  NET_MAP_ITEM  *Item;

  //
  // Find the Token first.
  //
  Item = NetMapFindKey (TokenMap, (VOID *) Token);

  if (Item != NULL) {
    //
    // Remove the token if it's found in the map.
    //
    NetMapRemoveItem (TokenMap, Item, NULL);

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}


/**
  This function is the packet transmitting notify function registered to the IpIo
  interface. It's called to signal the udp TxToken when IpIo layer completes the
  transmitting of the udp datagram.

  @param[in]  Status                 The completion status of the output udp datagram.
  @param[in]  Context                Pointer to the context data.
  @param[in]  Sender                 Specify a pointer of EFI_IP4_PROTOCOL for sending.
  @param[in]  NotifyData             Pointer to the notify data.

**/
VOID
EFIAPI
Udp4DgramSent (
  IN EFI_STATUS        Status,
  IN VOID              *Context,
  IN IP_IO_IP_PROTOCOL Sender,
  IN VOID              *NotifyData
  )
{
  UDP4_INSTANCE_DATA         *Instance;
  EFI_UDP4_COMPLETION_TOKEN  *Token;

  Instance = (UDP4_INSTANCE_DATA *) Context;
  Token    = (EFI_UDP4_COMPLETION_TOKEN *) NotifyData;

  if (Udp4RemoveToken (&Instance->TxTokens, Token) == EFI_SUCCESS) {
    //
    // The token may be cancelled. Only signal it if the remove operation succeeds.
    //
    Token->Status = Status;
    gBS->SignalEvent (Token->Event);
    DispatchDpc ();
  }
}


/**
  This function processes the received datagram passed up by the IpIo layer.

  @param[in]  Status             The status of this udp datagram.
  @param[in]  IcmpError          The IcmpError code, only available when Status is
                                 EFI_ICMP_ERROR.
  @param[in]  NetSession         Pointer to the EFI_NET_SESSION_DATA.
  @param[in]  Packet             Pointer to the NET_BUF containing the received udp
                                 datagram.
  @param[in]  Context            Pointer to the context data.

**/
VOID
EFIAPI
Udp4DgramRcvd (
  IN EFI_STATUS            Status,
  IN UINT8                 IcmpError,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Packet,
  IN VOID                  *Context
  )
{
  NET_CHECK_SIGNATURE (Packet, NET_BUF_SIGNATURE);

  //
  // IpIo only passes received packets with Status EFI_SUCCESS or EFI_ICMP_ERROR.
  //
  if (Status == EFI_SUCCESS) {
    //
    // Demultiplex the received datagram.
    //
    Udp4Demultiplex ((UDP4_SERVICE_DATA *) Context, NetSession, Packet);
  } else {
    //
    // Handle the ICMP_ERROR packet.
    //
    Udp4IcmpHandler ((UDP4_SERVICE_DATA *) Context, IcmpError, NetSession, Packet);
  }

  //
  // Dispatch the DPC queued by the NotifyFunction of the rx token's events
  // which are signaled with received data.
  //
  DispatchDpc ();
}


/**
  This function removes the multicast group specified by Arg from the Map.

  @param[in, out] Map            Pointer to the NET_MAP.
  @param[in]      Item           Pointer to the NET_MAP_ITEM.
  @param[in]      Arg            Pointer to the Arg, it's the pointer to a
                                 multicast IPv4 Address.

  @retval EFI_SUCCESS            The multicast address is removed.
  @retval EFI_ABORTED            The specified multicast address is removed and the
                                 Arg is not NULL.

**/
EFI_STATUS
EFIAPI
Udp4LeaveGroup (
  IN OUT NET_MAP       *Map,
  IN     NET_MAP_ITEM  *Item,
  IN     VOID          *Arg OPTIONAL
  )
{
  EFI_IPv4_ADDRESS  *McastIp;

  McastIp = Arg;

  if ((McastIp != NULL) && (!EFI_IP4_EQUAL (McastIp, &(Item->Key)))) {
    //
    // McastIp is not NULL and the multicast address contained in the Item
    // is not the same as McastIp.
    //
    return EFI_SUCCESS;
  }

  //
  // Remove this Item.
  //
  NetMapRemoveItem (Map, Item, NULL);

  if (McastIp != NULL) {
    //
    // Return EFI_ABORTED in case McastIp is not NULL to terminate the iteration.
    //
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}


/**
  This function cancels the token specified by Arg in the Map. This is a callback
  used by Udp4InstanceCancelToken().

  @param[in]  Map                Pointer to the NET_MAP.
  @param[in]  Item               Pointer to the NET_MAP_ITEM.
  @param[in]  Arg                Pointer to the token to be cancelled, if NULL,
                                 the token specified by Item is cancelled.

  @retval EFI_SUCCESS            The token is cancelled if Arg is NULL or the token
                                 is not the same as that in the Item if Arg is not
                                 NULL.
  @retval EFI_ABORTED            Arg is not NULL, and the token specified by Arg is
                                 cancelled.

**/
EFI_STATUS
EFIAPI
Udp4CancelTokens (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg OPTIONAL
  )
{
  EFI_UDP4_COMPLETION_TOKEN  *TokenToCancel;
  NET_BUF                    *Packet;
  IP_IO                      *IpIo;

  if ((Arg != NULL) && (Item->Key != Arg)) {
    return EFI_SUCCESS;
  }

  if (Item->Value != NULL) {
    //
    // If the token is a transmit token, the corresponding Packet is recorded in
    // Item->Value, invoke IpIo to cancel this packet first. The IpIoCancelTxToken
    // will invoke Udp4DgramSent, the token will be signaled and this Item will
    // be removed from the Map there.
    //
    Packet = (NET_BUF *) (Item->Value);
    IpIo   = (IP_IO *) (*((UINTN *) &Packet->ProtoData[0]));

    IpIoCancelTxToken (IpIo, Packet);
  } else {
    //
    // The token is a receive token. Abort it and remove it from the Map.
    //
    TokenToCancel = (EFI_UDP4_COMPLETION_TOKEN *) Item->Key;
    NetMapRemoveItem (Map, Item, NULL);

    TokenToCancel->Status = EFI_ABORTED;
    gBS->SignalEvent (TokenToCancel->Event);
  }

  if (Arg != NULL) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}


/**
  This function removes all the Wrap datas in the RcvdDgramQue.

  @param[in]  Instance           Pointer to the udp instance context data.

**/
VOID
Udp4FlushRcvdDgram (
  IN UDP4_INSTANCE_DATA  *Instance
  )
{
  UDP4_RXDATA_WRAP  *Wrap;

  while (!IsListEmpty (&Instance->RcvdDgramQue)) {
    //
    // Iterate all the Wraps in the RcvdDgramQue.
    //
    Wrap = NET_LIST_HEAD (&Instance->RcvdDgramQue, UDP4_RXDATA_WRAP, Link);

    //
    // The Wrap will be removed from the RcvdDgramQue by this function call.
    //
    Udp4RecycleRxDataWrap (NULL, (VOID *) Wrap);
  }
}



/**
  Cancel Udp4 tokens from the Udp4 instance.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  Token              Pointer to the token to be canceled, if NULL, all
                                 tokens in this instance will be cancelled.

  @retval EFI_SUCCESS            The Token is cancelled.
  @retval EFI_NOT_FOUND          The Token is not found.

**/
EFI_STATUS
Udp4InstanceCancelToken (
  IN UDP4_INSTANCE_DATA         *Instance,
  IN EFI_UDP4_COMPLETION_TOKEN  *Token OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // Cancel this token from the TxTokens map.
  //
  Status = NetMapIterate (&Instance->TxTokens, Udp4CancelTokens, Token);

  if ((Token != NULL) && (Status == EFI_ABORTED)) {
    //
    // If Token isn't NULL and Status is EFI_ABORTED, the token is cancelled from
    // the TxTokens, just return success.
    //
    return EFI_SUCCESS;
  }

  //
  // Try to cancel this token from the RxTokens map in condition either the Token
  // is NULL or the specified Token is not in TxTokens.
  //
  Status = NetMapIterate (&Instance->RxTokens, Udp4CancelTokens, Token);

  if ((Token != NULL) && (Status == EFI_SUCCESS)) {
    //
    // If Token isn't NULL and Status is EFI_SUCCESS, the token is neither in the
    // TxTokens nor the RxTokens, or say, it's not found.
    //
    return EFI_NOT_FOUND;
  }

  ASSERT ((Token != NULL) || ((0 == NetMapGetCount (&Instance->TxTokens))
    && (0 == NetMapGetCount (&Instance->RxTokens))));

  return EFI_SUCCESS;
}


/**
  This function matches the received udp datagram with the Instance.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  Udp4Session        Pointer to the EFI_UDP4_SESSION_DATA abstracted
                                 from the received udp datagram.

  @retval TRUE       The udp datagram matches the receiving requirments of the
                     udp Instance.
  @retval FALSE      Otherwise.

**/
BOOLEAN
Udp4MatchDgram (
  IN UDP4_INSTANCE_DATA     *Instance,
  IN EFI_UDP4_SESSION_DATA  *Udp4Session
  )
{
  EFI_UDP4_CONFIG_DATA  *ConfigData;
  IP4_ADDR              Destination;

  ConfigData = &Instance->ConfigData;

  if (ConfigData->AcceptPromiscuous) {
    //
    // Always matches if this instance is in the promiscuous state.
    //
    return TRUE;
  }

  if ((!ConfigData->AcceptAnyPort && (Udp4Session->DestinationPort != ConfigData->StationPort)) ||
      ((ConfigData->RemotePort != 0) && (Udp4Session->SourcePort != ConfigData->RemotePort))
      ) {
    //
    // The local port or the remote port doesn't match.
    //
    return FALSE;
  }

  if (!EFI_IP4_EQUAL (&ConfigData->RemoteAddress, &mZeroIp4Addr) &&
      !EFI_IP4_EQUAL (&ConfigData->RemoteAddress, &Udp4Session->SourceAddress)
      ) {
    //
    // This datagram doesn't come from the instance's specified sender.
    //
    return FALSE;
  }

  if (EFI_IP4_EQUAL (&ConfigData->StationAddress, &mZeroIp4Addr) ||
      EFI_IP4_EQUAL (&Udp4Session->DestinationAddress, &ConfigData->StationAddress)
      ) {
    //
    // The instance is configured to receive datagrams destined to any station IP or
    // the destination address of this datagram matches the configured station IP.
    //
    return TRUE;
  }

  CopyMem (&Destination, &Udp4Session->DestinationAddress, sizeof (IP4_ADDR));

  if (IP4_IS_LOCAL_BROADCAST (Destination) && ConfigData->AcceptBroadcast) {
    //
    // The instance is configured to receive broadcast and this is a broadcast packet.
    //
    return TRUE;
  }

  if (IP4_IS_MULTICAST (NTOHL (Destination)) &&
      NetMapFindKey (&Instance->McastIps, (VOID *) (UINTN) Destination) != NULL
      ) {
    //
    // It's a multicast packet and the multicast address is accepted by this instance.
    //
    return TRUE;
  }

  return FALSE;
}


/**
  This function removes the Wrap specified by Context and release relevant resources.

  @param[in]  Event              The Event this notify function registered to.
  @param[in]  Context            Pointer to the context data.

**/
VOID
EFIAPI
Udp4RecycleRxDataWrap (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UDP4_RXDATA_WRAP  *Wrap;

  Wrap = (UDP4_RXDATA_WRAP *) Context;

  //
  // Remove the Wrap from the list it belongs to.
  //
  RemoveEntryList (&Wrap->Link);

  //
  // Free the Packet associated with this Wrap.
  //
  NetbufFree (Wrap->Packet);

  //
  // Close the event.
  //
  gBS->CloseEvent (Wrap->RxData.RecycleSignal);

  FreePool (Wrap);
}


/**
  This function wraps the Packet and the RxData.

  @param[in]  Instance               Pointer to the instance context data.
  @param[in]  Packet                 Pointer to the buffer containing the received
                                     datagram.
  @param[in]  RxData                 Pointer to the EFI_UDP4_RECEIVE_DATA of this
                                     datagram.

  @return Pointer to the structure wrapping the RxData and the Packet.

**/
UDP4_RXDATA_WRAP *
Udp4WrapRxData (
  IN UDP4_INSTANCE_DATA     *Instance,
  IN NET_BUF                *Packet,
  IN EFI_UDP4_RECEIVE_DATA  *RxData
  )
{
  EFI_STATUS            Status;
  UDP4_RXDATA_WRAP      *Wrap;

  //
  // Allocate buffer for the Wrap.
  //
  Wrap = AllocatePool (sizeof (UDP4_RXDATA_WRAP) +
         (Packet->BlockOpNum - 1) * sizeof (EFI_UDP4_FRAGMENT_DATA));
  if (Wrap == NULL) {
    return NULL;
  }

  InitializeListHead (&Wrap->Link);

  CopyMem (&Wrap->RxData, RxData, sizeof (Wrap->RxData));

  //
  // Create the Recycle event.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Udp4RecycleRxDataWrap,
                  Wrap,
                  &Wrap->RxData.RecycleSignal
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Wrap);
    return NULL;
  }

  Wrap->Packet      = Packet;
  Wrap->TimeoutTick = Instance->ConfigData.ReceiveTimeout;

  return Wrap;
}


/**
  This function enqueues the received datagram into the instances' receiving queues.

  @param[in]  Udp4Service            Pointer to the udp service context data.
  @param[in]  Packet                 Pointer to the buffer containing the received
                                     datagram.
  @param[in]  RxData                 Pointer to the EFI_UDP4_RECEIVE_DATA of this
                                     datagram.

  @return The times this datagram is enqueued.

**/
UINTN
Udp4EnqueueDgram (
  IN UDP4_SERVICE_DATA      *Udp4Service,
  IN NET_BUF                *Packet,
  IN EFI_UDP4_RECEIVE_DATA  *RxData
  )
{
  LIST_ENTRY          *Entry;
  UDP4_INSTANCE_DATA  *Instance;
  UDP4_RXDATA_WRAP    *Wrap;
  UINTN               Enqueued;

  Enqueued = 0;

  NET_LIST_FOR_EACH (Entry, &Udp4Service->ChildrenList) {
    //
    // Iterate the instances.
    //
    Instance = NET_LIST_USER_STRUCT (Entry, UDP4_INSTANCE_DATA, Link);

    if (!Instance->Configured) {
      continue;
    }

    if (Udp4MatchDgram (Instance, &RxData->UdpSession)) {
      //
      // Wrap the RxData and put this Wrap into the instances RcvdDgramQue.
      //
      Wrap = Udp4WrapRxData (Instance, Packet, RxData);
      if (Wrap == NULL) {
        continue;
      }

      NET_GET_REF (Packet);

      InsertTailList (&Instance->RcvdDgramQue, &Wrap->Link);

      Enqueued++;
    }
  }

  return Enqueued;
}


/**
  This function delivers the received datagrams for the specified instance.

  @param[in]  Instance               Pointer to the instance context data.

**/
VOID
Udp4InstanceDeliverDgram (
  IN UDP4_INSTANCE_DATA  *Instance
  )
{
  UDP4_RXDATA_WRAP           *Wrap;
  EFI_UDP4_COMPLETION_TOKEN  *Token;
  NET_BUF                    *Dup;
  EFI_UDP4_RECEIVE_DATA      *RxData;
  EFI_TPL                    OldTpl;

  if (!IsListEmpty (&Instance->RcvdDgramQue) &&
      !NetMapIsEmpty (&Instance->RxTokens)) {

    Wrap = NET_LIST_HEAD (&Instance->RcvdDgramQue, UDP4_RXDATA_WRAP, Link);

    if (NET_BUF_SHARED (Wrap->Packet)) {
      //
      // Duplicate the Packet if it is shared between instances.
      //
      Dup = NetbufDuplicate (Wrap->Packet, NULL, 0);
      if (Dup == NULL) {
        return;
      }

      NetbufFree (Wrap->Packet);

      Wrap->Packet = Dup;
    }

    NetListRemoveHead (&Instance->RcvdDgramQue);

    Token = (EFI_UDP4_COMPLETION_TOKEN *) NetMapRemoveHead (&Instance->RxTokens, NULL);

    //
    // Build the FragmentTable and set the FragmentCount in RxData.
    //
    RxData                = &Wrap->RxData;
    RxData->FragmentCount = Wrap->Packet->BlockOpNum;

    NetbufBuildExt (
      Wrap->Packet,
      (NET_FRAGMENT *) RxData->FragmentTable,
      &RxData->FragmentCount
      );

    Token->Status        = EFI_SUCCESS;
    Token->Packet.RxData = &Wrap->RxData;

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    InsertTailList (&Instance->DeliveredDgramQue, &Wrap->Link);
    gBS->RestoreTPL (OldTpl);

    gBS->SignalEvent (Token->Event);
  }
}


/**
  This function delivers the datagrams enqueued in the instances.

  @param[in]  Udp4Service            Pointer to the udp service context data.

**/
VOID
Udp4DeliverDgram (
  IN UDP4_SERVICE_DATA  *Udp4Service
  )
{
  LIST_ENTRY          *Entry;
  UDP4_INSTANCE_DATA  *Instance;

  NET_LIST_FOR_EACH (Entry, &Udp4Service->ChildrenList) {
    //
    // Iterate the instances.
    //
    Instance = NET_LIST_USER_STRUCT (Entry, UDP4_INSTANCE_DATA, Link);

    if (!Instance->Configured) {
      continue;
    }

    //
    // Deliver the datagrams of this instance.
    //
    Udp4InstanceDeliverDgram (Instance);
  }
}


/**
  This function demultiplexes the received udp datagram to the apropriate instances.

  @param[in]  Udp4Service            Pointer to the udp service context data.
  @param[in]  NetSession             Pointer to the EFI_NET_SESSION_DATA abstrated from
                                     the received datagram.
  @param[in]  Packet                 Pointer to the buffer containing the received udp
                                     datagram.

**/
VOID
Udp4Demultiplex (
  IN UDP4_SERVICE_DATA     *Udp4Service,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Packet
  )
{
  EFI_UDP_HEADER        *Udp4Header;
  UINT16                 HeadSum;
  EFI_UDP4_RECEIVE_DATA  RxData;
  EFI_UDP4_SESSION_DATA  *Udp4Session;
  UINTN                  Enqueued;

  //
  // Get the datagram header from the packet buffer.
  //
  Udp4Header = (EFI_UDP_HEADER *) NetbufGetByte (Packet, 0, NULL);
  ASSERT (Udp4Header != NULL);

  if (Udp4Header->Checksum != 0) {
    //
    // check the checksum.
    //
    HeadSum = NetPseudoHeadChecksum (
                NetSession->Source.Addr[0],
                NetSession->Dest.Addr[0],
                EFI_IP_PROTO_UDP,
                0
                );

    if (Udp4Checksum (Packet, HeadSum) != 0) {
      //
      // Wrong checksum.
      //
      return;
    }
  }

  gRT->GetTime (&RxData.TimeStamp, NULL);

  Udp4Session                  = &RxData.UdpSession;
  Udp4Session->SourcePort      = NTOHS (Udp4Header->SrcPort);
  Udp4Session->DestinationPort = NTOHS (Udp4Header->DstPort);

  CopyMem (&Udp4Session->SourceAddress, &NetSession->Source, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Udp4Session->DestinationAddress, &NetSession->Dest, sizeof (EFI_IPv4_ADDRESS));

  //
  // Trim the UDP header.
  //
  NetbufTrim (Packet, UDP4_HEADER_SIZE, TRUE);

  RxData.DataLength = (UINT32) Packet->TotalSize;

  //
  // Try to enqueue this datagram into the instances.
  //
  Enqueued = Udp4EnqueueDgram (Udp4Service, Packet, &RxData);

  if (Enqueued == 0) {
    //
    // Send the port unreachable ICMP packet before we free this NET_BUF
    //
    Udp4SendPortUnreach (Udp4Service->IpIo, NetSession, Udp4Header);
  }

  //
  // Try to free the packet before deliver it.
  //
  NetbufFree (Packet);

  if (Enqueued > 0) {
    //
    // Deliver the datagram.
    //
    Udp4DeliverDgram (Udp4Service);
  }
}


/**
  This function builds and sends out a icmp port unreachable message.

  @param[in]  IpIo                   Pointer to the IP_IO instance.
  @param[in]  NetSession             Pointer to the EFI_NET_SESSION_DATA of the packet
                                     causes this icmp error message.
  @param[in]  Udp4Header             Pointer to the udp header of the datagram causes
                                     this icmp error message.

**/
VOID
Udp4SendPortUnreach (
  IN IP_IO                 *IpIo,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN VOID                  *Udp4Header
  )
{
  NET_BUF              *Packet;
  UINT32               Len;
  IP4_ICMP_ERROR_HEAD  *IcmpErrHdr;
  EFI_IP4_HEADER       *IpHdr;
  UINT8                *Ptr;
  IP_IO_OVERRIDE       Override;
  IP_IO_IP_INFO        *IpSender;

  IpSender = IpIoFindSender (&IpIo, NetSession->IpVersion, &NetSession->Dest);
  if (IpSender == NULL) {
    //
    // No apropriate sender, since we cannot send out the ICMP message through
    // the default zero station address IP instance, abort.
    //
    return;
  }

  IpHdr = NetSession->IpHdr.Ip4Hdr;

  //
  // Calculate the requried length of the icmp error message.
  //
  Len = sizeof (IP4_ICMP_ERROR_HEAD) + (EFI_IP4_HEADER_LEN (IpHdr) -
        sizeof (IP4_HEAD)) + ICMP_ERROR_PACKET_LENGTH;

  //
  // Allocate buffer for the icmp error message.
  //
  Packet = NetbufAlloc (Len);
  if (Packet == NULL) {
    return;
  }

  //
  // Allocate space for the IP4_ICMP_ERROR_HEAD.
  //
  IcmpErrHdr = (IP4_ICMP_ERROR_HEAD *) NetbufAllocSpace (Packet, Len, FALSE);
  ASSERT (IcmpErrHdr != NULL);

  //
  // Set the required fields for the icmp port unreachable message.
  //
  IcmpErrHdr->Head.Type     = ICMP_TYPE_UNREACH;
  IcmpErrHdr->Head.Code     = ICMP_CODE_UNREACH_PORT;
  IcmpErrHdr->Head.Checksum = 0;
  IcmpErrHdr->Fourth        = 0;

  //
  // Copy the IP header of the datagram tragged the error.
  //
  CopyMem (&IcmpErrHdr->IpHead, IpHdr, EFI_IP4_HEADER_LEN (IpHdr));

  //
  // Copy the UDP header.
  //
  Ptr = (UINT8 *) &IcmpErrHdr->IpHead + EFI_IP4_HEADER_LEN (IpHdr);
  CopyMem (Ptr, Udp4Header, ICMP_ERROR_PACKET_LENGTH);

  //
  // Calculate the checksum.
  //
  IcmpErrHdr->Head.Checksum = (UINT16) ~(NetbufChecksum (Packet));

  //
  // Fill the override data.
  //
  Override.Ip4OverrideData.DoNotFragment = FALSE;
  Override.Ip4OverrideData.TypeOfService = 0;
  Override.Ip4OverrideData.TimeToLive    = 255;
  Override.Ip4OverrideData.Protocol      = EFI_IP_PROTO_ICMP;

  CopyMem (&Override.Ip4OverrideData.SourceAddress, &NetSession->Dest, sizeof (EFI_IPv4_ADDRESS));
  ZeroMem (&Override.Ip4OverrideData.GatewayAddress, sizeof (EFI_IPv4_ADDRESS));

  //
  // Send out this icmp packet.
  //
  IpIoSend (IpIo, Packet, IpSender, NULL, NULL, &NetSession->Source, &Override);

  NetbufFree (Packet);
}


/**
  This function handles the received Icmp Error message and demultiplexes it to the
  instance.

  @param[in]  Udp4Service            Pointer to the udp service context data.
  @param[in]  IcmpError              The icmp error code.
  @param[in]  NetSession             Pointer to the EFI_NET_SESSION_DATA abstracted
                                 from the received Icmp Error packet.
  @param[in]  Packet                 Pointer to the Icmp Error packet.

**/
VOID
Udp4IcmpHandler (
  IN UDP4_SERVICE_DATA     *Udp4Service,
  IN UINT8                 IcmpError,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Packet
  )
{
  EFI_UDP_HEADER        *Udp4Header;
  EFI_UDP4_SESSION_DATA  Udp4Session;
  LIST_ENTRY             *Entry;
  UDP4_INSTANCE_DATA     *Instance;

  Udp4Header = (EFI_UDP_HEADER *) NetbufGetByte (Packet, 0, NULL);
  ASSERT (Udp4Header != NULL);

  CopyMem (&Udp4Session.SourceAddress, &NetSession->Source, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Udp4Session.DestinationAddress, &NetSession->Dest, sizeof (EFI_IPv4_ADDRESS));

  Udp4Session.SourcePort      = NTOHS (Udp4Header->DstPort);
  Udp4Session.DestinationPort = NTOHS (Udp4Header->SrcPort);

  NET_LIST_FOR_EACH (Entry, &Udp4Service->ChildrenList) {
    //
    // Iterate all the instances.
    //
    Instance = NET_LIST_USER_STRUCT (Entry, UDP4_INSTANCE_DATA, Link);

    if (!Instance->Configured) {
      continue;
    }

    if (Udp4MatchDgram (Instance, &Udp4Session)) {
      //
      // Translate the Icmp Error code according to the udp spec.
      //
      Instance->IcmpError = IpIoGetIcmpErrStatus (IcmpError, IP_VERSION_4, NULL, NULL);

      if (IcmpError > ICMP_ERR_UNREACH_PORT) {
        Instance->IcmpError = EFI_ICMP_ERROR;
      }

      //
      // Notify the instance with the received Icmp Error.
      //
      Udp4ReportIcmpError (Instance);

      break;
    }
  }

  NetbufFree (Packet);
}


/**
  This function reports the received ICMP error.

  @param[in]  Instance               Pointer to the udp instance context data.

**/
VOID
Udp4ReportIcmpError (
  IN UDP4_INSTANCE_DATA  *Instance
  )
{
  EFI_UDP4_COMPLETION_TOKEN  *Token;

  if (NetMapIsEmpty (&Instance->RxTokens)) {
    //
    // There are no receive tokens to deliver the ICMP error.
    //
    return;
  }

  if (EFI_ERROR (Instance->IcmpError)) {
    //
    // Try to get a RxToken from the RxTokens map.
    //
    Token = (EFI_UDP4_COMPLETION_TOKEN *) NetMapRemoveHead (&Instance->RxTokens, NULL);

    if (Token != NULL) {
      //
      // Report the error through the Token.
      //
      Token->Status = Instance->IcmpError;
      gBS->SignalEvent (Token->Event);

      //
      // Clear the IcmpError.
      //
      Instance->IcmpError = EFI_SUCCESS;
    }
  }
}


/**
  This function is a dummy ext-free function for the NET_BUF created for the output
  udp datagram.

  @param[in]  Context                Pointer to the context data.

**/
VOID
EFIAPI
Udp4NetVectorExtFree (
  VOID  *Context
  )
{
}


/**
  Set the Udp4 variable data.

  @param[in] Udp4Service         Udp4 service data.

  @retval EFI_OUT_OF_RESOURCES   There are not enough resources to set the
                                 variable.
  @retval EFI_SUCCESS            Set variable successfully.
  @retval other                  Set variable failed.

**/
EFI_STATUS
Udp4SetVariableData (
  IN UDP4_SERVICE_DATA  *Udp4Service
  )
{
  UINT32                  NumConfiguredInstance;
  LIST_ENTRY              *Entry;
  UINTN                   VariableDataSize;
  EFI_UDP4_VARIABLE_DATA  *Udp4VariableData;
  EFI_UDP4_SERVICE_POINT  *Udp4ServicePoint;
  UDP4_INSTANCE_DATA      *Udp4Instance;
  CHAR16                  *NewMacString;
  EFI_STATUS              Status;

  NumConfiguredInstance = 0;

  //
  // Go through the children list to count the configured children.
  //
  NET_LIST_FOR_EACH (Entry, &Udp4Service->ChildrenList) {
    Udp4Instance = NET_LIST_USER_STRUCT_S (
                     Entry,
                     UDP4_INSTANCE_DATA,
                     Link,
                     UDP4_INSTANCE_DATA_SIGNATURE
                     );

    if (Udp4Instance->Configured) {
      NumConfiguredInstance++;
    }
  }

  //
  // Calculate the size of the Udp4VariableData. As there may be no Udp4 child,
  // we should add extra buffer for the service points only if the number of configured
  // children is more than 1.
  //
  VariableDataSize = sizeof (EFI_UDP4_VARIABLE_DATA);

  if (NumConfiguredInstance > 1) {
    VariableDataSize += sizeof (EFI_UDP4_SERVICE_POINT) * (NumConfiguredInstance - 1);
  }

  Udp4VariableData = AllocatePool (VariableDataSize);
  if (Udp4VariableData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Udp4VariableData->DriverHandle = Udp4Service->ImageHandle;
  Udp4VariableData->ServiceCount = NumConfiguredInstance;

  Udp4ServicePoint = &Udp4VariableData->Services[0];

  //
  // Go through the children list to fill the configured children's address pairs.
  //
  NET_LIST_FOR_EACH (Entry, &Udp4Service->ChildrenList) {
    Udp4Instance = NET_LIST_USER_STRUCT_S (
                     Entry,
                     UDP4_INSTANCE_DATA,
                     Link,
                     UDP4_INSTANCE_DATA_SIGNATURE
                     );

    if (Udp4Instance->Configured) {
      Udp4ServicePoint->InstanceHandle = Udp4Instance->ChildHandle;
      Udp4ServicePoint->LocalAddress   = Udp4Instance->ConfigData.StationAddress;
      Udp4ServicePoint->LocalPort      = Udp4Instance->ConfigData.StationPort;
      Udp4ServicePoint->RemoteAddress  = Udp4Instance->ConfigData.RemoteAddress;
      Udp4ServicePoint->RemotePort     = Udp4Instance->ConfigData.RemotePort;

      Udp4ServicePoint++;
    }
  }

  //
  // Get the mac string.
  //
  Status = NetLibGetMacString (
             Udp4Service->ControllerHandle,
             Udp4Service->ImageHandle,
             &NewMacString
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  if (Udp4Service->MacString != NULL) {
    //
    // The variable is set already, we're going to update it.
    //
    if (StrCmp (Udp4Service->MacString, NewMacString) != 0) {
      //
      // The mac address is changed, delete the previous variable first.
      //
      gRT->SetVariable (
             Udp4Service->MacString,
             &gEfiUdp4ServiceBindingProtocolGuid,
             EFI_VARIABLE_BOOTSERVICE_ACCESS,
             0,
             NULL
             );
    }

    FreePool (Udp4Service->MacString);
  }

  Udp4Service->MacString = NewMacString;

  Status = gRT->SetVariable (
                  Udp4Service->MacString,
                  &gEfiUdp4ServiceBindingProtocolGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  VariableDataSize,
                  (VOID *) Udp4VariableData
                  );

ON_ERROR:

  FreePool (Udp4VariableData);

  return Status;
}


/**
  Clear the variable and free the resource.

  @param[[in]  Udp4Service            Udp4 service data.

**/
VOID
Udp4ClearVariableData (
  IN UDP4_SERVICE_DATA  *Udp4Service
  )
{
  ASSERT (Udp4Service->MacString != NULL);

  gRT->SetVariable (
         Udp4Service->MacString,
         &gEfiUdp4ServiceBindingProtocolGuid,
         EFI_VARIABLE_BOOTSERVICE_ACCESS,
         0,
         NULL
         );

  FreePool (Udp4Service->MacString);
  Udp4Service->MacString = NULL;
}
