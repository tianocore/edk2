/** @file
  Udp6 driver's whole implementation.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Udp6Impl.h"

UINT16  mUdp6RandomPort;

/**
  This function checks and timeouts the I/O datagrams holding by the corresponding
  service context.

  @param[in]  Event              The event this function is registered to.
  @param[in]  Context            The context data registered during the creation of
                                 the Event.

**/
VOID
EFIAPI
Udp6CheckTimeout (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  This function finds the udp instance by the specified <Address, Port> pair.

  @param[in]  InstanceList       Pointer to the head of the list linking the udp
                                 instances.
  @param[in]  Address            Pointer to the specified IPv6 address.
  @param[in]  Port               The udp port number.

  @retval TRUE     The specified <Address, Port> pair is found.
  @retval FALSE    Otherwise.

**/
BOOLEAN
Udp6FindInstanceByPort (
  IN LIST_ENTRY        *InstanceList,
  IN EFI_IPv6_ADDRESS  *Address,
  IN UINT16            Port
  );

/**
  This function is the packet transmitting notify function registered to the IpIo
  interface. It's called to signal the udp TxToken when the IpIo layer completes
  transmitting of the udp datagram.

  @param[in]  Status            The completion status of the output udp datagram.
  @param[in]  Context           Pointer to the context data.
  @param[in]  Sender            Specify a EFI_IP6_PROTOCOL for sending.
  @param[in]  NotifyData        Pointer to the notify data.

**/
VOID
EFIAPI
Udp6DgramSent (
  IN EFI_STATUS        Status,
  IN VOID              *Context,
  IN IP_IO_IP_PROTOCOL Sender,
  IN VOID              *NotifyData
  );

/**
  This function processes the received datagram passed up by the IpIo layer.

  @param[in]  Status            The status of this udp datagram.
  @param[in]  IcmpError         The IcmpError code, only available when Status is
                                EFI_ICMP_ERROR.
  @param[in]  NetSession        Pointer to the EFI_NET_SESSION_DATA.
  @param[in]  Packet            Pointer to the NET_BUF containing the received udp
                                datagram.
  @param[in]  Context           Pointer to the context data.

**/
VOID
EFIAPI
Udp6DgramRcvd (
  IN EFI_STATUS            Status,
  IN UINT8                 IcmpError,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Packet,
  IN VOID                  *Context
  );

/**
  This function cancle the token specified by Arg in the Map.

  @param[in]  Map             Pointer to the NET_MAP.
  @param[in]  Item            Pointer to the NET_MAP_ITEM.
  @param[in]  Arg             Pointer to the token to be cancelled, if NULL, all
                              the tokens in this Map will be cancelled.
                              This parameter is optional and may be NULL.

  @retval EFI_SUCCESS         The token is cancelled if Arg is NULL or the token
                              is not the same as that in the Item if Arg is not
                              NULL.
  @retval EFI_ABORTED         Arg is not NULL, and the token specified by Arg is
                              cancelled.

**/
EFI_STATUS
EFIAPI
Udp6CancelTokens (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg OPTIONAL
  );

/**
  This function check if the received udp datagram matches with the Instance.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  Udp6Session        Pointer to the EFI_UDP6_SESSION_DATA abstracted
                                 from the received udp datagram.

  @retval TRUE     The udp datagram matches the receiving requirements of the Instance.
  @retval FALSE    The udp datagram doe not match the receiving requirements of the Instance.

**/
BOOLEAN
Udp6MatchDgram (
  IN UDP6_INSTANCE_DATA     *Instance,
  IN EFI_UDP6_SESSION_DATA  *Udp6Session
  );

/**
  This function removes the Wrap specified by Context and releases relevant resources.

  @param[in]  Event                  The Event this notify function is registered to.
  @param[in]  Context                Pointer to the context data.

**/
VOID
EFIAPI
Udp6RecycleRxDataWrap (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  This function wraps the Packet into RxData.

  @param[in]  Instance           Pointer to the instance context data.
  @param[in]  Packet             Pointer to the buffer containing the received
                                 datagram.
  @param[in]  RxData             Pointer to the EFI_UDP6_RECEIVE_DATA of this
                                 datagram.

  @return Pointer to the structure wrapping the RxData and the Packet.

**/
UDP6_RXDATA_WRAP *
Udp6WrapRxData (
  IN UDP6_INSTANCE_DATA     *Instance,
  IN NET_BUF                *Packet,
  IN EFI_UDP6_RECEIVE_DATA  *RxData
  );

/**
  This function enqueues the received datagram into the instances' receiving queues.

  @param[in]  Udp6Service        Pointer to the udp service context data.
  @param[in]  Packet             Pointer to the buffer containing the received
                                 datagram.
  @param[in]  RxData             Pointer to the EFI_UDP6_RECEIVE_DATA of this
                                 datagram.

  @return The times this datagram is enqueued.

**/
UINTN
Udp6EnqueueDgram (
  IN UDP6_SERVICE_DATA      *Udp6Service,
  IN NET_BUF                *Packet,
  IN EFI_UDP6_RECEIVE_DATA  *RxData
  );

/**
  This function delivers the datagrams enqueued in the instances.

  @param[in]  Udp6Service            Pointer to the udp service context data.

**/
VOID
Udp6DeliverDgram (
  IN UDP6_SERVICE_DATA  *Udp6Service
  );

/**
  This function demultiplexes the received udp datagram to the apropriate instances.

  @param[in]  Udp6Service        Pointer to the udp service context data.
  @param[in]  NetSession         Pointer to the EFI_NET_SESSION_DATA abstrated from
                                 the received datagram.
  @param[in]  Packet             Pointer to the buffer containing the received udp
                                 datagram.

**/
VOID
Udp6Demultiplex (
  IN UDP6_SERVICE_DATA     *Udp6Service,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Packet
  );

/**
  This function handles the received Icmp Error message and demultiplexes it to the
  instance.

  @param[in]       Udp6Service        Pointer to the udp service context data.
  @param[in]       IcmpError          The icmp error code.
  @param[in]       NetSession         Pointer to the EFI_NET_SESSION_DATA abstracted
                                      from the received Icmp Error packet.
  @param[in, out]  Packet             Pointer to the Icmp Error packet.

**/
VOID
Udp6IcmpHandler (
  IN UDP6_SERVICE_DATA     *Udp6Service,
  IN UINT8                 IcmpError,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN OUT NET_BUF           *Packet
  );

/**
  This function builds and sends out a icmp port unreachable message.

  @param[in]  IpIo               Pointer to the IP_IO instance.
  @param[in]  NetSession         Pointer to the EFI_NET_SESSION_DATA of the packet
                                 causes this icmp error message.
  @param[in]  Udp6Header         Pointer to the udp header of the datagram causes
                                 this icmp error message.

**/
VOID
Udp6SendPortUnreach (
  IN IP_IO                 *IpIo,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN VOID                  *Udp6Header
  );

/**
  Find the key in the netmap

  @param[in]  Map                    The netmap to search within.
  @param[in]  Key                    The key to search.

  @return The point to the item contains the Key, or NULL if Key isn't in the map.

**/
NET_MAP_ITEM *
Udp6MapMultiCastAddr (
  IN  NET_MAP               *Map,
  IN  VOID                  *Key
  );

/**
  Create the Udp service context data.

  @param[in]  Udp6Service        Pointer to the UDP6_SERVICE_DATA.
  @param[in]  ImageHandle        The image handle of this udp6 driver.
  @param[in]  ControllerHandle   The controller handle this udp6 driver binds on.

  @retval EFI_SUCCESS            The udp6 service context data was created and
                                 initialized.
  @retval EFI_OUT_OF_RESOURCES   Cannot allocate memory.
  @retval Others                 An error condition occurred.

**/
EFI_STATUS
Udp6CreateService (
  IN UDP6_SERVICE_DATA  *Udp6Service,
  IN EFI_HANDLE         ImageHandle,
  IN EFI_HANDLE         ControllerHandle
  )
{
  EFI_STATUS       Status;
  IP_IO_OPEN_DATA  OpenData;

  ZeroMem (Udp6Service, sizeof (UDP6_SERVICE_DATA));

  Udp6Service->Signature        = UDP6_SERVICE_DATA_SIGNATURE;
  Udp6Service->ServiceBinding   = mUdp6ServiceBinding;
  Udp6Service->ImageHandle      = ImageHandle;
  Udp6Service->ControllerHandle = ControllerHandle;
  Udp6Service->ChildrenNumber   = 0;

  InitializeListHead (&Udp6Service->ChildrenList);

  //
  // Create the IpIo for this service context.
  //
  Udp6Service->IpIo = IpIoCreate (ImageHandle, ControllerHandle, IP_VERSION_6);
  if (Udp6Service->IpIo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set the OpenData used to open the IpIo.
  //
  CopyMem (
    &OpenData.IpConfigData.Ip6CfgData,
    &mIp6IoDefaultIpConfigData,
    sizeof (EFI_IP6_CONFIG_DATA)
    );
  OpenData.RcvdContext           = (VOID *) Udp6Service;
  OpenData.SndContext            = NULL;
  OpenData.PktRcvdNotify         = Udp6DgramRcvd;
  OpenData.PktSentNotify         = Udp6DgramSent;

  //
  // Configure and start the IpIo.
  //
  Status = IpIoOpen (Udp6Service->IpIo, &OpenData);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Create the event for Udp timeout checking.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Udp6CheckTimeout,
                  Udp6Service,
                  &Udp6Service->TimeoutEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Start the timeout timer event.
  //
  Status = gBS->SetTimer (
                  Udp6Service->TimeoutEvent,
                  TimerPeriodic,
                  UDP6_TIMEOUT_INTERVAL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (Udp6Service->TimeoutEvent != NULL) {
    gBS->CloseEvent (Udp6Service->TimeoutEvent);
  }

  IpIoDestroy (Udp6Service->IpIo);
  Udp6Service->IpIo = NULL;
  
  return Status;
}


/**
  Clean the Udp service context data.

  @param[in, out]  Udp6Service      Pointer to the UDP6_SERVICE_DATA.

**/
VOID
Udp6CleanService (
  IN OUT UDP6_SERVICE_DATA  *Udp6Service
  )
{
  //
  // Close the TimeoutEvent timer.
  //
  gBS->CloseEvent (Udp6Service->TimeoutEvent);

  //
  // Destroy the IpIo.
  //
  IpIoDestroy (Udp6Service->IpIo);
  Udp6Service->IpIo = NULL;
  
  ZeroMem (Udp6Service, sizeof (UDP6_SERVICE_DATA));
}


/**
  This function checks and times out the I/O datagrams listed in the
  UDP6_SERVICE_DATA which is specified by the input parameter Context.


  @param[in]  Event              The event this function registered to.
  @param[in]  Context            The context data registered during the creation of
                                 the Event.

**/
VOID
EFIAPI
Udp6CheckTimeout (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UDP6_SERVICE_DATA   *Udp6Service;
  LIST_ENTRY          *Entry;
  UDP6_INSTANCE_DATA  *Instance;
  LIST_ENTRY          *WrapEntry;
  LIST_ENTRY          *NextEntry;
  UDP6_RXDATA_WRAP    *Wrap;

  Udp6Service = (UDP6_SERVICE_DATA *) Context;
  NET_CHECK_SIGNATURE (Udp6Service, UDP6_SERVICE_DATA_SIGNATURE);

  NET_LIST_FOR_EACH (Entry, &Udp6Service->ChildrenList) {
    //
    // Iterate all the instances belonging to this service context.
    //
    Instance = NET_LIST_USER_STRUCT (Entry, UDP6_INSTANCE_DATA, Link);
    NET_CHECK_SIGNATURE (Instance, UDP6_INSTANCE_DATA_SIGNATURE);

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
      Wrap = NET_LIST_USER_STRUCT (WrapEntry, UDP6_RXDATA_WRAP, Link);

      if (Wrap->TimeoutTick < UDP6_TIMEOUT_INTERVAL / 10) {
        //
        // Remove this RxData if it timeouts.
        //
        Udp6RecycleRxDataWrap (NULL, (VOID *) Wrap);
      } else {
        Wrap->TimeoutTick -= UDP6_TIMEOUT_INTERVAL / 10;
      }
    }
  }
}


/**
  This function intializes the new created udp instance.

  @param[in]       Udp6Service      Pointer to the UDP6_SERVICE_DATA.
  @param[in, out]  Instance         Pointer to the un-initialized UDP6_INSTANCE_DATA.

**/
VOID
Udp6InitInstance (
  IN UDP6_SERVICE_DATA       *Udp6Service,
  IN OUT UDP6_INSTANCE_DATA  *Instance
  )
{
  //
  // Set the signature.
  //
  Instance->Signature = UDP6_INSTANCE_DATA_SIGNATURE;

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
  // Save the pointer to the UDP6_SERVICE_DATA, and initialize other members.
  //
  Instance->Udp6Service = Udp6Service;
  CopyMem (&Instance->Udp6Proto, &mUdp6Protocol, sizeof (EFI_UDP6_PROTOCOL));
  Instance->IcmpError   = EFI_SUCCESS;
  Instance->Configured  = FALSE;
  Instance->IsNoMapping = FALSE;
  Instance->InDestroy   = FALSE;
}


/**
  This function cleans the udp instance.

  @param[in, out]  Instance       Pointer to the UDP6_INSTANCE_DATA to clean.

**/
VOID
Udp6CleanInstance (
  IN OUT UDP6_INSTANCE_DATA  *Instance
  )
{
  NetMapClean (&Instance->McastIps);
  NetMapClean (&Instance->RxTokens);
  NetMapClean (&Instance->TxTokens);
}


/**
  This function finds the udp instance by the specified <Address, Port> pair.

  @param[in]  InstanceList       Pointer to the head of the list linking the udp
                                 instances.
  @param[in]  Address            Pointer to the specified IPv6 address.
  @param[in]  Port               The udp port number.

  @retval TRUE     The specified <Address, Port> pair is found.
  @retval FALSE    Otherwise.

**/
BOOLEAN
Udp6FindInstanceByPort (
  IN LIST_ENTRY        *InstanceList,
  IN EFI_IPv6_ADDRESS  *Address,
  IN UINT16            Port
  )
{
  LIST_ENTRY            *Entry;
  UDP6_INSTANCE_DATA    *Instance;
  EFI_UDP6_CONFIG_DATA  *ConfigData;

  NET_LIST_FOR_EACH (Entry, InstanceList) {
    //
    // Iterate all the udp instances.
    //
    Instance   = NET_LIST_USER_STRUCT (Entry, UDP6_INSTANCE_DATA, Link);
    ConfigData = &Instance->ConfigData;

    if (!Instance->Configured || ConfigData->AcceptAnyPort) {
      //
      // If the instance is not configured, or the configdata of the instance indicates
      // this instance accepts any port, skip it.
      //
      continue;
    }

    if (EFI_IP6_EQUAL (&ConfigData->StationAddress, Address) &&
        (ConfigData->StationPort == Port)
        ) {
      //
      // If both the address and the port are the same, return TRUE.
      //
      return TRUE;
    }
  }

  //
  // Return FALSE when matching fails.
  //
  return FALSE;
}


/**
  This function tries to bind the udp instance according to the configured port
  allocation stragety.

  @param[in]  InstanceList       Pointer to the head of the list linking the udp
                                 instances.
  @param[in]  ConfigData         Pointer to the ConfigData of the instance to be
                                 bound.

  @retval EFI_SUCCESS            The bound operation completed successfully.
  @retval EFI_ACCESS_DENIED      The <Address, Port> specified by the ConfigData is
                                 already used by other instance.
  @retval EFI_OUT_OF_RESOURCES   No available port resources.

**/
EFI_STATUS
Udp6Bind (
  IN LIST_ENTRY            *InstanceList,
  IN EFI_UDP6_CONFIG_DATA  *ConfigData
  )
{
  EFI_IPv6_ADDRESS  *StationAddress;
  UINT16            StartPort;

  if (ConfigData->AcceptAnyPort) {
    return EFI_SUCCESS;
  }

  StationAddress = &ConfigData->StationAddress;

  if (ConfigData->StationPort != 0) {

    if (!ConfigData->AllowDuplicatePort &&
        Udp6FindInstanceByPort (InstanceList, StationAddress, ConfigData->StationPort)
        ) {
      //
      // Do not allow duplicate ports and the port is already used by other instance.
      //
      return EFI_ACCESS_DENIED;
    }
  } else {
    //
    // Select a random port for this instance.
    //
    if (ConfigData->AllowDuplicatePort) {
      //
      // Just pick up the random port if the instance allows duplicate port.
      //
      ConfigData->StationPort = mUdp6RandomPort;
    } else {

      StartPort = mUdp6RandomPort;

      while (Udp6FindInstanceByPort (InstanceList, StationAddress, mUdp6RandomPort)) {

        mUdp6RandomPort++;
        if (mUdp6RandomPort == 0) {
          mUdp6RandomPort = UDP6_PORT_KNOWN;
        }

        if (mUdp6RandomPort == StartPort) {
          //
          // No available port.
          //
          return EFI_OUT_OF_RESOURCES;
        }
      }

      ConfigData->StationPort = mUdp6RandomPort;
    }

    mUdp6RandomPort++;
    if (mUdp6RandomPort == 0) {
      mUdp6RandomPort = UDP6_PORT_KNOWN;
    }
  }
  return EFI_SUCCESS;
}


/**
  This function is used to check whether the NewConfigData has any un-reconfigurable
  parameters changed compared to the OldConfigData.

  @param[in]  OldConfigData    Pointer to the current ConfigData the udp instance
                               uses.
  @param[in]  NewConfigData    Pointer to the new ConfigData.

  @retval TRUE     The instance is reconfigurable according to the NewConfigData.
  @retval FALSE    Otherwise.

**/
BOOLEAN
Udp6IsReconfigurable (
  IN EFI_UDP6_CONFIG_DATA  *OldConfigData,
  IN EFI_UDP6_CONFIG_DATA  *NewConfigData
  )
{
  if ((NewConfigData->AcceptAnyPort != OldConfigData->AcceptAnyPort) ||
      (NewConfigData->AcceptPromiscuous != OldConfigData->AcceptPromiscuous) ||
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

  if (!EFI_IP6_EQUAL (&NewConfigData->StationAddress, &OldConfigData->StationAddress)) {
    //
    //  The StationAddress is not the same.
    //
    return FALSE;
  }


  if (!EFI_IP6_EQUAL (&NewConfigData->RemoteAddress, &OldConfigData->RemoteAddress)) {
    //
    // The remoteaddress is not the same.
    //
    return FALSE;
  }

  if (!NetIp6IsUnspecifiedAddr (&NewConfigData->RemoteAddress) &&
      (NewConfigData->RemotePort != OldConfigData->RemotePort)
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
  This function builds the Ip6 configdata from the Udp6ConfigData.

  @param[in]       Udp6ConfigData         Pointer to the EFI_UDP6_CONFIG_DATA.
  @param[in, out]  Ip6ConfigData          Pointer to the EFI_IP6_CONFIG_DATA.

**/
VOID
Udp6BuildIp6ConfigData (
  IN EFI_UDP6_CONFIG_DATA      *Udp6ConfigData,
  IN OUT EFI_IP6_CONFIG_DATA   *Ip6ConfigData
  )
{
  CopyMem (
    Ip6ConfigData,
    &mIp6IoDefaultIpConfigData,
    sizeof (EFI_IP6_CONFIG_DATA)
    );
  Ip6ConfigData->DefaultProtocol      = EFI_IP_PROTO_UDP;
  Ip6ConfigData->AcceptPromiscuous    = Udp6ConfigData->AcceptPromiscuous;
  IP6_COPY_ADDRESS (&Ip6ConfigData->StationAddress, &Udp6ConfigData->StationAddress);
  IP6_COPY_ADDRESS (&Ip6ConfigData->DestinationAddress, &Udp6ConfigData->RemoteAddress);
  //
  // Use the -1 magic number to disable the receiving process of the ip instance.
  //
  Ip6ConfigData->ReceiveTimeout    = (UINT32) (-1);
}


/**
  This function validates the TxToken. It returns the error code according to the spec.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  TxToken            Pointer to the token to be checked.

  @retval EFI_SUCCESS            The TxToken is valid.
  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE:
                                 Token.Event is NULL;
                                 Token.Packet.TxData is NULL;
                                 Token.Packet.TxData.FragmentCount is zero;
                                 Token.Packet.TxData.DataLength is not equal to the
                                 sum of fragment lengths;
                                 One or more of the
                                 Token.Packet.TxData.FragmentTable[].FragmentLength
                                 fields is zero;
                                 One or more of the
                                 Token.Packet.TxData.FragmentTable[].FragmentBuffer
                                 fields is NULL;
                                 UdpSessionData.DestinationAddress are not valid
                                 unicast IPv6 addresses if the UdpSessionData is
                                 not NULL;
                                 UdpSessionData.DestinationPort and
                                 ConfigData.RemotePort are all zero if the
                                 UdpSessionData is not NULL.
  @retval EFI_BAD_BUFFER_SIZE    The data length is greater than the maximum UDP
                                 packet size.

**/
EFI_STATUS
Udp6ValidateTxToken (
  IN UDP6_INSTANCE_DATA         *Instance,
  IN EFI_UDP6_COMPLETION_TOKEN  *TxToken
  )
{
  EFI_UDP6_TRANSMIT_DATA  *TxData;
  UINT32                  Index;
  UINT32                  TotalLen;
  EFI_UDP6_CONFIG_DATA    *ConfigData;
  EFI_UDP6_SESSION_DATA   *UdpSessionData;


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
        (TxData->FragmentTable[Index].FragmentLength == 0)
        ) {
      //
      // If the FragmentBuffer is NULL, or the FragmentLeng is zero.
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

  ConfigData     = &Instance->ConfigData;
  UdpSessionData = TxData->UdpSessionData;

  if (UdpSessionData != NULL) {

    if ((UdpSessionData->DestinationPort == 0) && (ConfigData->RemotePort == 0)) {
      //
      // Ambiguous; no avalaible DestinationPort for this token.
      //
      return EFI_INVALID_PARAMETER;
    }

    if (NetIp6IsUnspecifiedAddr (&UdpSessionData->DestinationAddress) &&
        NetIp6IsUnspecifiedAddr (&ConfigData->RemoteAddress)
        ) {
      //
      // The DestinationAddress is not specificed.
      //
      return EFI_INVALID_PARAMETER;
    }

    if (!NetIp6IsUnspecifiedAddr (&UdpSessionData->DestinationAddress) &&
        !NetIp6IsUnspecifiedAddr (&ConfigData->RemoteAddress)
        ) {
      //
      // The ConfigData.RemoteAddress is not zero and the UdpSessionData.DestinationAddress
      // is not zero too.
      //
      return EFI_INVALID_PARAMETER;
    }
  } else if (NetIp6IsUnspecifiedAddr (&ConfigData->RemoteAddress)) {
    //
    // The configured RemoteAddress is all zero, and the user doesn't override the
    // destination address.
    //
    return EFI_INVALID_PARAMETER;
  }

  if (TxData->DataLength > UDP6_MAX_DATA_SIZE) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}


/**
  This function checks whether the specified Token duplicates the one in the Map.

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
Udp6TokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  EFI_UDP6_COMPLETION_TOKEN  *Token;
  EFI_UDP6_COMPLETION_TOKEN  *TokenInItem;

  Token       = (EFI_UDP6_COMPLETION_TOKEN *) Context;
  TokenInItem = (EFI_UDP6_COMPLETION_TOKEN *) Item->Key;

  if ((Token == TokenInItem) || (Token->Event == TokenInItem->Event)) {
    //
    // The Token duplicates with the TokenInItem in case either the two pointers are the
    // same, or the Events of these two tokens are the same.
    //
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}


/**
  This function calculates the checksum for the Packet, utilizing the pre-calculated
  pseudo HeadSum to reduce some overhead.

  @param[in]  Packet           Pointer to the NET_BUF contains the udp datagram.
  @param[in]  HeadSum          Checksum of the pseudo header, execpt the length
                               field.

  @return The 16-bit checksum of this udp datagram.

**/
UINT16
Udp6Checksum (
  IN NET_BUF *Packet,
  IN UINT16  HeadSum
  )
{
  UINT16  Checksum;

  Checksum  = NetbufChecksum (Packet);
  Checksum  = NetAddChecksum (Checksum, HeadSum);

  Checksum  = NetAddChecksum (Checksum, HTONS ((UINT16) Packet->TotalSize));
  Checksum  = (UINT16) (~Checksum);
  return Checksum;
}


/**
  This function removes the specified Token from the TokenMap.

  @param[in]  TokenMap           Pointer to the NET_MAP containing the tokens.
  @param[in]  Token              Pointer to the Token to be removed.

  @retval EFI_SUCCESS            The specified Token is removed from the TokenMap.
  @retval EFI_NOT_FOUND          The specified Token is not found in the TokenMap.

**/
EFI_STATUS
Udp6RemoveToken (
  IN NET_MAP                    *TokenMap,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token
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

  @param[in]  Status            The completion status of the output udp datagram.
  @param[in]  Context           Pointer to the context data.
  @param[in]  Sender            Specify a EFI_IP6_PROTOCOL for sending.
  @param[in]  NotifyData        Pointer to the notify data.

**/
VOID
EFIAPI
Udp6DgramSent (
  IN EFI_STATUS        Status,
  IN VOID              *Context,
  IN IP_IO_IP_PROTOCOL Sender,
  IN VOID              *NotifyData
  )
{
  UDP6_INSTANCE_DATA         *Instance;
  EFI_UDP6_COMPLETION_TOKEN  *Token;

  Instance = (UDP6_INSTANCE_DATA *) Context;
  Token    = (EFI_UDP6_COMPLETION_TOKEN *) NotifyData;

  if (Udp6RemoveToken (&Instance->TxTokens, Token) == EFI_SUCCESS) {
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

  @param[in]  Status            The status of this udp datagram.
  @param[in]  IcmpError         The IcmpError code, only available when Status is
                                EFI_ICMP_ERROR.
  @param[in]  NetSession        Pointer to the EFI_NET_SESSION_DATA.
  @param[in]  Packet            Pointer to the NET_BUF containing the received udp
                                datagram.
  @param[in]  Context           Pointer to the context data.

**/
VOID
EFIAPI
Udp6DgramRcvd (
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
    Udp6Demultiplex ((UDP6_SERVICE_DATA *) Context, NetSession, Packet);
  } else {
    //
    // Handle the ICMP6 Error packet.
    //
    Udp6IcmpHandler ((UDP6_SERVICE_DATA *) Context, IcmpError, NetSession, Packet);
  }

  //
  // Dispatch the DPC queued by the NotifyFunction of the rx token's events
  // that are signaled with received data.
  //
  DispatchDpc ();
}


/**
  This function removes the multicast group specified by Arg from the Map.

  @param[in]  Map                Pointer to the NET_MAP.
  @param[in]  Item               Pointer to the NET_MAP_ITEM.
  @param[in]  Arg                Pointer to the Arg, it's the pointer to a
                                 multicast IPv6 Address. This parameter is
                                 optional and may be NULL.

  @retval EFI_SUCCESS            The multicast address is removed.
  @retval EFI_ABORTED            The specified multicast address is removed, and the
                                 Arg is not NULL.

**/
EFI_STATUS
EFIAPI
Udp6LeaveGroup (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg OPTIONAL
  )
{
  EFI_IPv6_ADDRESS  *McastIp;

  McastIp = Arg;

  if ((McastIp != NULL) &&
      !EFI_IP6_EQUAL (McastIp, ((EFI_IPv6_ADDRESS *)Item->Key))
      ) {
    //
    // McastIp is not NULL and the multicast address contained in the Item
    // is not the same as McastIp.
    //
    return EFI_SUCCESS;
  }

  FreePool (Item->Key);

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
  This function cancle the token specified by Arg in the Map.

  @param[in]  Map             Pointer to the NET_MAP.
  @param[in]  Item            Pointer to the NET_MAP_ITEM.
  @param[in]  Arg             Pointer to the token to be cancelled. If NULL, all
                              the tokens in this Map will be cancelled.
                              This parameter is optional and may be NULL.

  @retval EFI_SUCCESS         The token is cancelled if Arg is NULL, or the token
                              is not the same as that in the Item, if Arg is not
                              NULL.
  @retval EFI_ABORTED         Arg is not NULL, and the token specified by Arg is
                              cancelled.

**/
EFI_STATUS
EFIAPI
Udp6CancelTokens (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg OPTIONAL
  )
{
  EFI_UDP6_COMPLETION_TOKEN  *TokenToCancel;
  NET_BUF                    *Packet;
  IP_IO                      *IpIo;

  if ((Arg != NULL) && (Item->Key != Arg)) {
    return EFI_SUCCESS;
  }

  if (Item->Value != NULL) {
    //
    // If the token is a transmit token, the corresponding Packet is recorded in
    // Item->Value, invoke IpIo to cancel this packet first. The IpIoCancelTxToken
    // will invoke Udp6DgramSent, the token will be signaled and this Item will
    // be removed from the Map there.
    //
    Packet  = (NET_BUF *) (Item->Value);
    IpIo    = (IP_IO *) (*((UINTN *) &Packet->ProtoData[0]));

    IpIoCancelTxToken (IpIo, Packet);
  } else {
    //
    // The token is a receive token. Abort it and remove it from the Map.
    //
    TokenToCancel = (EFI_UDP6_COMPLETION_TOKEN *) Item->Key;
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

  @param[in]  Instance    Pointer to the Udp6 Instance.

**/
VOID
Udp6FlushRcvdDgram (
  IN UDP6_INSTANCE_DATA  *Instance
  )
{
  UDP6_RXDATA_WRAP  *Wrap;

  while (!IsListEmpty (&Instance->RcvdDgramQue)) {
    //
    // Iterate all the Wraps in the RcvdDgramQue.
    //
    Wrap = NET_LIST_HEAD (&Instance->RcvdDgramQue, UDP6_RXDATA_WRAP, Link);

    //
    // The Wrap will be removed from the RcvdDgramQue by this function call.
    //
    Udp6RecycleRxDataWrap (NULL, (VOID *) Wrap);
  }
}



/**
  Cancel Udp6 tokens from the Udp6 instance.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  Token              Pointer to the token to be canceled. If NULL, all
                                 tokens in this instance will be cancelled.
                                 This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The Token is cancelled.
  @retval EFI_NOT_FOUND          The Token is not found.

**/
EFI_STATUS
Udp6InstanceCancelToken (
  IN UDP6_INSTANCE_DATA         *Instance,
  IN EFI_UDP6_COMPLETION_TOKEN  *Token OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // Cancel this token from the TxTokens map.
  //
  Status = NetMapIterate (&Instance->TxTokens, Udp6CancelTokens, Token);

  if ((Token != NULL) && (Status == EFI_ABORTED)) {
    //
    // If Token isn't NULL and Status is EFI_ABORTED, the token is cancelled from
    // the TxTokens and returns success.
    //
    return EFI_SUCCESS;
  }

  //
  // Try to cancel this token from the RxTokens map in condition either the Token
  // is NULL or the specified Token is not in TxTokens.
  //
  Status = NetMapIterate (&Instance->RxTokens, Udp6CancelTokens, Token);

  if ((Token != NULL) && (Status == EFI_SUCCESS)) {
    //
    // If Token isn't NULL and Status is EFI_SUCCESS, the token is neither in the
    // TxTokens nor the RxTokens, or say, it's not found.
    //
    return EFI_NOT_FOUND;
  }

  ASSERT ((Token != NULL) ||
          ((0 == NetMapGetCount (&Instance->TxTokens)) &&
          (0 == NetMapGetCount (&Instance->RxTokens)))
          );

  return EFI_SUCCESS;
}


/**
  This function checks if the received udp datagram matches with the Instance.

  @param[in]  Instance           Pointer to the udp instance context data.
  @param[in]  Udp6Session        Pointer to the EFI_UDP6_SESSION_DATA abstracted
                                 from the received udp datagram.

  @retval TRUE     The udp datagram matches the receiving requirements of the Instance.
  @retval FALSE    The udp datagram does not matche the receiving requirements of the Instance.

**/
BOOLEAN
Udp6MatchDgram (
  IN UDP6_INSTANCE_DATA     *Instance,
  IN EFI_UDP6_SESSION_DATA  *Udp6Session
  )
{
  EFI_UDP6_CONFIG_DATA  *ConfigData;
  EFI_IPv6_ADDRESS      Destination;

  ConfigData = &Instance->ConfigData;

  if (ConfigData->AcceptPromiscuous) {
    //
    // Always matches if this instance is in the promiscuous state.
    //
    return TRUE;
  }

  if ((!ConfigData->AcceptAnyPort && (Udp6Session->DestinationPort != ConfigData->StationPort)) ||
      ((ConfigData->RemotePort != 0) && (Udp6Session->SourcePort != ConfigData->RemotePort))
      ) {
    //
    // The local port or the remote port doesn't match.
    //
    return FALSE;
  }

  if (!NetIp6IsUnspecifiedAddr (&ConfigData->RemoteAddress) &&
      !EFI_IP6_EQUAL (&ConfigData->RemoteAddress, &Udp6Session->SourceAddress)
      ) {
    //
    // This datagram doesn't come from the instance's specified sender.
    //
    return FALSE;
  }

  if (NetIp6IsUnspecifiedAddr (&ConfigData->StationAddress) ||
      EFI_IP6_EQUAL (&Udp6Session->DestinationAddress, &ConfigData->StationAddress)
      ) {
    //
    // The instance is configured to receive datagrams destinated to any station IP or
    // the destination address of this datagram matches the configured station IP.
    //
    return TRUE;
  }

  IP6_COPY_ADDRESS (&Destination, &Udp6Session->DestinationAddress);

  if (IP6_IS_MULTICAST (&Destination) &&
      (NULL != Udp6MapMultiCastAddr (&Instance->McastIps, &Destination))
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

  @param[in]  Event                  The Event this notify function registered to.
  @param[in]  Context                Pointer to the context data.

**/
VOID
EFIAPI
Udp6RecycleRxDataWrap (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UDP6_RXDATA_WRAP  *Wrap;

  Wrap = (UDP6_RXDATA_WRAP *) Context;

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
  This function wraps the Packet into RxData.

  @param[in]  Instance           Pointer to the instance context data.
  @param[in]  Packet             Pointer to the buffer containing the received
                                 datagram.
  @param[in]  RxData             Pointer to the EFI_UDP6_RECEIVE_DATA of this
                                 datagram.

  @return Pointer to the structure wrapping the RxData and the Packet.

**/
UDP6_RXDATA_WRAP *
Udp6WrapRxData (
  IN UDP6_INSTANCE_DATA     *Instance,
  IN NET_BUF                *Packet,
  IN EFI_UDP6_RECEIVE_DATA  *RxData
  )
{
  EFI_STATUS            Status;
  UDP6_RXDATA_WRAP      *Wrap;

  //
  // Allocate buffer for the Wrap.
  //
  Wrap = AllocateZeroPool (sizeof (UDP6_RXDATA_WRAP) +
         (Packet->BlockOpNum - 1) * sizeof (EFI_UDP6_FRAGMENT_DATA));
  if (Wrap == NULL) {
    return NULL;
  }

  InitializeListHead (&Wrap->Link);

  CopyMem (&Wrap->RxData, RxData, sizeof(EFI_UDP6_RECEIVE_DATA));
  //
  // Create the Recycle event.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Udp6RecycleRxDataWrap,
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

  @param[in]  Udp6Service        Pointer to the udp service context data.
  @param[in]  Packet             Pointer to the buffer containing the received
                                 datagram.
  @param[in]  RxData             Pointer to the EFI_UDP6_RECEIVE_DATA of this
                                 datagram.

  @return The times this datagram is enqueued.

**/
UINTN
Udp6EnqueueDgram (
  IN UDP6_SERVICE_DATA      *Udp6Service,
  IN NET_BUF                *Packet,
  IN EFI_UDP6_RECEIVE_DATA  *RxData
  )
{
  LIST_ENTRY          *Entry;
  UDP6_INSTANCE_DATA  *Instance;
  UDP6_RXDATA_WRAP    *Wrap;
  UINTN               Enqueued;

  Enqueued = 0;

  NET_LIST_FOR_EACH (Entry, &Udp6Service->ChildrenList) {
    //
    // Iterate the instances.
    //
    Instance = NET_LIST_USER_STRUCT (Entry, UDP6_INSTANCE_DATA, Link);

    if (!Instance->Configured) {
      continue;
    }

    if (Udp6MatchDgram (Instance, &RxData->UdpSession)) {
      //
      // Wrap the RxData and put this Wrap into the instances RcvdDgramQue.
      //
      Wrap = Udp6WrapRxData (Instance, Packet, RxData);
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
  This function delivers the received datagrams to the specified instance.

  @param[in]  Instance               Pointer to the instance context data.

**/
VOID
Udp6InstanceDeliverDgram (
  IN UDP6_INSTANCE_DATA  *Instance
  )
{
  UDP6_RXDATA_WRAP           *Wrap;
  EFI_UDP6_COMPLETION_TOKEN  *Token;
  NET_BUF                    *Dup;
  EFI_UDP6_RECEIVE_DATA      *RxData;
  EFI_TPL                    OldTpl;

  if (!IsListEmpty (&Instance->RcvdDgramQue) &&
      !NetMapIsEmpty (&Instance->RxTokens)
      ) {

    Wrap = NET_LIST_HEAD (&Instance->RcvdDgramQue, UDP6_RXDATA_WRAP, Link);

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

    Token = (EFI_UDP6_COMPLETION_TOKEN *) NetMapRemoveHead (&Instance->RxTokens, NULL);

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

  @param[in]  Udp6Service            Pointer to the udp service context data.

**/
VOID
Udp6DeliverDgram (
  IN UDP6_SERVICE_DATA  *Udp6Service
  )
{
  LIST_ENTRY          *Entry;
  UDP6_INSTANCE_DATA  *Instance;

  NET_LIST_FOR_EACH (Entry, &Udp6Service->ChildrenList) {
    //
    // Iterate the instances.
    //
    Instance = NET_LIST_USER_STRUCT (Entry, UDP6_INSTANCE_DATA, Link);

    if (!Instance->Configured) {
      continue;
    }

    //
    // Deliver the datagrams of this instance.
    //
    Udp6InstanceDeliverDgram (Instance);
  }
}


/**
  This function demultiplexes the received udp datagram to the appropriate instances.

  @param[in]  Udp6Service        Pointer to the udp service context data.
  @param[in]  NetSession         Pointer to the EFI_NET_SESSION_DATA abstrated from
                                 the received datagram.
  @param[in]  Packet             Pointer to the buffer containing the received udp
                                 datagram.

**/
VOID
Udp6Demultiplex (
  IN UDP6_SERVICE_DATA     *Udp6Service,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN NET_BUF               *Packet
  )
{
  EFI_UDP_HEADER        *Udp6Header;
  UINT16                 HeadSum;
  EFI_UDP6_RECEIVE_DATA  RxData;
  EFI_UDP6_SESSION_DATA  *Udp6Session;
  UINTN                  Enqueued;

  //
  // Get the datagram header from the packet buffer.
  //
  Udp6Header = (EFI_UDP_HEADER *) NetbufGetByte (Packet, 0, NULL);
  ASSERT (Udp6Header != NULL);

  if (Udp6Header->Checksum != 0) {
    //
    // check the checksum.
    //
    HeadSum = NetIp6PseudoHeadChecksum (
                &NetSession->Source.v6,
                &NetSession->Dest.v6,
                EFI_IP_PROTO_UDP,
                0
                );

    if (Udp6Checksum (Packet, HeadSum) != 0) {
      //
      // Wrong checksum.
      //
      return;
    }
  }

  Udp6Session                  = &RxData.UdpSession;
  Udp6Session->SourcePort      = NTOHS (Udp6Header->SrcPort);
  Udp6Session->DestinationPort = NTOHS (Udp6Header->DstPort);

  IP6_COPY_ADDRESS (&Udp6Session->SourceAddress, &NetSession->Source);
  IP6_COPY_ADDRESS (&Udp6Session->DestinationAddress, &NetSession->Dest);

  //
  // Trim the UDP header.
  //
  NetbufTrim (Packet, UDP6_HEADER_SIZE, TRUE);

  RxData.DataLength = (UINT32) Packet->TotalSize;

  //
  // Try to enqueue this datagram into the instances.
  //
  Enqueued = Udp6EnqueueDgram (Udp6Service, Packet, &RxData);

  if (Enqueued == 0) {
    //
    // Send the port unreachable ICMP packet before we free this NET_BUF
    //
    Udp6SendPortUnreach (Udp6Service->IpIo, NetSession, Udp6Header);
  }

  //
  // Try to free the packet before deliver it.
  //
  NetbufFree (Packet);

  if (Enqueued > 0) {
    //
    // Deliver the datagram.
    //
    Udp6DeliverDgram (Udp6Service);
  }
}


/**
  This function builds and sends out a icmp port unreachable message.

  @param[in]  IpIo               Pointer to the IP_IO instance.
  @param[in]  NetSession         Pointer to the EFI_NET_SESSION_DATA of the packet
                                 causes this icmp error message.
  @param[in]  Udp6Header         Pointer to the udp header of the datagram causes
                                 this icmp error message.

**/
VOID
Udp6SendPortUnreach (
  IN IP_IO                 *IpIo,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN VOID                  *Udp6Header
  )
{
  NET_BUF              *Packet;
  UINT32               Len;
  IP6_ICMP_ERROR_HEAD  *IcmpErrHdr;
  UINT8                *Ptr;
  IP_IO_OVERRIDE       Override;
  IP_IO_IP_INFO        *IpSender;
  EFI_IP6_MODE_DATA    *Ip6ModeData;
  EFI_STATUS           Status;
  EFI_IP6_PROTOCOL     *Ip6Protocol;

  Ip6ModeData = NULL;

  //
  // An ICMPv6 error message MUST NOT be originated as A packet destined to
  // 1) an IPv6 multicast address 2) The IPv6 Unspecified Address
  //
  if (NetSession->IpVersion == IP_VERSION_6) {
    if (NetIp6IsUnspecifiedAddr (&NetSession->Dest.v6) ||
      IP6_IS_MULTICAST (&NetSession->Dest.v6)
      ) {
      goto EXIT;
    }
  }


  IpSender    = IpIoFindSender (&IpIo, NetSession->IpVersion, &NetSession->Dest);

  //
  // Get the Ipv6 Mode Data.
  //
  Ip6ModeData = AllocateZeroPool (sizeof (EFI_IP6_MODE_DATA));
  ASSERT (Ip6ModeData != NULL);

  //
  // If not finding the related IpSender use the default IpIo to send out
  // the port unreachable ICMP message.
  //
  if (IpSender == NULL) {
    Ip6Protocol = IpIo->Ip.Ip6;
  } else {
    Ip6Protocol = IpSender->Ip.Ip6;
  }

  Status = Ip6Protocol->GetModeData (
                          Ip6Protocol,
                          Ip6ModeData,
                          NULL,
                          NULL
                          );

  if (EFI_ERROR (Status)) {
    goto EXIT;
  }
  //
  // The ICMP6 packet length, includes whole invoking packet and ICMP6 error header.
  //
  Len = NetSession->IpHdrLen +
        NTOHS(((EFI_UDP_HEADER *) Udp6Header)->Length) +
        sizeof (IP6_ICMP_ERROR_HEAD);

  //
  // If the ICMP6 packet length larger than IP MTU, adjust its length to MTU.
  //
  if (Ip6ModeData->MaxPacketSize < Len) {
    Len = Ip6ModeData->MaxPacketSize;
  }

  //
  // Allocate buffer for the icmp error message.
  //
  Packet = NetbufAlloc (Len);
  if (Packet == NULL) {
    goto EXIT;
  }

  //
  // Allocate space for the IP6_ICMP_ERROR_HEAD.
  //
  IcmpErrHdr = (IP6_ICMP_ERROR_HEAD *) NetbufAllocSpace (Packet, Len, FALSE);
  ASSERT (IcmpErrHdr != NULL);

  //
  // Set the required fields for the icmp port unreachable message.
  //
  IcmpErrHdr->Head.Type     = ICMP_V6_DEST_UNREACHABLE;
  IcmpErrHdr->Head.Code     = ICMP_V6_PORT_UNREACHABLE;
  IcmpErrHdr->Head.Checksum = 0;
  IcmpErrHdr->Fourth        = 0;

  //
  // Copy as much of invoking Packet as possible without the ICMPv6 packet
  // exceeding the minimum Ipv6 MTU. The length of IP6_ICMP_ERROR_HEAD contains
  // the length of EFI_IP6_HEADER, so when using the length of IP6_ICMP_ERROR_HEAD
  // for pointer movement that fact should be considered.
  //
  Ptr = (VOID *) &IcmpErrHdr->Head;
  Ptr = (UINT8 *) (UINTN) ((UINTN) Ptr + sizeof (IP6_ICMP_ERROR_HEAD) - sizeof (EFI_IP6_HEADER));
  CopyMem (Ptr, NetSession->IpHdr.Ip6Hdr, NetSession->IpHdrLen);
  CopyMem (
    Ptr + NetSession->IpHdrLen,
    Udp6Header,
    Len - NetSession->IpHdrLen - sizeof (IP6_ICMP_ERROR_HEAD) + sizeof (EFI_IP6_HEADER)
    );

  //
  // Set the checksum as zero, and IP6 driver will calcuate it with pseudo header.
  //
  IcmpErrHdr->Head.Checksum = 0;

  //
  // Fill the override data.
  //
  Override.Ip6OverrideData.FlowLabel     = 0;
  Override.Ip6OverrideData.HopLimit      = 255;
  Override.Ip6OverrideData.Protocol      = IP6_ICMP;

  //
  // Send out this icmp packet.
  //
  IpIoSend (IpIo, Packet, IpSender, NULL, NULL, &NetSession->Source, &Override);

  NetbufFree (Packet);

EXIT:
  if (Ip6ModeData != NULL) {
    FreePool (Ip6ModeData);
  }
}


/**
  This function handles the received Icmp Error message and de-multiplexes it to the
  instance.

  @param[in]       Udp6Service        Pointer to the udp service context data.
  @param[in]       IcmpError          The icmp error code.
  @param[in]       NetSession         Pointer to the EFI_NET_SESSION_DATA abstracted
                                      from the received Icmp Error packet.
  @param[in, out]  Packet             Pointer to the Icmp Error packet.

**/
VOID
Udp6IcmpHandler (
  IN UDP6_SERVICE_DATA     *Udp6Service,
  IN UINT8                 IcmpError,
  IN EFI_NET_SESSION_DATA  *NetSession,
  IN OUT NET_BUF           *Packet
  )
{
  EFI_UDP_HEADER         *Udp6Header;
  EFI_UDP6_SESSION_DATA  Udp6Session;
  LIST_ENTRY             *Entry;
  UDP6_INSTANCE_DATA     *Instance;

  Udp6Header = (EFI_UDP_HEADER *) NetbufGetByte (Packet, 0, NULL);
  ASSERT (Udp6Header != NULL);

  IP6_COPY_ADDRESS (&Udp6Session.SourceAddress, &NetSession->Source);
  IP6_COPY_ADDRESS (&Udp6Session.DestinationAddress, &NetSession->Dest);

  Udp6Session.SourcePort      = NTOHS (Udp6Header->DstPort);
  Udp6Session.DestinationPort = NTOHS (Udp6Header->SrcPort);

  NET_LIST_FOR_EACH (Entry, &Udp6Service->ChildrenList) {
    //
    // Iterate all the instances.
    //
    Instance = NET_LIST_USER_STRUCT (Entry, UDP6_INSTANCE_DATA, Link);

    if (!Instance->Configured) {
      continue;
    }

    if (Udp6MatchDgram (Instance, &Udp6Session)) {
      //
      // Translate the Icmp Error code according to the udp spec.
      //
      Instance->IcmpError = IpIoGetIcmpErrStatus (IcmpError, IP_VERSION_6, NULL, NULL);

      if (IcmpError > ICMP_ERR_UNREACH_PORT) {
        Instance->IcmpError = EFI_ICMP_ERROR;
      }

      //
      // Notify the instance with the received Icmp Error.
      //
      Udp6ReportIcmpError (Instance);

      break;
    }
  }

  NetbufFree (Packet);
}


/**
  This function reports the received ICMP error.

  @param[in]  Instance          Pointer to the udp instance context data.

**/
VOID
Udp6ReportIcmpError (
  IN UDP6_INSTANCE_DATA  *Instance
  )
{
  EFI_UDP6_COMPLETION_TOKEN  *Token;

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
    Token = (EFI_UDP6_COMPLETION_TOKEN *) NetMapRemoveHead (&Instance->RxTokens, NULL);

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
Udp6NetVectorExtFree (
  IN VOID  *Context
  )
{
} 

/**
  Find the key in the netmap.

  @param[in]  Map                    The netmap to search within.
  @param[in]  Key                    The key to search.

  @return The point to the item contains the Key, or NULL, if Key isn't in the map.

**/
NET_MAP_ITEM *
Udp6MapMultiCastAddr (
  IN  NET_MAP               *Map,
  IN  VOID                  *Key
  )
{
  LIST_ENTRY        *Entry;
  NET_MAP_ITEM      *Item;
  EFI_IPv6_ADDRESS  *Addr;

  ASSERT (Map != NULL);
  NET_LIST_FOR_EACH (Entry, &Map->Used) {
    Item  = NET_LIST_USER_STRUCT (Entry, NET_MAP_ITEM, Link);
    Addr  = (EFI_IPv6_ADDRESS *) Item->Key;
    if (EFI_IP6_EQUAL (Addr, Key)) {
      return Item;
    }
  }
  return NULL;
}

