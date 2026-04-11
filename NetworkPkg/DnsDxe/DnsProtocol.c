/** @file
Implementation of EFI_DNS4_PROTOCOL and EFI_DNS6_PROTOCOL interfaces.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DnsImpl.h"

EFI_DNS4_PROTOCOL  mDns4Protocol = {
  Dns4GetModeData,
  Dns4Configure,
  Dns4HostNameToIp,
  Dns4IpToHostName,
  Dns4GeneralLookUp,
  Dns4UpdateDnsCache,
  Dns4Poll,
  Dns4Cancel
};

EFI_DNS6_PROTOCOL  mDns6Protocol = {
  Dns6GetModeData,
  Dns6Configure,
  Dns6HostNameToIp,
  Dns6IpToHostName,
  Dns6GeneralLookUp,
  Dns6UpdateDnsCache,
  Dns6Poll,
  Dns6Cancel
};

/**
  Retrieve mode data of this DNS instance.

  This function is used to retrieve DNS mode data for this DNS instance.

  @param[in]   This               Pointer to EFI_DNS4_PROTOCOL instance.
  @param[out]  DnsModeData        Point to the mode data.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_NOT_STARTED         When DnsConfigData is queried, no configuration data
                                  is available because this instance has not been
                                  configured.
  @retval EFI_INVALID_PARAMETER   This is NULL or DnsModeData is NULL.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns4GetModeData (
  IN  EFI_DNS4_PROTOCOL   *This,
  OUT EFI_DNS4_MODE_DATA  *DnsModeData
  )
{
  DNS_INSTANCE  *Instance;

  EFI_TPL  OldTpl;

  UINTN  Index;

  LIST_ENTRY  *Entry;
  LIST_ENTRY  *Next;

  DNS4_SERVER_IP        *ServerItem;
  EFI_IPv4_ADDRESS      *ServerList;
  DNS4_CACHE            *CacheItem;
  EFI_DNS4_CACHE_ENTRY  *CacheList;
  EFI_STATUS            Status;

  ServerItem = NULL;
  ServerList = NULL;
  CacheItem  = NULL;
  CacheList  = NULL;
  Status     = EFI_SUCCESS;

  if ((This == NULL) || (DnsModeData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL4 (This);
  if (Instance->State == DNS_STATE_UNCONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  ZeroMem (DnsModeData, sizeof (EFI_DNS4_MODE_DATA));

  //
  // Get the current configuration data of this instance.
  //
  Status = Dns4CopyConfigure (&DnsModeData->DnsConfigData, &Instance->Dns4CfgData);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Get the DnsServerCount and DnsServerList
  //
  Index = 0;
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns4ServerList) {
    Index++;
  }
  DnsModeData->DnsServerCount = (UINT32)Index;
  ServerList                  = AllocatePool (sizeof (EFI_IPv4_ADDRESS) * DnsModeData->DnsServerCount);
  if (ServerList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    Dns4CleanConfigure (&DnsModeData->DnsConfigData);
    goto ON_EXIT;
  }

  Index = 0;
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns4ServerList) {
    ServerItem = NET_LIST_USER_STRUCT (Entry, DNS4_SERVER_IP, AllServerLink);
    CopyMem (ServerList + Index, &ServerItem->Dns4ServerIp, sizeof (EFI_IPv4_ADDRESS));
    Index++;
  }
  DnsModeData->DnsServerList = ServerList;

  //
  // Get the DnsCacheCount and DnsCacheList
  //
  Index = 0;
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns4CacheList) {
    Index++;
  }
  DnsModeData->DnsCacheCount = (UINT32)Index;
  CacheList                  = AllocatePool (sizeof (EFI_DNS4_CACHE_ENTRY) * DnsModeData->DnsCacheCount);
  if (CacheList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    Dns4CleanConfigure (&DnsModeData->DnsConfigData);
    FreePool (ServerList);
    goto ON_EXIT;
  }

  Index = 0;
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns4CacheList) {
    CacheItem = NET_LIST_USER_STRUCT (Entry, DNS4_CACHE, AllCacheLink);
    CopyMem (CacheList + Index, &CacheItem->DnsCache, sizeof (EFI_DNS4_CACHE_ENTRY));
    Index++;
  }
  DnsModeData->DnsCacheList = CacheList;

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Configure this DNS instance.

  This function is used to configure DNS mode data for this DNS instance.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  DnsConfigData       Point to the Configuration data.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         The designated protocol is not supported.
  @retval EFI_INVALID_PARAMETER   This is NULL.
                                  The StationIp address provided in DnsConfigData is not a
                                  valid unicast.
                                  DnsServerList is NULL while DnsServerListCount
                                  is not ZERO.
                                  DnsServerListCount is ZERO while DnsServerList
                                  is not NULL
  @retval EFI_OUT_OF_RESOURCES    The DNS instance data or required space could not be
                                  allocated.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. The
                                  EFI DNSv4 Protocol instance is not configured.
  @retval EFI_ALREADY_STARTED     Second call to Configure() with DnsConfigData. To
                                  reconfigure the instance the caller must call Configure()
                                  with NULL first to return driver to unconfigured state.
**/
EFI_STATUS
EFIAPI
Dns4Configure (
  IN EFI_DNS4_PROTOCOL     *This,
  IN EFI_DNS4_CONFIG_DATA  *DnsConfigData
  )
{
  EFI_STATUS    Status;
  DNS_INSTANCE  *Instance;

  EFI_TPL   OldTpl;
  IP4_ADDR  Ip;
  IP4_ADDR  Netmask;

  UINT32            ServerListCount;
  EFI_IPv4_ADDRESS  *ServerList;

  Status     = EFI_SUCCESS;
  ServerList = NULL;

  if ((This == NULL) ||
      ((DnsConfigData != NULL) && (((DnsConfigData->DnsServerListCount != 0) && (DnsConfigData->DnsServerList == NULL)) ||
                                   ((DnsConfigData->DnsServerListCount == 0) && (DnsConfigData->DnsServerList != NULL)))))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((DnsConfigData != NULL) && (DnsConfigData->Protocol != DNS_PROTOCOL_UDP)) {
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL4 (This);

  if (DnsConfigData == NULL) {
    ZeroMem (&Instance->SessionDnsServer, sizeof (EFI_IP_ADDRESS));

    //
    // Reset the Instance if ConfigData is NULL
    //
    if (!NetMapIsEmpty (&Instance->Dns4TxTokens)) {
      Dns4InstanceCancelToken (Instance, NULL);
    }

    if (Instance->UdpIo != NULL) {
      UdpIoCleanIo (Instance->UdpIo);
    }

    if (Instance->Dns4CfgData.DnsServerList != NULL) {
      FreePool (Instance->Dns4CfgData.DnsServerList);
    }

    ZeroMem (&Instance->Dns4CfgData, sizeof (EFI_DNS4_CONFIG_DATA));

    Instance->State = DNS_STATE_UNCONFIGED;
  } else {
    //
    // Configure the parameters for new operation.
    //
    CopyMem (&Ip, &DnsConfigData->StationIp, sizeof (IP4_ADDR));
    CopyMem (&Netmask, &DnsConfigData->SubnetMask, sizeof (IP4_ADDR));

    Ip      = NTOHL (Ip);
    Netmask = NTOHL (Netmask);

    if (!DnsConfigData->UseDefaultSetting &&
        ((!IP4_IS_VALID_NETMASK (Netmask) || ((Netmask != 0) && !NetIp4IsUnicast (Ip, Netmask)))))
    {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    Status = Dns4CopyConfigure (&Instance->Dns4CfgData, DnsConfigData);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    if (DnsConfigData->DnsServerListCount == 0) {
      gBS->RestoreTPL (OldTpl);

      //
      // The DNS instance will retrieve DNS server from DHCP Server
      //
      Status = GetDns4ServerFromDhcp4 (
                 Instance,
                 &ServerListCount,
                 &ServerList
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      ASSERT (ServerList != NULL);

      OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

      CopyMem (&Instance->SessionDnsServer.v4, &ServerList[0], sizeof (EFI_IPv4_ADDRESS));
    } else {
      CopyMem (&Instance->SessionDnsServer.v4, &DnsConfigData->DnsServerList[0], sizeof (EFI_IPv4_ADDRESS));
    }

    //
    // Config UDP
    //
    Status = Dns4ConfigUdp (Instance, Instance->UdpIo);
    if (EFI_ERROR (Status)) {
      if (Instance->Dns4CfgData.DnsServerList != NULL) {
        FreePool (Instance->Dns4CfgData.DnsServerList);
        Instance->Dns4CfgData.DnsServerList = NULL;
      }

      goto ON_EXIT;
    }

    //
    // Add configured DNS server used by this instance to ServerList.
    //
    Status = AddDns4ServerIp (&mDriverData->Dns4ServerList, Instance->SessionDnsServer.v4);
    if (EFI_ERROR (Status)) {
      if (Instance->Dns4CfgData.DnsServerList != NULL) {
        FreePool (Instance->Dns4CfgData.DnsServerList);
        Instance->Dns4CfgData.DnsServerList = NULL;
      }

      goto ON_EXIT;
    }

    Instance->State = DNS_STATE_CONFIGED;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Host name to host address translation.

  The HostNameToIp () function is used to translate the host name to host IP address. A
  type A query is used to get the one or more IP addresses for this host.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  HostName            Host name.
  @param[in]  Token               Point to the completion token to translate host name
                                  to host address.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  HostName is NULL. HostName string is unsupported format.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_NOT_STARTED         This instance has not been started.
**/
EFI_STATUS
EFIAPI
Dns4HostNameToIp (
  IN  EFI_DNS4_PROTOCOL          *This,
  IN  CHAR16                     *HostName,
  IN  EFI_DNS4_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS  Status;

  DNS_INSTANCE  *Instance;

  EFI_DNS4_CONFIG_DATA  *ConfigData;

  UINTN       Index;
  DNS4_CACHE  *Item;
  LIST_ENTRY  *Entry;
  LIST_ENTRY  *Next;

  CHAR8  *QueryName;

  DNS4_TOKEN_ENTRY  *TokenEntry;
  NET_BUF           *Packet;

  EFI_TPL  OldTpl;

  Status     = EFI_SUCCESS;
  Item       = NULL;
  QueryName  = NULL;
  TokenEntry = NULL;
  Packet     = NULL;

  //
  // Validate the parameters
  //
  if ((This == NULL) || (HostName == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL4 (This);

  ConfigData = &(Instance->Dns4CfgData);

  if (Instance->State != DNS_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  Token->Status = EFI_NOT_READY;

  //
  // If zero, use the parameter configured through Dns.Configure() interface.
  //
  if (Token->RetryCount == 0) {
    Token->RetryCount = ConfigData->RetryCount;
  }

  //
  // If zero, use the parameter configured through Dns.Configure() interface.
  //
  if (Token->RetryInterval == 0) {
    Token->RetryInterval = ConfigData->RetryInterval;
  }

  //
  // Minimum interval of retry is 2 second. If the retry interval is less than 2 second, then use the 2 second.
  //
  if (Token->RetryInterval < DNS_DEFAULT_TIMEOUT) {
    Token->RetryInterval = DNS_DEFAULT_TIMEOUT;
  }

  //
  // Check cache
  //
  if (ConfigData->EnableDnsCache) {
    Index = 0;
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns4CacheList) {
      Item = NET_LIST_USER_STRUCT (Entry, DNS4_CACHE, AllCacheLink);
      if (StrCmp (HostName, Item->DnsCache.HostName) == 0) {
        Index++;
      }
    }

    if (Index != 0) {
      Token->RspData.H2AData = AllocatePool (sizeof (DNS_HOST_TO_ADDR_DATA));
      if (Token->RspData.H2AData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }

      Token->RspData.H2AData->IpCount = (UINT32)Index;
      Token->RspData.H2AData->IpList  = AllocatePool (sizeof (EFI_IPv4_ADDRESS) * Index);
      if (Token->RspData.H2AData->IpList == NULL) {
        if (Token->RspData.H2AData != NULL) {
          FreePool (Token->RspData.H2AData);
        }

        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }

      Index = 0;
      NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns4CacheList) {
        Item = NET_LIST_USER_STRUCT (Entry, DNS4_CACHE, AllCacheLink);
        if (((UINT32)Index < Token->RspData.H2AData->IpCount) && (StrCmp (HostName, Item->DnsCache.HostName) == 0)) {
          CopyMem ((Token->RspData.H2AData->IpList) + Index, Item->DnsCache.IpAddress, sizeof (EFI_IPv4_ADDRESS));
          Index++;
        }
      }

      Token->Status = EFI_SUCCESS;

      if (Token->Event != NULL) {
        gBS->SignalEvent (Token->Event);
        DispatchDpc ();
      }

      Status = Token->Status;
      goto ON_EXIT;
    }
  }

  //
  // Construct DNS TokenEntry.
  //
  TokenEntry = AllocateZeroPool (sizeof (DNS4_TOKEN_ENTRY));
  if (TokenEntry == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  TokenEntry->PacketToLive  = Token->RetryInterval;
  TokenEntry->Token         = Token;
  TokenEntry->QueryHostName = AllocateZeroPool (StrSize (HostName));
  if (TokenEntry->QueryHostName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  CopyMem (TokenEntry->QueryHostName, HostName, StrSize (HostName));

  //
  // Construct QName.
  //
  QueryName = NetLibCreateDnsQName (TokenEntry->QueryHostName);
  if (QueryName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Construct DNS Query Packet.
  //
  Status = ConstructDNSQuery (Instance, QueryName, DNS_TYPE_A, DNS_CLASS_INET, &Packet);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  ASSERT (Packet != NULL);

  //
  // Save the token into the Dns4TxTokens map.
  //
  Status = NetMapInsertTail (&Instance->Dns4TxTokens, TokenEntry, Packet);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Dns Query Ip
  //
  Status = DoDnsQuery (Instance, Packet);
  if (EFI_ERROR (Status)) {
    Dns4RemoveTokenEntry (&Instance->Dns4TxTokens, TokenEntry);
  }

ON_EXIT:

  if (EFI_ERROR (Status)) {
    if (TokenEntry != NULL) {
      if (TokenEntry->QueryHostName != NULL) {
        FreePool (TokenEntry->QueryHostName);
      }

      FreePool (TokenEntry);
    }

    if (Packet != NULL) {
      NetbufFree (Packet);
    }
  }

  if (QueryName != NULL) {
    FreePool (QueryName);
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  IPv4 address to host name translation also known as Reverse DNS lookup.

  The IpToHostName() function is used to translate the host address to host name. A type PTR
  query is used to get the primary name of the host. Support of this function is optional.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  IpAddress           Ip Address.
  @param[in]  Token               Point to the completion token to translate host
                                  address to host name.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         This function is not supported.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  IpAddress is not valid IP address .
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_ALREADY_STARTED     This Token is being used in another DNS session.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns4IpToHostName (
  IN  EFI_DNS4_PROTOCOL          *This,
  IN  EFI_IPv4_ADDRESS           IpAddress,
  IN  EFI_DNS4_COMPLETION_TOKEN  *Token
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Retrieve arbitrary information from the DNS server.

  This GeneralLookup() function retrieves arbitrary information from the DNS. The caller
  supplies a QNAME, QTYPE, and QCLASS, and all of the matching RRs are returned. All
  RR content (e.g., TTL) was returned. The caller need parse the returned RR to get
  required information. The function is optional.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  QName               Pointer to Query Name.
  @param[in]  QType               Query Type.
  @param[in]  QClass              Query Name.
  @param[in]  Token               Point to the completion token to retrieve arbitrary
                                  information.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         This function is not supported. Or the requested
                                  QType is not supported
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  QName is NULL.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_ALREADY_STARTED     This Token is being used in another DNS session.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns4GeneralLookUp (
  IN  EFI_DNS4_PROTOCOL          *This,
  IN  CHAR8                      *QName,
  IN  UINT16                     QType,
  IN  UINT16                     QClass,
  IN  EFI_DNS4_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS  Status;

  DNS_INSTANCE  *Instance;

  EFI_DNS4_CONFIG_DATA  *ConfigData;

  DNS4_TOKEN_ENTRY  *TokenEntry;
  NET_BUF           *Packet;

  EFI_TPL  OldTpl;

  Status     = EFI_SUCCESS;
  TokenEntry = NULL;
  Packet     = NULL;

  //
  // Validate the parameters
  //
  if ((This == NULL) || (QName == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL4 (This);

  ConfigData = &(Instance->Dns4CfgData);

  if (Instance->State != DNS_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  Token->Status = EFI_NOT_READY;

  //
  // If zero, use the parameter configured through Dns.Configure() interface.
  //
  if (Token->RetryCount == 0) {
    Token->RetryCount = ConfigData->RetryCount;
  }

  //
  // If zero, use the parameter configured through Dns.Configure() interface.
  //
  if (Token->RetryInterval == 0) {
    Token->RetryInterval = ConfigData->RetryInterval;
  }

  //
  // Minimum interval of retry is 2 second. If the retry interval is less than 2 second, then use the 2 second.
  //
  if (Token->RetryInterval < DNS_DEFAULT_TIMEOUT) {
    Token->RetryInterval = DNS_DEFAULT_TIMEOUT;
  }

  //
  // Construct DNS TokenEntry.
  //
  TokenEntry = AllocateZeroPool (sizeof (DNS4_TOKEN_ENTRY));
  if (TokenEntry == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  TokenEntry->PacketToLive  = Token->RetryInterval;
  TokenEntry->GeneralLookUp = TRUE;
  TokenEntry->Token         = Token;

  //
  // Construct DNS Query Packet.
  //
  Status = ConstructDNSQuery (Instance, QName, QType, QClass, &Packet);
  if (EFI_ERROR (Status)) {
    if (TokenEntry != NULL) {
      FreePool (TokenEntry);
    }

    goto ON_EXIT;
  }

  ASSERT (Packet != NULL);

  //
  // Save the token into the Dns4TxTokens map.
  //
  Status = NetMapInsertTail (&Instance->Dns4TxTokens, TokenEntry, Packet);
  if (EFI_ERROR (Status)) {
    if (TokenEntry != NULL) {
      FreePool (TokenEntry);
    }

    NetbufFree (Packet);

    goto ON_EXIT;
  }

  //
  // Dns Query Ip
  //
  Status = DoDnsQuery (Instance, Packet);
  if (EFI_ERROR (Status)) {
    Dns4RemoveTokenEntry (&Instance->Dns4TxTokens, TokenEntry);

    if (TokenEntry != NULL) {
      FreePool (TokenEntry);
    }

    NetbufFree (Packet);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  This function is to update the DNS Cache.

  The UpdateDnsCache() function is used to add/delete/modify DNS cache entry. DNS cache
  can be normally dynamically updated after the DNS resolve succeeds. This function
  provided capability to manually add/delete/modify the DNS cache.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  DeleteFlag          If FALSE, this function is to add one entry to the
                                  DNS Cache. If TRUE, this function will delete
                                  matching DNS Cache entry.
  @param[in]  Override            If TRUE, the matching DNS cache entry will be
                                  overwritten with the supplied parameter. If FALSE,
                                  EFI_ACCESS_DENIED will be returned if the entry to
                                  be added is already existed.
  @param[in]  DnsCacheEntry       Pointer to DNS Cache entry.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  DnsCacheEntry.HostName is NULL.
                                  DnsCacheEntry.IpAddress is NULL.
                                  DnsCacheEntry.Timeout is zero.
  @retval EFI_ACCESS_DENIED       The DNS cache entry already exists and Override is
                                  not TRUE.
**/
EFI_STATUS
EFIAPI
Dns4UpdateDnsCache (
  IN EFI_DNS4_PROTOCOL     *This,
  IN BOOLEAN               DeleteFlag,
  IN BOOLEAN               Override,
  IN EFI_DNS4_CACHE_ENTRY  DnsCacheEntry
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  Status = EFI_SUCCESS;

  if ((DnsCacheEntry.HostName == NULL) || (DnsCacheEntry.IpAddress == NULL) || (DnsCacheEntry.Timeout == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Update Dns4Cache here.
  //
  Status = UpdateDns4Cache (&mDriverData->Dns4CacheList, DeleteFlag, Override, DnsCacheEntry);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications to increase the
  rate that data packets are moved between the communications device and the transmit
  and receive queues.
  In some systems, the periodic timer event in the managed network driver may not poll
  the underlying communications device fast enough to transmit and/or receive all data
  packets without missing incoming packets or dropping outgoing packets. Drivers and
  applications that are experiencing packet loss should try calling the Poll()
  function more often.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.

  @retval EFI_SUCCESS             Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED         This EFI DNS Protocol instance has not been started.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_TIMEOUT             Data was dropped out of the transmit and/or receive
                                  queue. Consider increasing the polling rate.
**/
EFI_STATUS
EFIAPI
Dns4Poll (
  IN EFI_DNS4_PROTOCOL  *This
  )
{
  DNS_INSTANCE       *Instance;
  EFI_UDP4_PROTOCOL  *Udp;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL4 (This);

  if (Instance->State == DNS_STATE_UNCONFIGED) {
    return EFI_NOT_STARTED;
  } else if (Instance->State == DNS_STATE_DESTROY) {
    return EFI_DEVICE_ERROR;
  }

  Udp = Instance->UdpIo->Protocol.Udp4;

  return Udp->Poll (Udp);
}

/**
  Abort an asynchronous DNS operation, including translation between IP and Host, and
  general look up behavior.

  The Cancel() function is used to abort a pending resolution request. After calling
  this function, Token.Status will be set to EFI_ABORTED and then Token.Event will be
  signaled. If the token is not in one of the queues, which usually means that the
  asynchronous operation has completed, this function will not signal the token and
  EFI_NOT_FOUND is returned.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  Token               Pointer to a token that has been issued by
                                  EFI_DNS4_PROTOCOL.HostNameToIp (),
                                  EFI_DNS4_PROTOCOL.IpToHostName() or
                                  EFI_DNS4_PROTOCOL.GeneralLookup().
                                  If NULL, all pending tokens are aborted.

  @retval EFI_SUCCESS             Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED         This EFI DNS4 Protocol instance has not been started.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_NOT_FOUND           When Token is not NULL, and the asynchronous DNS
                                  operation was not found in the transmit queue. It
                                  was either completed or was not issued by
                                  HostNameToIp(), IpToHostName() or GeneralLookup().
**/
EFI_STATUS
EFIAPI
Dns4Cancel (
  IN  EFI_DNS4_PROTOCOL          *This,
  IN  EFI_DNS4_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS    Status;
  DNS_INSTANCE  *Instance;
  EFI_TPL       OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL4 (This);

  if (Instance->State == DNS_STATE_UNCONFIGED) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Cancel the tokens specified by Token for this instance.
  //
  Status = Dns4InstanceCancelToken (Instance, Token);

  //
  // Dispatch the DPC queued by the NotifyFunction of the canceled token's events.
  //
  DispatchDpc ();

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Retrieve mode data of this DNS instance.

  This function is used to retrieve DNS mode data for this DNS instance.

  @param[in]   This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[out]  DnsModeData         Pointer to the caller-allocated storage for the
                                   EFI_DNS6_MODE_DATA data.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_NOT_STARTED         When DnsConfigData is queried, no configuration data
                                  is available because this instance has not been
                                  configured.
  @retval EFI_INVALID_PARAMETER   This is NULL or DnsModeData is NULL.
  @retval EFI_OUT_OF_RESOURCE     Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6GetModeData (
  IN  EFI_DNS6_PROTOCOL   *This,
  OUT EFI_DNS6_MODE_DATA  *DnsModeData
  )
{
  DNS_INSTANCE  *Instance;

  EFI_TPL  OldTpl;

  UINTN  Index;

  LIST_ENTRY  *Entry;
  LIST_ENTRY  *Next;

  DNS6_SERVER_IP        *ServerItem;
  EFI_IPv6_ADDRESS      *ServerList;
  DNS6_CACHE            *CacheItem;
  EFI_DNS6_CACHE_ENTRY  *CacheList;
  EFI_STATUS            Status;

  ServerItem = NULL;
  ServerList = NULL;
  CacheItem  = NULL;
  CacheList  = NULL;
  Status     = EFI_SUCCESS;

  if ((This == NULL) || (DnsModeData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL6 (This);
  if (Instance->State == DNS_STATE_UNCONFIGED) {
    Status =  EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  ZeroMem (DnsModeData, sizeof (EFI_DNS6_MODE_DATA));

  //
  // Get the current configuration data of this instance.
  //
  Status = Dns6CopyConfigure (&DnsModeData->DnsConfigData, &Instance->Dns6CfgData);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Get the DnsServerCount and DnsServerList
  //
  Index = 0;
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns6ServerList) {
    Index++;
  }
  DnsModeData->DnsServerCount = (UINT32)Index;
  ServerList                  = AllocatePool (sizeof (EFI_IPv6_ADDRESS) * DnsModeData->DnsServerCount);
  if (ServerList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    Dns6CleanConfigure (&DnsModeData->DnsConfigData);
    goto ON_EXIT;
  }

  Index = 0;
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns6ServerList) {
    ServerItem = NET_LIST_USER_STRUCT (Entry, DNS6_SERVER_IP, AllServerLink);
    CopyMem (ServerList + Index, &ServerItem->Dns6ServerIp, sizeof (EFI_IPv6_ADDRESS));
    Index++;
  }
  DnsModeData->DnsServerList = ServerList;

  //
  // Get the DnsCacheCount and DnsCacheList
  //
  Index = 0;
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns6CacheList) {
    Index++;
  }
  DnsModeData->DnsCacheCount = (UINT32)Index;
  CacheList                  = AllocatePool (sizeof (EFI_DNS6_CACHE_ENTRY) * DnsModeData->DnsCacheCount);
  if (CacheList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    Dns6CleanConfigure (&DnsModeData->DnsConfigData);
    FreePool (ServerList);
    goto ON_EXIT;
  }

  Index = 0;
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns6CacheList) {
    CacheItem = NET_LIST_USER_STRUCT (Entry, DNS6_CACHE, AllCacheLink);
    CopyMem (CacheList + Index, &CacheItem->DnsCache, sizeof (EFI_DNS6_CACHE_ENTRY));
    Index++;
  }
  DnsModeData->DnsCacheList = CacheList;

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Configure this DNS instance.

  The Configure() function is used to set and change the configuration data for this
  EFI DNSv6 Protocol driver instance. Reset the DNS instance if DnsConfigData is NULL.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  DnsConfigData       Pointer to the configuration data structure. All associated
                                  storage to be allocated and released by caller.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER    This is NULL.
                                  The StationIp address provided in DnsConfigData is not zero and not a valid unicast.
                                  DnsServerList is NULL while DnsServerList Count is not ZERO.
                                  DnsServerList Count is ZERO while DnsServerList is not NULL.
  @retval EFI_OUT_OF_RESOURCES    The DNS instance data or required space could not be allocated.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. The
                                  EFI DNSv6 Protocol instance is not configured.
  @retval EFI_UNSUPPORTED         The designated protocol is not supported.
  @retval EFI_ALREADY_STARTED     Second call to Configure() with DnsConfigData. To
                                  reconfigure the instance the caller must call Configure() with
                                  NULL first to return driver to unconfigured state.
**/
EFI_STATUS
EFIAPI
Dns6Configure (
  IN EFI_DNS6_PROTOCOL     *This,
  IN EFI_DNS6_CONFIG_DATA  *DnsConfigData
  )
{
  EFI_STATUS    Status;
  DNS_INSTANCE  *Instance;

  EFI_TPL  OldTpl;

  UINT32            ServerListCount;
  EFI_IPv6_ADDRESS  *ServerList;

  Status     = EFI_SUCCESS;
  ServerList = NULL;

  if ((This == NULL) ||
      ((DnsConfigData != NULL) && (((DnsConfigData->DnsServerCount != 0) && (DnsConfigData->DnsServerList == NULL)) ||
                                   ((DnsConfigData->DnsServerCount == 0) && (DnsConfigData->DnsServerList != NULL)))))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((DnsConfigData != NULL) && (DnsConfigData->Protocol != DNS_PROTOCOL_UDP)) {
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL6 (This);

  if (DnsConfigData == NULL) {
    ZeroMem (&Instance->SessionDnsServer, sizeof (EFI_IP_ADDRESS));

    //
    // Reset the Instance if ConfigData is NULL
    //
    if (!NetMapIsEmpty (&Instance->Dns6TxTokens)) {
      Dns6InstanceCancelToken (Instance, NULL);
    }

    if (Instance->UdpIo != NULL) {
      UdpIoCleanIo (Instance->UdpIo);
    }

    if (Instance->Dns6CfgData.DnsServerList != NULL) {
      FreePool (Instance->Dns6CfgData.DnsServerList);
    }

    ZeroMem (&Instance->Dns6CfgData, sizeof (EFI_DNS6_CONFIG_DATA));

    Instance->State = DNS_STATE_UNCONFIGED;
  } else {
    //
    // Configure the parameters for new operation.
    //
    if (!NetIp6IsUnspecifiedAddr (&DnsConfigData->StationIp) && !NetIp6IsValidUnicast (&DnsConfigData->StationIp)) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    Status = Dns6CopyConfigure (&Instance->Dns6CfgData, DnsConfigData);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    if (DnsConfigData->DnsServerCount == 0) {
      gBS->RestoreTPL (OldTpl);

      //
      // The DNS instance will retrieve DNS server from DHCP Server.
      //
      Status = GetDns6ServerFromDhcp6 (
                 Instance->Service->ImageHandle,
                 Instance->Service->ControllerHandle,
                 &ServerListCount,
                 &ServerList
                 );
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      ASSERT (ServerList != NULL);

      OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

      CopyMem (&Instance->SessionDnsServer.v6, &ServerList[0], sizeof (EFI_IPv6_ADDRESS));
    } else {
      CopyMem (&Instance->SessionDnsServer.v6, &DnsConfigData->DnsServerList[0], sizeof (EFI_IPv6_ADDRESS));
    }

    //
    // Config UDP
    //
    gBS->RestoreTPL (OldTpl);
    Status = Dns6ConfigUdp (Instance, Instance->UdpIo);
    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
    if (EFI_ERROR (Status)) {
      if (Instance->Dns6CfgData.DnsServerList != NULL) {
        FreePool (Instance->Dns6CfgData.DnsServerList);
        Instance->Dns6CfgData.DnsServerList = NULL;
      }

      goto ON_EXIT;
    }

    //
    // Add configured DNS server used by this instance to ServerList.
    //
    Status = AddDns6ServerIp (&mDriverData->Dns6ServerList, Instance->SessionDnsServer.v6);
    if (EFI_ERROR (Status)) {
      if (Instance->Dns6CfgData.DnsServerList != NULL) {
        FreePool (Instance->Dns6CfgData.DnsServerList);
        Instance->Dns6CfgData.DnsServerList = NULL;
      }

      goto ON_EXIT;
    }

    Instance->State = DNS_STATE_CONFIGED;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Host name to host address translation.

  The HostNameToIp () function is used to translate the host name to host IP address. A
  type AAAA query is used to get the one or more IPv6 addresses for this host.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  HostName            Host name.
  @param[in]  Token               Point to the completion token to translate host name
                                  to host address.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  HostName is NULL or buffer contained unsupported characters.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_ALREADY_STARTED     This Token is being used in another DNS session.
  @retval EFI_NOT_STARTED         This instance has not been started.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6HostNameToIp (
  IN  EFI_DNS6_PROTOCOL          *This,
  IN  CHAR16                     *HostName,
  IN  EFI_DNS6_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS  Status;

  DNS_INSTANCE  *Instance;

  EFI_DNS6_CONFIG_DATA  *ConfigData;

  UINTN       Index;
  DNS6_CACHE  *Item;
  LIST_ENTRY  *Entry;
  LIST_ENTRY  *Next;

  CHAR8  *QueryName;

  DNS6_TOKEN_ENTRY  *TokenEntry;
  NET_BUF           *Packet;

  EFI_TPL  OldTpl;

  Status     = EFI_SUCCESS;
  Item       = NULL;
  QueryName  = NULL;
  TokenEntry = NULL;
  Packet     = NULL;

  //
  // Validate the parameters
  //
  if ((This == NULL) || (HostName == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL6 (This);

  ConfigData = &(Instance->Dns6CfgData);

  if (Instance->State != DNS_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  Token->Status = EFI_NOT_READY;

  //
  // If zero, use the parameter configured through Dns.Configure() interface.
  //
  if (Token->RetryCount == 0) {
    Token->RetryCount = ConfigData->RetryCount;
  }

  //
  // If zero, use the parameter configured through Dns.Configure() interface.
  //
  if (Token->RetryInterval == 0) {
    Token->RetryInterval = ConfigData->RetryInterval;
  }

  //
  // Minimum interval of retry is 2 second. If the retry interval is less than 2 second, then use the 2 second.
  //
  if (Token->RetryInterval < DNS_DEFAULT_TIMEOUT) {
    Token->RetryInterval = DNS_DEFAULT_TIMEOUT;
  }

  //
  // Check cache
  //
  if (ConfigData->EnableDnsCache) {
    Index = 0;
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns6CacheList) {
      Item = NET_LIST_USER_STRUCT (Entry, DNS6_CACHE, AllCacheLink);
      if (StrCmp (HostName, Item->DnsCache.HostName) == 0) {
        Index++;
      }
    }

    if (Index != 0) {
      Token->RspData.H2AData = AllocatePool (sizeof (DNS6_HOST_TO_ADDR_DATA));
      if (Token->RspData.H2AData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }

      Token->RspData.H2AData->IpCount = (UINT32)Index;
      Token->RspData.H2AData->IpList  = AllocatePool (sizeof (EFI_IPv6_ADDRESS) * Index);
      if (Token->RspData.H2AData->IpList == NULL) {
        if (Token->RspData.H2AData != NULL) {
          FreePool (Token->RspData.H2AData);
        }

        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }

      Index = 0;
      NET_LIST_FOR_EACH_SAFE (Entry, Next, &mDriverData->Dns6CacheList) {
        Item = NET_LIST_USER_STRUCT (Entry, DNS6_CACHE, AllCacheLink);
        if (((UINT32)Index < Token->RspData.H2AData->IpCount) && (StrCmp (HostName, Item->DnsCache.HostName) == 0)) {
          CopyMem ((Token->RspData.H2AData->IpList) + Index, Item->DnsCache.IpAddress, sizeof (EFI_IPv6_ADDRESS));
          Index++;
        }
      }

      Token->Status = EFI_SUCCESS;

      if (Token->Event != NULL) {
        gBS->SignalEvent (Token->Event);
        DispatchDpc ();
      }

      Status = Token->Status;
      goto ON_EXIT;
    }
  }

  //
  // Construct DNS TokenEntry.
  //
  TokenEntry = AllocateZeroPool (sizeof (DNS6_TOKEN_ENTRY));
  if (TokenEntry == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  TokenEntry->PacketToLive  = Token->RetryInterval;
  TokenEntry->Token         = Token;
  TokenEntry->QueryHostName = AllocateZeroPool (StrSize (HostName));
  if (TokenEntry->QueryHostName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  CopyMem (TokenEntry->QueryHostName, HostName, StrSize (HostName));

  //
  // Construct QName.
  //
  QueryName = NetLibCreateDnsQName (TokenEntry->QueryHostName);
  if (QueryName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Construct DNS Query Packet.
  //
  Status = ConstructDNSQuery (Instance, QueryName, DNS_TYPE_AAAA, DNS_CLASS_INET, &Packet);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  ASSERT (Packet != NULL);

  //
  // Save the token into the Dns6TxTokens map.
  //
  Status = NetMapInsertTail (&Instance->Dns6TxTokens, TokenEntry, Packet);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Dns Query Ip
  //
  Status = DoDnsQuery (Instance, Packet);
  if (EFI_ERROR (Status)) {
    Dns6RemoveTokenEntry (&Instance->Dns6TxTokens, TokenEntry);
  }

ON_EXIT:

  if (EFI_ERROR (Status)) {
    if (TokenEntry != NULL) {
      if (TokenEntry->QueryHostName != NULL) {
        FreePool (TokenEntry->QueryHostName);
      }

      FreePool (TokenEntry);
    }

    if (Packet != NULL) {
      NetbufFree (Packet);
    }
  }

  if (QueryName != NULL) {
    FreePool (QueryName);
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Host address to host name translation.

  The IpToHostName () function is used to translate the host address to host name. A
  type PTR query is used to get the primary name of the host. Implementation can choose
  to support this function or not.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  IpAddress           Ip Address.
  @param[in]  Token               Point to the completion token to translate host
                                  address to host name.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         This function is not supported.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  IpAddress is not valid IP address.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_NOT_STARTED         This instance has not been started.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6IpToHostName (
  IN  EFI_DNS6_PROTOCOL          *This,
  IN  EFI_IPv6_ADDRESS           IpAddress,
  IN  EFI_DNS6_COMPLETION_TOKEN  *Token
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function provides capability to retrieve arbitrary information from the DNS
  server.

  This GeneralLookup() function retrieves arbitrary information from the DNS. The caller
  supplies a QNAME, QTYPE, and QCLASS, and all of the matching RRs are returned. All
  RR content (e.g., TTL) was returned. The caller need parse the returned RR to get
  required information. The function is optional. Implementation can choose to support
  it or not.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  QName               Pointer to Query Name.
  @param[in]  QType               Query Type.
  @param[in]  QClass              Query Name.
  @param[in]  Token               Point to the completion token to retrieve arbitrary
                                  information.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         This function is not supported. Or the requested
                                  QType is not supported
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  QName is NULL.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_NOT_STARTED         This instance has not been started.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6GeneralLookUp (
  IN  EFI_DNS6_PROTOCOL          *This,
  IN  CHAR8                      *QName,
  IN  UINT16                     QType,
  IN  UINT16                     QClass,
  IN  EFI_DNS6_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS  Status;

  DNS_INSTANCE  *Instance;

  EFI_DNS6_CONFIG_DATA  *ConfigData;

  DNS6_TOKEN_ENTRY  *TokenEntry;
  NET_BUF           *Packet;

  EFI_TPL  OldTpl;

  Status     = EFI_SUCCESS;
  TokenEntry = NULL;
  Packet     = NULL;

  //
  // Validate the parameters
  //
  if ((This == NULL) || (QName == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL6 (This);

  ConfigData = &(Instance->Dns6CfgData);

  if (Instance->State != DNS_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  Token->Status = EFI_NOT_READY;

  //
  // If zero, use the parameter configured through Dns.Configure() interface.
  //
  if (Token->RetryCount == 0) {
    Token->RetryCount = ConfigData->RetryCount;
  }

  //
  // If zero, use the parameter configured through Dns.Configure() interface.
  //
  if (Token->RetryInterval == 0) {
    Token->RetryInterval = ConfigData->RetryInterval;
  }

  //
  // Minimum interval of retry is 2 second. If the retry interval is less than 2 second, then use the 2 second.
  //
  if (Token->RetryInterval < DNS_DEFAULT_TIMEOUT) {
    Token->RetryInterval = DNS_DEFAULT_TIMEOUT;
  }

  //
  // Construct DNS TokenEntry.
  //
  TokenEntry = AllocateZeroPool (sizeof (DNS6_TOKEN_ENTRY));
  if (TokenEntry == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  TokenEntry->PacketToLive  = Token->RetryInterval;
  TokenEntry->GeneralLookUp = TRUE;
  TokenEntry->Token         = Token;

  //
  // Construct DNS Query Packet.
  //
  Status = ConstructDNSQuery (Instance, QName, QType, QClass, &Packet);
  if (EFI_ERROR (Status)) {
    if (TokenEntry != NULL) {
      FreePool (TokenEntry);
    }

    goto ON_EXIT;
  }

  ASSERT (Packet != NULL);

  //
  // Save the token into the Dns6TxTokens map.
  //
  Status = NetMapInsertTail (&Instance->Dns6TxTokens, TokenEntry, Packet);
  if (EFI_ERROR (Status)) {
    if (TokenEntry != NULL) {
      FreePool (TokenEntry);
    }

    NetbufFree (Packet);

    goto ON_EXIT;
  }

  //
  // Dns Query Ip
  //
  Status = DoDnsQuery (Instance, Packet);
  if (EFI_ERROR (Status)) {
    Dns6RemoveTokenEntry (&Instance->Dns6TxTokens, TokenEntry);

    if (TokenEntry != NULL) {
      FreePool (TokenEntry);
    }

    NetbufFree (Packet);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  This function is to update the DNS Cache.

  The UpdateDnsCache() function is used to add/delete/modify DNS cache entry. DNS cache
  can be normally dynamically updated after the DNS resolve succeeds. This function
  provided capability to manually add/delete/modify the DNS cache.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  DeleteFlag          If FALSE, this function is to add one entry to the
                                  DNS Cache. If TRUE, this function will delete
                                  matching DNS Cache entry.
  @param[in]  Override            If TRUE, the matching DNS cache entry will be
                                  overwritten with the supplied parameter. If FALSE,
                                  EFI_ACCESS_DENIED will be returned if the entry to
                                  be added is already existed.
  @param[in]  DnsCacheEntry       Pointer to DNS Cache entry.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  DnsCacheEntry.HostName is NULL.
                                  DnsCacheEntry.IpAddress is NULL.
                                  DnsCacheEntry.Timeout is zero.
  @retval EFI_ACCESS_DENIED       The DNS cache entry already exists and Override is
                                  not TRUE.
  @retval EFI_OUT_OF_RESOURCE     Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6UpdateDnsCache (
  IN EFI_DNS6_PROTOCOL     *This,
  IN BOOLEAN               DeleteFlag,
  IN BOOLEAN               Override,
  IN EFI_DNS6_CACHE_ENTRY  DnsCacheEntry
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  Status = EFI_SUCCESS;

  if ((DnsCacheEntry.HostName == NULL) || (DnsCacheEntry.IpAddress == NULL) || (DnsCacheEntry.Timeout == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Update Dns6Cache here.
  //
  Status = UpdateDns6Cache (&mDriverData->Dns6CacheList, DeleteFlag, Override, DnsCacheEntry);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications to increase the
  rate that data packets are moved between the communications device and the transmit
  and receive queues.

  In some systems, the periodic timer event in the managed network driver may not poll
  the underlying communications device fast enough to transmit and/or receive all data
  packets without missing incoming packets or dropping outgoing packets. Drivers and
  applications that are experiencing packet loss should try calling the Poll()
  function more often.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.

  @retval EFI_SUCCESS             Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED         This EFI DNS Protocol instance has not been started.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_NO_MAPPING          There is no source address is available for use.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_TIMEOUT             Data was dropped out of the transmit and/or receive
                                  queue. Consider increasing the polling rate.
**/
EFI_STATUS
EFIAPI
Dns6Poll (
  IN EFI_DNS6_PROTOCOL  *This
  )
{
  DNS_INSTANCE       *Instance;
  EFI_UDP6_PROTOCOL  *Udp;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL6 (This);

  if (Instance->State == DNS_STATE_UNCONFIGED) {
    return EFI_NOT_STARTED;
  } else if (Instance->State == DNS_STATE_DESTROY) {
    return EFI_DEVICE_ERROR;
  }

  Udp = Instance->UdpIo->Protocol.Udp6;

  return Udp->Poll (Udp);
}

/**
  Abort an asynchronous DNS operation, including translation between IP and Host, and
  general look up behavior.

  The Cancel() function is used to abort a pending resolution request. After calling
  this function, Token.Status will be set to EFI_ABORTED and then Token.Event will be
  signaled. If the token is not in one of the queues, which usually means that the
  asynchronous operation has completed, this function will not signal the token and
  EFI_NOT_FOUND is returned.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  Token               Pointer to a token that has been issued by
                                  EFI_DNS6_PROTOCOL.HostNameToIp (),
                                  EFI_DNS6_PROTOCOL.IpToHostName() or
                                  EFI_DNS6_PROTOCOL.GeneralLookup().
                                  If NULL, all pending tokens are aborted.

  @retval EFI_SUCCESS             Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED         This EFI DNS6 Protocol instance has not been started.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_NOT_FOUND           When Token is not NULL, and the asynchronous DNS
                                  operation was not found in the transmit queue. It
                                  was either completed or was not issued by
                                  HostNameToIp(), IpToHostName() or GeneralLookup().
**/
EFI_STATUS
EFIAPI
Dns6Cancel (
  IN  EFI_DNS6_PROTOCOL          *This,
  IN  EFI_DNS6_COMPLETION_TOKEN  *Token
  )
{
  EFI_STATUS    Status;
  DNS_INSTANCE  *Instance;
  EFI_TPL       OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL6 (This);

  if (Instance->State == DNS_STATE_UNCONFIGED) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Cancel the tokens specified by Token for this instance.
  //
  Status = Dns6InstanceCancelToken (Instance, Token);

  //
  // Dispatch the DPC queued by the NotifyFunction of the canceled token's events.
  //
  DispatchDpc ();

  gBS->RestoreTPL (OldTpl);

  return Status;
}
