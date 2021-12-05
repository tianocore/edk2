/** @file

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip4Impl.h"

EFI_IPSEC2_PROTOCOL  *mIpSec = NULL;

/**
  Gets the current operational settings for this instance of the EFI IPv4 Protocol driver.

  The GetModeData() function returns the current operational mode data for this
  driver instance. The data fields in EFI_IP4_MODE_DATA are read only. This
  function is used optionally to retrieve the operational mode data of underlying
  networks or drivers.

  @param[in]   This          Pointer to the EFI_IP4_PROTOCOL instance.
  @param[out]  Ip4ModeData   Pointer to the EFI IPv4 Protocol mode data structure.
  @param[out]  MnpConfigData Pointer to the managed network configuration data structure.
  @param[out]  SnpModeData   Pointer to the simple network mode data structure.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_OUT_OF_RESOURCES  The required mode data could not be allocated.

**/
EFI_STATUS
EFIAPI
EfiIp4GetModeData (
  IN  CONST EFI_IP4_PROTOCOL                 *This,
  OUT       EFI_IP4_MODE_DATA                *Ip4ModeData     OPTIONAL,
  OUT       EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData   OPTIONAL,
  OUT       EFI_SIMPLE_NETWORK_MODE          *SnpModeData     OPTIONAL
  );

/**
  Assigns an IPv4 address and subnet mask to this EFI IPv4 Protocol driver instance.

  The Configure() function is used to set, change, or reset the operational
  parameters and filter settings for this EFI IPv4 Protocol instance. Until these
  parameters have been set, no network traffic can be sent or received by this
  instance. Once the parameters have been reset (by calling this function with
  IpConfigData set to NULL), no more traffic can be sent or received until these
  parameters have been set again. Each EFI IPv4 Protocol instance can be started
  and stopped independently of each other by enabling or disabling their receive
  filter settings with the Configure() function.

  When IpConfigData.UseDefaultAddress is set to FALSE, the new station address will
  be appended as an alias address into the addresses list in the EFI IPv4 Protocol
  driver. While set to TRUE, Configure() will trigger the EFI_IP4_CONFIG_PROTOCOL
  to retrieve the default IPv4 address if it is not available yet. Clients could
  frequently call GetModeData() to check the status to ensure that the default IPv4
  address is ready.

  If operational parameters are reset or changed, any pending transmit and receive
  requests will be cancelled. Their completion token status will be set to EFI_ABORTED
  and their events will be signaled.

  @param[in]  This              Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  IpConfigData      Pointer to the EFI IPv4 Protocol configuration data structure.

  @retval EFI_SUCCESS           The driver instance was successfully opened.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
  @retval EFI_UNSUPPORTED       One or more of the following conditions is TRUE:
                                A configuration protocol (DHCP, BOOTP, RARP, etc.) could
                                not be located when clients choose to use the default IPv4
                                address. This EFI IPv4 Protocol implementation does not
                                support this requested filter or timeout setting.
  @retval EFI_OUT_OF_RESOURCES  The EFI IPv4 Protocol driver instance data could not be allocated.
  @retval EFI_ALREADY_STARTED   The interface is already open and must be stopped before the
                                IPv4 address or subnet mask can be changed. The interface must
                                also be stopped when switching to/from raw packet mode.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred. The EFI IPv4
                                Protocol driver instance is not opened.

**/
EFI_STATUS
EFIAPI
EfiIp4Configure (
  IN EFI_IP4_PROTOCOL     *This,
  IN EFI_IP4_CONFIG_DATA  *IpConfigData       OPTIONAL
  );

/**
  Joins and leaves multicast groups.

  The Groups() function is used to join and leave multicast group sessions. Joining
  a group will enable reception of matching multicast packets. Leaving a group will
  disable the multicast packet reception.

  If JoinFlag is FALSE and GroupAddress is NULL, all joined groups will be left.

  @param[in]  This                  Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  JoinFlag              Set to TRUE to join the multicast group session and FALSE to leave.
  @param[in]  GroupAddress          Pointer to the IPv4 multicast address.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following is TRUE:
                                - This is NULL.
                                - JoinFlag is TRUE and GroupAddress is NULL.
                                - GroupAddress is not NULL and *GroupAddress is
                                not a multicast IPv4 address.
  @retval EFI_NOT_STARTED       This instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_OUT_OF_RESOURCES  System resources could not be allocated.
  @retval EFI_UNSUPPORTED       This EFI IPv4 Protocol implementation does not support multicast groups.
  @retval EFI_ALREADY_STARTED   The group address is already in the group table (when
                                JoinFlag is TRUE).
  @retval EFI_NOT_FOUND         The group address is not in the group table (when JoinFlag is FALSE).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiIp4Groups (
  IN EFI_IP4_PROTOCOL  *This,
  IN BOOLEAN           JoinFlag,
  IN EFI_IPv4_ADDRESS  *GroupAddress     OPTIONAL
  );

/**
  Adds and deletes routing table entries.

  The Routes() function adds a route to or deletes a route from the routing table.

  Routes are determined by comparing the SubnetAddress with the destination IPv4
  address arithmetically AND-ed with the SubnetMask. The gateway address must be
  on the same subnet as the configured station address.

  The default route is added with SubnetAddress and SubnetMask both set to 0.0.0.0.
  The default route matches all destination IPv4 addresses that do not match any
  other routes.

  A GatewayAddress that is zero is a nonroute. Packets are sent to the destination
  IP address if it can be found in the ARP cache or on the local subnet. One automatic
  nonroute entry will be inserted into the routing table for outgoing packets that
  are addressed to a local subnet (gateway address of 0.0.0.0).

  Each EFI IPv4 Protocol instance has its own independent routing table. Those EFI
  IPv4 Protocol instances that use the default IPv4 address will also have copies
  of the routing table that was provided by the EFI_IP4_CONFIG_PROTOCOL, and these
  copies will be updated whenever the EIF IPv4 Protocol driver reconfigures its
  instances. As a result, client modification to the routing table will be lost.

  @param[in]  This                   Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  DeleteRoute            Set to TRUE to delete this route from the routing table. Set to
                                     FALSE to add this route to the routing table. SubnetAddress
                                     and SubnetMask are used as the key to each route entry.
  @param[in]  SubnetAddress          The address of the subnet that needs to be routed.
  @param[in]  SubnetMask             The subnet mask of SubnetAddress.
  @param[in]  GatewayAddress         The unicast gateway IPv4 address for this route.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_NOT_STARTED        The driver instance has not been started.
  @retval EFI_NO_MAPPING         When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - SubnetAddress is NULL.
                                 - SubnetMask is NULL.
                                 - GatewayAddress is NULL.
                                 - *SubnetAddress is not a valid subnet address.
                                 - *SubnetMask is not a valid subnet mask.
                                 - *GatewayAddress is not a valid unicast IPv4 address.
  @retval EFI_OUT_OF_RESOURCES   Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND          This route is not in the routing table (when DeleteRoute is TRUE).
  @retval EFI_ACCESS_DENIED      The route is already defined in the routing table (when
                                  DeleteRoute is FALSE).

**/
EFI_STATUS
EFIAPI
EfiIp4Routes (
  IN EFI_IP4_PROTOCOL  *This,
  IN BOOLEAN           DeleteRoute,
  IN EFI_IPv4_ADDRESS  *SubnetAddress,
  IN EFI_IPv4_ADDRESS  *SubnetMask,
  IN EFI_IPv4_ADDRESS  *GatewayAddress
  );

/**
  Places outgoing data packets into the transmit queue.

  The Transmit() function places a sending request in the transmit queue of this
  EFI IPv4 Protocol instance. Whenever the packet in the token is sent out or some
  errors occur, the event in the token will be signaled and the status is updated.

  @param[in]  This  Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  Token Pointer to the transmit token.

  @retval  EFI_SUCCESS           The data has been queued for transmission.
  @retval  EFI_NOT_STARTED       This instance has not been started.
  @retval  EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval  EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval  EFI_ACCESS_DENIED     The transmit completion token with the same Token.Event
                                 was already in the transmit queue.
  @retval  EFI_NOT_READY         The completion token could not be queued because the transmit
                                 queue is full.
  @retval  EFI_NOT_FOUND         Not route is found to destination address.
  @retval  EFI_OUT_OF_RESOURCES  Could not queue the transmit data.
  @retval  EFI_BUFFER_TOO_SMALL  Token.Packet.TxData.TotalDataLength is too
                                 short to transmit.
  @retval  EFI_BAD_BUFFER_SIZE   The length of the IPv4 header + option length + total data length is
                                 greater than MTU (or greater than the maximum packet size if
                                 Token.Packet.TxData.OverrideData.
                                 DoNotFragment is TRUE.)

**/
EFI_STATUS
EFIAPI
EfiIp4Transmit (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token
  );

/**
  Places a receiving request into the receiving queue.

  The Receive() function places a completion token into the receive packet queue.
  This function is always asynchronous.

  The Token.Event field in the completion token must be filled in by the caller
  and cannot be NULL. When the receive operation completes, the EFI IPv4 Protocol
  driver updates the Token.Status and Token.Packet.RxData fields and the Token.Event
  is signaled.

  @param[in]  This  Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  Token Pointer to a token that is associated with the receive data descriptor.

  @retval EFI_SUCCESS           The receive completion token was cached.
  @retval EFI_NOT_STARTED       This EFI IPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP, RARP, etc.)
                                is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                - This is NULL.
                                - Token is NULL.
                                - Token.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES  The receive completion token could not be queued due to a lack of system
                                resources (usually memory).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
                                The EFI IPv4 Protocol instance has been reset to startup defaults.
                                EFI_ACCESS_DENIED The receive completion token with the same Token.Event was already
                                in the receive queue.
  @retval EFI_NOT_READY         The receive request could not be queued because the receive queue is full.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received.

**/
EFI_STATUS
EFIAPI
EfiIp4Receive (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token
  );

/**
  Abort an asynchronous transmit or receive request.

  The Cancel() function is used to abort a pending transmit or receive request.
  If the token is in the transmit or receive request queues, after calling this
  function, Token->Status will be set to EFI_ABORTED and then Token->Event will
  be signaled. If the token is not in one of the queues, which usually means the
  asynchronous operation has completed, this function will not signal the token
  and EFI_NOT_FOUND is returned.

  @param[in]  This  Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  Token Pointer to a token that has been issued by
                    EFI_IP4_PROTOCOL.Transmit() or
                    EFI_IP4_PROTOCOL.Receive(). If NULL, all pending
                    tokens are aborted. Type EFI_IP4_COMPLETION_TOKEN is
                    defined in EFI_IP4_PROTOCOL.Transmit().

  @retval EFI_SUCCESS           The asynchronous I/O request was aborted and
                                Token.->Event was signaled. When Token is NULL, all
                                pending requests were aborted and their events were signaled.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_NOT_STARTED       This instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_NOT_FOUND         When Token is not NULL, the asynchronous I/O request was
                                not found in the transmit or receive queue. It has either completed
                                or was not issued by Transmit() and Receive().

**/
EFI_STATUS
EFIAPI
EfiIp4Cancel (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token    OPTIONAL
  );

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function polls for incoming data packets and processes outgoing data
  packets. Network drivers and applications can call the EFI_IP4_PROTOCOL.Poll()
  function to increase the rate that data packets are moved between the communications
  device and the transmit and receive queues.

  In some systems the periodic timer event may not poll the underlying communications
  device fast enough to transmit and/or receive all data packets without missing
  incoming packets or dropping outgoing packets. Drivers and applications that are
  experiencing packet loss should try calling the EFI_IP4_PROTOCOL.Poll() function
  more often.

  @param[in]  This               Pointer to the EFI_IP4_PROTOCOL instance.

  @retval  EFI_SUCCESS           Incoming or outgoing data was processed.
  @retval  EFI_NOT_STARTED       This EFI IPv4 Protocol instance has not been started.
  @retval  EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval  EFI_INVALID_PARAMETER This is NULL.
  @retval  EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval  EFI_NOT_READY         No incoming or outgoing data is processed.
  @retval  EFI_TIMEOUT           Data was dropped out of the transmit and/or receive queue.
                                 Consider increasing the polling rate.

**/
EFI_STATUS
EFIAPI
EfiIp4Poll (
  IN EFI_IP4_PROTOCOL  *This
  );

EFI_IP4_PROTOCOL
  mEfiIp4ProtocolTemplete = {
  EfiIp4GetModeData,
  EfiIp4Configure,
  EfiIp4Groups,
  EfiIp4Routes,
  EfiIp4Transmit,
  EfiIp4Receive,
  EfiIp4Cancel,
  EfiIp4Poll
};

/**
  Gets the current operational settings for this instance of the EFI IPv4 Protocol driver.

  The GetModeData() function returns the current operational mode data for this
  driver instance. The data fields in EFI_IP4_MODE_DATA are read only. This
  function is used optionally to retrieve the operational mode data of underlying
  networks or drivers.

  @param[in]   This          Pointer to the EFI_IP4_PROTOCOL instance.
  @param[out]  Ip4ModeData   Pointer to the EFI IPv4 Protocol mode data structure.
  @param[out]  MnpConfigData Pointer to the managed network configuration data structure.
  @param[out]  SnpModeData   Pointer to the simple network mode data structure.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_OUT_OF_RESOURCES  The required mode data could not be allocated.

**/
EFI_STATUS
EFIAPI
EfiIp4GetModeData (
  IN  CONST EFI_IP4_PROTOCOL                 *This,
  OUT       EFI_IP4_MODE_DATA                *Ip4ModeData     OPTIONAL,
  OUT       EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData   OPTIONAL,
  OUT       EFI_SIMPLE_NETWORK_MODE          *SnpModeData     OPTIONAL
  )
{
  IP4_PROTOCOL         *IpInstance;
  IP4_SERVICE          *IpSb;
  EFI_IP4_CONFIG_DATA  *Config;
  EFI_STATUS           Status;
  EFI_TPL              OldTpl;
  IP4_ADDR             Ip;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl     = gBS->RaiseTPL (TPL_CALLBACK);
  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;

  if (Ip4ModeData != NULL) {
    //
    // IsStarted is "whether the EfiIp4Configure has been called".
    // IsConfigured is "whether the station address has been configured"
    //
    Ip4ModeData->IsStarted = (BOOLEAN)(IpInstance->State == IP4_STATE_CONFIGED);
    CopyMem (&Ip4ModeData->ConfigData, &IpInstance->ConfigData, sizeof (Ip4ModeData->ConfigData));
    Ip4ModeData->IsConfigured = FALSE;

    Ip4ModeData->GroupCount = IpInstance->GroupCount;
    Ip4ModeData->GroupTable = (EFI_IPv4_ADDRESS *)IpInstance->Groups;

    Ip4ModeData->IcmpTypeCount = 23;
    Ip4ModeData->IcmpTypeList  = mIp4SupportedIcmp;

    Ip4ModeData->RouteTable = NULL;
    Ip4ModeData->RouteCount = 0;

    Ip4ModeData->MaxPacketSize = IpSb->MaxPacketSize;

    //
    // return the current station address for this IP child. So,
    // the user can get the default address through this. Some
    // application wants to know it station address even it is
    // using the default one, such as a ftp server.
    //
    if (Ip4ModeData->IsStarted) {
      Config = &Ip4ModeData->ConfigData;

      Ip = HTONL (IpInstance->Interface->Ip);
      CopyMem (&Config->StationAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

      Ip = HTONL (IpInstance->Interface->SubnetMask);
      CopyMem (&Config->SubnetMask, &Ip, sizeof (EFI_IPv4_ADDRESS));

      Ip4ModeData->IsConfigured = IpInstance->Interface->Configured;

      //
      // Build a EFI route table for user from the internal route table.
      //
      Status = Ip4BuildEfiRouteTable (IpInstance);

      if (EFI_ERROR (Status)) {
        gBS->RestoreTPL (OldTpl);
        return Status;
      }

      Ip4ModeData->RouteTable = IpInstance->EfiRouteTable;
      Ip4ModeData->RouteCount = IpInstance->EfiRouteCount;
    }
  }

  //
  // Get fresh mode data from MNP, since underlying media status may change
  //
  Status = IpSb->Mnp->GetModeData (IpSb->Mnp, MnpConfigData, SnpModeData);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Config the MNP parameter used by IP. The IP driver use one MNP
  child to transmit/receive frames. By default, it configures MNP
  to receive unicast/multicast/broadcast. And it will enable/disable
  the promiscous receive according to whether there is IP child
  enable that or not. If Force is FALSE, it will iterate through
  all the IP children to check whether the promiscuous receive
  setting has been changed. If it hasn't been changed, it won't
  reconfigure the MNP. If Force is TRUE, the MNP is configured no
  matter whether that is changed or not.

  @param[in]  IpSb               The IP4 service instance that is to be changed.
  @param[in]  Force              Force the configuration or not.

  @retval EFI_SUCCESS            The MNP is successfully configured/reconfigured.
  @retval Others                 Configuration failed.

**/
EFI_STATUS
Ip4ServiceConfigMnp (
  IN IP4_SERVICE  *IpSb,
  IN BOOLEAN      Force
  )
{
  LIST_ENTRY     *Entry;
  LIST_ENTRY     *ProtoEntry;
  IP4_INTERFACE  *IpIf;
  IP4_PROTOCOL   *IpInstance;
  BOOLEAN        Reconfig;
  BOOLEAN        PromiscReceive;
  EFI_STATUS     Status;

  Reconfig       = FALSE;
  PromiscReceive = FALSE;

  if (!Force) {
    //
    // Iterate through the IP children to check whether promiscuous
    // receive setting has been changed. Update the interface's receive
    // filter also.
    //
    NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
      IpIf              = NET_LIST_USER_STRUCT (Entry, IP4_INTERFACE, Link);
      IpIf->PromiscRecv = FALSE;

      NET_LIST_FOR_EACH (ProtoEntry, &IpIf->IpInstances) {
        IpInstance = NET_LIST_USER_STRUCT (ProtoEntry, IP4_PROTOCOL, AddrLink);

        if (IpInstance->ConfigData.AcceptPromiscuous) {
          IpIf->PromiscRecv = TRUE;
          PromiscReceive    = TRUE;
        }
      }
    }

    //
    // If promiscuous receive isn't changed, it isn't necessary to reconfigure.
    //
    if (PromiscReceive == IpSb->MnpConfigData.EnablePromiscuousReceive) {
      return EFI_SUCCESS;
    }

    Reconfig                                     = TRUE;
    IpSb->MnpConfigData.EnablePromiscuousReceive = PromiscReceive;
  }

  Status = IpSb->Mnp->Configure (IpSb->Mnp, &IpSb->MnpConfigData);

  //
  // recover the original configuration if failed to set the configure.
  //
  if (EFI_ERROR (Status) && Reconfig) {
    IpSb->MnpConfigData.EnablePromiscuousReceive = (BOOLEAN) !PromiscReceive;
  }

  return Status;
}

/**
  Initialize the IP4_PROTOCOL structure to the unconfigured states.

  @param  IpSb                   The IP4 service instance.
  @param  IpInstance             The IP4 child instance.

**/
VOID
Ip4InitProtocol (
  IN     IP4_SERVICE   *IpSb,
  IN OUT IP4_PROTOCOL  *IpInstance
  )
{
  ASSERT ((IpSb != NULL) && (IpInstance != NULL));

  ZeroMem (IpInstance, sizeof (IP4_PROTOCOL));

  IpInstance->Signature = IP4_PROTOCOL_SIGNATURE;
  CopyMem (&IpInstance->Ip4Proto, &mEfiIp4ProtocolTemplete, sizeof (IpInstance->Ip4Proto));
  IpInstance->State     = IP4_STATE_UNCONFIGED;
  IpInstance->InDestroy = FALSE;
  IpInstance->Service   = IpSb;

  InitializeListHead (&IpInstance->Link);
  NetMapInit (&IpInstance->RxTokens);
  NetMapInit (&IpInstance->TxTokens);
  InitializeListHead (&IpInstance->Received);
  InitializeListHead (&IpInstance->Delivered);
  InitializeListHead (&IpInstance->AddrLink);

  EfiInitializeLock (&IpInstance->RecycleLock, TPL_NOTIFY);
}

/**
  Configure the IP4 child. If the child is already configured,
  change the configuration parameter. Otherwise configure it
  for the first time. The caller should validate the configuration
  before deliver them to it. It also don't do configure NULL.

  @param[in, out]  IpInstance         The IP4 child to configure.
  @param[in]       Config             The configure data.

  @retval EFI_SUCCESS            The IP4 child is successfully configured.
  @retval EFI_DEVICE_ERROR       Failed to free the pending transive or to
                                 configure  underlying MNP or other errors.
  @retval EFI_NO_MAPPING         The IP4 child is configured to use default
                                 address, but the default address hasn't been
                                 configured. The IP4 child doesn't need to be
                                 reconfigured when default address is configured.
  @retval EFI_OUT_OF_RESOURCES   No more memory space is available.
  @retval other                  Other error occurs.

**/
EFI_STATUS
Ip4ConfigProtocol (
  IN OUT IP4_PROTOCOL         *IpInstance,
  IN     EFI_IP4_CONFIG_DATA  *Config
  )
{
  IP4_SERVICE               *IpSb;
  IP4_INTERFACE             *IpIf;
  EFI_STATUS                Status;
  IP4_ADDR                  Ip;
  IP4_ADDR                  Netmask;
  EFI_ARP_PROTOCOL          *Arp;
  EFI_IP4_CONFIG2_PROTOCOL  *Ip4Config2;
  EFI_IP4_CONFIG2_POLICY    Policy;

  IpSb = IpInstance->Service;

  Ip4Config2 = NULL;

  //
  // User is changing packet filters. It must be stopped
  // before the station address can be changed.
  //
  if (IpInstance->State == IP4_STATE_CONFIGED) {
    //
    // Cancel all the pending transmit/receive from upper layer
    //
    Status = Ip4Cancel (IpInstance, NULL);

    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    CopyMem (&IpInstance->ConfigData, Config, sizeof (IpInstance->ConfigData));
    return EFI_SUCCESS;
  }

  //
  // Configure a fresh IP4 protocol instance. Create a route table.
  // Each IP child has its own route table, which may point to the
  // default table if it is using default address.
  //
  Status                 = EFI_OUT_OF_RESOURCES;
  IpInstance->RouteTable = Ip4CreateRouteTable ();

  if (IpInstance->RouteTable == NULL) {
    return Status;
  }

  //
  // Set up the interface.
  //
  CopyMem (&Ip, &Config->StationAddress, sizeof (IP4_ADDR));
  CopyMem (&Netmask, &Config->SubnetMask, sizeof (IP4_ADDR));

  Ip      = NTOHL (Ip);
  Netmask = NTOHL (Netmask);

  if (!Config->UseDefaultAddress) {
    //
    // Find whether there is already an interface with the same
    // station address. All the instances with the same station
    // address shares one interface.
    //
    IpIf = Ip4FindStationAddress (IpSb, Ip, Netmask);

    if (IpIf != NULL) {
      NET_GET_REF (IpIf);
    } else {
      IpIf = Ip4CreateInterface (IpSb->Mnp, IpSb->Controller, IpSb->Image);

      if (IpIf == NULL) {
        goto ON_ERROR;
      }

      Status = Ip4SetAddress (IpIf, Ip, Netmask);

      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        Ip4FreeInterface (IpIf, IpInstance);
        goto ON_ERROR;
      }

      InsertTailList (&IpSb->Interfaces, &IpIf->Link);
    }

    //
    // Add a route to this connected network in the instance route table.
    //
    Ip4AddRoute (
      IpInstance->RouteTable,
      Ip & Netmask,
      Netmask,
      IP4_ALLZERO_ADDRESS
      );
  } else {
    //
    // Use the default address. Check the state.
    //
    if (IpSb->State == IP4_SERVICE_UNSTARTED) {
      //
      // Trigger the EFI_IP4_CONFIG2_PROTOCOL to retrieve the
      // default IPv4 address if it is not available yet.
      //
      Policy = IpSb->Ip4Config2Instance.Policy;
      if (Policy != Ip4Config2PolicyDhcp) {
        Ip4Config2 = &IpSb->Ip4Config2Instance.Ip4Config2;
        Policy     = Ip4Config2PolicyDhcp;
        Status     = Ip4Config2->SetData (
                                   Ip4Config2,
                                   Ip4Config2DataTypePolicy,
                                   sizeof (EFI_IP4_CONFIG2_POLICY),
                                   &Policy
                                   );
        if (EFI_ERROR (Status)) {
          goto ON_ERROR;
        }
      }
    }

    IpIf = IpSb->DefaultInterface;
    NET_GET_REF (IpSb->DefaultInterface);

    //
    // If default address is used, so is the default route table.
    // Any route set by the instance has the precedence over the
    // routes in the default route table. Link the default table
    // after the instance's table. Routing will search the local
    // table first.
    //
    NET_GET_REF (IpSb->DefaultRouteTable);
    IpInstance->RouteTable->Next = IpSb->DefaultRouteTable;
  }

  IpInstance->Interface = IpIf;
  if (IpIf->Arp != NULL) {
    Arp    = NULL;
    Status = gBS->OpenProtocol (
                    IpIf->ArpHandle,
                    &gEfiArpProtocolGuid,
                    (VOID **)&Arp,
                    gIp4DriverBinding.DriverBindingHandle,
                    IpInstance->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (EFI_ERROR (Status)) {
      Ip4FreeInterface (IpIf, IpInstance);
      goto ON_ERROR;
    }
  }

  InsertTailList (&IpIf->IpInstances, &IpInstance->AddrLink);

  CopyMem (&IpInstance->ConfigData, Config, sizeof (IpInstance->ConfigData));
  IpInstance->State = IP4_STATE_CONFIGED;

  //
  // Although EFI_NO_MAPPING is an error code, the IP child has been
  // successfully configured and doesn't need reconfiguration when
  // default address is acquired.
  //
  if (Config->UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    return EFI_NO_MAPPING;
  }

  return EFI_SUCCESS;

ON_ERROR:
  Ip4FreeRouteTable (IpInstance->RouteTable);
  IpInstance->RouteTable = NULL;
  return Status;
}

/**
  Clean up the IP4 child, release all the resources used by it.

  @param[in]  IpInstance         The IP4 child to clean up.

  @retval EFI_SUCCESS            The IP4 child is cleaned up.
  @retval EFI_DEVICE_ERROR       Some resources failed to be released.

**/
EFI_STATUS
Ip4CleanProtocol (
  IN  IP4_PROTOCOL  *IpInstance
  )
{
  if (EFI_ERROR (Ip4Cancel (IpInstance, NULL))) {
    return EFI_DEVICE_ERROR;
  }

  if (EFI_ERROR (Ip4Groups (IpInstance, FALSE, NULL))) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Some packets haven't been recycled. It is because either the
  // user forgets to recycle the packets, or because the callback
  // hasn't been called. Just leave it alone.
  //
  if (!IsListEmpty (&IpInstance->Delivered)) {
  }

  if (IpInstance->Interface != NULL) {
    RemoveEntryList (&IpInstance->AddrLink);
    if (IpInstance->Interface->Arp != NULL) {
      gBS->CloseProtocol (
             IpInstance->Interface->ArpHandle,
             &gEfiArpProtocolGuid,
             gIp4DriverBinding.DriverBindingHandle,
             IpInstance->Handle
             );
    }

    Ip4FreeInterface (IpInstance->Interface, IpInstance);
    IpInstance->Interface = NULL;
  }

  if (IpInstance->RouteTable != NULL) {
    if (IpInstance->RouteTable->Next != NULL) {
      Ip4FreeRouteTable (IpInstance->RouteTable->Next);
    }

    Ip4FreeRouteTable (IpInstance->RouteTable);
    IpInstance->RouteTable = NULL;
  }

  if (IpInstance->EfiRouteTable != NULL) {
    FreePool (IpInstance->EfiRouteTable);
    IpInstance->EfiRouteTable = NULL;
    IpInstance->EfiRouteCount = 0;
  }

  if (IpInstance->Groups != NULL) {
    FreePool (IpInstance->Groups);
    IpInstance->Groups     = NULL;
    IpInstance->GroupCount = 0;
  }

  NetMapClean (&IpInstance->TxTokens);

  NetMapClean (&IpInstance->RxTokens);

  return EFI_SUCCESS;
}

/**
  Assigns an IPv4 address and subnet mask to this EFI IPv4 Protocol driver instance.

  The Configure() function is used to set, change, or reset the operational
  parameters and filter settings for this EFI IPv4 Protocol instance. Until these
  parameters have been set, no network traffic can be sent or received by this
  instance. Once the parameters have been reset (by calling this function with
  IpConfigData set to NULL), no more traffic can be sent or received until these
  parameters have been set again. Each EFI IPv4 Protocol instance can be started
  and stopped independently of each other by enabling or disabling their receive
  filter settings with the Configure() function.

  When IpConfigData.UseDefaultAddress is set to FALSE, the new station address will
  be appended as an alias address into the addresses list in the EFI IPv4 Protocol
  driver. While set to TRUE, Configure() will trigger the EFI_IP4_CONFIG_PROTOCOL
  to retrieve the default IPv4 address if it is not available yet. Clients could
  frequently call GetModeData() to check the status to ensure that the default IPv4
  address is ready.

  If operational parameters are reset or changed, any pending transmit and receive
  requests will be cancelled. Their completion token status will be set to EFI_ABORTED
  and their events will be signaled.

  @param[in]  This              Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  IpConfigData      Pointer to the EFI IPv4 Protocol configuration data structure.

  @retval EFI_SUCCESS           The driver instance was successfully opened.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
  @retval EFI_UNSUPPORTED       One or more of the following conditions is TRUE:
                                A configuration protocol (DHCP, BOOTP, RARP, etc.) could
                                not be located when clients choose to use the default IPv4
                                address. This EFI IPv4 Protocol implementation does not
                                support this requested filter or timeout setting.
  @retval EFI_OUT_OF_RESOURCES  The EFI IPv4 Protocol driver instance data could not be allocated.
  @retval EFI_ALREADY_STARTED   The interface is already open and must be stopped before the
                                IPv4 address or subnet mask can be changed. The interface must
                                also be stopped when switching to/from raw packet mode.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred. The EFI IPv4
                                Protocol driver instance is not opened.

**/
EFI_STATUS
EFIAPI
EfiIp4Configure (
  IN EFI_IP4_PROTOCOL     *This,
  IN EFI_IP4_CONFIG_DATA  *IpConfigData       OPTIONAL
  )
{
  IP4_PROTOCOL         *IpInstance;
  EFI_IP4_CONFIG_DATA  *Current;
  EFI_TPL              OldTpl;
  EFI_STATUS           Status;
  BOOLEAN              AddrOk;
  IP4_ADDR             IpAddress;
  IP4_ADDR             SubnetMask;

  //
  // First, validate the parameters
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);
  OldTpl     = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Validate the configuration first.
  //
  if (IpConfigData != NULL) {
    CopyMem (&IpAddress, &IpConfigData->StationAddress, sizeof (IP4_ADDR));
    CopyMem (&SubnetMask, &IpConfigData->SubnetMask, sizeof (IP4_ADDR));

    IpAddress  = NTOHL (IpAddress);
    SubnetMask = NTOHL (SubnetMask);

    //
    // Check whether the station address is a valid unicast address
    //
    if (!IpConfigData->UseDefaultAddress) {
      AddrOk = Ip4StationAddressValid (IpAddress, SubnetMask);

      if (!AddrOk) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }
    }

    //
    // User can only update packet filters when already configured.
    // If it wants to change the station address, it must configure(NULL)
    // the instance first.
    //
    if (IpInstance->State == IP4_STATE_CONFIGED) {
      Current = &IpInstance->ConfigData;

      if (Current->UseDefaultAddress != IpConfigData->UseDefaultAddress) {
        Status = EFI_ALREADY_STARTED;
        goto ON_EXIT;
      }

      if (!Current->UseDefaultAddress &&
          (!EFI_IP4_EQUAL (&Current->StationAddress, &IpConfigData->StationAddress) ||
           !EFI_IP4_EQUAL (&Current->SubnetMask, &IpConfigData->SubnetMask)))
      {
        Status = EFI_ALREADY_STARTED;
        goto ON_EXIT;
      }

      if (Current->UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
        Status = EFI_NO_MAPPING;
        goto ON_EXIT;
      }
    }
  }

  //
  // Configure the instance or clean it up.
  //
  if (IpConfigData != NULL) {
    Status = Ip4ConfigProtocol (IpInstance, IpConfigData);
  } else {
    Status = Ip4CleanProtocol (IpInstance);

    //
    // Consider the following valid sequence: Mnp is unloaded-->Ip Stopped-->Udp Stopped,
    // Configure (ThisIp, NULL). If the state is changed to UNCONFIGED,
    // the unload fails miserably.
    //
    if (IpInstance->State == IP4_STATE_CONFIGED) {
      IpInstance->State = IP4_STATE_UNCONFIGED;
    }
  }

  //
  // Update the MNP's configure data. Ip4ServiceConfigMnp will check
  // whether it is necessary to reconfigure the MNP.
  //
  Ip4ServiceConfigMnp (IpInstance->Service, FALSE);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Change the IP4 child's multicast setting. The caller
  should make sure that the parameters is valid.

  @param[in]  IpInstance             The IP4 child to change the setting.
  @param[in]  JoinFlag               TRUE to join the group, otherwise leave it.
  @param[in]  GroupAddress           The target group address.

  @retval EFI_ALREADY_STARTED    Want to join the group, but already a member of it.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate some resources.
  @retval EFI_DEVICE_ERROR       Failed to set the group configuration.
  @retval EFI_SUCCESS            Successfully updated the group setting.
  @retval EFI_NOT_FOUND          Try to leave the group which it isn't a member.

**/
EFI_STATUS
Ip4Groups (
  IN IP4_PROTOCOL      *IpInstance,
  IN BOOLEAN           JoinFlag,
  IN EFI_IPv4_ADDRESS  *GroupAddress       OPTIONAL
  )
{
  IP4_ADDR  *Members;
  IP4_ADDR  Group;
  UINT32    Index;

  //
  // Add it to the instance's Groups, and join the group by IGMP.
  // IpInstance->Groups is in network byte order. IGMP operates in
  // host byte order
  //
  if (JoinFlag) {
    //
    // When JoinFlag is TRUE, GroupAddress shouldn't be NULL.
    //
    ASSERT (GroupAddress != NULL);
    CopyMem (&Group, GroupAddress, sizeof (IP4_ADDR));

    for (Index = 0; Index < IpInstance->GroupCount; Index++) {
      if (IpInstance->Groups[Index] == Group) {
        return EFI_ALREADY_STARTED;
      }
    }

    Members = Ip4CombineGroups (IpInstance->Groups, IpInstance->GroupCount, Group);

    if (Members == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (EFI_ERROR (Ip4JoinGroup (IpInstance, NTOHL (Group)))) {
      FreePool (Members);
      return EFI_DEVICE_ERROR;
    }

    if (IpInstance->Groups != NULL) {
      FreePool (IpInstance->Groups);
    }

    IpInstance->Groups = Members;
    IpInstance->GroupCount++;

    return EFI_SUCCESS;
  }

  //
  // Leave the group. Leave all the groups if GroupAddress is NULL.
  // Must iterate from the end to the beginning because the GroupCount
  // is decremented each time an address is removed..
  //
  for (Index = IpInstance->GroupCount; Index > 0; Index--) {
    ASSERT (IpInstance->Groups != NULL);
    Group = IpInstance->Groups[Index - 1];
    if ((GroupAddress == NULL) || EFI_IP4_EQUAL (&Group, GroupAddress)) {
      if (EFI_ERROR (Ip4LeaveGroup (IpInstance, NTOHL (Group)))) {
        return EFI_DEVICE_ERROR;
      }

      Ip4RemoveGroupAddr (IpInstance->Groups, IpInstance->GroupCount, Group);
      IpInstance->GroupCount--;

      if (IpInstance->GroupCount == 0) {
        ASSERT (Index == 1);

        FreePool (IpInstance->Groups);
        IpInstance->Groups = NULL;
      }

      if (GroupAddress != NULL) {
        return EFI_SUCCESS;
      }
    }
  }

  return ((GroupAddress != NULL) ? EFI_NOT_FOUND : EFI_SUCCESS);
}

/**
  Joins and leaves multicast groups.

  The Groups() function is used to join and leave multicast group sessions. Joining
  a group will enable reception of matching multicast packets. Leaving a group will
  disable the multicast packet reception.

  If JoinFlag is FALSE and GroupAddress is NULL, all joined groups will be left.

  @param[in]  This                  Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  JoinFlag              Set to TRUE to join the multicast group session and FALSE to leave.
  @param[in]  GroupAddress          Pointer to the IPv4 multicast address.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following is TRUE:
                                - This is NULL.
                                - JoinFlag is TRUE and GroupAddress is NULL.
                                - GroupAddress is not NULL and *GroupAddress is
                                not a multicast IPv4 address.
  @retval EFI_NOT_STARTED       This instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_OUT_OF_RESOURCES  System resources could not be allocated.
  @retval EFI_UNSUPPORTED       This EFI IPv4 Protocol implementation does not support multicast groups.
  @retval EFI_ALREADY_STARTED   The group address is already in the group table (when
                                JoinFlag is TRUE).
  @retval EFI_NOT_FOUND         The group address is not in the group table (when JoinFlag is FALSE).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiIp4Groups (
  IN EFI_IP4_PROTOCOL  *This,
  IN BOOLEAN           JoinFlag,
  IN EFI_IPv4_ADDRESS  *GroupAddress     OPTIONAL
  )
{
  IP4_PROTOCOL  *IpInstance;
  EFI_STATUS    Status;
  EFI_TPL       OldTpl;
  IP4_ADDR      McastIp;

  if ((This == NULL) || (JoinFlag && (GroupAddress == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if (GroupAddress != NULL) {
    CopyMem (&McastIp, GroupAddress, sizeof (IP4_ADDR));

    if (!IP4_IS_MULTICAST (NTOHL (McastIp))) {
      return EFI_INVALID_PARAMETER;
    }
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);
  OldTpl     = gBS->RaiseTPL (TPL_CALLBACK);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (IpInstance->ConfigData.UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto ON_EXIT;
  }

  Status = Ip4Groups (IpInstance, JoinFlag, GroupAddress);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Adds and deletes routing table entries.

  The Routes() function adds a route to or deletes a route from the routing table.

  Routes are determined by comparing the SubnetAddress with the destination IPv4
  address arithmetically AND-ed with the SubnetMask. The gateway address must be
  on the same subnet as the configured station address.

  The default route is added with SubnetAddress and SubnetMask both set to 0.0.0.0.
  The default route matches all destination IPv4 addresses that do not match any
  other routes.

  A GatewayAddress that is zero is a nonroute. Packets are sent to the destination
  IP address if it can be found in the ARP cache or on the local subnet. One automatic
  nonroute entry will be inserted into the routing table for outgoing packets that
  are addressed to a local subnet (gateway address of 0.0.0.0).

  Each EFI IPv4 Protocol instance has its own independent routing table. Those EFI
  IPv4 Protocol instances that use the default IPv4 address will also have copies
  of the routing table that was provided by the EFI_IP4_CONFIG_PROTOCOL, and these
  copies will be updated whenever the EIF IPv4 Protocol driver reconfigures its
  instances. As a result, client modification to the routing table will be lost.

  @param[in]  This                   Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  DeleteRoute            Set to TRUE to delete this route from the routing table. Set to
                                     FALSE to add this route to the routing table. SubnetAddress
                                     and SubnetMask are used as the key to each route entry.
  @param[in]  SubnetAddress          The address of the subnet that needs to be routed.
  @param[in]  SubnetMask             The subnet mask of SubnetAddress.
  @param[in]  GatewayAddress         The unicast gateway IPv4 address for this route.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_NOT_STARTED        The driver instance has not been started.
  @retval EFI_NO_MAPPING         When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - SubnetAddress is NULL.
                                 - SubnetMask is NULL.
                                 - GatewayAddress is NULL.
                                 - *SubnetAddress is not a valid subnet address.
                                 - *SubnetMask is not a valid subnet mask.
                                 - *GatewayAddress is not a valid unicast IPv4 address.
  @retval EFI_OUT_OF_RESOURCES   Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND          This route is not in the routing table (when DeleteRoute is TRUE).
  @retval EFI_ACCESS_DENIED      The route is already defined in the routing table (when
                                  DeleteRoute is FALSE).

**/
EFI_STATUS
EFIAPI
EfiIp4Routes (
  IN EFI_IP4_PROTOCOL  *This,
  IN BOOLEAN           DeleteRoute,
  IN EFI_IPv4_ADDRESS  *SubnetAddress,
  IN EFI_IPv4_ADDRESS  *SubnetMask,
  IN EFI_IPv4_ADDRESS  *GatewayAddress
  )
{
  IP4_PROTOCOL   *IpInstance;
  IP4_INTERFACE  *IpIf;
  IP4_ADDR       Dest;
  IP4_ADDR       Netmask;
  IP4_ADDR       Nexthop;
  EFI_STATUS     Status;
  EFI_TPL        OldTpl;

  //
  // First, validate the parameters
  //
  if ((This == NULL) || (SubnetAddress == NULL) ||
      (SubnetMask == NULL) || (GatewayAddress == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);
  OldTpl     = gBS->RaiseTPL (TPL_CALLBACK);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (IpInstance->ConfigData.UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto ON_EXIT;
  }

  CopyMem (&Dest, SubnetAddress, sizeof (IP4_ADDR));
  CopyMem (&Netmask, SubnetMask, sizeof (IP4_ADDR));
  CopyMem (&Nexthop, GatewayAddress, sizeof (IP4_ADDR));

  Dest    = NTOHL (Dest);
  Netmask = NTOHL (Netmask);
  Nexthop = NTOHL (Nexthop);

  IpIf = IpInstance->Interface;

  if (!IP4_IS_VALID_NETMASK (Netmask)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // the gateway address must be a unicast on the connected network if not zero.
  //
  if ((Nexthop != IP4_ALLZERO_ADDRESS) &&
      (((IpIf->SubnetMask != IP4_ALLONE_ADDRESS) && !IP4_NET_EQUAL (Nexthop, IpIf->Ip, IpIf->SubnetMask)) ||
       IP4_IS_BROADCAST (Ip4GetNetCast (Nexthop, IpIf))))
  {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if (DeleteRoute) {
    Status = Ip4DelRoute (IpInstance->RouteTable, Dest, Netmask, Nexthop);
  } else {
    Status = Ip4AddRoute (IpInstance->RouteTable, Dest, Netmask, Nexthop);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Check whether the user's token or event has already
  been enqueued on IP4's list.

  @param[in]  Map                    The container of either user's transmit or receive
                                     token.
  @param[in]  Item                   Current item to check against.
  @param[in]  Context                The Token to check against.

  @retval EFI_ACCESS_DENIED      The token or event has already been enqueued in IP.
  @retval EFI_SUCCESS            The current item isn't the same token/event as the
                                 context.

**/
EFI_STATUS
EFIAPI
Ip4TokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  EFI_IP4_COMPLETION_TOKEN  *Token;
  EFI_IP4_COMPLETION_TOKEN  *TokenInItem;

  Token       = (EFI_IP4_COMPLETION_TOKEN *)Context;
  TokenInItem = (EFI_IP4_COMPLETION_TOKEN *)Item->Key;

  if ((Token == TokenInItem) || (Token->Event == TokenInItem->Event)) {
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}

/**
  Validate the user's token against current station address.

  @param[in]  Token              User's token to validate.
  @param[in]  IpIf               The IP4 child's interface.
  @param[in]  RawData            Set to TRUE to send unformatted packets.

  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_BAD_BUFFER_SIZE    The user's option/data is too long.
  @retval EFI_SUCCESS            The token is valid.

**/
EFI_STATUS
Ip4TxTokenValid (
  IN EFI_IP4_COMPLETION_TOKEN  *Token,
  IN IP4_INTERFACE             *IpIf,
  IN BOOLEAN                   RawData
  )
{
  EFI_IP4_TRANSMIT_DATA  *TxData;
  EFI_IP4_OVERRIDE_DATA  *Override;
  IP4_ADDR               Src;
  IP4_ADDR               Gateway;
  UINT32                 Offset;
  UINT32                 Index;
  UINT32                 HeadLen;

  if ((Token == NULL) || (Token->Event == NULL) || (Token->Packet.TxData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TxData = Token->Packet.TxData;

  //
  // Check the fragment table: no empty fragment, and length isn't bogus.
  //
  if ((TxData->TotalDataLength == 0) || (TxData->FragmentCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = TxData->TotalDataLength;

  if (Offset > IP4_MAX_PACKET_SIZE) {
    return EFI_BAD_BUFFER_SIZE;
  }

  for (Index = 0; Index < TxData->FragmentCount; Index++) {
    if ((TxData->FragmentTable[Index].FragmentBuffer == NULL) ||
        (TxData->FragmentTable[Index].FragmentLength == 0))
    {
      return EFI_INVALID_PARAMETER;
    }

    Offset -= TxData->FragmentTable[Index].FragmentLength;
  }

  if (Offset != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // NOTE that OptionsLength/OptionsBuffer/OverrideData are ignored if RawData
  // is TRUE.
  //
  if (RawData) {
    return EFI_SUCCESS;
  }

  //
  // Check the IP options: no more than 40 bytes and format is OK
  //
  if (TxData->OptionsLength != 0) {
    if ((TxData->OptionsLength > 40) || (TxData->OptionsBuffer == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if (!Ip4OptionIsValid (TxData->OptionsBuffer, TxData->OptionsLength, FALSE)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Check the source and gateway: they must be a valid unicast.
  // Gateway must also be on the connected network.
  //
  if (TxData->OverrideData != NULL) {
    Override = TxData->OverrideData;

    CopyMem (&Src, &Override->SourceAddress, sizeof (IP4_ADDR));
    CopyMem (&Gateway, &Override->GatewayAddress, sizeof (IP4_ADDR));

    Src     = NTOHL (Src);
    Gateway = NTOHL (Gateway);

    if ((NetGetIpClass (Src) > IP4_ADDR_CLASSC) ||
        (Src == IP4_ALLONE_ADDRESS) ||
        IP4_IS_BROADCAST (Ip4GetNetCast (Src, IpIf)))
    {
      return EFI_INVALID_PARAMETER;
    }

    //
    // If gateway isn't zero, it must be a unicast address, and
    // on the connected network.
    //
    if ((Gateway != IP4_ALLZERO_ADDRESS) &&
        ((NetGetIpClass (Gateway) > IP4_ADDR_CLASSC) ||
         !IP4_NET_EQUAL (Gateway, IpIf->Ip, IpIf->SubnetMask) ||
         IP4_IS_BROADCAST (Ip4GetNetCast (Gateway, IpIf))))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Check the packet length: Head length and packet length all has a limit
  //
  HeadLen = sizeof (IP4_HEAD) + ((TxData->OptionsLength + 3) &~0x03);

  if ((HeadLen > IP4_MAX_HEADLEN) ||
      (TxData->TotalDataLength + HeadLen > IP4_MAX_PACKET_SIZE))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}

/**
  The callback function for the net buffer which wraps the user's
  transmit token. Although it seems this function is pretty simple,
  there are some subtle things.
  When user requests the IP to transmit a packet by passing it a
  token, the token is wrapped in an IP4_TXTOKEN_WRAP and the data
  is wrapped in an net buffer. the net buffer's Free function is
  set to Ip4FreeTxToken. The Token and token wrap are added to the
  IP child's TxToken map. Then the buffer is passed to Ip4Output for
  transmission. If something error happened before that, the buffer
  is freed, which in turn will free the token wrap. The wrap may
  have been added to the TxToken map or not, and the user's event
  shouldn't be fired because we are still in the EfiIp4Transmit. If
  the buffer has been sent by Ip4Output, it should be removed from
  the TxToken map and user's event signaled. The token wrap and buffer
  are bound together. Check the comments in Ip4Output for information
  about IP fragmentation.

  @param[in]  Context                The token's wrap.

**/
VOID
EFIAPI
Ip4FreeTxToken (
  IN VOID  *Context
  )
{
  IP4_TXTOKEN_WRAP  *Wrap;
  NET_MAP_ITEM      *Item;

  Wrap = (IP4_TXTOKEN_WRAP *)Context;

  //
  // Signal IpSecRecycleEvent to inform IPsec free the memory
  //
  if (Wrap->IpSecRecycleSignal != NULL) {
    gBS->SignalEvent (Wrap->IpSecRecycleSignal);
  }

  //
  // Find the token in the instance's map. EfiIp4Transmit put the
  // token to the map. If that failed, NetMapFindKey will return NULL.
  //
  Item = NetMapFindKey (&Wrap->IpInstance->TxTokens, Wrap->Token);

  if (Item != NULL) {
    NetMapRemoveItem (&Wrap->IpInstance->TxTokens, Item, NULL);
  }

  if (Wrap->Sent) {
    gBS->SignalEvent (Wrap->Token->Event);

    //
    // Dispatch the DPC queued by the NotifyFunction of Token->Event.
    //
    DispatchDpc ();
  }

  FreePool (Wrap);
}

/**
  The callback function to Ip4Output to update the transmit status.

  @param  Ip4Instance            The Ip4Instance that request the transmit.
  @param  Packet                 The user's transmit request.
  @param  IoStatus               The result of the transmission.
  @param  Flag                   Not used during transmission.
  @param  Context                The token's wrap.

**/
VOID
Ip4OnPacketSent (
  IP4_PROTOCOL  *Ip4Instance,
  NET_BUF       *Packet,
  EFI_STATUS    IoStatus,
  UINT32        Flag,
  VOID          *Context
  )
{
  IP4_TXTOKEN_WRAP  *Wrap;

  //
  // This is the transmission request from upper layer,
  // not the IP4 driver itself.
  //
  ASSERT (Ip4Instance != NULL);

  //
  // The first fragment of the packet has been sent. Update
  // the token's status. That is, if fragmented, the transmit's
  // status is the first fragment's status. The Wrap will be
  // release when all the fragments are release. Check the comments
  // in Ip4FreeTxToken and Ip4Output for information.
  //
  Wrap                = (IP4_TXTOKEN_WRAP *)Context;
  Wrap->Token->Status = IoStatus;

  NetbufFree (Wrap->Packet);
}

/**
  Places outgoing data packets into the transmit queue.

  The Transmit() function places a sending request in the transmit queue of this
  EFI IPv4 Protocol instance. Whenever the packet in the token is sent out or some
  errors occur, the event in the token will be signaled and the status is updated.

  @param[in]  This  Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  Token Pointer to the transmit token.

  @retval  EFI_SUCCESS           The data has been queued for transmission.
  @retval  EFI_NOT_STARTED       This instance has not been started.
  @retval  EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval  EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval  EFI_ACCESS_DENIED     The transmit completion token with the same Token.Event
                                 was already in the transmit queue.
  @retval  EFI_NOT_READY         The completion token could not be queued because the transmit
                                 queue is full.
  @retval  EFI_NOT_FOUND         Not route is found to destination address.
  @retval  EFI_OUT_OF_RESOURCES  Could not queue the transmit data.
  @retval  EFI_BUFFER_TOO_SMALL  Token.Packet.TxData.TotalDataLength is too
                                 short to transmit.
  @retval  EFI_BAD_BUFFER_SIZE   The length of the IPv4 header + option length + total data length is
                                 greater than MTU (or greater than the maximum packet size if
                                 Token.Packet.TxData.OverrideData.
                                 DoNotFragment is TRUE).

**/
EFI_STATUS
EFIAPI
EfiIp4Transmit (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token
  )
{
  IP4_SERVICE            *IpSb;
  IP4_PROTOCOL           *IpInstance;
  IP4_INTERFACE          *IpIf;
  IP4_TXTOKEN_WRAP       *Wrap;
  EFI_IP4_TRANSMIT_DATA  *TxData;
  EFI_IP4_CONFIG_DATA    *Config;
  EFI_IP4_OVERRIDE_DATA  *Override;
  IP4_HEAD               Head;
  IP4_ADDR               GateWay;
  EFI_STATUS             Status;
  EFI_TPL                OldTpl;
  BOOLEAN                DontFragment;
  UINT32                 HeadLen;
  UINT8                  RawHdrLen;
  UINT32                 OptionsLength;
  UINT8                  *OptionsBuffer;
  VOID                   *FirstFragment;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  IpSb   = IpInstance->Service;
  IpIf   = IpInstance->Interface;
  Config = &IpInstance->ConfigData;

  if (Config->UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto ON_EXIT;
  }

  //
  // make sure that token is properly formatted
  //
  Status = Ip4TxTokenValid (Token, IpIf, Config->RawData);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Check whether the token or signal already existed.
  //
  if (EFI_ERROR (NetMapIterate (&IpInstance->TxTokens, Ip4TokenExist, Token))) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Build the IP header, need to fill in the Tos, TotalLen, Id,
  // fragment, Ttl, protocol, Src, and Dst.
  //
  TxData = Token->Packet.TxData;

  FirstFragment = NULL;

  if (Config->RawData) {
    //
    // When RawData is TRUE, first buffer in FragmentTable points to a raw
    // IPv4 fragment including IPv4 header and options.
    //
    FirstFragment = TxData->FragmentTable[0].FragmentBuffer;
    CopyMem (&RawHdrLen, FirstFragment, sizeof (UINT8));

    RawHdrLen = (UINT8)(RawHdrLen & 0x0f);
    if (RawHdrLen < 5) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    RawHdrLen = (UINT8)(RawHdrLen << 2);

    CopyMem (&Head, FirstFragment, IP4_MIN_HEADLEN);

    Ip4NtohHead (&Head);
    HeadLen      = 0;
    DontFragment = IP4_DO_NOT_FRAGMENT (Head.Fragment);

    if (!DontFragment) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    GateWay = IP4_ALLZERO_ADDRESS;

    //
    // Get IPv4 options from first fragment.
    //
    if (RawHdrLen == IP4_MIN_HEADLEN) {
      OptionsLength = 0;
      OptionsBuffer = NULL;
    } else {
      OptionsLength = RawHdrLen - IP4_MIN_HEADLEN;
      OptionsBuffer = (UINT8 *)FirstFragment + IP4_MIN_HEADLEN;
    }

    //
    // Trim off IPv4 header and options from first fragment.
    //
    TxData->FragmentTable[0].FragmentBuffer = (UINT8 *)FirstFragment + RawHdrLen;
    TxData->FragmentTable[0].FragmentLength = TxData->FragmentTable[0].FragmentLength - RawHdrLen;
  } else {
    CopyMem (&Head.Dst, &TxData->DestinationAddress, sizeof (IP4_ADDR));
    Head.Dst = NTOHL (Head.Dst);

    if (TxData->OverrideData != NULL) {
      Override      = TxData->OverrideData;
      Head.Protocol = Override->Protocol;
      Head.Tos      = Override->TypeOfService;
      Head.Ttl      = Override->TimeToLive;
      DontFragment  = Override->DoNotFragment;

      CopyMem (&Head.Src, &Override->SourceAddress, sizeof (IP4_ADDR));
      CopyMem (&GateWay, &Override->GatewayAddress, sizeof (IP4_ADDR));

      Head.Src = NTOHL (Head.Src);
      GateWay  = NTOHL (GateWay);
    } else {
      Head.Src      = IpIf->Ip;
      GateWay       = IP4_ALLZERO_ADDRESS;
      Head.Protocol = Config->DefaultProtocol;
      Head.Tos      = Config->TypeOfService;
      Head.Ttl      = Config->TimeToLive;
      DontFragment  = Config->DoNotFragment;
    }

    Head.Fragment = IP4_HEAD_FRAGMENT_FIELD (DontFragment, FALSE, 0);
    HeadLen       = (TxData->OptionsLength + 3) & (~0x03);

    OptionsLength = TxData->OptionsLength;
    OptionsBuffer = (UINT8 *)(TxData->OptionsBuffer);
  }

  //
  // If don't fragment and fragment needed, return error
  //
  if (DontFragment && (TxData->TotalDataLength + HeadLen > IpSb->MaxPacketSize)) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto ON_EXIT;
  }

  //
  // OK, it survives all the validation check. Wrap the token in
  // a IP4_TXTOKEN_WRAP and the data in a netbuf
  //
  Status = EFI_OUT_OF_RESOURCES;
  Wrap   = AllocateZeroPool (sizeof (IP4_TXTOKEN_WRAP));
  if (Wrap == NULL) {
    goto ON_EXIT;
  }

  Wrap->IpInstance = IpInstance;
  Wrap->Token      = Token;
  Wrap->Sent       = FALSE;
  Wrap->Life       = IP4_US_TO_SEC (Config->TransmitTimeout);
  Wrap->Packet     = NetbufFromExt (
                       (NET_FRAGMENT *)TxData->FragmentTable,
                       TxData->FragmentCount,
                       IP4_MAX_HEADLEN,
                       0,
                       Ip4FreeTxToken,
                       Wrap
                       );

  if (Wrap->Packet == NULL) {
    FreePool (Wrap);
    goto ON_EXIT;
  }

  Token->Status = EFI_NOT_READY;

  if (EFI_ERROR (NetMapInsertTail (&IpInstance->TxTokens, Token, Wrap))) {
    //
    // NetbufFree will call Ip4FreeTxToken, which in turn will
    // free the IP4_TXTOKEN_WRAP. Now, the token wrap hasn't been
    // enqueued.
    //
    if (Config->RawData) {
      //
      // Restore pointer of first fragment in RawData mode.
      //
      TxData->FragmentTable[0].FragmentBuffer = (UINT8 *)FirstFragment;
    }

    NetbufFree (Wrap->Packet);
    goto ON_EXIT;
  }

  //
  // Mark the packet sent before output it. Mark it not sent again if the
  // returned status is not EFI_SUCCESS;
  //
  Wrap->Sent = TRUE;

  Status = Ip4Output (
             IpSb,
             IpInstance,
             Wrap->Packet,
             &Head,
             OptionsBuffer,
             OptionsLength,
             GateWay,
             Ip4OnPacketSent,
             Wrap
             );

  if (EFI_ERROR (Status)) {
    Wrap->Sent = FALSE;

    if (Config->RawData) {
      //
      // Restore pointer of first fragment in RawData mode.
      //
      TxData->FragmentTable[0].FragmentBuffer = (UINT8 *)FirstFragment;
    }

    NetbufFree (Wrap->Packet);
  }

  if (Config->RawData) {
    //
    // Restore pointer of first fragment in RawData mode.
    //
    TxData->FragmentTable[0].FragmentBuffer = (UINT8 *)FirstFragment;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Places a receiving request into the receiving queue.

  The Receive() function places a completion token into the receive packet queue.
  This function is always asynchronous.

  The Token.Event field in the completion token must be filled in by the caller
  and cannot be NULL. When the receive operation completes, the EFI IPv4 Protocol
  driver updates the Token.Status and Token.Packet.RxData fields and the Token.Event
  is signaled.

  @param[in]  This  Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  Token Pointer to a token that is associated with the receive data descriptor.

  @retval EFI_SUCCESS           The receive completion token was cached.
  @retval EFI_NOT_STARTED       This EFI IPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP, RARP, etc.)
                                is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                - This is NULL.
                                - Token is NULL.
                                - Token.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES  The receive completion token could not be queued due to a lack of system
                                resources (usually memory).
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
                                The EFI IPv4 Protocol instance has been reset to startup defaults.
                                EFI_ACCESS_DENIED The receive completion token with the same Token.Event was already
                                in the receive queue.
  @retval EFI_NOT_READY         The receive request could not be queued because the receive queue is full.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received.

**/
EFI_STATUS
EFIAPI
EfiIp4Receive (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token
  )
{
  IP4_PROTOCOL  *IpInstance;
  EFI_STATUS    Status;
  EFI_TPL       OldTpl;

  //
  // First validate the parameters
  //
  if ((This == NULL) || (Token == NULL) || (Token->Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  //
  // Check whether the toke is already on the receive queue.
  //
  Status = NetMapIterate (&IpInstance->RxTokens, Ip4TokenExist, Token);

  if (EFI_ERROR (Status)) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Queue the token then check whether there is pending received packet.
  //
  Status = NetMapInsertTail (&IpInstance->RxTokens, Token, NULL);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = Ip4InstanceDeliverPacket (IpInstance);

  //
  // Dispatch the DPC queued by the NotifyFunction of this instane's receive
  // event.
  //
  DispatchDpc ();

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Cancel the transmitted but not recycled packet. If a matching
  token is found, it will call Ip4CancelPacket to cancel the
  packet. Ip4CancelPacket will cancel all the fragments of the
  packet. When all the fragments are freed, the IP4_TXTOKEN_WRAP
  will be deleted from the Map, and user's event signalled.
  Because Ip4CancelPacket and other functions are all called in
  line, so, after Ip4CancelPacket returns, the Item has been freed.

  @param[in]  Map                The IP4 child's transmit queue.
  @param[in]  Item               The current transmitted packet to test.
  @param[in]  Context            The user's token to cancel.

  @retval EFI_SUCCESS            Continue to check the next Item.
  @retval EFI_ABORTED            The user's Token (Token != NULL) is cancelled.

**/
EFI_STATUS
EFIAPI
Ip4CancelTxTokens (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  EFI_IP4_COMPLETION_TOKEN  *Token;
  IP4_TXTOKEN_WRAP          *Wrap;

  Token = (EFI_IP4_COMPLETION_TOKEN *)Context;

  //
  // Return EFI_SUCCESS to check the next item in the map if
  // this one doesn't match.
  //
  if ((Token != NULL) && (Token != Item->Key)) {
    return EFI_SUCCESS;
  }

  Wrap = (IP4_TXTOKEN_WRAP *)Item->Value;
  ASSERT (Wrap != NULL);

  //
  // Don't access the Item, Wrap and Token's members after this point.
  // Item and wrap has been freed. And we no longer own the Token.
  //
  Ip4CancelPacket (Wrap->IpInstance->Interface, Wrap->Packet, EFI_ABORTED);

  //
  // If only one item is to be cancel, return EFI_ABORTED to stop
  // iterating the map any more.
  //
  if (Token != NULL) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Cancel the receive request. This is quiet simple, because
  it is only enqueued in our local receive map.

  @param[in]  Map                The IP4 child's receive queue.
  @param[in]  Item               Current receive request to cancel.
  @param[in]  Context            The user's token to cancel.

  @retval EFI_SUCCESS            Continue to check the next receive request on the
                                 queue.
  @retval EFI_ABORTED            The user's token (token != NULL) has been
                                 cancelled.

**/
EFI_STATUS
EFIAPI
Ip4CancelRxTokens (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  EFI_IP4_COMPLETION_TOKEN  *Token;
  EFI_IP4_COMPLETION_TOKEN  *This;

  Token = (EFI_IP4_COMPLETION_TOKEN *)Context;
  This  = Item->Key;

  if ((Token != NULL) && (Token != This)) {
    return EFI_SUCCESS;
  }

  NetMapRemoveItem (Map, Item, NULL);

  This->Status        = EFI_ABORTED;
  This->Packet.RxData = NULL;
  gBS->SignalEvent (This->Event);

  if (Token != NULL) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Cancel the user's receive/transmit request.

  @param[in]  IpInstance         The IP4 child.
  @param[in]  Token              The token to cancel. If NULL, all token will be
                                 cancelled.

  @retval EFI_SUCCESS            The token is cancelled.
  @retval EFI_NOT_FOUND          The token isn't found on either the
                                 transmit/receive queue.
  @retval EFI_DEVICE_ERROR       Not all token is cancelled when Token is NULL.

**/
EFI_STATUS
Ip4Cancel (
  IN IP4_PROTOCOL              *IpInstance,
  IN EFI_IP4_COMPLETION_TOKEN  *Token          OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // First check the transmitted packet. Ip4CancelTxTokens returns
  // EFI_ABORTED to mean that the token has been cancelled when
  // token != NULL. So, return EFI_SUCCESS for this condition.
  //
  Status = NetMapIterate (&IpInstance->TxTokens, Ip4CancelTxTokens, Token);

  if (EFI_ERROR (Status)) {
    if ((Token != NULL) && (Status == EFI_ABORTED)) {
      return EFI_SUCCESS;
    }

    return Status;
  }

  //
  // Check the receive queue. Ip4CancelRxTokens also returns EFI_ABORT
  // for Token!=NULL and it is cancelled.
  //
  Status = NetMapIterate (&IpInstance->RxTokens, Ip4CancelRxTokens, Token);
  //
  // Dispatch the DPCs queued by the NotifyFunction of the canceled rx token's
  // events.
  //
  DispatchDpc ();
  if (EFI_ERROR (Status)) {
    if ((Token != NULL) && (Status == EFI_ABORTED)) {
      return EFI_SUCCESS;
    }

    return Status;
  }

  //
  // OK, if the Token is found when Token != NULL, the NetMapIterate
  // will return EFI_ABORTED, which has been interrupted as EFI_SUCCESS.
  //
  if (Token != NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // If Token == NULL, cancel all the tokens. return error if no
  // all of them are cancelled.
  //
  if (!NetMapIsEmpty (&IpInstance->TxTokens) ||
      !NetMapIsEmpty (&IpInstance->RxTokens))
  {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Abort an asynchronous transmit or receive request.

  The Cancel() function is used to abort a pending transmit or receive request.
  If the token is in the transmit or receive request queues, after calling this
  function, Token->Status will be set to EFI_ABORTED and then Token->Event will
  be signaled. If the token is not in one of the queues, which usually means the
  asynchronous operation has completed, this function will not signal the token
  and EFI_NOT_FOUND is returned.

  @param[in]  This  Pointer to the EFI_IP4_PROTOCOL instance.
  @param[in]  Token Pointer to a token that has been issued by
                    EFI_IP4_PROTOCOL.Transmit() or
                    EFI_IP4_PROTOCOL.Receive(). If NULL, all pending
                    tokens are aborted. Type EFI_IP4_COMPLETION_TOKEN is
                    defined in EFI_IP4_PROTOCOL.Transmit().

  @retval EFI_SUCCESS           The asynchronous I/O request was aborted and
                                Token.->Event was signaled. When Token is NULL, all
                                pending requests were aborted and their events were signaled.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_NOT_STARTED       This instance has not been started.
  @retval EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_NOT_FOUND         When Token is not NULL, the asynchronous I/O request was
                                not found in the transmit or receive queue. It has either completed
                                or was not issued by Transmit() and Receive().

**/
EFI_STATUS
EFIAPI
EfiIp4Cancel (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token    OPTIONAL
  )
{
  IP4_PROTOCOL  *IpInstance;
  EFI_STATUS    Status;
  EFI_TPL       OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (IpInstance->ConfigData.UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto ON_EXIT;
  }

  Status = Ip4Cancel (IpInstance, Token);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function polls for incoming data packets and processes outgoing data
  packets. Network drivers and applications can call the EFI_IP4_PROTOCOL.Poll()
  function to increase the rate that data packets are moved between the communications
  device and the transmit and receive queues.

  In some systems the periodic timer event may not poll the underlying communications
  device fast enough to transmit and/or receive all data packets without missing
  incoming packets or dropping outgoing packets. Drivers and applications that are
  experiencing packet loss should try calling the EFI_IP4_PROTOCOL.Poll() function
  more often.

  @param[in]  This               Pointer to the EFI_IP4_PROTOCOL instance.

  @retval  EFI_SUCCESS           Incoming or outgoing data was processed.
  @retval  EFI_NOT_STARTED       This EFI IPv4 Protocol instance has not been started.
  @retval  EFI_NO_MAPPING        When using the default address, configuration (DHCP, BOOTP,
                                 RARP, etc.) is not finished yet.
  @retval  EFI_INVALID_PARAMETER This is NULL.
  @retval  EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval  EFI_NOT_READY         No incoming or outgoing data is processed.
  @retval  EFI_TIMEOUT           Data was dropped out of the transmit and/or receive queue.
                                 Consider increasing the polling rate.

**/
EFI_STATUS
EFIAPI
EfiIp4Poll (
  IN EFI_IP4_PROTOCOL  *This
  )
{
  IP4_PROTOCOL                  *IpInstance;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);

  if (IpInstance->State == IP4_STATE_UNCONFIGED) {
    return EFI_NOT_STARTED;
  }

  Mnp = IpInstance->Service->Mnp;

  //
  // Don't lock the Poll function to enable the deliver of
  // the packet polled up.
  //
  return Mnp->Poll (Mnp);
}

/**
  Decrease the life of the transmitted packets. If it is
  decreased to zero, cancel the packet. This function is
  called by Ip4PacketTimerTicking which time out both the
  received-but-not-delivered and transmitted-but-not-recycle
  packets.

  @param[in]  Map                    The IP4 child's transmit map.
  @param[in]  Item                   Current transmitted packet.
  @param[in]  Context                Not used.

  @retval EFI_SUCCESS            Always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
Ip4SentPacketTicking (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  IP4_TXTOKEN_WRAP  *Wrap;

  Wrap = (IP4_TXTOKEN_WRAP *)Item->Value;
  ASSERT (Wrap != NULL);

  if ((Wrap->Life > 0) && (--Wrap->Life == 0)) {
    Ip4CancelPacket (Wrap->IpInstance->Interface, Wrap->Packet, EFI_ABORTED);
  }

  return EFI_SUCCESS;
}

/**
  This heart beat timer of IP4 service instance times out all of its IP4 children's
  received-but-not-delivered and transmitted-but-not-recycle packets, and provides
  time input for its IGMP protocol.

  @param[in]  Event                  The IP4 service instance's heart beat timer.
  @param[in]  Context                The IP4 service instance.

**/
VOID
EFIAPI
Ip4TimerTicking (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  IP4_SERVICE  *IpSb;

  IpSb = (IP4_SERVICE *)Context;
  NET_CHECK_SIGNATURE (IpSb, IP4_SERVICE_SIGNATURE);

  Ip4PacketTimerTicking (IpSb);
  Ip4IgmpTicking (IpSb);
}

/**
  This dedicated timer is used to poll underlying network media status. In case
  of cable swap or wireless network switch, a new round auto configuration will
  be initiated. The timer will signal the IP4 to run DHCP configuration again.
  IP4 driver will free old IP address related resource, such as route table and
  Interface, then initiate a DHCP process to acquire new IP, eventually create
  route table for new IP address.

  @param[in]  Event                  The IP4 service instance's heart beat timer.
  @param[in]  Context                The IP4 service instance.

**/
VOID
EFIAPI
Ip4TimerReconfigChecking (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  IP4_SERVICE              *IpSb;
  BOOLEAN                  OldMediaPresent;
  EFI_STATUS               Status;
  EFI_SIMPLE_NETWORK_MODE  SnpModeData;

  IpSb = (IP4_SERVICE *)Context;
  NET_CHECK_SIGNATURE (IpSb, IP4_SERVICE_SIGNATURE);

  OldMediaPresent = IpSb->MediaPresent;

  //
  // Get fresh mode data from MNP, since underlying media status may change.
  // Here, it needs to mention that the MediaPresent can also be checked even if
  // EFI_NOT_STARTED returned while this MNP child driver instance isn't configured.
  //
  Status = IpSb->Mnp->GetModeData (IpSb->Mnp, NULL, &SnpModeData);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    return;
  }

  IpSb->MediaPresent = SnpModeData.MediaPresent;
  //
  // Media transimit Unpresent to Present means new link movement is detected.
  //
  if (!OldMediaPresent && IpSb->MediaPresent && (IpSb->Ip4Config2Instance.Policy == Ip4Config2PolicyDhcp)) {
    //
    // Signal the IP4 to run the dhcp configuration again. IP4 driver will free
    // old IP address related resource, such as route table and Interface, then
    // initiate a DHCP round to acquire new IP, eventually
    // create route table for new IP address.
    //
    if (IpSb->ReconfigEvent != NULL) {
      Status = gBS->SignalEvent (IpSb->ReconfigEvent);
      DispatchDpc ();
    }
  }
}
