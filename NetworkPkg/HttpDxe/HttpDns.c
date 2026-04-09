/** @file
  Routines for HttpDxe driver to perform DNS resolution based on UEFI DNS protocols.

Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpDriver.h"

/**
  Retrieve the host address using the EFI_DNS4_PROTOCOL.

  @param[in]  HttpInstance        Pointer to HTTP_PROTOCOL instance.
  @param[in]  HostName            Pointer to buffer containing hostname.
  @param[out] IpAddress           On output, pointer to buffer containing IPv4 address.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
HttpDns4 (
  IN     HTTP_PROTOCOL  *HttpInstance,
  IN     CHAR16         *HostName,
  OUT EFI_IPv4_ADDRESS  *IpAddress
  )
{
  EFI_STATUS                 Status;
  EFI_DNS4_PROTOCOL          *Dns4;
  EFI_DNS4_CONFIG_DATA       Dns4CfgData;
  EFI_DNS4_COMPLETION_TOKEN  Token;
  BOOLEAN                    IsDone;
  HTTP_SERVICE               *Service;
  EFI_HANDLE                 Dns4Handle;
  EFI_IP4_CONFIG2_PROTOCOL   *Ip4Config2;
  UINTN                      DnsServerListCount;
  EFI_IPv4_ADDRESS           *DnsServerList;
  UINTN                      DataSize;

  Service = HttpInstance->Service;
  ASSERT (Service != NULL);

  DnsServerList      = NULL;
  DnsServerListCount = 0;
  ZeroMem (&Token, sizeof (EFI_DNS4_COMPLETION_TOKEN));

  //
  // Get DNS server list from EFI IPv4 Configuration II protocol.
  //
  Status = gBS->HandleProtocol (Service->ControllerHandle, &gEfiIp4Config2ProtocolGuid, (VOID **)&Ip4Config2);
  if (!EFI_ERROR (Status)) {
    //
    // Get the required size.
    //
    DataSize = 0;
    Status   = Ip4Config2->GetData (Ip4Config2, Ip4Config2DataTypeDnsServer, &DataSize, NULL);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      DnsServerList = AllocatePool (DataSize);
      if (DnsServerList == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = Ip4Config2->GetData (Ip4Config2, Ip4Config2DataTypeDnsServer, &DataSize, DnsServerList);
      if (EFI_ERROR (Status)) {
        FreePool (DnsServerList);
        DnsServerList = NULL;
      } else {
        DnsServerListCount = DataSize / sizeof (EFI_IPv4_ADDRESS);
      }
    }
  }

  Dns4Handle = NULL;
  Dns4       = NULL;

  //
  // Create a DNS child instance and get the protocol.
  //
  Status = NetLibCreateServiceChild (
             Service->ControllerHandle,
             Service->Ip4DriverBindingHandle,
             &gEfiDns4ServiceBindingProtocolGuid,
             &Dns4Handle
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (
                  Dns4Handle,
                  &gEfiDns4ProtocolGuid,
                  (VOID **)&Dns4,
                  Service->Ip4DriverBindingHandle,
                  Service->ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Configure DNS4 instance for the DNS server address and protocol.
  //
  ZeroMem (&Dns4CfgData, sizeof (Dns4CfgData));
  Dns4CfgData.DnsServerListCount = DnsServerListCount;
  Dns4CfgData.DnsServerList      = DnsServerList;
  Dns4CfgData.UseDefaultSetting  = HttpInstance->IPv4Node.UseDefaultAddress;
  Dns4CfgData.RetryInterval      = PcdGet32 (PcdHttpDnsRetryInterval);
  Dns4CfgData.RetryCount         = PcdGet32 (PcdHttpDnsRetryCount);
  if (!Dns4CfgData.UseDefaultSetting) {
    IP4_COPY_ADDRESS (&Dns4CfgData.StationIp, &HttpInstance->IPv4Node.LocalAddress);
    IP4_COPY_ADDRESS (&Dns4CfgData.SubnetMask, &HttpInstance->IPv4Node.LocalSubnet);
  }

  Dns4CfgData.EnableDnsCache = TRUE;
  Dns4CfgData.Protocol       = EFI_IP_PROTO_UDP;
  Status                     = Dns4->Configure (
                                       Dns4,
                                       &Dns4CfgData
                                       );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Create event to set the is done flag when name resolution is finished.
  //
  ZeroMem (&Token, sizeof (Token));
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpCommonNotify,
                  &IsDone,
                  &Token.Event
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Start asynchronous name resolution.
  //
  Token.Status = EFI_NOT_READY;
  IsDone       = FALSE;
  Status       = Dns4->HostNameToIp (Dns4, HostName, &Token);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  while (!IsDone) {
    Dns4->Poll (Dns4);
  }

  //
  // Name resolution is done, check result.
  //
  Status = Token.Status;
  if (!EFI_ERROR (Status)) {
    if (Token.RspData.H2AData == NULL) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    if ((Token.RspData.H2AData->IpCount == 0) || (Token.RspData.H2AData->IpList == NULL)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    //
    // We just return the first IP address from DNS protocol.
    //
    IP4_COPY_ADDRESS (IpAddress, Token.RspData.H2AData->IpList);
    Status = EFI_SUCCESS;
  }

Exit:

  if (Token.Event != NULL) {
    gBS->CloseEvent (Token.Event);
  }

  if (Token.RspData.H2AData != NULL) {
    if (Token.RspData.H2AData->IpList != NULL) {
      FreePool (Token.RspData.H2AData->IpList);
    }

    FreePool (Token.RspData.H2AData);
  }

  if (Dns4 != NULL) {
    Dns4->Configure (Dns4, NULL);

    gBS->CloseProtocol (
           Dns4Handle,
           &gEfiDns4ProtocolGuid,
           Service->Ip4DriverBindingHandle,
           Service->ControllerHandle
           );
  }

  if (Dns4Handle != NULL) {
    NetLibDestroyServiceChild (
      Service->ControllerHandle,
      Service->Ip4DriverBindingHandle,
      &gEfiDns4ServiceBindingProtocolGuid,
      Dns4Handle
      );
  }

  if (DnsServerList != NULL) {
    FreePool (DnsServerList);
  }

  return Status;
}

/**
  Retrieve the host address using the EFI_DNS6_PROTOCOL.

  @param[in]  HttpInstance        Pointer to HTTP_PROTOCOL instance.
  @param[in]  HostName            Pointer to buffer containing hostname.
  @param[out] IpAddress           On output, pointer to buffer containing IPv6 address.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
HttpDns6 (
  IN     HTTP_PROTOCOL  *HttpInstance,
  IN     CHAR16         *HostName,
  OUT EFI_IPv6_ADDRESS  *IpAddress
  )
{
  EFI_STATUS                 Status;
  HTTP_SERVICE               *Service;
  EFI_DNS6_PROTOCOL          *Dns6;
  EFI_DNS6_CONFIG_DATA       Dns6ConfigData;
  EFI_DNS6_COMPLETION_TOKEN  Token;
  EFI_HANDLE                 Dns6Handle;
  EFI_IP6_CONFIG_PROTOCOL    *Ip6Config;
  EFI_IPv6_ADDRESS           *DnsServerList;
  UINTN                      DnsServerListCount;
  UINTN                      DataSize;
  BOOLEAN                    IsDone;

  Service = HttpInstance->Service;
  ASSERT (Service != NULL);

  DnsServerList      = NULL;
  DnsServerListCount = 0;
  Dns6               = NULL;
  Dns6Handle         = NULL;
  ZeroMem (&Token, sizeof (EFI_DNS6_COMPLETION_TOKEN));

  //
  // Get DNS server list from EFI IPv6 Configuration protocol.
  //
  Status = gBS->HandleProtocol (Service->ControllerHandle, &gEfiIp6ConfigProtocolGuid, (VOID **)&Ip6Config);
  if (!EFI_ERROR (Status)) {
    //
    // Get the required size.
    //
    DataSize = 0;
    Status   = Ip6Config->GetData (Ip6Config, Ip6ConfigDataTypeDnsServer, &DataSize, NULL);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      DnsServerList = AllocatePool (DataSize);
      if (DnsServerList == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = Ip6Config->GetData (Ip6Config, Ip6ConfigDataTypeDnsServer, &DataSize, DnsServerList);
      if (EFI_ERROR (Status)) {
        FreePool (DnsServerList);
        DnsServerList = NULL;
      } else {
        DnsServerListCount = DataSize / sizeof (EFI_IPv6_ADDRESS);
      }
    }
  }

  //
  // Create a DNSv6 child instance and get the protocol.
  //
  Status = NetLibCreateServiceChild (
             Service->ControllerHandle,
             Service->Ip6DriverBindingHandle,
             &gEfiDns6ServiceBindingProtocolGuid,
             &Dns6Handle
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (
                  Dns6Handle,
                  &gEfiDns6ProtocolGuid,
                  (VOID **)&Dns6,
                  Service->Ip6DriverBindingHandle,
                  Service->ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Configure DNS6 instance for the DNS server address and protocol.
  //
  ZeroMem (&Dns6ConfigData, sizeof (EFI_DNS6_CONFIG_DATA));
  Dns6ConfigData.DnsServerCount = (UINT32)DnsServerListCount;
  Dns6ConfigData.DnsServerList  = DnsServerList;
  Dns6ConfigData.EnableDnsCache = TRUE;
  Dns6ConfigData.Protocol       = EFI_IP_PROTO_UDP;
  Dns6ConfigData.RetryInterval  = PcdGet32 (PcdHttpDnsRetryInterval);
  Dns6ConfigData.RetryCount     = PcdGet32 (PcdHttpDnsRetryCount);
  IP6_COPY_ADDRESS (&Dns6ConfigData.StationIp, &HttpInstance->Ipv6Node.LocalAddress);
  Status = Dns6->Configure (
                   Dns6,
                   &Dns6ConfigData
                   );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Token.Status = EFI_NOT_READY;
  IsDone       = FALSE;
  //
  // Create event to set the  IsDone flag when name resolution is finished.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpCommonNotify,
                  &IsDone,
                  &Token.Event
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Start asynchronous name resolution.
  //
  Status = Dns6->HostNameToIp (Dns6, HostName, &Token);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  while (!IsDone) {
    Dns6->Poll (Dns6);
  }

  //
  // Name resolution is done, check result.
  //
  Status = Token.Status;
  if (!EFI_ERROR (Status)) {
    if (Token.RspData.H2AData == NULL) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    if ((Token.RspData.H2AData->IpCount == 0) || (Token.RspData.H2AData->IpList == NULL)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    //
    // We just return the first IPv6 address from DNS protocol.
    //
    IP6_COPY_ADDRESS (IpAddress, Token.RspData.H2AData->IpList);
    Status = EFI_SUCCESS;
  }

Exit:

  if (Token.Event != NULL) {
    gBS->CloseEvent (Token.Event);
  }

  if (Token.RspData.H2AData != NULL) {
    if (Token.RspData.H2AData->IpList != NULL) {
      FreePool (Token.RspData.H2AData->IpList);
    }

    FreePool (Token.RspData.H2AData);
  }

  if (Dns6 != NULL) {
    Dns6->Configure (Dns6, NULL);

    gBS->CloseProtocol (
           Dns6Handle,
           &gEfiDns6ProtocolGuid,
           Service->Ip6DriverBindingHandle,
           Service->ControllerHandle
           );
  }

  if (Dns6Handle != NULL) {
    NetLibDestroyServiceChild (
      Service->ControllerHandle,
      Service->Ip6DriverBindingHandle,
      &gEfiDns6ServiceBindingProtocolGuid,
      Dns6Handle
      );
  }

  if (DnsServerList != NULL) {
    FreePool (DnsServerList);
  }

  return Status;
}
