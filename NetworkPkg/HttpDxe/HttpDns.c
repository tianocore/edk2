/** @file
  Routines for HttpDxe driver to perform DNS resolution based on UEFI DNS protocols.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  IN     HTTP_PROTOCOL            *HttpInstance,
  IN     CHAR16                   *HostName,
     OUT EFI_IPv4_ADDRESS         *IpAddress                
  )
{
  EFI_STATUS                      Status;
  EFI_DNS4_PROTOCOL               *Dns4;
  EFI_DNS4_CONFIG_DATA            Dns4CfgData;
  EFI_DNS4_COMPLETION_TOKEN       Token;
  BOOLEAN                         IsDone;
  HTTP_SERVICE                    *Service;
  EFI_HANDLE                      Dns4Handle;
  EFI_IP4_CONFIG2_PROTOCOL        *Ip4Config2;
  UINTN                           DnsServerListCount;
  EFI_IPv4_ADDRESS                *DnsServerList;
  UINTN                           DataSize;
  

  Service = HttpInstance->Service;
  ASSERT (Service != NULL);

  DnsServerList      = NULL;
  DnsServerListCount = 0;
  ZeroMem (&Token, sizeof (EFI_DNS4_COMPLETION_TOKEN));

  //
  // Get DNS server list from EFI IPv4 Configuration II protocol.
  //
  Status = gBS->HandleProtocol (Service->ControllerHandle, &gEfiIp4Config2ProtocolGuid, (VOID **) &Ip4Config2);
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

      Status   = Ip4Config2->GetData (Ip4Config2, Ip4Config2DataTypeDnsServer, &DataSize, DnsServerList);
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
             Service->ImageHandle,
             &gEfiDns4ServiceBindingProtocolGuid,
             &Dns4Handle
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }  

  Status = gBS->OpenProtocol (
                  Dns4Handle,
                  &gEfiDns4ProtocolGuid,
                  (VOID **) &Dns4,
                  Service->ImageHandle,
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
  if (!Dns4CfgData.UseDefaultSetting) {
    IP4_COPY_ADDRESS (&Dns4CfgData.StationIp, &HttpInstance->IPv4Node.LocalAddress);
    IP4_COPY_ADDRESS (&Dns4CfgData.SubnetMask, &HttpInstance->IPv4Node.LocalSubnet);
  }
  Dns4CfgData.EnableDnsCache     = TRUE;
  Dns4CfgData.Protocol           = EFI_IP_PROTO_UDP;
  Status = Dns4->Configure (
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
  Status = Dns4->HostNameToIp (Dns4, HostName, &Token);
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
    if (Token.RspData.H2AData->IpCount == 0 || Token.RspData.H2AData->IpList == NULL) {
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
          Service->ImageHandle,
          Service->ControllerHandle
          );
  }

  if (Dns4Handle != NULL) {
    NetLibDestroyServiceChild (
      Service->ControllerHandle,
      Service->ImageHandle,
      &gEfiDns4ServiceBindingProtocolGuid,
      Dns4Handle
      );
  }

  if (DnsServerList != NULL) {
    FreePool (DnsServerList);
  }
  
  return Status;
}
