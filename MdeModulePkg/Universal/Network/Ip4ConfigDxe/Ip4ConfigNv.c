/** @file
  Helper functions for configuring or getting the parameters relating to Ip4.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip4ConfigNv.h"
#include "NicIp4Variable.h"

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
Ip4StrToIp (
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
  Convert the IPv4 address into a dotted string.

  @param[in]   Ip   The IPv4 address.
  @param[out]  Str  The dotted IP string.
**/
VOID
Ip4ConfigIpToStr (
  IN  EFI_IPv4_ADDRESS  *Ip,
  OUT CHAR16            *Str
  )
{
  UnicodeSPrint (Str, 2 * IP4_STR_MAX_SIZE, L"%d.%d.%d.%d", Ip->Addr[0], Ip->Addr[1], Ip->Addr[2], Ip->Addr[3]);
}


/**
  Convert the network configuration data into the IFR data.

  @param[in]   Ip4ConfigInstance The IP4Config instance
  @param[out]  IfrFormNvData     The IFR nv data.
**/
VOID
Ip4ConfigConvertDeviceConfigDataToIfrNvData (
  IN  IP4_CONFIG_INSTANCE       *Ip4ConfigInstance,
  OUT IP4_CONFIG_IFR_NVDATA     *IfrFormNvData
  )
{
  NIC_IP4_CONFIG_INFO  *NicConfig;

  NicConfig = EfiNicIp4ConfigGetInfo (Ip4ConfigInstance);
  if (NicConfig != NULL) {
    IfrFormNvData->Configure = 1;
    Ip4ConfigInstance->Ip4ConfigCallbackInfo.Configured = TRUE;
    if (NicConfig->Source == IP4_CONFIG_SOURCE_DHCP) {
      IfrFormNvData->DhcpEnable = 1;
      Ip4ConfigInstance->Ip4ConfigCallbackInfo.DhcpEnabled = TRUE;
    } else {
      IfrFormNvData->DhcpEnable = 0;
      Ip4ConfigIpToStr (&NicConfig->Ip4Info.StationAddress, IfrFormNvData->StationAddress);
      Ip4ConfigIpToStr (&NicConfig->Ip4Info.SubnetMask, IfrFormNvData->SubnetMask);
      Ip4ConfigIpToStr (&NicConfig->Ip4Info.RouteTable[1].GatewayAddress, IfrFormNvData->GatewayAddress);

      Ip4ConfigInstance->Ip4ConfigCallbackInfo.DhcpEnabled = FALSE;
      CopyMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo.LocalIp, &NicConfig->Ip4Info.StationAddress, sizeof (EFI_IPv4_ADDRESS));
      CopyMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo.SubnetMask, &NicConfig->Ip4Info.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
      CopyMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo.Gateway, &NicConfig->Ip4Info.RouteTable[1].GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
    }

    FreePool (NicConfig);
  } else {
    IfrFormNvData->Configure = 0;
    Ip4ConfigInstance->Ip4ConfigCallbackInfo.Configured = FALSE;
  }
}

/**
  Convert the IFR data into the network configuration data and set the IP
  configure parameters for the NIC.

  @param[in]       IfrFormNvData     The IFR NV data.
  @param[in, out]  Ip4ConfigInstance The IP4Config instance.

  @retval EFI_SUCCESS            The configure parameter for this NIC was
                                 set successfully.
  @retval EFI_ALREADY_STARTED    There is a pending auto configuration.
  @retval EFI_NOT_FOUND          No auto configure parameter is found.

**/
EFI_STATUS
Ip4ConfigConvertIfrNvDataToDeviceConfigData (
  IN     IP4_CONFIG_IFR_NVDATA     *IfrFormNvData,
  IN OUT IP4_CONFIG_INSTANCE       *Ip4ConfigInstance
  )
{
  EFI_STATUS                Status;
  EFI_IP_ADDRESS            HostIp;
  EFI_IP_ADDRESS            SubnetMask;
  EFI_IP_ADDRESS            Gateway;
  EFI_INPUT_KEY             Key;
  NIC_IP4_CONFIG_INFO       *NicInfo;
  EFI_IP_ADDRESS            Ip;

  ZeroMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo, sizeof (IP4_SETTING_INFO));

  Ip4ConfigInstance->Ip4ConfigCallbackInfo.Configured = IfrFormNvData->Configure;
  Ip4ConfigInstance->Ip4ConfigCallbackInfo.DhcpEnabled = IfrFormNvData->DhcpEnable;
  Ip4StrToIp (IfrFormNvData->StationAddress, &Ip4ConfigInstance->Ip4ConfigCallbackInfo.LocalIp);
  Ip4StrToIp (IfrFormNvData->SubnetMask, &Ip4ConfigInstance->Ip4ConfigCallbackInfo.SubnetMask);
  Ip4StrToIp (IfrFormNvData->GatewayAddress, &Ip4ConfigInstance->Ip4ConfigCallbackInfo.Gateway);

  if (!Ip4ConfigInstance->Ip4ConfigCallbackInfo.Configured) {
    //
    // Clear the variable
    //
    ZeroMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo, sizeof (IP4_SETTING_INFO));

    Status = EfiNicIp4ConfigSetInfo (Ip4ConfigInstance, NULL, TRUE);
    if (Status == EFI_NOT_FOUND) {
      return EFI_SUCCESS;
    }

    return Status;
  }

  NicInfo = AllocateZeroPool (sizeof (NIC_IP4_CONFIG_INFO) + 2 * sizeof (EFI_IP4_ROUTE_TABLE));
  ASSERT (NicInfo != NULL);

  NicInfo->Ip4Info.RouteTable = (EFI_IP4_ROUTE_TABLE *) (NicInfo + 1);

  if (!Ip4ConfigInstance->Ip4ConfigCallbackInfo.DhcpEnabled) {
    CopyMem (&HostIp.v4, &Ip4ConfigInstance->Ip4ConfigCallbackInfo.LocalIp, sizeof (HostIp.v4));
    CopyMem (&SubnetMask.v4, &Ip4ConfigInstance->Ip4ConfigCallbackInfo.SubnetMask, sizeof (SubnetMask.v4));
    CopyMem (&Gateway.v4, &Ip4ConfigInstance->Ip4ConfigCallbackInfo.Gateway, sizeof (Gateway.v4));

    if (!NetIp4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid IP address!", NULL);
      return EFI_INVALID_PARAMETER;
    }
    if (EFI_IP4_EQUAL (&SubnetMask, &mZeroIp4Addr)) {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Subnet Mask!", NULL);
      return EFI_INVALID_PARAMETER;
    }

    if ((Gateway.Addr[0] != 0)) {
      if (SubnetMask.Addr[0] == 0) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Gateway address is set but subnet mask is zero.", NULL);
        return EFI_INVALID_PARAMETER;

      } else if (!IP4_NET_EQUAL (HostIp.Addr[0], Gateway.Addr[0], SubnetMask.Addr[0])) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Local IP and Gateway are not in the same subnet.", NULL);
        return EFI_INVALID_PARAMETER;      }
    }

    NicInfo->Source = IP4_CONFIG_SOURCE_STATIC;
    NicInfo->Ip4Info.RouteTableSize = 2;

    CopyMem (&NicInfo->Ip4Info.StationAddress, &HostIp.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&NicInfo->Ip4Info.SubnetMask, &SubnetMask.v4, sizeof (EFI_IPv4_ADDRESS));

    Ip.Addr[0] = HostIp.Addr[0] & SubnetMask.Addr[0];

    CopyMem (&NicInfo->Ip4Info.RouteTable[0].SubnetAddress, &Ip.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&NicInfo->Ip4Info.RouteTable[0].SubnetMask, &SubnetMask.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&NicInfo->Ip4Info.RouteTable[1].GatewayAddress, &Gateway.v4, sizeof (EFI_IPv4_ADDRESS));

  } else {
    NicInfo->Source = IP4_CONFIG_SOURCE_DHCP;
    ZeroMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo.LocalIp, sizeof (EFI_IPv4_ADDRESS));
    ZeroMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
    ZeroMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo.Gateway, sizeof (EFI_IPv4_ADDRESS));
  }

  NicInfo->Permanent = TRUE;
  CopyMem (&NicInfo->NicAddr, &Ip4ConfigInstance->NicAddr, sizeof (NIC_ADDR));

  return EfiNicIp4ConfigSetInfo (Ip4ConfigInstance, NicInfo, TRUE);
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
Ip4DeviceExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  NIC_IP4_CONFIG_INFO              *IfrDeviceNvData;
  NIC_IP4_CONFIG_INFO              *NicConfig;
  IP4_CONFIG_INSTANCE              *Ip4ConfigInstance;
  IP4_CONFIG_IFR_NVDATA            *IfrFormNvData;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  EFI_STRING                       DeviceResult;
  EFI_STRING                       FormResult;
  CHAR16                           *StrPointer;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;
  UINTN                            BufferSize;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress     = Request;
  Size          = 0;
  DeviceResult  = NULL;
  FormResult    = NULL;
  ConfigRequest = NULL;
  Status        = EFI_SUCCESS;
  AllocatedRequest  = FALSE;
  Ip4ConfigInstance = IP4_CONFIG_INSTANCE_FROM_CONFIG_ACCESS (This);

  //
  // Check Request data in <ConfigHdr>.
  //
  if ((Request == NULL) || HiiIsConfigHdrMatch (Request, &gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE)) {
    IfrDeviceNvData = AllocateZeroPool (NIC_ITEM_CONFIG_SIZE);
    if (IfrDeviceNvData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    NicConfig = EfiNicIp4ConfigGetInfo (Ip4ConfigInstance);
    if (NicConfig == NULL) {
      return EFI_NOT_FOUND;
    }
    CopyMem (IfrDeviceNvData, NicConfig, SIZEOF_NIC_IP4_CONFIG_INFO (NicConfig));
    FreePool (NicConfig);

    ConfigRequest = Request;
    if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
      //
      // Request has no request element, construct full request string.
      // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
      // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
      //
      ConfigRequestHdr = HiiConstructConfigHdr (&gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE, Ip4ConfigInstance->ChildHandle);
      Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
      ConfigRequest = AllocateZeroPool (Size);
      ASSERT (ConfigRequest != NULL);
      AllocatedRequest = TRUE;
      BufferSize = NIC_ITEM_CONFIG_SIZE;
      UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
      FreePool (ConfigRequestHdr);
    }

    //
    // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
    //
    Status = gHiiConfigRouting->BlockToConfig (
                                  gHiiConfigRouting,
                                  ConfigRequest,
                                  (UINT8 *) IfrDeviceNvData,
                                  NIC_ITEM_CONFIG_SIZE,
                                  &DeviceResult,
                                  Progress
                                  );

    FreePool (IfrDeviceNvData);
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

  if ((Request == NULL) || HiiIsConfigHdrMatch (Request, &gNicIp4ConfigNvDataGuid, EFI_NIC_IP4_CONFIG_VARIABLE)) {

    IfrFormNvData = AllocateZeroPool (sizeof (IP4_CONFIG_IFR_NVDATA));
    if (IfrFormNvData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Ip4ConfigConvertDeviceConfigDataToIfrNvData (Ip4ConfigInstance, IfrFormNvData);

    ConfigRequest = Request;
    if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
      //
      // Request has no request element, construct full request string.
      // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
      // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
      //
      ConfigRequestHdr = HiiConstructConfigHdr (&gNicIp4ConfigNvDataGuid, EFI_NIC_IP4_CONFIG_VARIABLE, Ip4ConfigInstance->ChildHandle);
      Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
      ConfigRequest = AllocateZeroPool (Size);
      ASSERT (ConfigRequest != NULL);
      AllocatedRequest = TRUE;
      BufferSize = sizeof (IP4_CONFIG_IFR_NVDATA);
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
                                  sizeof (IP4_CONFIG_IFR_NVDATA),
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

  if (Request == NULL) {
    Size = StrLen (DeviceResult);
    Size = Size + 1;
    Size = Size + StrLen (FormResult) + 1;
    *Results = AllocateZeroPool (Size * sizeof (CHAR16));
    ASSERT (*Results != NULL);
    StrPointer  = *Results;
    StrCpy (StrPointer, DeviceResult);
    StrPointer  = StrPointer + StrLen (StrPointer);
    *StrPointer = L'&';
    StrCpy (StrPointer + 1, FormResult);
    FreePool (DeviceResult);
    FreePool (FormResult);
  } else if (HiiIsConfigHdrMatch (Request, &gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE)) {
    *Results = DeviceResult;
  } else if (HiiIsConfigHdrMatch (Request, &gNicIp4ConfigNvDataGuid, EFI_NIC_IP4_CONFIG_VARIABLE)) {
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
Ip4DeviceRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  NIC_IP4_CONFIG_INFO              *IfrDeviceNvData;
  IP4_CONFIG_IFR_NVDATA            *IfrFormNvData;
  NIC_IP4_CONFIG_INFO              *NicInfo;
  IP4_CONFIG_INSTANCE              *Ip4ConfigInstance;
  EFI_MAC_ADDRESS                  ZeroMac;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Reclaim Ip4Config variable
  //
  Ip4ConfigReclaimVariable ();

  *Progress = Configuration;

  Ip4ConfigInstance = IP4_CONFIG_INSTANCE_FROM_CONFIG_ACCESS (This);

  //
  // Check Routing data in <ConfigHdr>.
  //
  if (HiiIsConfigHdrMatch (Configuration, &gNicIp4ConfigNvDataGuid, EFI_NIC_IP4_CONFIG_VARIABLE)) {
    //
    // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
    //
    IfrFormNvData = AllocateZeroPool (sizeof (IP4_CONFIG_IFR_NVDATA));
    if (IfrFormNvData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    BufferSize = NIC_ITEM_CONFIG_SIZE;
    Status = gHiiConfigRouting->ConfigToBlock (
                                  gHiiConfigRouting,
                                  Configuration,
                                  (UINT8 *) IfrFormNvData,
                                  &BufferSize,
                                  Progress
                                  );
    if (!EFI_ERROR (Status)) {
      Status = Ip4ConfigConvertIfrNvDataToDeviceConfigData (IfrFormNvData, Ip4ConfigInstance);
    }

    FreePool (IfrFormNvData);

  } else if (HiiIsConfigHdrMatch (Configuration, &gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE)) {

    IfrDeviceNvData = AllocateZeroPool (NIC_ITEM_CONFIG_SIZE);
    if (IfrDeviceNvData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    BufferSize = NIC_ITEM_CONFIG_SIZE;
    Status = gHiiConfigRouting->ConfigToBlock (
                                  gHiiConfigRouting,
                                  Configuration,
                                  (UINT8 *) IfrDeviceNvData,
                                  &BufferSize,
                                  Progress
                                  );
    if (!EFI_ERROR (Status)) {
      ZeroMem (&ZeroMac, sizeof (EFI_MAC_ADDRESS));
      if (CompareMem (&IfrDeviceNvData->NicAddr.MacAddr, &ZeroMac, IfrDeviceNvData->NicAddr.Len) != 0) {
        BufferSize = SIZEOF_NIC_IP4_CONFIG_INFO (IfrDeviceNvData);
        NicInfo = AllocateCopyPool (BufferSize, IfrDeviceNvData);
        if (NicInfo == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
        Status = EfiNicIp4ConfigSetInfo (Ip4ConfigInstance, NicInfo, TRUE);
        FreePool (NicInfo);
      } else {
        ZeroMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo, sizeof (IP4_SETTING_INFO));
        Status = EfiNicIp4ConfigSetInfo (Ip4ConfigInstance, NULL, TRUE);
      }
    }

    FreePool (IfrDeviceNvData);

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
  @param[out]  ActionRequest     On return, points to the action requested by the
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
  IP4_CONFIG_INSTANCE       *Ip4ConfigInstance;
  IP4_CONFIG_IFR_NVDATA     *IfrFormNvData;
  EFI_IP_ADDRESS            HostIp;
  EFI_IP_ADDRESS            SubnetMask;
  EFI_IP_ADDRESS            Gateway;
  EFI_STATUS                Status;
  EFI_INPUT_KEY             Key;

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    Ip4ConfigInstance = IP4_CONFIG_INSTANCE_FROM_CONFIG_ACCESS (This);

    IfrFormNvData = AllocateZeroPool (sizeof (IP4_CONFIG_IFR_NVDATA));
    if (IfrFormNvData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Retrive uncommitted data from Browser
    //
    if (!HiiGetBrowserData (&gNicIp4ConfigNvDataGuid, EFI_NIC_IP4_CONFIG_VARIABLE, sizeof (IP4_CONFIG_IFR_NVDATA), (UINT8 *) IfrFormNvData)) {
      FreePool (IfrFormNvData);
      return EFI_NOT_FOUND;
    }

    Status = EFI_SUCCESS;

    switch (QuestionId) {
    case KEY_LOCAL_IP:
      Status = Ip4StrToIp (IfrFormNvData->StationAddress, &HostIp.v4);
      if (EFI_ERROR (Status) || !NetIp4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid IP address!", NULL);
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo.LocalIp, &HostIp.v4, sizeof (HostIp.v4));
      }

      break;

    case KEY_SUBNET_MASK:
      Status = Ip4StrToIp (IfrFormNvData->SubnetMask, &SubnetMask.v4);
      if (EFI_ERROR (Status) || ((SubnetMask.Addr[0] != 0) && (GetSubnetMaskPrefixLength (&SubnetMask.v4) == 0))) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Subnet Mask!", NULL);
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo.SubnetMask, &SubnetMask.v4, sizeof (SubnetMask.v4));
      }

      break;

    case KEY_GATE_WAY:
      Status = Ip4StrToIp (IfrFormNvData->GatewayAddress, &Gateway.v4);
      if (EFI_ERROR (Status) || ((Gateway.Addr[0] != 0) && !NetIp4IsUnicast (NTOHL (Gateway.Addr[0]), 0))) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Gateway!", NULL);
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Ip4ConfigInstance->Ip4ConfigCallbackInfo.Gateway, &Gateway.v4, sizeof (Gateway.v4));
      }

      break;

    case KEY_SAVE_CHANGES:
      Status = Ip4ConfigConvertIfrNvDataToDeviceConfigData (IfrFormNvData, Ip4ConfigInstance);
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

  @param[in]  Instance            The IP4 Config instance.

  @retval EFI_SUCCESS              The HII Config Access protocol is installed.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.
**/
EFI_STATUS
Ip4ConfigDeviceInit (
  IN IP4_CONFIG_INSTANCE         *Instance
  )
{
  EFI_STATUS                     Status;
  EFI_HII_CONFIG_ACCESS_PROTOCOL *ConfigAccess;
  VENDOR_DEVICE_PATH             VendorDeviceNode;
  EFI_SERVICE_BINDING_PROTOCOL   *MnpSb;
  CHAR16                         *MacString;
  CHAR16                         MenuString[128];
  CHAR16                         PortString[128];
  CHAR16                         *OldMenuString;

  ConfigAccess = &Instance->HiiConfigAccessProtocol;
  ConfigAccess->ExtractConfig = Ip4DeviceExtractConfig;
  ConfigAccess->RouteConfig   = Ip4DeviceRouteConfig;
  ConfigAccess->Callback      = Ip4FormCallback;

  //
  // Construct device path node for EFI HII Config Access protocol,
  // which consists of controller physical device path and one hardware
  // vendor guid node.
  //
  ZeroMem (&VendorDeviceNode, sizeof (VENDOR_DEVICE_PATH));
  VendorDeviceNode.Header.Type = HARDWARE_DEVICE_PATH;
  VendorDeviceNode.Header.SubType = HW_VENDOR_DP;

  CopyGuid (&VendorDeviceNode.Guid, &gEfiNicIp4ConfigVariableGuid);

  SetDevicePathNodeLength (&VendorDeviceNode.Header, sizeof (VENDOR_DEVICE_PATH));
  Instance->HiiVendorDevicePath = AppendDevicePathNode (
                                    Instance->ParentDevicePath,
                                    (EFI_DEVICE_PATH_PROTOCOL *) &VendorDeviceNode
                                    );

  Instance->ChildHandle = NULL;
  //
  // Install Device Path Protocol and Config Access protocol on new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Instance->ChildHandle,
                  &gEfiDevicePathProtocolGuid,
                  Instance->HiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  ConfigAccess,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Open the Parent Handle for the child
    //
    Status = gBS->OpenProtocol (
                    Instance->Controller,
                    &gEfiManagedNetworkServiceBindingProtocolGuid,
                    (VOID **) &MnpSb,
                    Instance->Image,
                    Instance->ChildHandle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
  }

  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  Instance->RegisteredHandle = HiiAddPackages (
                                 &gNicIp4ConfigNvDataGuid,
                                 Instance->ChildHandle,
                                 Ip4ConfigDxeStrings,
                                 Ip4ConfigDxeBin,
                                 NULL
                                 );
  if (Instance->RegisteredHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Append MAC string in the menu help string and tile help string
  //
  Status = NetLibGetMacString (Instance->Controller, Instance->Image, &MacString);
  if (!EFI_ERROR (Status)) {
    OldMenuString = HiiGetString (Instance->RegisteredHandle, STRING_TOKEN (STR_IP4_CONFIG_FORM_HELP), NULL);
    UnicodeSPrint (MenuString, 128, L"%s (MAC:%s)", OldMenuString, MacString);
    HiiSetString (Instance->RegisteredHandle, STRING_TOKEN (STR_IP4_CONFIG_FORM_HELP), MenuString, NULL);

    UnicodeSPrint (PortString, 128, L"MAC:%s", MacString);
    HiiSetString (Instance->RegisteredHandle, STRING_TOKEN (STR_IP4_DEVICE_FORM_HELP), PortString, NULL);
    FreePool (MacString);
  }

  return Status;
}

/**
  Uninstall HII Config Access protocol for network device and free resource.

  @param[in]  Instance            The IP4 Config instance.

  @retval EFI_SUCCESS             The HII Config Access protocol is uninstalled.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
Ip4ConfigDeviceUnload (
  IN IP4_CONFIG_INSTANCE              *Instance
  )
{
  //
  // Remove HII package list
  //
  HiiRemovePackages (Instance->RegisteredHandle);

  //
  // Close the child handle
  //
  gBS->CloseProtocol (
         Instance->Controller,
         &gEfiManagedNetworkServiceBindingProtocolGuid,
         Instance->Image,
         Instance->ChildHandle
         );

  //
  // Uninstall EFI_HII_CONFIG_ACCESS_PROTOCOL
  //
  gBS->UninstallMultipleProtocolInterfaces (
         Instance->ChildHandle,
         &gEfiDevicePathProtocolGuid,
         Instance->HiiVendorDevicePath,
         &gEfiHiiConfigAccessProtocolGuid,
         &Instance->HiiConfigAccessProtocol,
         NULL
         );

  return EFI_SUCCESS;
}
