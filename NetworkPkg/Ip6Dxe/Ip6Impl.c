/** @file
  Implementation of EFI_IP6_PROTOCOL protocol interfaces.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

EFI_IPSEC2_PROTOCOL    *mIpSec = NULL;

EFI_IP6_PROTOCOL mEfiIp6ProtocolTemplete = {
  EfiIp6GetModeData,
  EfiIp6Configure,
  EfiIp6Groups,
  EfiIp6Routes,
  EfiIp6Neighbors,
  EfiIp6Transmit,
  EfiIp6Receive,
  EfiIp6Cancel,
  EfiIp6Poll
};

/**
  Gets the current operational settings for this instance of the EFI IPv6 Protocol driver.

  The GetModeData() function returns the current operational mode data for this driver instance.
  The data fields in EFI_IP6_MODE_DATA are read only. This function is used optionally to
  retrieve the operational mode data of underlying networks or drivers.

  @param[in]  This               Pointer to the EFI_IP6_PROTOCOL instance.
  @param[out] Ip6ModeData        Pointer to the EFI IPv6 Protocol mode data structure.
  @param[out] MnpConfigData      Pointer to the managed network configuration data structure.
  @param[out] SnpModeData        Pointer to the simple network mode data structure.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_OUT_OF_RESOURCES   The required mode data could not be allocated.

**/
EFI_STATUS
EFIAPI
EfiIp6GetModeData (
  IN EFI_IP6_PROTOCOL                 *This,
  OUT EFI_IP6_MODE_DATA               *Ip6ModeData     OPTIONAL,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA *MnpConfigData   OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE         *SnpModeData     OPTIONAL
  )
{
  IP6_PROTOCOL              *IpInstance;
  IP6_SERVICE               *IpSb;
  IP6_INTERFACE             *IpIf;
  EFI_IP6_CONFIG_DATA       *Config;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl     = gBS->RaiseTPL (TPL_CALLBACK);
  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;
  IpIf       = IpInstance->Interface;

  if (IpSb->LinkLocalDadFail) {
    return EFI_INVALID_PARAMETER;
  }

  if (Ip6ModeData != NULL) {
    //
    // IsStarted is "whether the EfiIp6Configure has been called".
    // IsConfigured is "whether the station address has been configured"
    //
    Ip6ModeData->IsStarted     = (BOOLEAN) (IpInstance->State == IP6_STATE_CONFIGED);
    Ip6ModeData->MaxPacketSize = IpSb->MaxPacketSize;
    CopyMem (&Ip6ModeData->ConfigData, &IpInstance->ConfigData, sizeof (EFI_IP6_CONFIG_DATA));
    Ip6ModeData->IsConfigured  = FALSE;

    Ip6ModeData->AddressCount  = 0;
    Ip6ModeData->AddressList   = NULL;

    Ip6ModeData->GroupCount    = IpInstance->GroupCount;
    Ip6ModeData->GroupTable    = NULL;

    Ip6ModeData->RouteCount    = 0;
    Ip6ModeData->RouteTable    = NULL;

    Ip6ModeData->NeighborCount = 0;
    Ip6ModeData->NeighborCache = NULL;

    Ip6ModeData->PrefixCount   = 0;
    Ip6ModeData->PrefixTable   = NULL;

    Ip6ModeData->IcmpTypeCount = 23;
    Ip6ModeData->IcmpTypeList  = AllocateCopyPool (
                                   Ip6ModeData->IcmpTypeCount * sizeof (EFI_IP6_ICMP_TYPE),
                                   mIp6SupportedIcmp
                                   );
    if (Ip6ModeData->IcmpTypeList == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    //
    // Return the currently configured IPv6 addresses and corresponding prefix lengths.
    //
    Status = Ip6BuildEfiAddressList (
               IpSb,
               &Ip6ModeData->AddressCount,
               &Ip6ModeData->AddressList
               );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    //
    // Return the current station address for this IP child.
    // If UseAnyStationAddress is set to TRUE, IP6 driver will
    // select a source address from its address list. Otherwise use the
    // StationAddress in config data.
    //
    if (Ip6ModeData->IsStarted) {
      Config = &Ip6ModeData->ConfigData;

      if (IpIf->Configured || NetIp6IsUnspecifiedAddr (&Config->DestinationAddress)) {
        Ip6ModeData->IsConfigured = TRUE;
      } else {
        Ip6ModeData->IsConfigured = FALSE;
      }

      //
      // Build a EFI route table for user from the internal route table.
      //
      Status = Ip6BuildEfiRouteTable (
                 IpSb->RouteTable,
                 &Ip6ModeData->RouteCount,
                 &Ip6ModeData->RouteTable
                 );

      if (EFI_ERROR (Status)) {
        goto Error;
      }
    }

    if (Ip6ModeData->IsConfigured) {
      //
      // Return the joined multicast group addresses.
      //
      if (IpInstance->GroupCount != 0) {
        Ip6ModeData->GroupTable = AllocateCopyPool (
                                    IpInstance->GroupCount * sizeof (EFI_IPv6_ADDRESS),
                                    IpInstance->GroupList
                                    );
        if (Ip6ModeData->GroupTable == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Error;
        }
      }
      //
      // Return the neighbor cache entries
      //
      Status = Ip6BuildEfiNeighborCache (
                 IpInstance,
                 &Ip6ModeData->NeighborCount,
                 &Ip6ModeData->NeighborCache
                 );
      if (EFI_ERROR (Status)) {
        goto Error;
      }

      //
      // Return the prefix table entries
      //
      Status = Ip6BuildPrefixTable (
                 IpInstance,
                 &Ip6ModeData->PrefixCount,
                 &Ip6ModeData->PrefixTable
                 );
      if (EFI_ERROR (Status)) {
        goto Error;
      }

    }
  }

  //
  // Get fresh mode data from MNP, since underlying media status may change
  //
  Status = IpSb->Mnp->GetModeData (IpSb->Mnp, MnpConfigData, SnpModeData);

  goto Exit;

Error:
  if (Ip6ModeData != NULL) {
    if (Ip6ModeData->AddressList != NULL) {
      FreePool (Ip6ModeData->AddressList);
    }

    if (Ip6ModeData->GroupTable != NULL) {
      FreePool (Ip6ModeData->GroupTable);
    }

    if (Ip6ModeData->RouteTable != NULL) {
      FreePool (Ip6ModeData->RouteTable);
    }

    if (Ip6ModeData->NeighborCache != NULL) {
      FreePool (Ip6ModeData->NeighborCache);
    }

    if (Ip6ModeData->PrefixTable != NULL) {
      FreePool (Ip6ModeData->PrefixTable);
    }

    if (Ip6ModeData->IcmpTypeList != NULL) {
      FreePool (Ip6ModeData->IcmpTypeList);
    }
  }

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Validate that Ipv6 address is OK to be used as station address or next hop address/ neighbor.

  @param[in]  IpSb               The IP6 service instance.
  @param[in]  Ip                 The IPv6 address to validate.
  @param[in]  Flag               If TRUE, validate if the address is OK to be used
                                 as station address. If FALSE, validate if the
                                 address is OK to be used as the next hop address/
                                 neighbor.

  @retval TRUE                   The Ip address is valid and could be used.
  @retval FALSE                  Invalid Ip address.

**/
BOOLEAN
Ip6IsValidAddress (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *Ip,
  IN BOOLEAN                Flag
  )
{
  if (!NetIp6IsUnspecifiedAddr (Ip)) {
    if (!NetIp6IsValidUnicast(Ip)) {
      return FALSE;
    }
    if (Ip6IsOneOfSetAddress (IpSb, Ip, NULL, NULL)) {
      return Flag;
    }
  } else {
    return Flag;
  }

  return (BOOLEAN) !Flag;
}

/**
  Validate whether the value of protocol is illegal or not. Protocol is the 'Next Header' field
  in the last IPv6 extension header, or basic IPv6 header is there's no extension header.

  @param[in]  Protocol           Default value of 'Next Header'

  @retval TRUE                   The protocol is illegal.
  @retval FALSE                  The protocol is legal.

**/
BOOLEAN
Ip6IsIllegalProtocol (
  IN UINT8                  Protocol
  )
{
  if (Protocol == IP6_HOP_BY_HOP || Protocol == EFI_IP_PROTO_ICMP || Protocol == IP4_PROTO_IGMP) {
    return TRUE;
  }

  if (Protocol == 41 || Protocol == 43 || Protocol == 44 || Protocol == 59 || Protocol == 60 || Protocol == 124) {
    return TRUE;
  }

  return FALSE;
}

/**
  Initialize the IP6_PROTOCOL structure to the unconfigured states.

  @param[in]       IpSb                   The IP6 service instance.
  @param[in, out]  IpInstance             The IP6 child instance.

**/
VOID
Ip6InitProtocol (
  IN IP6_SERVICE            *IpSb,
  IN OUT IP6_PROTOCOL       *IpInstance
  )
{
  ASSERT ((IpSb != NULL) && (IpInstance != NULL));

  ZeroMem (IpInstance, sizeof (IP6_PROTOCOL));

  IpInstance->Signature = IP6_PROTOCOL_SIGNATURE;
  IpInstance->State     = IP6_STATE_UNCONFIGED;
  IpInstance->Service   = IpSb;
  IpInstance->GroupList = NULL;
  CopyMem (&IpInstance->Ip6Proto, &mEfiIp6ProtocolTemplete, sizeof (EFI_IP6_PROTOCOL));

  NetMapInit  (&IpInstance->RxTokens);
  NetMapInit  (&IpInstance->TxTokens);
  InitializeListHead (&IpInstance->Received);
  InitializeListHead (&IpInstance->Delivered);

  EfiInitializeLock (&IpInstance->RecycleLock, TPL_NOTIFY);
}

/**
  Configure the IP6 child. If the child is already configured,
  change the configuration parameter. Otherwise, configure it
  for the first time. The caller should validate the configuration
  before deliver them to it. It also don't do configure NULL.

  @param[in, out]  IpInstance         The IP6 child to configure.
  @param[in]       Config             The configure data.

  @retval EFI_SUCCESS            The IP6 child is successfully configured.
  @retval EFI_DEVICE_ERROR       Failed to free the pending transive or to
                                 configure  underlying MNP, or other errors.
  @retval EFI_NO_MAPPING         The IP6 child is configured to use the default
                                 address, but the default address hasn't been
                                 configured. The IP6 child doesn't need to be
                                 reconfigured when the default address is configured.
  @retval EFI_OUT_OF_RESOURCES   No more memory space is available.
  @retval other                  Other error occurs.

**/
EFI_STATUS
Ip6ConfigProtocol (
  IN OUT IP6_PROTOCOL        *IpInstance,
  IN     EFI_IP6_CONFIG_DATA *Config
  )
{
  IP6_SERVICE               *IpSb;
  IP6_INTERFACE             *IpIf;
  EFI_STATUS                Status;
  EFI_IP6_CONFIG_DATA       *Current;
  IP6_ADDRESS_INFO          *AddressInfo;
  BOOLEAN                   StationZero;
  BOOLEAN                   DestZero;
  EFI_IPv6_ADDRESS          Source;
  BOOLEAN                   AddrOk;

  IpSb    = IpInstance->Service;
  Current = &IpInstance->ConfigData;

  //
  // User is changing packet filters. It must be stopped
  // before the station address can be changed.
  //
  if (IpInstance->State == IP6_STATE_CONFIGED) {
    //
    // Cancel all the pending transmit/receive from upper layer
    //
    Status = Ip6Cancel (IpInstance, NULL);

    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    CopyMem (Current, Config, sizeof (EFI_IP6_CONFIG_DATA));
    return EFI_SUCCESS;
  }

  //
  // Set up the interface.
  //
  StationZero = NetIp6IsUnspecifiedAddr (&Config->StationAddress);
  DestZero    = NetIp6IsUnspecifiedAddr (&Config->DestinationAddress);

  if (StationZero && DestZero) {
    //
    // StationAddress is still zero.
    //

    NET_GET_REF (IpSb->DefaultInterface);
    IpInstance->Interface = IpSb->DefaultInterface;
    InsertTailList (&IpSb->DefaultInterface->IpInstances, &IpInstance->AddrLink);

    CopyMem (Current, Config, sizeof (EFI_IP6_CONFIG_DATA));
    IpInstance->State = IP6_STATE_CONFIGED;

    return EFI_SUCCESS;
  }

  if (StationZero && !DestZero) {
    Status = Ip6SelectSourceAddress (IpSb, &Config->DestinationAddress, &Source);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    IP6_COPY_ADDRESS (&Source, &Config->StationAddress);
  }

  AddrOk = Ip6IsOneOfSetAddress (IpSb, &Source, &IpIf, &AddressInfo);
  if (AddrOk) {
    if (AddressInfo != NULL) {
      IpInstance->PrefixLength = AddressInfo->PrefixLength;
    } else {
      IpInstance->PrefixLength = IP6_LINK_LOCAL_PREFIX_LENGTH;
    }
  } else {
    //
    // The specified source address is not one of the addresses IPv6 maintains.
    //
    return EFI_INVALID_PARAMETER;
  }


  NET_GET_REF (IpIf);
  IpInstance->Interface = IpIf;
  InsertTailList (&IpIf->IpInstances, &IpInstance->AddrLink);

  CopyMem (Current, Config, sizeof (EFI_IP6_CONFIG_DATA));
  IP6_COPY_ADDRESS (&Current->StationAddress, &Source);
  IpInstance->State = IP6_STATE_CONFIGED;

  return EFI_SUCCESS;
}

/**
  Clean up the IP6 child, and release all the resources used by it.

  @param[in, out]  IpInstance    The IP6 child to clean up.

  @retval EFI_SUCCESS            The IP6 child is cleaned up.
  @retval EFI_DEVICE_ERROR       Some resources failed to be released.

**/
EFI_STATUS
Ip6CleanProtocol (
  IN OUT IP6_PROTOCOL            *IpInstance
  )
{
  if (EFI_ERROR (Ip6Cancel (IpInstance, NULL))) {
    return EFI_DEVICE_ERROR;
  }

  if (EFI_ERROR (Ip6Groups (IpInstance, FALSE, NULL))) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Some packets haven't been recycled. It is because either the
  // user forgets to recycle the packets, or because the callback
  // hasn't been called. Just leave it alone.
  //
  if (!IsListEmpty (&IpInstance->Delivered)) {
    ;
  }

  if (IpInstance->Interface != NULL) {
    RemoveEntryList (&IpInstance->AddrLink);
    Ip6CleanInterface (IpInstance->Interface, IpInstance);
    IpInstance->Interface = NULL;
  }

  if (IpInstance->GroupList != NULL) {
    FreePool (IpInstance->GroupList);
    IpInstance->GroupList   = NULL;
    IpInstance->GroupCount  = 0;
  }

  NetMapClean (&IpInstance->TxTokens);

  NetMapClean (&IpInstance->RxTokens);

  return EFI_SUCCESS;
}

/**
  Configure the MNP parameter used by IP. The IP driver uses one MNP
  child to transmit/receive frames. By default, it configures MNP
  to receive unicast/multicast/broadcast. Also, it will enable/disable
  the promiscuous receive according to whether there is IP child
  enable that or not. If Force is FALSE, it will iterate through
  all the IP children to check whether the promiscuous receive
  setting has been changed. If it hasn't been changed, it won't
  reconfigure the MNP. If Force is TRUE, the MNP is configured
  whether that is changed or not.

  @param[in]  IpSb               The IP6 service instance that is to be changed.
  @param[in]  Force              Force the configuration or not.

  @retval EFI_SUCCESS            The MNP successfully configured/reconfigured.
  @retval Others                 Configuration failed.

**/
EFI_STATUS
Ip6ServiceConfigMnp (
  IN IP6_SERVICE            *IpSb,
  IN BOOLEAN                Force
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *ProtoEntry;
  IP6_INTERFACE             *IpIf;
  IP6_PROTOCOL              *IpInstance;
  BOOLEAN                   Reconfig;
  BOOLEAN                   PromiscReceive;
  EFI_STATUS                Status;

  Reconfig       = FALSE;
  PromiscReceive = FALSE;

  if (!Force) {
    //
    // Iterate through the IP children to check whether promiscuous
    // receive setting has been changed. Update the interface's receive
    // filter also.
    //
    NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {

      IpIf              = NET_LIST_USER_STRUCT (Entry, IP6_INTERFACE, Link);
      IpIf->PromiscRecv = FALSE;

      NET_LIST_FOR_EACH (ProtoEntry, &IpIf->IpInstances) {
        IpInstance = NET_LIST_USER_STRUCT (ProtoEntry, IP6_PROTOCOL, AddrLink);

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

    Reconfig  = TRUE;
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
  Assigns an IPv6 address and subnet mask to this EFI IPv6 Protocol driver instance.

  The Configure() function is used to set, change, or reset the operational parameters and filter
  settings for this EFI IPv6 Protocol instance. Until these parameters have been set, no network traffic
  can be sent or received by this instance. Once the parameters have been reset (by calling this
  function with Ip6ConfigData set to NULL), no more traffic can be sent or received until these
  parameters have been set again. Each EFI IPv6 Protocol instance can be started and stopped
  independently of each other by enabling or disabling their receive filter settings with the
  Configure() function.

  If Ip6ConfigData.StationAddress is a valid non-zero IPv6 unicast address, it is required
  to be one of the currently configured IPv6 addresses listed in the EFI IPv6 drivers, or else
  EFI_INVALID_PARAMETER will be returned. If Ip6ConfigData.StationAddress is
  unspecified, the IPv6 driver will bind a source address according to the source address selection
  algorithm. Clients could frequently call GetModeData() to check get currently configured IPv6
  address list in the EFI IPv6 driver. If both Ip6ConfigData.StationAddress and
  Ip6ConfigData.Destination are unspecified, when transmitting the packet afterwards, the
  source address filled in each outgoing IPv6 packet is decided based on the destination of this packet.

  If operational parameters are reset or changed, any pending transmit and receive requests will be
  cancelled. Their completion token status will be set to EFI_ABORTED and their events will be
  signaled.

  @param[in]  This               Pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  Ip6ConfigData      Pointer to the EFI IPv6 Protocol configuration data structure.
                                 If NULL, reset the configuration data.

  @retval EFI_SUCCESS            The driver instance was successfully opened.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - Ip6ConfigData.StationAddress is neither zero nor
                                   a unicast IPv6 address.
                                 - Ip6ConfigData.StationAddress is neither zero nor
                                   one of the configured IP addresses in the EFI IPv6 driver.
                                 - Ip6ConfigData.DefaultProtocol is illegal.
  @retval EFI_OUT_OF_RESOURCES   The EFI IPv6 Protocol driver instance data could not be allocated.
  @retval EFI_NO_MAPPING         The IPv6 driver was responsible for choosing a source address for
                                 this instance, but no source address was available for use.
  @retval EFI_ALREADY_STARTED    The interface is already open and must be stopped before the IPv6
                                 address or prefix length can be changed.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred. The EFI IPv6
                                 Protocol driver instance was not opened.
  @retval EFI_UNSUPPORTED        Default protocol specified through
                                 Ip6ConfigData.DefaultProtocol isn't supported.

**/
EFI_STATUS
EFIAPI
EfiIp6Configure (
  IN EFI_IP6_PROTOCOL          *This,
  IN EFI_IP6_CONFIG_DATA       *Ip6ConfigData OPTIONAL
  )
{
  IP6_PROTOCOL              *IpInstance;
  EFI_IP6_CONFIG_DATA       *Current;
  EFI_TPL                   OldTpl;
  EFI_STATUS                Status;
  IP6_SERVICE               *IpSb;

  //
  // First, validate the parameters
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;

  if (IpSb->LinkLocalDadFail && Ip6ConfigData != NULL) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl     = gBS->RaiseTPL (TPL_CALLBACK);

  Status     = EFI_INVALID_PARAMETER;

  //
  // Validate the configuration first.
  //
  if (Ip6ConfigData != NULL) {
    //
    // Check whether the station address is valid.
    //
    if (!Ip6IsValidAddress (IpSb, &Ip6ConfigData->StationAddress, TRUE)) {
       goto Exit;
    }
    //
    // Check whether the default protocol is valid.
    //
    if (Ip6IsIllegalProtocol (Ip6ConfigData->DefaultProtocol)) {
      goto Exit;
    }

    //
    // User can only update packet filters when already configured.
    // If it wants to change the station address, it must configure(NULL)
    // the instance firstly.
    //
    if (IpInstance->State == IP6_STATE_CONFIGED) {
      Current = &IpInstance->ConfigData;

      if (!EFI_IP6_EQUAL (&Current->StationAddress, &Ip6ConfigData->StationAddress)) {
        Status = EFI_ALREADY_STARTED;
        goto Exit;
      }

      if (NetIp6IsUnspecifiedAddr (&Current->StationAddress) && IP6_NO_MAPPING (IpInstance)) {
        Status = EFI_NO_MAPPING;
        goto Exit;
      }
    }
  }

  //
  // Configure the instance or clean it up.
  //
  if (Ip6ConfigData != NULL) {
    Status = Ip6ConfigProtocol (IpInstance, Ip6ConfigData);
  } else {
    Status = Ip6CleanProtocol (IpInstance);

    //
    // Don't change the state if it is DESTROY, consider the following
    // valid sequence: Mnp is unloaded-->Ip Stopped-->Udp Stopped,
    // Configure (ThisIp, NULL). If the state is changed to UNCONFIGED,
    // the unload fails miserably.
    //
    if (IpInstance->State == IP6_STATE_CONFIGED) {
      IpInstance->State = IP6_STATE_UNCONFIGED;
    }
  }

  //
  // Update the MNP's configure data. Ip6ServiceConfigMnp will check
  // whether it is necessary to reconfigure the MNP.
  //
  Ip6ServiceConfigMnp (IpInstance->Service, FALSE);

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Joins and leaves multicast groups.

  The Groups() function is used to join and leave multicast group sessions. Joining a group will
  enable reception of matching multicast packets. Leaving a group will disable reception of matching
  multicast packets. Source-Specific Multicast isn't required to be supported.

  If JoinFlag is FALSE and GroupAddress is NULL, all joined groups will be left.

  @param[in]  This               Pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  JoinFlag           Set to TRUE to join the multicast group session, and FALSE to leave.
  @param[in]  GroupAddress       Pointer to the IPv6 multicast address.
                                 This is an optional parameter that may be NULL.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more of the following is TRUE:
                                 - This is NULL.
                                 - JoinFlag is TRUE and GroupAddress is NULL.
                                 - GroupAddress is not NULL and *GroupAddress is
                                   not a multicast IPv6 address.
                                 - GroupAddress is not NULL and *GroupAddress is in the
                                   range of SSM destination address.
  @retval EFI_NOT_STARTED        This instance has not been started.
  @retval EFI_OUT_OF_RESOURCES   System resources could not be allocated.
  @retval EFI_UNSUPPORTED        This EFI IPv6 Protocol implementation does not support multicast groups.
  @retval EFI_ALREADY_STARTED    The group address is already in the group table (when
                                 JoinFlag is TRUE).
  @retval EFI_NOT_FOUND          The group address is not in the group table (when JoinFlag is FALSE).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiIp6Groups (
  IN EFI_IP6_PROTOCOL  *This,
  IN BOOLEAN           JoinFlag,
  IN EFI_IPv6_ADDRESS  *GroupAddress  OPTIONAL
  )
{
  EFI_TPL                   OldTpl;
  EFI_STATUS                Status;
  IP6_PROTOCOL              *IpInstance;
  IP6_SERVICE               *IpSb;

  if ((This == NULL) || (JoinFlag && GroupAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (GroupAddress != NULL && !IP6_IS_MULTICAST (GroupAddress)) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;

  if (IpSb->LinkLocalDadFail) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl     = gBS->RaiseTPL (TPL_CALLBACK);

  if (IpInstance->State != IP6_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  Status = Ip6Groups (IpInstance, JoinFlag, GroupAddress);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Adds and deletes routing table entries.

  The Routes() function adds a route to, or deletes a route from, the routing table.

  Routes are determined by comparing the leftmost PrefixLength bits of Destination with
  the destination IPv6 address arithmetically. The gateway address must be on the same subnet as the
  configured station address.

  The default route is added with Destination and PrefixLength both set to all zeros. The
  default route matches all destination IPv6 addresses that do not match any other routes.

  All EFI IPv6 Protocol instances share a routing table.

  @param[in]  This               Pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  DeleteRoute        Set to TRUE to delete this route from the routing table. Set to
                                 FALSE to add this route to the routing table. Destination,
                                 PrefixLength and Gateway are used as the key to each
                                 route entry.
  @param[in]  Destination        The address prefix of the subnet that needs to be routed.
                                 This is an optional parameter that may be NULL.
  @param[in]  PrefixLength       The prefix length of Destination. Ignored if Destination
                                 is NULL.
  @param[in]  GatewayAddress     The unicast gateway IPv6 address for this route.
                                 This is an optional parameter that may be NULL.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_NOT_STARTED        The driver instance has not been started.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - When DeleteRoute is TRUE, both Destination and
                                   GatewayAddress are NULL.
                                 - When DeleteRoute is FALSE, either Destination or
                                   GatewayAddress is NULL.
                                 - *GatewayAddress is not a valid unicast IPv6 address.
                                 - *GatewayAddress is one of the local configured IPv6
                                   addresses.
  @retval EFI_OUT_OF_RESOURCES   Could not add the entry to the routing table.
  @retval EFI_NOT_FOUND          This route is not in the routing table (when DeleteRoute is TRUE).
  @retval EFI_ACCESS_DENIED      The route is already defined in the routing table (when
                                 DeleteRoute is FALSE).

**/
EFI_STATUS
EFIAPI
EfiIp6Routes (
  IN EFI_IP6_PROTOCOL    *This,
  IN BOOLEAN             DeleteRoute,
  IN EFI_IPv6_ADDRESS    *Destination    OPTIONAL,
  IN UINT8               PrefixLength,
  IN EFI_IPv6_ADDRESS    *GatewayAddress OPTIONAL
  )
{
  IP6_PROTOCOL              *IpInstance;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  IP6_SERVICE               *IpSb;

  if ((This == NULL) || (PrefixLength > IP6_PREFIX_MAX)) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;

  if (IpSb->LinkLocalDadFail) {
    return EFI_DEVICE_ERROR;
  }

  if (IpInstance->State != IP6_STATE_CONFIGED) {
    return EFI_NOT_STARTED;
  }

  if (DeleteRoute && (Destination == NULL) && (GatewayAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!DeleteRoute && (Destination == NULL || GatewayAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (GatewayAddress != NULL) {
    if (!Ip6IsValidAddress (IpSb, GatewayAddress, FALSE)) {
      return EFI_INVALID_PARAMETER;
    }

    if (!NetIp6IsUnspecifiedAddr (GatewayAddress) &&
        !NetIp6IsNetEqual (GatewayAddress, &IpInstance->ConfigData.StationAddress, PrefixLength)
          ) {
      return EFI_INVALID_PARAMETER;
    }
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Update the route table
  //
  if (DeleteRoute) {
    Status = Ip6DelRoute (IpSb->RouteTable, Destination, PrefixLength, GatewayAddress);
  } else {
    Status = Ip6AddRoute (IpSb->RouteTable, Destination, PrefixLength, GatewayAddress);
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Add or delete Neighbor cache entries.

  The Neighbors() function is used to add, update, or delete an entry from neighbor cache.
  IPv6 neighbor cache entries are typically inserted and updated by the network protocol driver as
  network traffic is processed. Most neighbor cache entries will timeout and be deleted if the network
  traffic stops. Neighbor cache entries that were inserted by Neighbors() may be static (will not
  timeout) or dynamic (will timeout).

  The implementation should follow the neighbor cache timeout mechanism which is defined in
  RFC4861. The default neighbor cache timeout value should be tuned for the expected network
  environment

  @param[in]  This               Pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  DeleteFlag         Set to TRUE to delete the specified cache entry, set to FALSE to
                                 add (or update, if it already exists and Override is TRUE) the
                                 specified cache entry. TargetIp6Address is used as the key
                                 to find the requested cache entry.
  @param[in]  TargetIp6Address   Pointer to the Target IPv6 address.
  @param[in]  TargetLinkAddress  Pointer to the link-layer address of the target. Ignored if NULL.
  @param[in]  Timeout            Time in 100-ns units that this entry will remain in the neighbor
                                 cache, it will be deleted after Timeout. A value of zero means that
                                 the entry is permanent. A non-zero value means that the entry is
                                 dynamic.
  @param[in]  Override           If TRUE, the cached link-layer address of the matching entry will
                                 be overridden and updated; if FALSE, EFI_ACCESS_DENIED
                                 will be returned if a corresponding cache entry already existed.

  @retval  EFI_SUCCESS           The data has been queued for transmission.
  @retval  EFI_NOT_STARTED       This instance has not been started.
  @retval  EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - TargetIpAddress is NULL.
                                 - *TargetLinkAddress is invalid when not NULL.
                                 - *TargetIpAddress is not a valid unicast IPv6 address.
                                 - *TargetIpAddress is one of the local configured IPv6
                                   addresses.
  @retval  EFI_OUT_OF_RESOURCES  Could not add the entry to the neighbor cache.
  @retval  EFI_NOT_FOUND         This entry is not in the neighbor cache (when DeleteFlag  is
                                 TRUE or when DeleteFlag  is FALSE while
                                 TargetLinkAddress is NULL.).
  @retval  EFI_ACCESS_DENIED     The to-be-added entry is already defined in the neighbor cache,
                                 and that entry is tagged as un-overridden (when Override
                                 is FALSE).

**/
EFI_STATUS
EFIAPI
EfiIp6Neighbors (
  IN EFI_IP6_PROTOCOL          *This,
  IN BOOLEAN                   DeleteFlag,
  IN EFI_IPv6_ADDRESS          *TargetIp6Address,
  IN EFI_MAC_ADDRESS           *TargetLinkAddress OPTIONAL,
  IN UINT32                    Timeout,
  IN BOOLEAN                   Override
  )
{
  EFI_TPL                   OldTpl;
  EFI_STATUS                Status;
  IP6_PROTOCOL              *IpInstance;
  IP6_SERVICE               *IpSb;

  if (This == NULL || TargetIp6Address == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (NetIp6IsUnspecifiedAddr (TargetIp6Address)) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;

  if (IpSb->LinkLocalDadFail) {
    return EFI_DEVICE_ERROR;
  }

  if (!Ip6IsValidAddress (IpSb, TargetIp6Address, FALSE)) {
    return EFI_INVALID_PARAMETER;
  }

  if (TargetLinkAddress != NULL) {
    if (!Ip6IsValidLinkAddress (IpSb, TargetLinkAddress)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (Ip6IsOneOfSetAddress (IpSb, TargetIp6Address, NULL, NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  if (IpInstance->State != IP6_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  if (DeleteFlag) {
    Status = Ip6DelNeighbor (IpInstance->Service, TargetIp6Address, TargetLinkAddress, Timeout, Override);
  } else {
    Status = Ip6AddNeighbor (IpInstance->Service, TargetIp6Address, TargetLinkAddress, Timeout, Override);
  }

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Check whether the user's token or event has already
  been enqueue on IP6's list.

  @param[in]  Map                The container of either user's transmit or receive
                                 token.
  @param[in]  Item               Current item to check against.
  @param[in]  Context            The Token to check against.

  @retval EFI_ACCESS_DENIED      The token or event has already been enqueued in IP
  @retval EFI_SUCCESS            The current item isn't the same token/event as the
                                 context.

**/
EFI_STATUS
EFIAPI
Ip6TokenExist (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  EFI_IP6_COMPLETION_TOKEN  *Token;
  EFI_IP6_COMPLETION_TOKEN  *TokenInItem;

  Token       = (EFI_IP6_COMPLETION_TOKEN *) Context;
  TokenInItem = (EFI_IP6_COMPLETION_TOKEN *) Item->Key;

  if (Token == TokenInItem || Token->Event == TokenInItem->Event) {
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}

/**
  Validate the user's token against the current station address.

  @param[in]  Token              User's token to validate.

  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_BAD_BUFFER_SIZE    The user's option/data is too long.
  @retval EFI_SUCCESS            The token is OK.

**/
EFI_STATUS
Ip6TxTokenValid (
  IN EFI_IP6_COMPLETION_TOKEN   *Token
  )
{
  EFI_IP6_TRANSMIT_DATA     *TxData;
  UINT32                    Index;
  UINT32                    DataLength;

  if (Token == NULL || Token->Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TxData = Token->Packet.TxData;

  if (TxData == NULL || (TxData->ExtHdrsLength != 0 && TxData->ExtHdrs == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (TxData->FragmentCount == 0 || TxData->DataLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  for (DataLength = 0, Index = 0; Index < TxData->FragmentCount; Index++) {
    if (TxData->FragmentTable[Index].FragmentLength == 0 || TxData->FragmentTable[Index].FragmentBuffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    DataLength += TxData->FragmentTable[Index].FragmentLength;
  }

  if (TxData->DataLength != DataLength) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // TODO: Token.Packet.TxData.DataLength is too short to transmit.
  // return EFI_BUFFER_TOO_SMALL;
  //

  //
  // If Token.Packet.TxData.DataLength is beyond the maximum that which can be
  // described through the Fragment Offset field in Fragment header when performing
  // fragmentation.
  //
  if (TxData->DataLength > 64 * 1024) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}

/**
  The callback function for the net buffer which wraps the user's
  transmit token. Although  this function seems simple, there
  are some subtle aspects.
  When user requests the IP to transmit a packet by passing it a
  token, the token is wrapped in an IP6_TXTOKEN_WRAP and the data
  is wrapped in an net buffer. The net buffer's Free function is
  set to Ip6FreeTxToken. The Token and token wrap are added to the
  IP child's TxToken map. Then the buffer is passed to Ip6Output for
  transmission. If an error happened before that, the buffer
  is freed, which in turn frees the token wrap. The wrap may
  have been added to the TxToken map or not, and the user's event
  shouldn't be fired because we are still in the EfiIp6Transmit. If
  the buffer has been sent by Ip6Output, it should be removed from
  the TxToken map and user's event signaled. The token wrap and buffer
  are bound together. Check the comments in Ip6Output for information
  about IP fragmentation.

  @param[in]  Context                The token's wrap.

**/
VOID
EFIAPI
Ip6FreeTxToken (
  IN VOID                   *Context
  )
{
  IP6_TXTOKEN_WRAP          *Wrap;
  NET_MAP_ITEM              *Item;

  Wrap = (IP6_TXTOKEN_WRAP *) Context;

  //
  // Signal IpSecRecycleEvent to inform IPsec free the memory
  //
  if (Wrap->IpSecRecycleSignal != NULL) {
    gBS->SignalEvent (Wrap->IpSecRecycleSignal);
  }

  //
  // Find the token in the instance's map. EfiIp6Transmit put the
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
  The callback function to Ip6Output to update the transmit status.

  @param[in]  Packet           The user's transmit packet.
  @param[in]  IoStatus         The result of the transmission.
  @param[in]  Flag             Not used during transmission.
  @param[in]  Context          The token's wrap.

**/
VOID
Ip6OnPacketSent (
  IN NET_BUF                   *Packet,
  IN EFI_STATUS                IoStatus,
  IN UINT32                    Flag,
  IN VOID                      *Context
  )
{
  IP6_TXTOKEN_WRAP             *Wrap;

  //
  // This is the transmission request from upper layer,
  // not the IP6 driver itself.
  //
  Wrap                = (IP6_TXTOKEN_WRAP *) Context;
  Wrap->Token->Status = IoStatus;

  NetbufFree (Wrap->Packet);
}

/**
  Places outgoing data packets into the transmit queue.

  The Transmit() function places a sending request in the transmit queue of this
  EFI IPv6 Protocol instance. Whenever the packet in the token is sent out or some
  errors occur, the event in the token will be signaled, and the status is updated.

  @param[in]  This               Pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  Token              Pointer to the transmit token.

  @retval  EFI_SUCCESS           The data has been queued for transmission.
  @retval  EFI_NOT_STARTED       This instance has not been started.
  @retval  EFI_NO_MAPPING        The IPv6 driver was responsible for choosing
                                 a source address for this transmission,
                                 but no source address was available for use.
  @retval  EFI_INVALID_PARAMETER One or more of the following is TRUE:
                                 - This is NULL.
                                 - Token is NULL.
                                 - Token.Event is NULL.
                                 - Token.Packet.TxData is NULL.
                                 - Token.Packet.ExtHdrsLength is not zero and
                                   Token.Packet.ExtHdrs is NULL.
                                 - Token.Packet.FragmentCount is zero.
                                 - One or more of the Token.Packet.TxData.
                                   FragmentTable[].FragmentLength fields is zero.
                                 - One or more of the Token.Packet.TxData.
                                   FragmentTable[].FragmentBuffer fields is NULL.
                                 - Token.Packet.TxData.DataLength is zero or not
                                   equal to the sum of fragment lengths.
                                 - Token.Packet.TxData.DestinationAddress is non
                                   zero when DestinationAddress is configured as
                                   non-zero when doing Configure() for this
                                   EFI IPv6 protocol instance.
                                 - Token.Packet.TxData.DestinationAddress is
                                   unspecified when DestinationAddress is unspecified
                                   when doing Configure() for this EFI IPv6 protocol
                                   instance.
  @retval  EFI_ACCESS_DENIED     The transmit completion token with the same Token.
                                 Event was already in the transmit queue.
  @retval  EFI_NOT_READY         The completion token could not be queued because
                                 the transmit queue is full.
  @retval  EFI_NOT_FOUND         Not route is found to destination address.
  @retval  EFI_OUT_OF_RESOURCES  Could not queue the transmit data.
  @retval  EFI_BUFFER_TOO_SMALL  Token.Packet.TxData.TotalDataLength is too
                                 short to transmit.
  @retval  EFI_BAD_BUFFER_SIZE   If Token.Packet.TxData.DataLength is beyond the
                                 maximum that which can be described through the
                                 Fragment Offset field in Fragment header when
                                 performing fragmentation.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiIp6Transmit (
  IN EFI_IP6_PROTOCOL          *This,
  IN EFI_IP6_COMPLETION_TOKEN  *Token
  )
{
  IP6_SERVICE               *IpSb;
  IP6_PROTOCOL              *IpInstance;
  EFI_IP6_CONFIG_DATA       *Config;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  EFI_IP6_HEADER            Head;
  EFI_IP6_TRANSMIT_DATA     *TxData;
  EFI_IP6_OVERRIDE_DATA     *Override;
  IP6_TXTOKEN_WRAP          *Wrap;
  UINT8                     *ExtHdrs;

  //
  // Check input parameters.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ExtHdrs = NULL;

  Status = Ip6TxTokenValid (Token);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;

  if (IpSb->LinkLocalDadFail) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl     = gBS->RaiseTPL (TPL_CALLBACK);

  if (IpInstance->State != IP6_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  Config = &IpInstance->ConfigData;

  //
  // Check whether the token or signal already existed.
  //
  if (EFI_ERROR (NetMapIterate (&IpInstance->TxTokens, Ip6TokenExist, Token))) {
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  //
  // Build the IP header, fill in the information from ConfigData or OverrideData
  //
  ZeroMem (&Head, sizeof(EFI_IP6_HEADER));
  TxData = Token->Packet.TxData;
  IP6_COPY_ADDRESS (&Head.SourceAddress, &Config->StationAddress);
  IP6_COPY_ADDRESS (&Head.DestinationAddress, &Config->DestinationAddress);

  Status = EFI_INVALID_PARAMETER;

  if (NetIp6IsUnspecifiedAddr (&TxData->DestinationAddress)) {
    if (NetIp6IsUnspecifiedAddr (&Config->DestinationAddress)) {
      goto Exit;
    }

    ASSERT (!NetIp6IsUnspecifiedAddr (&Config->StationAddress));

  } else {
    //
    // StationAddress is unspecified only when ConfigData.Dest is unspecified.
    // Use TxData.Dest to override the DestinationAddress.
    //
    if (!NetIp6IsUnspecifiedAddr (&Config->DestinationAddress)) {
      goto Exit;
    }

    if (NetIp6IsUnspecifiedAddr (&Config->StationAddress)) {
      Status = Ip6SelectSourceAddress (
                 IpSb,
                 &TxData->DestinationAddress,
                 &Head.SourceAddress
                 );
      if (EFI_ERROR (Status)) {
        goto Exit;
      }
    }

    IP6_COPY_ADDRESS (&Head.DestinationAddress, &TxData->DestinationAddress);
  }

  //
  // Fill in Head infos.
  //
  Head.NextHeader = Config->DefaultProtocol;
  if (TxData->ExtHdrsLength != 0) {
    Head.NextHeader = TxData->NextHeader;
  }

  if (TxData->OverrideData != NULL) {
    Override        = TxData->OverrideData;
    Head.NextHeader = Override->Protocol;
    Head.HopLimit   = Override->HopLimit;
    Head.FlowLabelL = HTONS ((UINT16) Override->FlowLabel);
    Head.FlowLabelH = (UINT8) ((Override->FlowLabel >> 16) & 0x0F);
  } else {
    Head.HopLimit   = Config->HopLimit;
    Head.FlowLabelL = HTONS ((UINT16) Config->FlowLabel);
    Head.FlowLabelH = (UINT8) ((Config->FlowLabel >> 16) & 0x0F);
  }

  Head.PayloadLength = HTONS ((UINT16) (TxData->ExtHdrsLength + TxData->DataLength));

  //
  // OK, it survives all the validation check. Wrap the token in
  // a IP6_TXTOKEN_WRAP and the data in a netbuf
  //
  Status = EFI_OUT_OF_RESOURCES;
  Wrap   = AllocateZeroPool (sizeof (IP6_TXTOKEN_WRAP));
  if (Wrap == NULL) {
    goto Exit;
  }

  Wrap->IpInstance  = IpInstance;
  Wrap->Token       = Token;
  Wrap->Sent        = FALSE;
  Wrap->Life        = IP6_US_TO_SEC (Config->TransmitTimeout);
  Wrap->Packet      = NetbufFromExt (
                        (NET_FRAGMENT *) TxData->FragmentTable,
                        TxData->FragmentCount,
                        IP6_MAX_HEADLEN,
                        0,
                        Ip6FreeTxToken,
                        Wrap
                        );

  if (Wrap->Packet == NULL) {
    FreePool (Wrap);
    goto Exit;
  }

  Token->Status = EFI_NOT_READY;

  Status = NetMapInsertTail (&IpInstance->TxTokens, Token, Wrap);
  if (EFI_ERROR (Status)) {
    //
    // NetbufFree will call Ip6FreeTxToken, which in turn will
    // free the IP6_TXTOKEN_WRAP. Now, the token wrap hasn't been
    // enqueued.
    //
    NetbufFree (Wrap->Packet);
    goto Exit;
  }

  //
  // Allocate a new buffer to store IPv6 extension headers to avoid updating
  // the original data in EFI_IP6_COMPLETION_TOKEN.
  //
  if (TxData->ExtHdrsLength != 0 && TxData->ExtHdrs != NULL) {
    ExtHdrs = (UINT8 *) AllocateCopyPool (TxData->ExtHdrsLength, TxData->ExtHdrs);
    if (ExtHdrs == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
  }

  //
  // Mark the packet sent before output it. Mark it not sent again if the
  // returned status is not EFI_SUCCESS;
  //
  Wrap->Sent = TRUE;

  Status = Ip6Output (
             IpSb,
             NULL,
             IpInstance,
             Wrap->Packet,
             &Head,
             ExtHdrs,
             TxData->ExtHdrsLength,
             Ip6OnPacketSent,
             Wrap
             );
  if (EFI_ERROR (Status)) {
    Wrap->Sent = FALSE;
    NetbufFree (Wrap->Packet);
  }

Exit:
  gBS->RestoreTPL (OldTpl);

  if (ExtHdrs != NULL) {
    FreePool (ExtHdrs);
  }

  return Status;
}

/**
  Places a receiving request into the receiving queue.

  The Receive() function places a completion token into the receive packet queue.
  This function is always asynchronous.

  The Token.Event field in the completion token must be filled in by the caller
  and cannot be NULL. When the receive operation completes, the EFI IPv6 Protocol
  driver updates the Token.Status and Token.Packet.RxData fields and the Token.Event
  is signaled.

  Current Udp implementation creates an IP child for each Udp child.
  It initiates a asynchronous receive immediately no matter whether
  there is no mapping or not. Therefore, disable the returning EFI_NO_MAPPING for now.
  To enable it, the following check must be performed:

  if (NetIp6IsUnspecifiedAddr (&Config->StationAddress) && IP6_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto Exit;
  }

  @param[in]  This               Pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  Token              Pointer to a token that is associated with the receive data descriptor.

  @retval EFI_SUCCESS            The receive completion token was cached.
  @retval EFI_NOT_STARTED        This EFI IPv6 Protocol instance has not been started.
  @retval EFI_NO_MAPPING         When IP6 driver responsible for binding source address to this instance,
                                 while no source address is available for use.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - Token is NULL.
                                 - Token.Event is NULL.
  @retval EFI_OUT_OF_RESOURCES   The receive completion token could not be queued due to a lack of system
                                 resources (usually memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The EFI IPv6 Protocol instance has been reset to startup defaults.
  @retval EFI_ACCESS_DENIED      The receive completion token with the same Token.Event was already
                                 in the receive queue.
  @retval EFI_NOT_READY          The receive request could not be queued because the receive queue is full.

**/
EFI_STATUS
EFIAPI
EfiIp6Receive (
  IN EFI_IP6_PROTOCOL          *This,
  IN EFI_IP6_COMPLETION_TOKEN  *Token
  )
{
  IP6_PROTOCOL              *IpInstance;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  IP6_SERVICE               *IpSb;

  if (This == NULL || Token == NULL || Token->Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;

  if (IpSb->LinkLocalDadFail) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (IpInstance->State != IP6_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  //
  // Check whether the toke is already on the receive queue.
  //
  Status = NetMapIterate (&IpInstance->RxTokens, Ip6TokenExist, Token);

  if (EFI_ERROR (Status)) {
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  //
  // Queue the token then check whether there is pending received packet.
  //
  Status = NetMapInsertTail (&IpInstance->RxTokens, Token, NULL);

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = Ip6InstanceDeliverPacket (IpInstance);

  //
  // Dispatch the DPC queued by the NotifyFunction of this instane's receive
  // event.
  //
  DispatchDpc ();

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Cancel the transmitted but not recycled packet. If a matching
  token is found, it will call Ip6CancelPacket to cancel the
  packet. Ip6CancelPacket cancels all the fragments of the
  packet. When all the fragments are freed, the IP6_TXTOKEN_WRAP
  is deleted from the Map, and user's event is signalled.
  Because Ip6CancelPacket and other functions are all called in
  line, after Ip6CancelPacket returns, the Item has been freed.

  @param[in]  Map                The IP6 child's transmit queue.
  @param[in]  Item               The current transmitted packet to test.
  @param[in]  Context            The user's token to cancel.

  @retval EFI_SUCCESS            Continue to check the next Item.
  @retval EFI_ABORTED            The user's Token (Token != NULL) is cancelled.

**/
EFI_STATUS
EFIAPI
Ip6CancelTxTokens (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  EFI_IP6_COMPLETION_TOKEN  *Token;
  IP6_TXTOKEN_WRAP          *Wrap;

  Token = (EFI_IP6_COMPLETION_TOKEN *) Context;

  //
  // Return EFI_SUCCESS to check the next item in the map if
  // this one doesn't match.
  //
  if ((Token != NULL) && (Token != Item->Key)) {
    return EFI_SUCCESS;
  }

  Wrap = (IP6_TXTOKEN_WRAP *) Item->Value;
  ASSERT (Wrap != NULL);

  //
  // Don't access the Item, Wrap and Token's members after this point.
  // Item and wrap has been freed. And we no longer own the Token.
  //
  Ip6CancelPacket (Wrap->IpInstance->Interface, Wrap->Packet, EFI_ABORTED);

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
  Cancel the receive request. This is simple, because
  it is only enqueued in our local receive map.

  @param[in]  Map                The IP6 child's receive queue.
  @param[in]  Item               Current receive request to cancel.
  @param[in]  Context            The user's token to cancel.


  @retval EFI_SUCCESS            Continue to check the next receive request on the
                                 queue.
  @retval EFI_ABORTED            The user's token (token != NULL) has been
                                 cancelled.

**/
EFI_STATUS
EFIAPI
Ip6CancelRxTokens (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  EFI_IP6_COMPLETION_TOKEN  *Token;
  EFI_IP6_COMPLETION_TOKEN  *This;

  Token = (EFI_IP6_COMPLETION_TOKEN *) Context;
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
  Cancel the user's receive/transmit request. It is the worker function of
  EfiIp6Cancel API.

  @param[in]  IpInstance         The IP6 child.
  @param[in]  Token              The token to cancel. If NULL, all token will be
                                 cancelled.

  @retval EFI_SUCCESS            The token is cancelled.
  @retval EFI_NOT_FOUND          The token isn't found on either the
                                 transmit/receive queue.
  @retval EFI_DEVICE_ERROR       Not all tokens are cancelled when Token is NULL.

**/
EFI_STATUS
Ip6Cancel (
  IN IP6_PROTOCOL             *IpInstance,
  IN EFI_IP6_COMPLETION_TOKEN *Token          OPTIONAL
  )
{
  EFI_STATUS                Status;

  //
  // First check the transmitted packet. Ip6CancelTxTokens returns
  // EFI_ABORTED to mean that the token has been cancelled when
  // token != NULL. So, return EFI_SUCCESS for this condition.
  //
  Status = NetMapIterate (&IpInstance->TxTokens, Ip6CancelTxTokens, Token);
  if (EFI_ERROR (Status)) {
    if ((Token != NULL) && (Status == EFI_ABORTED)) {
      return EFI_SUCCESS;
    }

    return Status;
  }

  //
  // Check the receive queue. Ip6CancelRxTokens also returns EFI_ABORT
  // for Token!=NULL and it is cancelled.
  //
  Status = NetMapIterate (&IpInstance->RxTokens, Ip6CancelRxTokens, Token);
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
  // If Token == NULL, cancel all the tokens. return error if not
  // all of them are cancelled.
  //
  if (!NetMapIsEmpty (&IpInstance->TxTokens) || !NetMapIsEmpty (&IpInstance->RxTokens)) {

    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Abort an asynchronous transmit or receive request.

  The Cancel() function is used to abort a pending transmit or receive request.
  If the token is in the transmit or receive request queues, after calling this
  function, Token->Status will be set to EFI_ABORTED, and then Token->Event will
  be signaled. If the token is not in one of the queues, which usually means the
  asynchronous operation has completed, this function will not signal the token,
  and EFI_NOT_FOUND is returned.

  @param[in]  This               Pointer to the EFI_IP6_PROTOCOL instance.
  @param[in]  Token              Pointer to a token that has been issued by
                                 EFI_IP6_PROTOCOL.Transmit() or
                                 EFI_IP6_PROTOCOL.Receive(). If NULL, all pending
                                 tokens are aborted. Type EFI_IP6_COMPLETION_TOKEN is
                                 defined in EFI_IP6_PROTOCOL.Transmit().

  @retval EFI_SUCCESS            The asynchronous I/O request was aborted and
                                 Token->Event was signaled. When Token is NULL, all
                                 pending requests were aborted, and their events were signaled.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        This instance has not been started.
  @retval EFI_NOT_FOUND          When Token is not NULL, the asynchronous I/O request was
                                 not found in the transmit or receive queue. It has either completed
                                 or was not issued by Transmit() and Receive().
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiIp6Cancel (
  IN EFI_IP6_PROTOCOL          *This,
  IN EFI_IP6_COMPLETION_TOKEN  *Token    OPTIONAL
  )
{
  IP6_PROTOCOL              *IpInstance;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (IpInstance->State != IP6_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  Status = Ip6Cancel (IpInstance, Token);

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Polls for incoming data packets, and processes outgoing data packets.

  The Poll() function polls for incoming data packets and processes outgoing data
  packets. Network drivers and applications can call the EFI_IP6_PROTOCOL.Poll()
  function to increase the rate that data packets are moved between the communications
  device and the transmit and receive queues.

  In some systems the periodic timer event may not poll the underlying communications
  device fast enough to transmit and/or receive all data packets without missing
  incoming packets or dropping outgoing packets. Drivers and applications that are
  experiencing packet loss should try calling the EFI_IP6_PROTOCOL.Poll() function
  more often.

  @param[in]  This               Pointer to the EFI_IP6_PROTOCOL instance.

  @retval  EFI_SUCCESS           Incoming or outgoing data was processed.
  @retval  EFI_NOT_STARTED       This EFI IPv6 Protocol instance has not been started.
  @retval  EFI_INVALID_PARAMETER This is NULL.
  @retval  EFI_DEVICE_ERROR      An unexpected system error or network error occurred.
  @retval  EFI_NOT_READY         No incoming or outgoing data was processed.
  @retval  EFI_TIMEOUT           Data was dropped out of the transmit and/or receive queue.
                                 Consider increasing the polling rate.

**/
EFI_STATUS
EFIAPI
EfiIp6Poll (
  IN EFI_IP6_PROTOCOL          *This
  )
{
  IP6_PROTOCOL                  *IpInstance;
  IP6_SERVICE                   *IpSb;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;

  if (IpSb->LinkLocalDadFail) {
    return EFI_DEVICE_ERROR;
  }

  if (IpInstance->State == IP6_STATE_UNCONFIGED) {
    return EFI_NOT_STARTED;
  }

  Mnp = IpInstance->Service->Mnp;

  //
  // Don't lock the Poll function to enable the deliver of
  // the packet polled up.
  //
  return Mnp->Poll (Mnp);

}

