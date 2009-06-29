/** @file
  Helper functions for configuring or getting the parameters relating to Ip4.

Copyright (c) 2009, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip4ConfigNv.h"

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  Ip4HexString[] = "0123456789ABCDEFabcdef";

UINTN           mNumberOfIp4Devices      = 0;

IP4_FORM_CALLBACK_INFO  *mCallbackInfo = NULL;

LIST_ENTRY      mIp4ConfigFormList = {
  &mIp4ConfigFormList,
  &mIp4ConfigFormList
};

HII_VENDOR_DEVICE_PATH  mIp4ConifgHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {6D3FD906-42B9-4220-9E63-9D8C972D58EE}
    //
    { 0x6d3fd906, 0x42b9, 0x4220, { 0x9e, 0x63, 0x9d, 0x8c, 0x97, 0x2d, 0x58, 0xee } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**
  Convert the mac address into a hexadecimal encoded "-" seperated string.

  @param[in]  Mac The mac address.
  @param[in]  Len  Length in bytes of the mac address.
  @param[out] Str The storage to return the mac string.

**/
VOID
Ip4MacAddrToStr (
  IN  EFI_MAC_ADDRESS  *Mac,
  IN  UINT32           Len,
  OUT CHAR16           *Str
  )
{
  UINT32  Index;

  for (Index = 0; Index < Len; Index++) {
    Str[3 * Index]      = (CHAR16) Ip4HexString[(Mac->Addr[Index] >> 4) & 0x0F];
    Str[3 * Index + 1]  = (CHAR16) Ip4HexString[Mac->Addr[Index] & 0x0F];
    Str[3 * Index + 2]  = L'-';
  }

  Str[3 * Index - 1] = L'\0';
}

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
  ReverseMask = (SubnetMask->Addr[0] << 24) | (SubnetMask->Addr[1] << 16) | (SubnetMask->Addr[2] << 8) | (SubnetMask->Addr[3]);

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
  @param[out]  Ip              The storage to return the ASCII string.

  @retval EFI_SUCCESS           The binary IP address is returned in Ip.
  @retval EFI_INVALID_PARAMETER The IP string is malformatted.
**/
EFI_STATUS
Ip4AsciiStrToIp (
  IN  CHAR8             *Str,
  OUT EFI_IPv4_ADDRESS  *Ip
  )
{
  UINTN Index;
  UINTN Number;

  Index = 0;

  while (*Str != 0) {

    if (Index > 3) {
      return EFI_INVALID_PARAMETER;
    }

    Number = 0;
    while (NET_IS_DIGIT (*Str)) {
      Number = Number * 10 + (*Str - '0');
      Str++;
    }

    if (Number > 0xFF) {
      return EFI_INVALID_PARAMETER;
    }

    Ip->Addr[Index] = (UINT8) Number;

    if ((*Str != '\0') && (*Str != '.')) {
      //
      // The current character should be either the NULL terminator or
      // the dot delimiter.
      //
      return EFI_INVALID_PARAMETER;
    }

    if (*Str == '.') {
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

  @param[in]   ConfigFormEntry The IP4 configuration form entry.
  @param[out]  IfrNvData       The IFR nv data.
**/
VOID
Ip4ConfigConvertDeviceConfigDataToIfrNvData (
  IN  IP4_CONFIG_INSTANCE       *Ip4ConfigInstance,
  OUT IP4_CONFIG_IFR_NVDATA     *IfrFormNvData
  )
{
  EFI_STATUS                    Status;
  NIC_IP4_CONFIG_INFO           *NicConfig;
  UINTN                         ConfigLen;

  IfrFormNvData->DhcpEnable = 1;

  ConfigLen = sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * 2;
  NicConfig = AllocateZeroPool (ConfigLen);
  ASSERT (NicConfig != NULL);
  Status = EfiNicIp4ConfigGetInfo (Ip4ConfigInstance, &ConfigLen, NicConfig);
  if (!EFI_ERROR (Status)) {
    if (NicConfig->Source == IP4_CONFIG_SOURCE_DHCP) {
      IfrFormNvData->DhcpEnable = 1;
    } else {
      IfrFormNvData->DhcpEnable = 0;
      Ip4ConfigIpToStr (&NicConfig->Ip4Info.StationAddress, IfrFormNvData->StationAddress);
      Ip4ConfigIpToStr (&NicConfig->Ip4Info.SubnetMask, IfrFormNvData->SubnetMask);
      Ip4ConfigIpToStr (&NicConfig->Ip4Info.RouteTable[1].GatewayAddress, IfrFormNvData->GatewayAddress);
    }
  }
  FreePool (NicConfig);
}

/**
  Get the IP4 configuration form entry by the index of the goto opcode actived.

  @param[in]  Index The 0-based index of the goto opcode actived.

  @return The IP4 configuration form entry found.
**/
IP4CONFIG_FORM_ENTRY *
Ip4GetConfigFormEntryByIndex (
  IN UINT32 Index
  )
{
  UINT32                  CurrentIndex;
  LIST_ENTRY              *Entry;
  IP4CONFIG_FORM_ENTRY    *ConfigFormEntry;

  CurrentIndex    = 0;
  ConfigFormEntry = NULL;

  NET_LIST_FOR_EACH (Entry, &mIp4ConfigFormList) {
    if (CurrentIndex == Index) {
      ConfigFormEntry = NET_LIST_USER_STRUCT (Entry, IP4CONFIG_FORM_ENTRY, Link);
      break;
    }

    CurrentIndex++;
  }

  return ConfigFormEntry;
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
  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL
                                  for the Request parameter
                                  would result in this type of
                                  error. In this case, the
                                  Progress parameter would be
                                  set to NULL.
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
  UINTN                            ConfigLen;
  NIC_IP4_CONFIG_INFO              *IfrDeviceNvData;
  IP4_FORM_CALLBACK_INFO           *Private;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  IP4_CONFIG_INSTANCE              *Ip4ConfigInstance;

  if (Request == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;

  //
  // Check Request data in <ConfigHdr>.
  //
  if (!HiiIsConfigHdrMatch (Request, &gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE)) {
    return EFI_NOT_FOUND;
  }

  Private = IP4CONFIG_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK (This);
  Ip4ConfigInstance = IP4_CONFIG_INSTANCE_FROM_IP4FORM_CALLBACK_INFO (Private);

  IfrDeviceNvData = AllocateZeroPool (NIC_ITEM_CONFIG_SIZE);
  if (IfrDeviceNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EfiNicIp4ConfigGetInfo (Ip4ConfigInstance, &ConfigLen, IfrDeviceNvData);
  if (EFI_ERROR (Status)) {
    FreePool (IfrDeviceNvData);
    return EFI_NOT_FOUND;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  HiiConfigRouting = Private->ConfigRouting;
  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               Request,
                               (UINT8 *) IfrDeviceNvData,
                               NIC_ITEM_CONFIG_SIZE,
                               Results,
                               Progress
                               );

  FreePool (IfrDeviceNvData);

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
  NIC_IP4_CONFIG_INFO              *NicInfo;
  IP4_FORM_CALLBACK_INFO           *Private;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  IP4_CONFIG_INSTANCE              *Ip4ConfigInstance;
  EFI_MAC_ADDRESS                  ZeroMac;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  //
  // Check Routing data in <ConfigHdr>.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE)) {
    return EFI_NOT_FOUND;
  }

  Private = IP4CONFIG_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK (This);
  Ip4ConfigInstance = IP4_CONFIG_INSTANCE_FROM_IP4FORM_CALLBACK_INFO (Private);

  IfrDeviceNvData = AllocateZeroPool (NIC_ITEM_CONFIG_SIZE);
  if (IfrDeviceNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  HiiConfigRouting = Private->ConfigRouting;
  BufferSize = NIC_ITEM_CONFIG_SIZE;
  Status = HiiConfigRouting->ConfigToBlock (
                               HiiConfigRouting,
                               Configuration,
                               (UINT8 *) IfrDeviceNvData,
                               &BufferSize,
                               Progress
                               );
  if (!EFI_ERROR (Status)) {
    ZeroMem (&ZeroMac, sizeof (EFI_MAC_ADDRESS));
    if (CompareMem (&IfrDeviceNvData->NicAddr.MacAddr, &ZeroMac, IfrDeviceNvData->NicAddr.Len) != 0) {
      BufferSize = sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * IfrDeviceNvData->Ip4Info.RouteTableSize;
      NicInfo = AllocateCopyPool (BufferSize, IfrDeviceNvData); 
      Status = EfiNicIp4ConfigSetInfo (Ip4ConfigInstance, NicInfo, TRUE);
    } else {
      Status = EfiNicIp4ConfigSetInfo (Ip4ConfigInstance, NULL, TRUE);
    }
  }

  FreePool (IfrDeviceNvData);
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
  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL
                                  for the Request parameter
                                  would result in this type of
                                  error. In this case, the
                                  Progress parameter would be
                                  set to NULL.
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
  if (Request == NULL || Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Request;
  return EFI_NOT_FOUND;
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
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  if (!HiiIsConfigHdrMatch (Configuration, &gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE)) {
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
  return EFI_SUCCESS;
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
  IP4_FORM_CALLBACK_INFO    *Private;
  CHAR8                     Ip4String[IP4_STR_MAX_SIZE];
  CHAR16                    PortString[128];
  EFI_STRING_ID             DeviceFormTitleToken;
  IP4_CONFIG_IFR_NVDATA     *IfrFormNvData;
  IP4CONFIG_FORM_ENTRY      *ConfigFormEntry;
  EFI_IP_ADDRESS            HostIp;
  EFI_IP_ADDRESS            SubnetMask;
  EFI_IP_ADDRESS            Gateway;
  EFI_STATUS                Status;
  EFI_INPUT_KEY             Key;
  NIC_IP4_CONFIG_INFO       *NicInfo;
  EFI_IP_ADDRESS            Ip;

  ConfigFormEntry = NULL;

  Private   = IP4CONFIG_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK (This);

  IfrFormNvData = AllocateZeroPool (sizeof (IP4_CONFIG_IFR_NVDATA));
  if (IfrFormNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Retrive uncommitted data from Browser
  //
  if (!HiiGetBrowserData (&gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE, sizeof (IP4_CONFIG_IFR_NVDATA), (UINT8 *) IfrFormNvData)) {
    FreePool (IfrFormNvData);
    return EFI_NOT_FOUND;
  }

  Status = EFI_SUCCESS;

  switch (QuestionId) {

  case KEY_DHCP_ENABLE:
    if (IfrFormNvData->DhcpEnable == 0) {
      Private->Current->SessionConfigData.Enabled = FALSE;
    } else {
      Private->Current->SessionConfigData.Enabled = TRUE;
    }

    break;

  case KEY_LOCAL_IP:
    UnicodeStrToAsciiStr (IfrFormNvData->StationAddress, Ip4String);
    Status = Ip4AsciiStrToIp (Ip4String, &HostIp.v4);
    if (EFI_ERROR (Status) || !Ip4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid IP address!", NULL);
      Status = EFI_INVALID_PARAMETER;
    } else {
      CopyMem (&Private->Current->SessionConfigData.LocalIp, &HostIp.v4, sizeof (HostIp.v4));
    }

    break;

  case KEY_SUBNET_MASK:
    UnicodeStrToAsciiStr (IfrFormNvData->SubnetMask, Ip4String);
    Status = Ip4AsciiStrToIp (Ip4String, &SubnetMask.v4);
    if (EFI_ERROR (Status) || ((SubnetMask.Addr[0] != 0) && (GetSubnetMaskPrefixLength (&SubnetMask.v4) == 0))) {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid SubnetMask!", NULL);
      Status = EFI_INVALID_PARAMETER;
    } else {
      CopyMem (&Private->Current->SessionConfigData.SubnetMask, &SubnetMask.v4, sizeof (SubnetMask.v4));
    }

    break;

  case KEY_GATE_WAY:
    UnicodeStrToAsciiStr (IfrFormNvData->GatewayAddress, Ip4String);
    Status = Ip4AsciiStrToIp (Ip4String, &Gateway.v4);
    if (EFI_ERROR (Status) || ((Gateway.Addr[0] != 0) && !Ip4IsUnicast (NTOHL (Gateway.Addr[0]), 0))) {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Gateway!", NULL);
      Status = EFI_INVALID_PARAMETER;
    } else {
      CopyMem (&Private->Current->SessionConfigData.Gateway, &Gateway.v4, sizeof (Gateway.v4));
    }

    break;

  case KEY_SAVE_CHANGES:
    Ip4ConfigInstance = Private->Current->Ip4ConfigInstance;
    NicInfo = AllocateZeroPool (sizeof (NIC_IP4_CONFIG_INFO) + 2 * sizeof (EFI_IP4_ROUTE_TABLE));
    ASSERT (NicInfo != NULL);

    NicInfo->Ip4Info.RouteTable = (EFI_IP4_ROUTE_TABLE *) (NicInfo + 1);

    if (!Private->Current->SessionConfigData.Enabled) {
      CopyMem (&HostIp.v4, &Private->Current->SessionConfigData.LocalIp, sizeof (HostIp.v4));
      CopyMem (&SubnetMask.v4, &Private->Current->SessionConfigData.SubnetMask, sizeof (SubnetMask.v4));
      CopyMem (&Gateway.v4, &Private->Current->SessionConfigData.Gateway, sizeof (Gateway.v4));

      if ((Gateway.Addr[0] != 0)) {
        if (SubnetMask.Addr[0] == 0) {
          CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Gateway address is set but subnet mask is zero.", NULL);
          Status = EFI_INVALID_PARAMETER;
          break;
        } else if (!IP4_NET_EQUAL (HostIp.Addr[0], Gateway.Addr[0], SubnetMask.Addr[0])) {
          CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Local IP and Gateway are not in the same subnet.", NULL);
          Status = EFI_INVALID_PARAMETER;
          break;
        }
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
    }

    NicInfo->Perment = TRUE;
    CopyMem (&NicInfo->NicAddr, &Ip4ConfigInstance->NicAddr, sizeof (NIC_ADDR));

    EfiNicIp4ConfigSetInfo (Ip4ConfigInstance, NicInfo, TRUE);

    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
    break;

  default:
    if ((QuestionId >= KEY_DEVICE_ENTRY_BASE) && (QuestionId < (mNumberOfIp4Devices + KEY_DEVICE_ENTRY_BASE))) {
      //
      // In case goto the device configuration form, update the device form title.
      //
      ConfigFormEntry = Ip4GetConfigFormEntryByIndex ((UINT32) (QuestionId - KEY_DEVICE_ENTRY_BASE));
      ASSERT (ConfigFormEntry != NULL);

      Ip4ConfigInstance = ConfigFormEntry->Ip4ConfigInstance;

      UnicodeSPrint (PortString, (UINTN) 128, L"Port %s", ConfigFormEntry->MacString);
      DeviceFormTitleToken = (EFI_STRING_ID) STR_IP4_DEVICE_FORM_TITLE;
      HiiSetString (Private->RegisteredHandle, DeviceFormTitleToken, PortString, NULL);

      Ip4ConfigConvertDeviceConfigDataToIfrNvData (Ip4ConfigInstance, IfrFormNvData);

      Private->Current = ConfigFormEntry;
    }

    break;
  }

  if (!EFI_ERROR (Status)) {

    //
    // Pass changed uncommitted data back to Form Browser
    //
    HiiSetBrowserData (&gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE, sizeof (IP4_CONFIG_IFR_NVDATA), (UINT8 *) IfrFormNvData, NULL);
  }

  FreePool (IfrFormNvData);
  return Status;
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
    IN IP4_CONFIG_INSTANCE              *Instance
  )
{
  EFI_STATUS                  Status;
  EFI_HII_DATABASE_PROTOCOL   *HiiDatabase;
  IP4_FORM_CALLBACK_INFO      *CallbackInfo;

  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&HiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CallbackInfo = &Instance->Ip4FormCallbackInfo;

  CallbackInfo->Signature   = IP4CONFIG_FORM_CALLBACK_INFO_SIGNATURE;
  CallbackInfo->HiiDatabase = HiiDatabase;

  CallbackInfo->ConfigAccess.ExtractConfig = Ip4DeviceExtractConfig;
  CallbackInfo->ConfigAccess.RouteConfig = Ip4DeviceRouteConfig;
  CallbackInfo->ConfigAccess.Callback = NULL;

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&CallbackInfo->ConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CallbackInfo->DriverHandle = Instance->Controller;
  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &CallbackInfo->DriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &CallbackInfo->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

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
  IP4_FORM_CALLBACK_INFO      *CallbackInfo;

  CallbackInfo = &Instance->Ip4FormCallbackInfo;

  Ip4ConfigUpdateForm (Instance, FALSE);

  //
  // Uninstall EFI_HII_CONFIG_ACCESS_PROTOCOL
  //
  gBS->UninstallMultipleProtocolInterfaces (
         CallbackInfo->DriverHandle,
         &gEfiHiiConfigAccessProtocolGuid,
         &CallbackInfo->ConfigAccess,
         NULL
         );

  return EFI_SUCCESS;
}

/**
  Unload the network configuration form, this includes: delete all the network
  device configuration entries, uninstall the form callback protocol and
  free the resources used.

  @retval EFI_SUCCESS             The network configuration form is unloaded.
**/
EFI_STATUS
Ip4ConfigFormUnload (
  VOID
  )
{
  IP4CONFIG_FORM_ENTRY     *ConfigFormEntry;

  while (!IsListEmpty (&mIp4ConfigFormList)) {
    //
    // Uninstall the device forms as the network driver instance may fail to
    // control the controller but still install the device configuration form.
    // In such case, upon driver unloading, the driver instance's driverbinding.
    // stop () won't be called, so we have to take this chance here to uninstall
    // the device form.
    //
    ConfigFormEntry = NET_LIST_USER_STRUCT (mIp4ConfigFormList.ForwardLink, IP4CONFIG_FORM_ENTRY, Link);
    Ip4ConfigUpdateForm (ConfigFormEntry->Ip4ConfigInstance, FALSE);
  }

  //
  // Remove HII package list
  //
  mCallbackInfo->HiiDatabase->RemovePackageList (
                                mCallbackInfo->HiiDatabase,
                                mCallbackInfo->RegisteredHandle
                                );

  //
  // Uninstall EFI_HII_CONFIG_ACCESS_PROTOCOL
  //
  gBS->UninstallMultipleProtocolInterfaces (
         mCallbackInfo->DriverHandle,
         &gEfiDevicePathProtocolGuid,
         &mIp4ConifgHiiVendorDevicePath,
         &gEfiHiiConfigAccessProtocolGuid,
         &mCallbackInfo->ConfigAccess,
         NULL
         );

  FreePool (mCallbackInfo);

  return EFI_SUCCESS;
}

/**
  Updates the network configuration form to add/delete an entry for the network
  device specified by the Instance.

  @param[in]  Instance            The IP4 Config instance.
  @param[in]  AddForm             Whether to add or delete a form entry.

  @retval EFI_SUCCESS             The network configuration form is updated.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
Ip4ConfigUpdateForm (
  IN IP4_CONFIG_INSTANCE              *Instance,
  IN BOOLEAN                          AddForm
  )
{
  LIST_ENTRY                  *Entry;
  IP4CONFIG_FORM_ENTRY        *ConfigFormEntry;
  BOOLEAN                     EntryExisted;
  EFI_STATUS                  Status;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  CHAR16                      PortString[128];
  UINT16                      FormIndex;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;

  ConfigFormEntry = NULL;
  EntryExisted    = FALSE;

  NET_LIST_FOR_EACH (Entry, &mIp4ConfigFormList) {
    ConfigFormEntry = NET_LIST_USER_STRUCT (Entry, IP4CONFIG_FORM_ENTRY, Link);

    if (ConfigFormEntry->Controller == Instance->Controller) {
      EntryExisted = TRUE;
      break;
    }
  }

  if (AddForm) {
    if (EntryExisted) {
      return EFI_SUCCESS;
    } else {
      //
      // Add a new form.
      //
      ConfigFormEntry = (IP4CONFIG_FORM_ENTRY *) AllocateZeroPool (sizeof (IP4CONFIG_FORM_ENTRY));
      if (ConfigFormEntry == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      ConfigFormEntry->Ip4ConfigInstance = Instance;
      InitializeListHead (&ConfigFormEntry->Link);
      ConfigFormEntry->Controller = Instance->Controller;

      //
      // Get the simple network protocol and convert the MAC address into
      // the formatted string.
      //
      Status = gBS->HandleProtocol (
                      Instance->Controller,
                      &gEfiSimpleNetworkProtocolGuid,
                      (VOID **)&Snp
                      );
      ASSERT (Status == EFI_SUCCESS);

      Ip4MacAddrToStr (&Snp->Mode->PermanentAddress, Snp->Mode->HwAddressSize, ConfigFormEntry->MacString);

      //
      // Compose the Port string and create a new EFI_STRING_ID.
      //
      UnicodeSPrint (PortString, 128, L"%s %s", Instance->NicName, ConfigFormEntry->MacString);
      ConfigFormEntry->PortTitleToken = HiiSetString (mCallbackInfo->RegisteredHandle, 0, PortString, NULL);

      //
      // Compose the help string of this port and create a new EFI_STRING_ID.
      //
      UnicodeSPrint (PortString, 128, L"Set the network parameters on eth%d %s", 0, ConfigFormEntry->MacString);
      ConfigFormEntry->PortTitleHelpToken = HiiSetString (mCallbackInfo->RegisteredHandle, 0, PortString, NULL);

      InsertTailList (&mIp4ConfigFormList, &ConfigFormEntry->Link);
      mNumberOfIp4Devices++;
    }
  } else {
    ASSERT (EntryExisted);

    mNumberOfIp4Devices--;
    RemoveEntryList (&ConfigFormEntry->Link);
    gBS->FreePool (ConfigFormEntry);
  }

  //
  // Init OpCode Handle
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = DEVICE_ENTRY_LABEL;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  FormIndex = 0;

  FormIndex = 0;
  NET_LIST_FOR_EACH (Entry, &mIp4ConfigFormList) {
    ConfigFormEntry = NET_LIST_USER_STRUCT (Entry, IP4CONFIG_FORM_ENTRY, Link);

    HiiCreateGotoOpCode (
      StartOpCodeHandle,                            // Container for dynamic created opcodes
      FORMID_DEVICE_FORM,                           // Target Form ID
      ConfigFormEntry->PortTitleToken,              // Prompt text
      ConfigFormEntry->PortTitleHelpToken,          // Help text
      EFI_IFR_FLAG_CALLBACK,                        // Question flag
      (UINT16)(KEY_DEVICE_ENTRY_BASE + FormIndex)   // Question ID
      );

    FormIndex++;
  }

  HiiUpdateForm (
    mCallbackInfo->RegisteredHandle,
    &gEfiNicIp4ConfigVariableGuid,
    FORMID_MAIN_FORM,
    StartOpCodeHandle, // Label DEVICE_ENTRY_LABEL
    EndOpCodeHandle    // LABEL_END
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return EFI_SUCCESS;
}

/**
  Initialize the network configuration form, this includes: delete all the network
  device configuration entries, install the form callback protocol and
  allocate the resources used.

  @retval EFI_SUCCESS             The network configuration form is unloaded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
Ip4ConfigFormInit (
    VOID
  )
{
  EFI_STATUS                  Status;
  EFI_HII_DATABASE_PROTOCOL   *HiiDatabase;
  IP4_FORM_CALLBACK_INFO      *CallbackInfo;

  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&HiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CallbackInfo = (IP4_FORM_CALLBACK_INFO *) AllocateZeroPool (sizeof (IP4_FORM_CALLBACK_INFO));
  if (CallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CallbackInfo->Signature   = IP4CONFIG_FORM_CALLBACK_INFO_SIGNATURE;
  CallbackInfo->HiiDatabase = HiiDatabase;

  CallbackInfo->ConfigAccess.ExtractConfig = Ip4FormExtractConfig;
  CallbackInfo->ConfigAccess.RouteConfig = Ip4FormRouteConfig;
  CallbackInfo->ConfigAccess.Callback = Ip4FormCallback;

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&CallbackInfo->ConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CallbackInfo->DriverHandle = NULL;
  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &CallbackInfo->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mIp4ConifgHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &CallbackInfo->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  CallbackInfo->RegisteredHandle = HiiAddPackages (
                                     &gEfiNicIp4ConfigVariableGuid,
                                     CallbackInfo->DriverHandle,
                                     Ip4ConfigDxeStrings,
                                     Ip4ConfigDxeBin,
                                     NULL
                                     );
  if (CallbackInfo->RegisteredHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mCallbackInfo = CallbackInfo;

  return Status;
}
