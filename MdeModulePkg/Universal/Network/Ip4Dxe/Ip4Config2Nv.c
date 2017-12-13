/** @file
  Helper functions for configuring or getting the parameters relating to Ip4.

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip4Impl.h"

CHAR16    mIp4Config2StorageName[]     = L"IP4_CONFIG2_IFR_NVDATA";

/**
  Calculate the prefix length of the IPv4 subnet mask.

  @param[in]  SubnetMask The IPv4 subnet mask.

  @return The prefix length of the subnet mask.
  @retval 0 Other errors as indicated.
  
**/
UINT8
GetSubnetMaskPrefixLength (
  IN EFI_IPv4_ADDRESS  *SubnetMask
  )
{
  UINT8   Len;
  UINT32  ReverseMask;

  //
  // The SubnetMask is in network byte order.
  //
  ReverseMask = SwapBytes32 (*(UINT32 *)&SubnetMask[0]);

  //
  // Reverse it.
  //
  ReverseMask = ~ReverseMask;

  if ((ReverseMask & (ReverseMask + 1)) != 0) {
    return 0;
  }

  Len = 0;

  while (ReverseMask != 0) {
    ReverseMask = ReverseMask >> 1;
    Len++;
  }

  return (UINT8) (32 - Len);
}

/**
  Convert the decimal dotted IPv4 address into the binary IPv4 address.

  @param[in]   Str             The UNICODE string.
  @param[out]  Ip              The storage to return the IPv4 address.

  @retval EFI_SUCCESS           The binary IP address is returned in Ip.
  @retval EFI_INVALID_PARAMETER The IP string is malformatted.
  
**/
EFI_STATUS
Ip4Config2StrToIp (
  IN  CHAR16            *Str,
  OUT EFI_IPv4_ADDRESS  *Ip
  )
{
  UINTN Index;
  UINTN Number;

  Index = 0;

  while (*Str != L'\0') {

    if (Index > 3) {
      return EFI_INVALID_PARAMETER;
    }

    Number = 0;
    while ((*Str >= L'0') && (*Str <= L'9')) {
      Number = Number * 10 + (*Str - L'0');
      Str++;
    }

    if (Number > 0xFF) {
      return EFI_INVALID_PARAMETER;
    }

    Ip->Addr[Index] = (UINT8) Number;

    if ((*Str != L'\0') && (*Str != L'.')) {
      //
      // The current character should be either the NULL terminator or
      // the dot delimiter.
      //
      return EFI_INVALID_PARAMETER;
    }

    if (*Str == L'.') {
      //
      // Skip the delimiter.
      //
      Str++;
    }

    Index++;
  }

  if (Index != 4) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Convert the decimal dotted IPv4 addresses separated by space into the binary IPv4 address list.

  @param[in]   Str             The UNICODE string contains IPv4 addresses.
  @param[out]  PtrIpList       The storage to return the IPv4 address list.
  @param[out]  IpCount         The size of the IPv4 address list.

  @retval EFI_SUCCESS           The binary IP address list is returned in PtrIpList.
  @retval EFI_OUT_OF_RESOURCES  Error occurs in allocating memory.
  @retval EFI_INVALID_PARAMETER The IP string is malformatted.
  
**/
EFI_STATUS
Ip4Config2StrToIpList (
  IN  CHAR16            *Str,
  OUT EFI_IPv4_ADDRESS  **PtrIpList,
  OUT UINTN             *IpCount
  )
{
  UINTN              BeginIndex;
  UINTN              EndIndex; 
  UINTN              Index;
  UINTN              IpIndex;
  CHAR16             *StrTemp;
  BOOLEAN            SpaceTag;
  
  BeginIndex = 0;
  EndIndex   = BeginIndex;
  Index      = 0;
  IpIndex    = 0;
  StrTemp    = NULL;
  SpaceTag   = TRUE;
  
  *PtrIpList = NULL;
  *IpCount   = 0;

  if (Str == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Get the number of Ip.
  //
  while (*(Str + Index) != L'\0') {
    if (*(Str + Index) == L' ') {
      SpaceTag = TRUE;
    } else {
      if (SpaceTag) {
        (*IpCount)++;
        SpaceTag = FALSE;
      }
    }
     
    Index++;
  }

  if (*IpCount == 0) {
    return EFI_SUCCESS;
  }
  
  //
  // Allocate buffer for IpList.
  //
  *PtrIpList = AllocateZeroPool(*IpCount * sizeof(EFI_IPv4_ADDRESS));
  if (*PtrIpList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get IpList from Str.
  //
  Index = 0;
  while (*(Str + Index) != L'\0') {
    if (*(Str + Index) == L' ') {
      if(!SpaceTag) {
        StrTemp = AllocateZeroPool((EndIndex - BeginIndex + 1) * sizeof(CHAR16));
        if (StrTemp == NULL) {
          FreePool(*PtrIpList);
          *PtrIpList = NULL;
          *IpCount = 0;
          return EFI_OUT_OF_RESOURCES;
        }
        
        CopyMem (StrTemp, Str + BeginIndex, (EndIndex - BeginIndex) * sizeof(CHAR16));
        *(StrTemp + (EndIndex - BeginIndex)) = L'\0';
          
        if (Ip4Config2StrToIp (StrTemp, &((*PtrIpList)[IpIndex])) != EFI_SUCCESS) {
          FreePool(StrTemp);
          FreePool(*PtrIpList);
          *PtrIpList = NULL;
          *IpCount = 0;
          return EFI_INVALID_PARAMETER;
        }
          
        BeginIndex = EndIndex;
        IpIndex++;

        FreePool(StrTemp);
      }

      BeginIndex++;
      EndIndex++;
      SpaceTag = TRUE;
    } else {
      EndIndex++;
      SpaceTag = FALSE;
    }
    
    Index++;
    
    if (*(Str + Index) == L'\0') {
      if (!SpaceTag) {
        StrTemp = AllocateZeroPool((EndIndex - BeginIndex + 1) * sizeof(CHAR16));
        if (StrTemp == NULL) {
          FreePool(*PtrIpList);
          *PtrIpList = NULL;
          *IpCount = 0;
          return EFI_OUT_OF_RESOURCES;
        }
        
        CopyMem (StrTemp, Str + BeginIndex, (EndIndex - BeginIndex) * sizeof(CHAR16));
        *(StrTemp + (EndIndex - BeginIndex)) = L'\0';
        
        if (Ip4Config2StrToIp (StrTemp, &((*PtrIpList)[IpIndex])) != EFI_SUCCESS) {
          FreePool(StrTemp);
          FreePool(*PtrIpList);
          *PtrIpList = NULL;
          *IpCount = 0;
          return EFI_INVALID_PARAMETER;
        }

        FreePool(StrTemp);
      }
    }
  } 

  return EFI_SUCCESS;
}

/**
  Convert the IPv4 address into a dotted string.

  @param[in]   Ip   The IPv4 address.
  @param[out]  Str  The dotted IP string.
  
**/
VOID
Ip4Config2IpToStr (
  IN  EFI_IPv4_ADDRESS  *Ip,
  OUT CHAR16            *Str
  )
{
  UnicodeSPrint (
    Str,
    2 * IP4_STR_MAX_SIZE, 
    L"%d.%d.%d.%d", 
    Ip->Addr[0],
    Ip->Addr[1],
    Ip->Addr[2],
    Ip->Addr[3]
    );
}


/**
  Convert the IPv4 address list into string consists of several decimal 
  dotted IPv4 addresses separated by space.

  @param[in]   Ip        The IPv4 address list.
  @param[in]   IpCount   The size of IPv4 address list.
  @param[out]  Str       The string contains several decimal dotted
                         IPv4 addresses separated by space.       

  @retval EFI_SUCCESS           Operation is success.
  @retval EFI_OUT_OF_RESOURCES  Error occurs in allocating memory.

**/
EFI_STATUS
Ip4Config2IpListToStr (
  IN  EFI_IPv4_ADDRESS  *Ip,
  IN  UINTN             IpCount,
  OUT CHAR16            *Str
  )
{
  UINTN            Index;
  UINTN            TemIndex;
  UINTN            StrIndex;
  CHAR16           *TempStr;
  EFI_IPv4_ADDRESS *TempIp;
  
  Index    = 0;
  TemIndex = 0;
  StrIndex = 0;
  TempStr  = NULL;
  TempIp   = NULL;

  for (Index = 0; Index < IpCount; Index ++) {
    TempIp = Ip + Index;
    if (TempStr == NULL) {
      TempStr = AllocateZeroPool(2 * IP4_STR_MAX_SIZE);
      if (TempStr == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    UnicodeSPrint (
      TempStr, 
      2 * IP4_STR_MAX_SIZE, 
      L"%d.%d.%d.%d", 
      TempIp->Addr[0],
      TempIp->Addr[1],
      TempIp->Addr[2],
      TempIp->Addr[3]
      );

    for (TemIndex = 0; TemIndex < IP4_STR_MAX_SIZE; TemIndex ++) {
      if (*(TempStr + TemIndex) == L'\0') {
        if (Index == IpCount - 1) {
          Str[StrIndex++] = L'\0';
        } else {
          Str[StrIndex++] = L' ';
        }  
        break;
      } else {
        Str[StrIndex++] = *(TempStr + TemIndex);
      }
    }
  }

  if (TempStr != NULL) {
    FreePool(TempStr);
  }

  return EFI_SUCCESS;
}

/**
  The notify function of create event when performing a manual configuration.

  @param[in]    Event        The pointer of Event.
  @param[in]    Context      The pointer of Context.
  
**/
VOID
EFIAPI
Ip4Config2ManualAddressNotify (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  *((BOOLEAN *) Context) = TRUE;
}

/**
  Convert the network configuration data into the IFR data.

  @param[in]       Instance          The IP4 config2 instance.
  @param[in, out]  IfrNvData         The IFR nv data.

  @retval EFI_SUCCESS            The configure parameter to IFR data was
                                 set successfully.
  @retval EFI_INVALID_PARAMETER  Source instance or target IFR data is not available.
  @retval Others                 Other errors as indicated.
  
**/
EFI_STATUS
Ip4Config2ConvertConfigNvDataToIfrNvData (
  IN     IP4_CONFIG2_INSTANCE       *Instance,
  IN OUT IP4_CONFIG2_IFR_NVDATA     *IfrNvData
  )
{
  IP4_SERVICE                                *IpSb;
  EFI_IP4_CONFIG2_PROTOCOL                   *Ip4Config2;
  EFI_IP4_CONFIG2_INTERFACE_INFO             *Ip4Info;
  EFI_IP4_CONFIG2_POLICY                     Policy;
  UINTN                                      DataSize;
  UINTN                                      GatewaySize;
  EFI_IPv4_ADDRESS                           GatewayAddress;
  EFI_STATUS                                 Status;
  UINTN                                      DnsSize;
  UINTN                                      DnsCount;
  EFI_IPv4_ADDRESS                           *DnsAddress;

  Status      = EFI_SUCCESS;
  Ip4Config2  = &Instance->Ip4Config2;
  Ip4Info     = NULL;
  DnsAddress  = NULL;
  GatewaySize = sizeof (EFI_IPv4_ADDRESS);
  
  if ((IfrNvData == NULL) || (Instance == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  NET_CHECK_SIGNATURE (Instance, IP4_CONFIG2_INSTANCE_SIGNATURE);

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);

  if (IpSb->DefaultInterface->Configured) {
    IfrNvData->Configure = 1;
  } else {
    IfrNvData->Configure = 0;
    goto Exit;
  }

  //
  // Get the Policy info.
  // 
  DataSize = sizeof (EFI_IP4_CONFIG2_POLICY);
  Status   = Ip4Config2->GetData (
                           Ip4Config2,
                           Ip4Config2DataTypePolicy,
                           &DataSize,
                           &Policy
                           );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  
  if (Policy == Ip4Config2PolicyStatic) {
    IfrNvData->DhcpEnable = FALSE;
  } else if (Policy == Ip4Config2PolicyDhcp) {
    IfrNvData->DhcpEnable = TRUE;
    goto Exit;
  }
  
  //
  // Get the interface info.
  //
  DataSize    = 0;
  Status = Ip4Config2->GetData (
                         Ip4Config2,
                         Ip4Config2DataTypeInterfaceInfo,
                         &DataSize,
                         NULL
                         );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }
  
  Ip4Info = AllocateZeroPool (DataSize);
  if (Ip4Info == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  Status = Ip4Config2->GetData (
                         Ip4Config2,
                         Ip4Config2DataTypeInterfaceInfo,
                         &DataSize,
                         Ip4Info
                         );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  
  //
  // Get the Gateway info.
  //
  Status = Ip4Config2->GetData (
                         Ip4Config2,
                         Ip4Config2DataTypeGateway,
                         &GatewaySize,
                         &GatewayAddress
                         );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Get the Dns info.
  //
  DnsSize = 0;
  Status = Ip4Config2->GetData (
                         Ip4Config2,
                         Ip4Config2DataTypeDnsServer,
                         &DnsSize,
                         NULL
                         );
  if ((Status != EFI_BUFFER_TOO_SMALL) && (Status != EFI_NOT_FOUND)) {
    goto Exit;
  }
  
  DnsCount = (UINT32) (DnsSize / sizeof (EFI_IPv4_ADDRESS));

  if (DnsSize > 0) {
    DnsAddress = AllocateZeroPool(DnsSize);
    if (DnsAddress == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    
    Status = Ip4Config2->GetData (
                           Ip4Config2,
                           Ip4Config2DataTypeDnsServer,
                           &DnsSize,
                           DnsAddress
                           );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  Ip4Config2IpToStr (&Ip4Info->StationAddress, IfrNvData->StationAddress);
  Ip4Config2IpToStr (&Ip4Info->SubnetMask, IfrNvData->SubnetMask);
  Ip4Config2IpToStr (&GatewayAddress, IfrNvData->GatewayAddress);
  Status = Ip4Config2IpListToStr (DnsAddress, DnsCount, IfrNvData->DnsAddress);

Exit:

  if (DnsAddress != NULL) {
    FreePool(DnsAddress);
  }

  if (Ip4Info != NULL) {
    FreePool(Ip4Info);
  }
  
  return Status;
}

/**
  Convert the IFR data into the network configuration data and set the IP
  configure parameters for the NIC.

  @param[in]       IfrFormNvData     The IFR NV data.
  @param[in, out]  Instance          The IP4 config2 instance.

  @retval EFI_SUCCESS            The configure parameter for this NIC was
                                 set successfully.
  @retval EFI_INVALID_PARAMETER  The address information for setting is invalid.
  @retval Others                 Other errors as indicated.
  
**/
EFI_STATUS
Ip4Config2ConvertIfrNvDataToConfigNvData (
  IN     IP4_CONFIG2_IFR_NVDATA     *IfrFormNvData,
  IN OUT IP4_CONFIG2_INSTANCE       *Instance
  )
{
  EFI_STATUS                       Status;  
  EFI_IP4_CONFIG2_PROTOCOL         *Ip4Cfg2;
  IP4_CONFIG2_NVDATA               *Ip4NvData; 

  EFI_IP_ADDRESS                   StationAddress;
  EFI_IP_ADDRESS                   SubnetMask;
  EFI_IP_ADDRESS                   Gateway;
  IP4_ADDR                         Ip;
  EFI_IPv4_ADDRESS                 *DnsAddress;
  UINTN                            DnsCount;
  UINTN                            Index;

  EFI_EVENT                        TimeoutEvent;
  EFI_EVENT                        SetAddressEvent;
  BOOLEAN                          IsAddressOk;
  UINTN                            DataSize;
  EFI_INPUT_KEY                    Key;

  Status          = EFI_SUCCESS;
  Ip4Cfg2         = &Instance->Ip4Config2;
  Ip4NvData       = &Instance->Ip4NvData;
  
  DnsCount        = 0;
  DnsAddress      = NULL; 
  
  TimeoutEvent    = NULL;
  SetAddressEvent = NULL;


  
  if (Instance == NULL || IfrFormNvData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IfrFormNvData->Configure != TRUE) {
    return EFI_SUCCESS;
  }
  
  if (IfrFormNvData->DhcpEnable == TRUE) {
    Ip4NvData->Policy = Ip4Config2PolicyDhcp;
    
    Status = Ip4Cfg2->SetData (
                        Ip4Cfg2,
                        Ip4Config2DataTypePolicy,
                        sizeof (EFI_IP4_CONFIG2_POLICY),
                        &Ip4NvData->Policy
                        );
    if (EFI_ERROR(Status)) {
      return Status;
    }
  } else {
    //
    // Get Ip4NvData from IfrFormNvData if it is valid.
    //
    Ip4NvData->Policy = Ip4Config2PolicyStatic;

    Status = Ip4Config2StrToIp (IfrFormNvData->SubnetMask, &SubnetMask.v4);
    if (EFI_ERROR (Status) || ((SubnetMask.Addr[0] != 0) && (GetSubnetMaskPrefixLength (&SubnetMask.v4) == 0))) {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Subnet Mask!", NULL);
      return EFI_INVALID_PARAMETER;
    }

    Status = Ip4Config2StrToIp (IfrFormNvData->StationAddress, &StationAddress.v4);
    if (EFI_ERROR (Status) || 
        (SubnetMask.Addr[0] != 0 && !NetIp4IsUnicast (NTOHL (StationAddress.Addr[0]), NTOHL (SubnetMask.Addr[0]))) || 
        !Ip4StationAddressValid (NTOHL (StationAddress.Addr[0]), NTOHL (SubnetMask.Addr[0]))) {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid IP address!", NULL);
      return EFI_INVALID_PARAMETER;
    }
    
    Status = Ip4Config2StrToIp (IfrFormNvData->GatewayAddress, &Gateway.v4);
    if (EFI_ERROR (Status) || 
        (Gateway.Addr[0] != 0 && SubnetMask.Addr[0] != 0 && !NetIp4IsUnicast (NTOHL (Gateway.Addr[0]), NTOHL (SubnetMask.Addr[0])))) {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Gateway!", NULL);
      return EFI_INVALID_PARAMETER;
    }

    Status = Ip4Config2StrToIpList (IfrFormNvData->DnsAddress, &DnsAddress, &DnsCount);
    if (!EFI_ERROR (Status) && DnsCount > 0) {
      for (Index = 0; Index < DnsCount; Index ++) {
        CopyMem (&Ip, &DnsAddress[Index], sizeof (IP4_ADDR));
        if (IP4_IS_UNSPECIFIED (NTOHL (Ip)) || IP4_IS_LOCAL_BROADCAST (NTOHL (Ip))) {
          CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Dns Server!", NULL);
          FreePool(DnsAddress);
          return EFI_INVALID_PARAMETER;
        } 
      } 
    } else {
      if (EFI_ERROR (Status)) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Dns Server!", NULL);
      }
    }
    
    if (Ip4NvData->ManualAddress != NULL) {
      FreePool(Ip4NvData->ManualAddress); 
    }
    Ip4NvData->ManualAddressCount = 1;
    Ip4NvData->ManualAddress = AllocateZeroPool(sizeof(EFI_IP4_CONFIG2_MANUAL_ADDRESS));
    if (Ip4NvData->ManualAddress == NULL) {
      if (DnsAddress != NULL) {
        FreePool(DnsAddress);
      }
      
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem(&Ip4NvData->ManualAddress->Address, &StationAddress.v4, sizeof(EFI_IPv4_ADDRESS));
    CopyMem(&Ip4NvData->ManualAddress->SubnetMask, &SubnetMask.v4, sizeof(EFI_IPv4_ADDRESS));
    
    if (Ip4NvData->GatewayAddress != NULL) {
      FreePool(Ip4NvData->GatewayAddress); 
    }
    Ip4NvData->GatewayAddressCount = 1;
    Ip4NvData->GatewayAddress = AllocateZeroPool(sizeof(EFI_IPv4_ADDRESS));
    if (Ip4NvData->GatewayAddress == NULL) {
      if (DnsAddress != NULL) {
        FreePool(DnsAddress);
      }
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem(Ip4NvData->GatewayAddress, &Gateway.v4, sizeof(EFI_IPv4_ADDRESS));
    
    if (Ip4NvData->DnsAddress != NULL) {
      FreePool(Ip4NvData->DnsAddress); 
    }
    Ip4NvData->DnsAddressCount = (UINT32) DnsCount;
    Ip4NvData->DnsAddress      = DnsAddress;

    //
    // Setting Ip4NvData.
    //
    Status = Ip4Cfg2->SetData (
                        Ip4Cfg2,
                        Ip4Config2DataTypePolicy,
                        sizeof (EFI_IP4_CONFIG2_POLICY),
                        &Ip4NvData->Policy
                        );
    if (EFI_ERROR(Status)) {
      return Status;
    }

    //
    // Create events & timers for asynchronous settings.
    //
    Status = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &TimeoutEvent
                    );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    Ip4Config2ManualAddressNotify,
                    &IsAddressOk,
                    &SetAddressEvent
                    );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    IsAddressOk = FALSE;
    
    Status = Ip4Cfg2->RegisterDataNotify (
                        Ip4Cfg2,
                        Ip4Config2DataTypeManualAddress,
                        SetAddressEvent
                        );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    //
    // Set ManualAddress.
    //
    DataSize = Ip4NvData->ManualAddressCount * sizeof (EFI_IP4_CONFIG2_MANUAL_ADDRESS);
    Status = Ip4Cfg2->SetData (
                        Ip4Cfg2,
                        Ip4Config2DataTypeManualAddress,
                        DataSize,
                        (VOID *) Ip4NvData->ManualAddress
                        );

    if (Status == EFI_NOT_READY) {
      gBS->SetTimer (TimeoutEvent, TimerRelative, 50000000);
      while (EFI_ERROR (gBS->CheckEvent (TimeoutEvent))) {
        if (IsAddressOk) {
          Status = EFI_SUCCESS;
          break;
        }
      }
    }

    Ip4Cfg2->UnregisterDataNotify (
               Ip4Cfg2,
               Ip4Config2DataTypeManualAddress,
               SetAddressEvent
               );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    //
    // Set gateway.
    //
    DataSize = Ip4NvData->GatewayAddressCount * sizeof (EFI_IPv4_ADDRESS);
    Status = Ip4Cfg2->SetData (
                        Ip4Cfg2,
                        Ip4Config2DataTypeGateway,
                        DataSize,
                        Ip4NvData->GatewayAddress
                        );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    //
    // Set DNS addresses.
    //
    if (Ip4NvData->DnsAddressCount > 0 && Ip4NvData->DnsAddress != NULL) {
      DataSize = Ip4NvData->DnsAddressCount * sizeof (EFI_IPv4_ADDRESS);
      Status = Ip4Cfg2->SetData (
                          Ip4Cfg2,
                          Ip4Config2DataTypeDnsServer,
                          DataSize,
                          Ip4NvData->DnsAddress
                          );
      
      if (EFI_ERROR (Status)) {
        goto Exit;
      } 
    } 
  } 

Exit:
  if (SetAddressEvent != NULL) {
    gBS->CloseEvent (SetAddressEvent);
  }

  if (TimeoutEvent != NULL) {
    gBS->CloseEvent (TimeoutEvent);
  }

  return Status;
}

/**
  This function allows the caller to request the current
  configuration for one or more named elements. The resulting
  string is in <ConfigAltResp> format. Any and all alternative
  configuration strings shall also be appended to the end of the
  current configuration string. If they are, they must appear
  after the current configuration. They must contain the same
  routing (GUID, NAME, PATH) as the current configuration string.
  They must have an additional description indicating the type of
  alternative configuration the string represents,
  "ALTCFG=<StringToken>". That <StringToken> (when
  converted from Hex UNICODE to binary) is a reference to a
  string in the associated string pack.

  @param[in] This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Request    A null-terminated Unicode string in
                        <ConfigRequest> format. Note that this
                        includes the routing information as well as
                        the configurable name / value pairs. It is
                        invalid for this string to be in
                        <MultiConfigRequest> format.
  @param[out] Progress  On return, points to a character in the
                        Request string. Points to the string's null
                        terminator if request was successful. Points
                        to the most recent "&" before the first
                        failing name / value pair (or the beginning
                        of the string if the failure is in the first
                        name / value pair) if the request was not
                        successful.
  @param[out] Results   A null-terminated Unicode string in
                        <ConfigAltResp> format which has all values
                        filled in for the names in the Request string.
                        String to be allocated by the called function.

  @retval EFI_SUCCESS             The Results string is filled with the
                                  values corresponding to all requested
                                  names.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_NOT_FOUND           Routing data doesn't match any
                                  known driver. Progress set to the
                                  first character in the routing header.
                                  Note: There is no requirement that the
                                  driver validate the routing data. It
                                  must skip the <ConfigHdr> in order to
                                  process the names.
  @retval EFI_INVALID_PARAMETER   Illegal syntax. Progress set
                                  to most recent & before the
                                  error or the beginning of the
                                  string.
  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question.Currently not implemented.
**/
EFI_STATUS
EFIAPI
Ip4FormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;  
  IP4_CONFIG2_INSTANCE             *Ip4Config2Instance;
  IP4_FORM_CALLBACK_INFO           *Private;  
  IP4_CONFIG2_IFR_NVDATA           *IfrFormNvData;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;  
  BOOLEAN                          AllocatedRequest;
  EFI_STRING                       FormResult;
  UINTN                            Size;
  UINTN                            BufferSize;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  Status             = EFI_SUCCESS; 
  IfrFormNvData      = NULL;
  ConfigRequest      = NULL;
  FormResult         = NULL; 
  Size               = 0;
  AllocatedRequest   = FALSE;    
  ConfigRequest      = Request; 
  Private            = IP4_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS(This);
  Ip4Config2Instance = IP4_CONFIG2_INSTANCE_FROM_FORM_CALLBACK(Private);
  BufferSize         = sizeof (IP4_CONFIG2_IFR_NVDATA);
  *Progress          = Request;
  
  //
  // Check Request data in <ConfigHdr>.
  //
  if ((Request == NULL) || HiiIsConfigHdrMatch (Request, &gIp4Config2NvDataGuid, mIp4Config2StorageName)) {
    IfrFormNvData = AllocateZeroPool (sizeof (IP4_CONFIG2_IFR_NVDATA));
    if (IfrFormNvData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    
    Ip4Config2ConvertConfigNvDataToIfrNvData (Ip4Config2Instance, IfrFormNvData);
    
    if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
      //
      // Request has no request element, construct full request string.
      // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
      // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
      //
      ConfigRequestHdr = HiiConstructConfigHdr (&gIp4Config2NvDataGuid, mIp4Config2StorageName, Private->ChildHandle);
      Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
      ConfigRequest = AllocateZeroPool (Size);
      if (ConfigRequest == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Failure;
      }
      AllocatedRequest = TRUE;
      
      UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
      FreePool (ConfigRequestHdr);
    }

    //
    // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
    //
    Status = gHiiConfigRouting->BlockToConfig (
                                  gHiiConfigRouting,
                                  ConfigRequest,
                                  (UINT8 *) IfrFormNvData,
                                  BufferSize,
                                  &FormResult,
                                  Progress
                                  );

    FreePool (IfrFormNvData);
    
    //
    // Free the allocated config request string.
    //
    if (AllocatedRequest) {
      FreePool (ConfigRequest);
      ConfigRequest = NULL;
    }

    if (EFI_ERROR (Status)) {
      goto Failure;
    }
  }
  
  if (Request == NULL || HiiIsConfigHdrMatch (Request, &gIp4Config2NvDataGuid, mIp4Config2StorageName)) {
    *Results = FormResult;
  } else {
    return EFI_NOT_FOUND;
  }

Failure:
  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration  A null-terminated Unicode string in
                             <ConfigString> format.
  @param[out] Progress       A pointer to a string filled in with the
                             offset of the most recent '&' before the
                             first failing name / value pair (or the
                             beginn ing of the string if the failure
                             is in the first name / value pair) or
                             the terminating NULL if all was
                             successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.
  @retval EFI_OUT_OF_MEMORY       Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.
**/
EFI_STATUS
EFIAPI
Ip4FormRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  IP4_CONFIG2_IFR_NVDATA           *IfrFormNvData;
  IP4_CONFIG2_INSTANCE             *Ip4Config2Instance;
  IP4_FORM_CALLBACK_INFO           *Private;

  Status        = EFI_SUCCESS;
  IfrFormNvData = NULL;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  Private            = IP4_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS(This);
  Ip4Config2Instance = IP4_CONFIG2_INSTANCE_FROM_FORM_CALLBACK(Private);

  //
  // Check Routing data in <ConfigHdr>.
  //
  if (HiiIsConfigHdrMatch (Configuration, &gIp4Config2NvDataGuid, mIp4Config2StorageName)) {
    //
    // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
    //
    IfrFormNvData = AllocateZeroPool (sizeof (IP4_CONFIG2_IFR_NVDATA));
    if (IfrFormNvData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    BufferSize = 0;

    Status = gHiiConfigRouting->ConfigToBlock (
                                  gHiiConfigRouting,
                                  Configuration,
                                  (UINT8 *) IfrFormNvData,
                                  &BufferSize,
                                  Progress
                                  );
    if (Status != EFI_BUFFER_TOO_SMALL) {
      return Status;
    }

    Status = gHiiConfigRouting->ConfigToBlock (
                                  gHiiConfigRouting,
                                  Configuration,
                                  (UINT8 *) IfrFormNvData,
                                  &BufferSize,
                                  Progress
                                  );
    if (!EFI_ERROR (Status)) {
      Status = Ip4Config2ConvertIfrNvDataToConfigNvData (IfrFormNvData, Ip4Config2Instance);
    }

    FreePool (IfrFormNvData);
  } else {
    return EFI_NOT_FOUND;
  }

  return Status;

}

/**
  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect. The format of the data tends to
                                 vary based on the opcode that enerated the callback.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out] ActionRequest      On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.Currently not implemented.
  @retval EFI_INVALID_PARAMETERS Passing in wrong parameter.
  @retval Others                 Other errors as indicated.
  
**/
EFI_STATUS
EFIAPI
Ip4FormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS                Status;
  IP4_CONFIG2_INSTANCE      *Instance;
  IP4_CONFIG2_IFR_NVDATA    *IfrFormNvData;
  IP4_FORM_CALLBACK_INFO    *Private;
  
  EFI_IP_ADDRESS            StationAddress;
  EFI_IP_ADDRESS            SubnetMask;
  EFI_IP_ADDRESS            Gateway;
  IP4_ADDR                  Ip;
  EFI_IPv4_ADDRESS          *DnsAddress;
  UINTN                     DnsCount;
  UINTN                     Index;
  EFI_INPUT_KEY             Key;
  
  IfrFormNvData = NULL;
  DnsCount      = 0;
  DnsAddress    = NULL; 

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    Private = IP4_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS(This);
    Instance = IP4_CONFIG2_INSTANCE_FROM_FORM_CALLBACK(Private);
    
    IfrFormNvData = AllocateZeroPool (sizeof (IP4_CONFIG2_IFR_NVDATA));
    if (IfrFormNvData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Retrieve uncommitted data from Browser
    //
    if (!HiiGetBrowserData (&gIp4Config2NvDataGuid, mIp4Config2StorageName, sizeof (IP4_CONFIG2_IFR_NVDATA), (UINT8 *) IfrFormNvData)) {
      FreePool (IfrFormNvData);
      return EFI_NOT_FOUND;
    }

    Status = EFI_SUCCESS;

    switch (QuestionId) {
    case KEY_LOCAL_IP:
      Status = Ip4Config2StrToIp (IfrFormNvData->StationAddress, &StationAddress.v4);
      if (EFI_ERROR (Status) || IP4_IS_UNSPECIFIED (NTOHL (StationAddress.Addr[0])) || IP4_IS_LOCAL_BROADCAST (NTOHL (StationAddress.Addr[0]))) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid IP address!", NULL);
        Status = EFI_INVALID_PARAMETER;
      }
      break;

    case KEY_SUBNET_MASK:
      Status = Ip4Config2StrToIp (IfrFormNvData->SubnetMask, &SubnetMask.v4);
      if (EFI_ERROR (Status) || ((SubnetMask.Addr[0] != 0) && (GetSubnetMaskPrefixLength (&SubnetMask.v4) == 0))) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Subnet Mask!", NULL);
        Status = EFI_INVALID_PARAMETER;
      }
      break;

    case KEY_GATE_WAY:
      Status = Ip4Config2StrToIp (IfrFormNvData->GatewayAddress, &Gateway.v4);
      if (EFI_ERROR (Status) || IP4_IS_LOCAL_BROADCAST(NTOHL(Gateway.Addr[0]))) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Gateway!", NULL);
        Status = EFI_INVALID_PARAMETER;
      }
      break;
    
    case KEY_DNS:
      Status = Ip4Config2StrToIpList (IfrFormNvData->DnsAddress, &DnsAddress, &DnsCount);
      if (!EFI_ERROR (Status) && DnsCount > 0) {
        for (Index = 0; Index < DnsCount; Index ++) {
          CopyMem (&Ip, &DnsAddress[Index], sizeof (IP4_ADDR));
          if (IP4_IS_UNSPECIFIED (NTOHL (Ip)) || IP4_IS_LOCAL_BROADCAST (NTOHL (Ip))) {
            CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Dns Server!", NULL);
            Status = EFI_INVALID_PARAMETER;
            break;
          } 
        }
      } else {
        if (EFI_ERROR (Status)) {
          CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Dns Server!", NULL);
        }
      }
      
      if(DnsAddress != NULL) {  
        FreePool(DnsAddress);
      }
      break;
      
    case KEY_SAVE_CHANGES:
      Status = Ip4Config2ConvertIfrNvDataToConfigNvData (IfrFormNvData, Instance);
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
      break;

    default:
      break;
    }

    FreePool (IfrFormNvData);

    return Status;
  }

  //
  // All other action return unsupported.
  //
  return EFI_UNSUPPORTED;
}

/**
  Install HII Config Access protocol for network device and allocate resource.

  @param[in, out]  Instance        The IP4 config2 Instance.

  @retval EFI_SUCCESS              The HII Config Access protocol is installed.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.
  
**/
EFI_STATUS
Ip4Config2FormInit (
  IN OUT IP4_CONFIG2_INSTANCE     *Instance
  )
{
  EFI_STATUS                     Status;
  IP4_SERVICE                    *IpSb;
  IP4_FORM_CALLBACK_INFO         *CallbackInfo;
  EFI_HII_CONFIG_ACCESS_PROTOCOL *ConfigAccess;
  VENDOR_DEVICE_PATH             VendorDeviceNode;
  EFI_SERVICE_BINDING_PROTOCOL   *MnpSb;
  CHAR16                         *MacString;
  CHAR16                         MenuString[128];
  CHAR16                         PortString[128];
  CHAR16                         *OldMenuString;
  EFI_DEVICE_PATH_PROTOCOL       *ParentDevicePath;

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);
  ASSERT (IpSb != NULL);
  
  CallbackInfo = &Instance->CallbackInfo;
  
  CallbackInfo->Signature = IP4_FORM_CALLBACK_INFO_SIGNATURE;

  Status = gBS->HandleProtocol (
                  IpSb->Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Construct device path node for EFI HII Config Access protocol,
  // which consists of controller physical device path and one hardware
  // vendor guid node.
  //
  ZeroMem (&VendorDeviceNode, sizeof (VENDOR_DEVICE_PATH));
  VendorDeviceNode.Header.Type    = HARDWARE_DEVICE_PATH;
  VendorDeviceNode.Header.SubType = HW_VENDOR_DP;

  CopyGuid (&VendorDeviceNode.Guid, &gEfiCallerIdGuid);

  SetDevicePathNodeLength (&VendorDeviceNode.Header, sizeof (VENDOR_DEVICE_PATH));
  CallbackInfo->HiiVendorDevicePath = AppendDevicePathNode (
                                        ParentDevicePath,
                                        (EFI_DEVICE_PATH_PROTOCOL *) &VendorDeviceNode
                                        );
  if (CallbackInfo->HiiVendorDevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  ConfigAccess                = &CallbackInfo->HiiConfigAccessProtocol;
  ConfigAccess->ExtractConfig = Ip4FormExtractConfig;
  ConfigAccess->RouteConfig   = Ip4FormRouteConfig;
  ConfigAccess->Callback      = Ip4FormCallback;

  //
  // Install Device Path Protocol and Config Access protocol on new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &CallbackInfo->ChildHandle,
                  &gEfiDevicePathProtocolGuid,
                  CallbackInfo->HiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  ConfigAccess,
                  NULL
                  );

  if (!EFI_ERROR (Status)) {
    //
    // Open the Parent Handle for the child
    //
    Status = gBS->OpenProtocol (
                    IpSb->Controller,
                    &gEfiManagedNetworkServiceBindingProtocolGuid,
                    (VOID **) &MnpSb,
                    IpSb->Image,
                    CallbackInfo->ChildHandle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
  }

  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Publish our HII data
  //
  CallbackInfo->RegisteredHandle = HiiAddPackages (
                                     &gIp4Config2NvDataGuid,
                                     CallbackInfo->ChildHandle,
                                     Ip4DxeStrings,
                                     Ip4Config2Bin,
                                     NULL
                                     );
  if (CallbackInfo->RegisteredHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  //
  // Append MAC string in the menu help string and tile help string
  //
  Status = NetLibGetMacString (IpSb->Controller, IpSb->Image, &MacString);
  if (!EFI_ERROR (Status)) {
    OldMenuString = HiiGetString (
                      CallbackInfo->RegisteredHandle, 
                      STRING_TOKEN (STR_IP4_CONFIG2_FORM_HELP), 
                      NULL
                      );
    UnicodeSPrint (MenuString, 128, L"%s (MAC:%s)", OldMenuString, MacString);
    HiiSetString (
      CallbackInfo->RegisteredHandle, 
      STRING_TOKEN (STR_IP4_CONFIG2_FORM_HELP), 
      MenuString, 
      NULL
      );

    UnicodeSPrint (PortString, 128, L"MAC:%s", MacString);
    HiiSetString (
      CallbackInfo->RegisteredHandle,  
      STRING_TOKEN (STR_IP4_DEVICE_FORM_HELP), 
      PortString, 
      NULL
      );
    
    FreePool (MacString);
    FreePool (OldMenuString);

    return EFI_SUCCESS;
  }

Error:
  Ip4Config2FormUnload (Instance);
  return Status;
}

/**
  Uninstall the HII Config Access protocol for network devices and free up the resources.

  @param[in, out]  Instance      The IP4 config2 instance to unload a form.

**/
VOID
Ip4Config2FormUnload (
  IN OUT IP4_CONFIG2_INSTANCE     *Instance
  )
{
  IP4_SERVICE                    *IpSb;
  IP4_FORM_CALLBACK_INFO         *CallbackInfo;
  IP4_CONFIG2_NVDATA             *Ip4NvData;

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);
  ASSERT (IpSb != NULL);

  CallbackInfo = &Instance->CallbackInfo;

  if (CallbackInfo->ChildHandle != NULL) {
    //
    // Close the child handle
    //
    gBS->CloseProtocol (
           IpSb->Controller,
           &gEfiManagedNetworkServiceBindingProtocolGuid,
           IpSb->Image,
           CallbackInfo->ChildHandle
           );
    
    //
    // Uninstall EFI_HII_CONFIG_ACCESS_PROTOCOL
    //
    gBS->UninstallMultipleProtocolInterfaces (
           CallbackInfo->ChildHandle,
           &gEfiDevicePathProtocolGuid,
           CallbackInfo->HiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &CallbackInfo->HiiConfigAccessProtocol,
           NULL
           );
  }

  if (CallbackInfo->HiiVendorDevicePath != NULL) {
    FreePool (CallbackInfo->HiiVendorDevicePath);
  }

  if (CallbackInfo->RegisteredHandle != NULL) {
    //
    // Remove HII package list
    //
    HiiRemovePackages (CallbackInfo->RegisteredHandle);
  }

  Ip4NvData = &Instance->Ip4NvData;

  if(Ip4NvData->ManualAddress != NULL) {
    FreePool(Ip4NvData->ManualAddress);
  }

  if(Ip4NvData->GatewayAddress != NULL) {
    FreePool(Ip4NvData->GatewayAddress);
  }

  if(Ip4NvData->DnsAddress != NULL) {
    FreePool(Ip4NvData->DnsAddress);
  }

  Ip4NvData->ManualAddressCount  = 0;
  Ip4NvData->GatewayAddressCount = 0;
  Ip4NvData->DnsAddressCount     = 0;
}
