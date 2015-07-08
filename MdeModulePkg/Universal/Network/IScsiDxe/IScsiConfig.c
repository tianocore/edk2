/** @file
  Helper functions for configuring or getting the parameters relating to iSCSI.

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

CHAR16          mVendorStorageName[]     = L"ISCSI_CONFIG_IFR_NVDATA";
BOOLEAN         mIScsiDeviceListUpdated  = FALSE;
UINTN           mNumberOfIScsiDevices    = 0;
ISCSI_FORM_CALLBACK_INFO  *mCallbackInfo = NULL;

LIST_ENTRY      mIScsiConfigFormList = {
  &mIScsiConfigFormList,
  &mIScsiConfigFormList
};

HII_VENDOR_DEVICE_PATH  mIScsiHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    IP4_ISCSI_CONFIG_GUID
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
  Convert the IPv4 address into a dotted string.

  @param[in]   Ip   The IPv4 address.
  @param[out]  Str  The dotted IP string.
**/
VOID
IScsiIpToStr (
  IN  EFI_IPv4_ADDRESS  *Ip,
  OUT CHAR16            *Str
  )
{
  UnicodeSPrint ( Str, 2 * IP4_STR_MAX_SIZE, L"%d.%d.%d.%d", Ip->Addr[0], Ip->Addr[1], Ip->Addr[2], Ip->Addr[3]);
}


/**
  Parse IsId in string format and convert it to binary.

  @param[in]        String  The buffer of the string to be parsed.
  @param[in, out]   IsId    The buffer to store IsId.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.

**/
EFI_STATUS
IScsiParseIsIdFromString (
  IN CONST CHAR16                    *String,
  IN OUT   UINT8                     *IsId
  )
{
  UINT8                          Index;
  CHAR16                         *IsIdStr;
  CHAR16                         TempStr[3];
  UINTN                          NodeVal;
  CHAR16                         PortString[ISCSI_NAME_IFR_MAX_SIZE];
  EFI_INPUT_KEY                  Key;

  if ((String == NULL) || (IsId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  IsIdStr = (CHAR16 *) String;

  if (StrLen (IsIdStr) != 6) {
    UnicodeSPrint (
      PortString,
      (UINTN) sizeof (PortString),
      L"Error! Input is incorrect, please input 6 hex numbers!\n"
      );

    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      PortString,
      NULL
      );

    return EFI_INVALID_PARAMETER;
  }

  for (Index = 3; Index < 6; Index++) {
    CopyMem (TempStr, IsIdStr, sizeof (TempStr));
    TempStr[2] = L'\0';

    //
    // Convert the string to IsId. StrHexToUintn stops at the first character
    // that is not a valid hex character, '\0' here.
    //
    NodeVal = StrHexToUintn (TempStr);

    IsId[Index] = (UINT8) NodeVal;

    IsIdStr = IsIdStr + 2;
  }

  return EFI_SUCCESS;
}

/**
  Convert IsId from binary to string format.

  @param[out]      String  The buffer to store the converted string.
  @param[in]       IsId    The buffer to store IsId.

  @retval EFI_SUCCESS              The string converted successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.

**/
EFI_STATUS
IScsiConvertIsIdToString (
  OUT CHAR16                         *String,
  IN  UINT8                          *IsId
  )
{
  UINT8                          Index;
  UINTN                          Number;

  if ((String == NULL) || (IsId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < 6; Index++) {
    if (IsId[Index] <= 0xF) {
      Number = UnicodeSPrint (
                 String,
                 2 * ISID_CONFIGURABLE_STORAGE,
                 L"0%X",
                 (UINTN) IsId[Index]
                 );
    } else {
      Number = UnicodeSPrint (
                 String,
                 2 * ISID_CONFIGURABLE_STORAGE,
                 L"%X",
                 (UINTN) IsId[Index]
                 );

    }

    String = String + Number;
  }

  *String = L'\0';

  return EFI_SUCCESS;
}


/**
  Update the list of iSCSI devices the iSCSI driver is controlling.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval Others                 Other errors as indicated.   
**/
EFI_STATUS
IScsiUpdateDeviceList (
  VOID
  )
{
  EFI_STATUS                  Status;
  ISCSI_DEVICE_LIST           *DeviceList;
  UINTN                       DataSize;
  UINTN                       NumHandles;
  EFI_HANDLE                  *Handles;
  UINTN                       HandleIndex;
  UINTN                       Index;
  UINTN                       LastDeviceIndex;
  EFI_MAC_ADDRESS             MacAddress;
  UINTN                       HwAddressSize;
  UINT16                      VlanId;
  ISCSI_MAC_INFO              *CurMacInfo;
  ISCSI_MAC_INFO              TempMacInfo;
  CHAR16                      MacString[70];
  UINTN                       DeviceListSize;

  //
  // Dump all the handles the Managed Network Service Binding Protocol is installed on.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  NULL,
                  &NumHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DataSize = 0;
  Status = gRT->GetVariable (
                  L"iSCSIDeviceList",
                  &gIp4IScsiConfigGuid,
                  NULL,
                  &DataSize,
                  NULL
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    DeviceList = (ISCSI_DEVICE_LIST *) AllocatePool (DataSize);
    ASSERT (DeviceList != NULL);

    gRT->GetVariable (
          L"iSCSIDeviceList",
          &gIp4IScsiConfigGuid,
          NULL,
          &DataSize,
          DeviceList
          );

    LastDeviceIndex = 0;

    for (HandleIndex = 0; HandleIndex < NumHandles; HandleIndex++) {
      Status = NetLibGetMacAddress (Handles[HandleIndex], &MacAddress, &HwAddressSize);
      ASSERT (Status == EFI_SUCCESS);
      VlanId = NetLibGetVlanId (Handles[HandleIndex]);

      for (Index = LastDeviceIndex; Index < DeviceList->NumDevice; Index++) {
        CurMacInfo = &DeviceList->MacInfo[Index];
        if ((CurMacInfo->Len == HwAddressSize) &&
            (CurMacInfo->VlanId == VlanId) &&
            (NET_MAC_EQUAL (&CurMacInfo->Mac, MacAddress.Addr, HwAddressSize))
            ) {
          //
          // The previous configured NIC is still here.
          //
          if (Index != LastDeviceIndex) {
            //
            // Swap the current MAC address entry with the one indexed by
            // LastDeviceIndex.
            //
            CopyMem (&TempMacInfo, CurMacInfo, sizeof (ISCSI_MAC_INFO));
            CopyMem (CurMacInfo, &DeviceList->MacInfo[LastDeviceIndex], sizeof (ISCSI_MAC_INFO));
            CopyMem (&DeviceList->MacInfo[LastDeviceIndex], &TempMacInfo, sizeof (ISCSI_MAC_INFO));
          }

          LastDeviceIndex++;
        }
      }

      if (LastDeviceIndex == DeviceList->NumDevice) {
        break;
      }
    }

    for (Index = LastDeviceIndex; Index < DeviceList->NumDevice; Index++) {
      //
      // delete the variables
      //
      CurMacInfo = &DeviceList->MacInfo[Index];
      IScsiMacAddrToStr (&CurMacInfo->Mac, CurMacInfo->Len, CurMacInfo->VlanId, MacString);
      gRT->SetVariable (MacString, &gEfiIScsiInitiatorNameProtocolGuid, 0, 0, NULL);
      gRT->SetVariable (MacString, &gIScsiCHAPAuthInfoGuid, 0, 0, NULL);
    }

    FreePool (DeviceList);
  } else if (Status != EFI_NOT_FOUND) {
    FreePool (Handles);
    return Status;
  }
  //
  // Construct the new iSCSI device list.
  //
  DeviceListSize        = sizeof (ISCSI_DEVICE_LIST) + (NumHandles - 1) * sizeof (ISCSI_MAC_INFO);
  DeviceList            = (ISCSI_DEVICE_LIST *) AllocatePool (DeviceListSize);
  ASSERT (DeviceList != NULL);
  DeviceList->NumDevice = (UINT8) NumHandles;

  for (Index = 0; Index < NumHandles; Index++) {
    NetLibGetMacAddress (Handles[Index], &MacAddress, &HwAddressSize);

    CurMacInfo  = &DeviceList->MacInfo[Index];
    CopyMem (&CurMacInfo->Mac, MacAddress.Addr, HwAddressSize);
    CurMacInfo->Len = (UINT8) HwAddressSize;
    CurMacInfo->VlanId = NetLibGetVlanId (Handles[Index]);
  }

  gRT->SetVariable (
        L"iSCSIDeviceList",
        &gIp4IScsiConfigGuid,
        ISCSI_CONFIG_VAR_ATTR,
        DeviceListSize,
        DeviceList
        );

  FreePool (DeviceList);
  FreePool (Handles);

  return Status;
}

/**
  Get the iSCSI configuration form entry by the index of the goto opcode actived.

  @param[in]  Index The 0-based index of the goto opcode actived.

  @return The iSCSI configuration form entry found.
**/
ISCSI_CONFIG_FORM_ENTRY *
IScsiGetConfigFormEntryByIndex (
  IN UINT32 Index
  )
{
  UINT32                  CurrentIndex;
  LIST_ENTRY              *Entry;
  ISCSI_CONFIG_FORM_ENTRY *ConfigFormEntry;

  CurrentIndex    = 0;
  ConfigFormEntry = NULL;

  NET_LIST_FOR_EACH (Entry, &mIScsiConfigFormList) {
    if (CurrentIndex == Index) {
      ConfigFormEntry = NET_LIST_USER_STRUCT (Entry, ISCSI_CONFIG_FORM_ENTRY, Link);
      break;
    }

    CurrentIndex++;
  }

  return ConfigFormEntry;
}

/**
  Convert the iSCSI configuration data into the IFR data.

  @param[in]   ConfigFormEntry The iSCSI configuration form entry.
  @param[out]  IfrNvData       The IFR nv data.

**/
VOID
IScsiConvertDeviceConfigDataToIfrNvData (
  IN ISCSI_CONFIG_FORM_ENTRY      *ConfigFormEntry,
  OUT ISCSI_CONFIG_IFR_NVDATA     *IfrNvData
  )
{
  ISCSI_SESSION_CONFIG_NVDATA   *SessionConfigData;
  ISCSI_CHAP_AUTH_CONFIG_NVDATA *AuthConfigData;

  //
  // Normal session configuration parameters.
  //
  SessionConfigData                 = &ConfigFormEntry->SessionConfigData;
  IfrNvData->Enabled                = SessionConfigData->Enabled;

  IfrNvData->InitiatorInfoFromDhcp  = SessionConfigData->InitiatorInfoFromDhcp;
  IfrNvData->TargetInfoFromDhcp     = SessionConfigData->TargetInfoFromDhcp;
  IfrNvData->TargetPort             = SessionConfigData->TargetPort;

  IScsiIpToStr (&SessionConfigData->LocalIp, IfrNvData->LocalIp);
  IScsiIpToStr (&SessionConfigData->SubnetMask, IfrNvData->SubnetMask);
  IScsiIpToStr (&SessionConfigData->Gateway, IfrNvData->Gateway);
  IScsiIpToStr (&SessionConfigData->TargetIp, IfrNvData->TargetIp);

  IScsiAsciiStrToUnicodeStr (SessionConfigData->TargetName, IfrNvData->TargetName);

  IScsiLunToUnicodeStr (SessionConfigData->BootLun, IfrNvData->BootLun);

  IScsiConvertIsIdToString (IfrNvData->IsId, SessionConfigData->IsId);

  //
  // CHAP authentication parameters.
  //
  AuthConfigData      = &ConfigFormEntry->AuthConfigData;

  IfrNvData->CHAPType = AuthConfigData->CHAPType;

  IScsiAsciiStrToUnicodeStr (AuthConfigData->CHAPName, IfrNvData->CHAPName);
  IScsiAsciiStrToUnicodeStr (AuthConfigData->CHAPSecret, IfrNvData->CHAPSecret);
  IScsiAsciiStrToUnicodeStr (AuthConfigData->ReverseCHAPName, IfrNvData->ReverseCHAPName);
  IScsiAsciiStrToUnicodeStr (AuthConfigData->ReverseCHAPSecret, IfrNvData->ReverseCHAPSecret);
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
IScsiFormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  CHAR8                            InitiatorName[ISCSI_NAME_MAX_SIZE];
  UINTN                            BufferSize;
  ISCSI_CONFIG_IFR_NVDATA          *IfrNvData;
  ISCSI_FORM_CALLBACK_INFO         *Private;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gIp4IScsiConfigGuid, mVendorStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  if (!mIScsiDeviceListUpdated) {
    //
    // Update the device list.
    //
    IScsiUpdateDeviceList ();
    mIScsiDeviceListUpdated = TRUE;
  }

  Private = ISCSI_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK (This);
  IfrNvData = AllocateZeroPool (sizeof (ISCSI_CONFIG_IFR_NVDATA));
  ASSERT (IfrNvData != NULL);
  if (Private->Current != NULL) {
    IScsiConvertDeviceConfigDataToIfrNvData (Private->Current, IfrNvData);
  }

  BufferSize  = ISCSI_NAME_MAX_SIZE;
  Status      = gIScsiInitiatorName.Get (&gIScsiInitiatorName, &BufferSize, InitiatorName);
  if (EFI_ERROR (Status)) {
    IfrNvData->InitiatorName[0] = L'\0';
  } else {
    IScsiAsciiStrToUnicodeStr (InitiatorName, IfrNvData->InitiatorName);
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  HiiConfigRouting = Private->ConfigRouting;
  BufferSize = sizeof (ISCSI_CONFIG_IFR_NVDATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gIp4IScsiConfigGuid, mVendorStorageName, Private->DriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }
  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               (UINT8 *) IfrNvData,
                               BufferSize,
                               Results,
                               Progress
                               );
  FreePool (IfrNvData);
  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }

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
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
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
IScsiFormRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gIp4IScsiConfigGuid, mVendorStorageName)) {
    *Progress = Configuration;
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
IScsiFormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  ISCSI_FORM_CALLBACK_INFO  *Private;
  UINTN                     BufferSize;
  CHAR8                     IScsiName[ISCSI_NAME_MAX_SIZE];
  CHAR16                    PortString[128];
  CHAR8                     Ip4String[IP4_STR_MAX_SIZE];
  CHAR8                     LunString[ISCSI_LUN_STR_MAX_LEN];
  UINT64                    Lun;
  EFI_STRING_ID             DeviceFormTitleToken;
  ISCSI_CONFIG_IFR_NVDATA   *IfrNvData;
  ISCSI_CONFIG_FORM_ENTRY   *ConfigFormEntry;
  EFI_IP_ADDRESS            HostIp;
  EFI_IP_ADDRESS            SubnetMask;
  EFI_IP_ADDRESS            Gateway;
  EFI_STATUS                Status;
  EFI_INPUT_KEY             Key;

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    return EFI_UNSUPPORTED;
  }

  Private   = ISCSI_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK (This);
  //
  // Retrive uncommitted data from Browser
  //
  IfrNvData = AllocateZeroPool (sizeof (ISCSI_CONFIG_IFR_NVDATA));
  ASSERT (IfrNvData != NULL);
  if (!HiiGetBrowserData (&gIp4IScsiConfigGuid, mVendorStorageName, sizeof (ISCSI_CONFIG_IFR_NVDATA), (UINT8 *) IfrNvData)) {
    FreePool (IfrNvData);
    return EFI_NOT_FOUND;
  }
  Status = EFI_SUCCESS;

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if ((QuestionId >= KEY_DEVICE_ENTRY_BASE) && (QuestionId < (mNumberOfIScsiDevices + KEY_DEVICE_ENTRY_BASE))) {
      //
      // In case goto the device configuration form, update the device form title.
      //
      ConfigFormEntry = IScsiGetConfigFormEntryByIndex ((UINT32) (QuestionId - KEY_DEVICE_ENTRY_BASE));
      ASSERT (ConfigFormEntry != NULL);

      UnicodeSPrint (PortString, (UINTN) sizeof (PortString), L"Port %s", ConfigFormEntry->MacString);
      DeviceFormTitleToken = (EFI_STRING_ID) STR_ISCSI_DEVICE_FORM_TITLE;
      HiiSetString (Private->RegisteredHandle, DeviceFormTitleToken, PortString, NULL);

      IScsiConvertDeviceConfigDataToIfrNvData (ConfigFormEntry, IfrNvData);

      Private->Current = ConfigFormEntry;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    switch (QuestionId) { 
    case KEY_INITIATOR_NAME:
      IScsiUnicodeStrToAsciiStr (IfrNvData->InitiatorName, IScsiName);
      BufferSize  = AsciiStrSize (IScsiName);

      Status      = gIScsiInitiatorName.Set (&gIScsiInitiatorName, &BufferSize, IScsiName);
      if (EFI_ERROR (Status)) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid iSCSI Name!", NULL);
      }

      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
      break;

    case KEY_LOCAL_IP:
      IScsiUnicodeStrToAsciiStr (IfrNvData->LocalIp, Ip4String);
      Status = IScsiAsciiStrToIp (Ip4String, &HostIp.v4);
      if (EFI_ERROR (Status) || !NetIp4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid IP address!", NULL);
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Private->Current->SessionConfigData.LocalIp, &HostIp.v4, sizeof (HostIp.v4));
      }

      break;

    case KEY_SUBNET_MASK:
      IScsiUnicodeStrToAsciiStr (IfrNvData->SubnetMask, Ip4String);
      Status = IScsiAsciiStrToIp (Ip4String, &SubnetMask.v4);
      if (EFI_ERROR (Status) || ((SubnetMask.Addr[0] != 0) && (IScsiGetSubnetMaskPrefixLength (&SubnetMask.v4) == 0))) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Subnet Mask!", NULL);
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Private->Current->SessionConfigData.SubnetMask, &SubnetMask.v4, sizeof (SubnetMask.v4));
      }

      break;

    case KEY_GATE_WAY:
      IScsiUnicodeStrToAsciiStr (IfrNvData->Gateway, Ip4String);
      Status = IScsiAsciiStrToIp (Ip4String, &Gateway.v4);
      if (EFI_ERROR (Status) || ((Gateway.Addr[0] != 0) && !NetIp4IsUnicast (NTOHL (Gateway.Addr[0]), 0))) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid Gateway!", NULL);
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Private->Current->SessionConfigData.Gateway, &Gateway.v4, sizeof (Gateway.v4));
      }

      break;

    case KEY_TARGET_IP:
      IScsiUnicodeStrToAsciiStr (IfrNvData->TargetIp, Ip4String);
      Status = IScsiAsciiStrToIp (Ip4String, &HostIp.v4);
      if (EFI_ERROR (Status) || !NetIp4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid IP address!", NULL);
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Private->Current->SessionConfigData.TargetIp, &HostIp.v4, sizeof (HostIp.v4));
      }

      break;

    case KEY_TARGET_NAME:
      IScsiUnicodeStrToAsciiStr (IfrNvData->TargetName, IScsiName);
      Status = IScsiNormalizeName (IScsiName, AsciiStrLen (IScsiName));
      if (EFI_ERROR (Status)) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid iSCSI Name!", NULL);
      } else {
        AsciiStrCpyS (Private->Current->SessionConfigData.TargetName, ISCSI_NAME_MAX_SIZE, IScsiName);
      }

      break;

    case KEY_DHCP_ENABLE:
      if (IfrNvData->InitiatorInfoFromDhcp == 0) {
        IfrNvData->TargetInfoFromDhcp = 0;
      }

      break;

    case KEY_BOOT_LUN:
      IScsiUnicodeStrToAsciiStr (IfrNvData->BootLun, LunString);
      Status = IScsiAsciiStrToLun (LunString, (UINT8 *) &Lun);
      if (EFI_ERROR (Status)) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Invalid LUN string!", NULL);
      } else {
        CopyMem (Private->Current->SessionConfigData.BootLun, &Lun, sizeof (Lun));
      }

      break;

    case KEY_CHAP_NAME:
      IScsiUnicodeStrToAsciiStr (IfrNvData->CHAPName, Private->Current->AuthConfigData.CHAPName);
      break;

    case KEY_CHAP_SECRET:
      IScsiUnicodeStrToAsciiStr (IfrNvData->CHAPSecret, Private->Current->AuthConfigData.CHAPSecret);
      break;

    case KEY_REVERSE_CHAP_NAME:
      IScsiUnicodeStrToAsciiStr (IfrNvData->ReverseCHAPName, Private->Current->AuthConfigData.ReverseCHAPName);
      break;

    case KEY_REVERSE_CHAP_SECRET:
      IScsiUnicodeStrToAsciiStr (IfrNvData->ReverseCHAPSecret, Private->Current->AuthConfigData.ReverseCHAPSecret);
      break;

    case KEY_CONFIG_ISID:
      IScsiParseIsIdFromString (IfrNvData->IsId, Private->Current->SessionConfigData.IsId);
      IScsiConvertIsIdToString (IfrNvData->IsId, Private->Current->SessionConfigData.IsId);

      break;

    case KEY_SAVE_CHANGES:
      //
      // First, update those fields which don't have INTERACTIVE set.
      //
      Private->Current->SessionConfigData.Enabled               = IfrNvData->Enabled;
      Private->Current->SessionConfigData.InitiatorInfoFromDhcp = IfrNvData->InitiatorInfoFromDhcp;
      Private->Current->SessionConfigData.TargetPort            = IfrNvData->TargetPort;
      if (Private->Current->SessionConfigData.TargetPort == 0) {
        Private->Current->SessionConfigData.TargetPort = ISCSI_WELL_KNOWN_PORT;
      }

      Private->Current->SessionConfigData.TargetInfoFromDhcp  = IfrNvData->TargetInfoFromDhcp;
      Private->Current->AuthConfigData.CHAPType               = IfrNvData->CHAPType;

      //
      // Only do full parameter validation if iSCSI is enabled on this device.
      //
      if (Private->Current->SessionConfigData.Enabled) {
        //
        // Validate the address configuration of the Initiator if DHCP isn't
        // deployed.
        //
        if (!Private->Current->SessionConfigData.InitiatorInfoFromDhcp) {
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
        }
        //
        // Validate target configuration if DHCP isn't deployed.
        //
        if (!Private->Current->SessionConfigData.TargetInfoFromDhcp) {
          CopyMem (&HostIp.v4, &Private->Current->SessionConfigData.TargetIp, sizeof (HostIp.v4));
          if (!NetIp4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
            CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Target IP is invalid!", NULL);
            Status = EFI_INVALID_PARAMETER;
            break;
          }

          //
          // Validate iSCSI target name configuration again:
          // The format of iSCSI target name is already verified when user input the name;
          // here we only check the case user does not input the name.
          //
          if (Private->Current->SessionConfigData.TargetName[0] == '\0') {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"iSCSI target name is NULL!",
              NULL
              );
            Status = EFI_INVALID_PARAMETER;
            break;
          }

        }

        if (IfrNvData->CHAPType != ISCSI_CHAP_NONE) {
          if ((IfrNvData->CHAPName[0] == '\0') || (IfrNvData->CHAPSecret[0] == '\0')) {
            CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"CHAP Name or CHAP Secret is invalid!", NULL);
            Status = EFI_INVALID_PARAMETER;
            break;
          }

          if ((IfrNvData->CHAPType == ISCSI_CHAP_MUTUAL) &&
              ((IfrNvData->ReverseCHAPName[0] == '\0') || (IfrNvData->ReverseCHAPSecret[0] == '\0'))
              ) {
            CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Reverse CHAP Name or Reverse CHAP Secret is invalid!", NULL);
            Status = EFI_INVALID_PARAMETER;
            break;
          }
        }
      }

      BufferSize = sizeof (Private->Current->SessionConfigData);
      gRT->SetVariable (
            Private->Current->MacString,
            &gEfiIScsiInitiatorNameProtocolGuid,
            ISCSI_CONFIG_VAR_ATTR,
            BufferSize,
            &Private->Current->SessionConfigData
            );

      BufferSize = sizeof (Private->Current->AuthConfigData);
      gRT->SetVariable (
            Private->Current->MacString,
            &gIScsiCHAPAuthInfoGuid,
            ISCSI_CONFIG_VAR_ATTR,
            BufferSize,
            &Private->Current->AuthConfigData
            );
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
      break;

    default:
      break;
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // Pass changed uncommitted data back to Form Browser
    //
    HiiSetBrowserData (&gIp4IScsiConfigGuid, mVendorStorageName, sizeof (ISCSI_CONFIG_IFR_NVDATA), (UINT8 *) IfrNvData, NULL);
  }
  
  FreePool (IfrNvData);
  
  return Status;
}

/**
  Updates the iSCSI configuration form to add/delete an entry for the iSCSI
  device specified by the Controller.

  @param[in]  DriverBindingHandle The driverbinding handle.
  @param[in]  Controller          The controller handle of the iSCSI device.
  @param[in]  AddForm             Whether to add or delete a form entry.

  @retval EFI_SUCCESS             The iSCSI configuration form is updated.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
IScsiConfigUpdateForm (
  IN EFI_HANDLE  DriverBindingHandle,
  IN EFI_HANDLE  Controller,
  IN BOOLEAN     AddForm
  )
{
  LIST_ENTRY                  *Entry;
  ISCSI_CONFIG_FORM_ENTRY     *ConfigFormEntry;
  BOOLEAN                     EntryExisted;
  EFI_STATUS                  Status;
  EFI_MAC_ADDRESS             MacAddress;
  UINTN                       HwAddressSize;
  UINT16                      VlanId;
  CHAR16                      PortString[128];
  UINT16                      FormIndex;
  UINTN                       BufferSize;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;

  ConfigFormEntry = NULL;
  EntryExisted    = FALSE;

  NET_LIST_FOR_EACH (Entry, &mIScsiConfigFormList) {
    ConfigFormEntry = NET_LIST_USER_STRUCT (Entry, ISCSI_CONFIG_FORM_ENTRY, Link);

    if (ConfigFormEntry->Controller == Controller) {
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
      ConfigFormEntry = (ISCSI_CONFIG_FORM_ENTRY *) AllocateZeroPool (sizeof (ISCSI_CONFIG_FORM_ENTRY));
      if (ConfigFormEntry == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      InitializeListHead (&ConfigFormEntry->Link);
      ConfigFormEntry->Controller = Controller;

      //
      // Get the MAC address and convert it into the formatted string.
      //
      Status = NetLibGetMacAddress (Controller, &MacAddress, &HwAddressSize);
      ASSERT (Status == EFI_SUCCESS);
      VlanId = NetLibGetVlanId (Controller);

      IScsiMacAddrToStr (&MacAddress, (UINT32) HwAddressSize, VlanId, ConfigFormEntry->MacString);

      //
      // Get the normal session configuration data.
      //
      BufferSize = sizeof (ConfigFormEntry->SessionConfigData);
      Status = gRT->GetVariable (
                      ConfigFormEntry->MacString,
                      &gEfiIScsiInitiatorNameProtocolGuid,
                      NULL,
                      &BufferSize,
                      &ConfigFormEntry->SessionConfigData
                      );
      if (EFI_ERROR (Status)) {
        ZeroMem (&ConfigFormEntry->SessionConfigData, sizeof (ConfigFormEntry->SessionConfigData));
        
        //
        // Generate OUI-format ISID based on MAC address.
        //
        CopyMem (ConfigFormEntry->SessionConfigData.IsId, &MacAddress, 6);
        ConfigFormEntry->SessionConfigData.IsId[0] = 
          (UINT8) (ConfigFormEntry->SessionConfigData.IsId[0] & 0x3F);
      }
      //
      // Get the CHAP authentication configuration data.
      //
      BufferSize = sizeof (ConfigFormEntry->AuthConfigData);
      Status = gRT->GetVariable (
                      ConfigFormEntry->MacString,
                      &gIScsiCHAPAuthInfoGuid,
                      NULL,
                      &BufferSize,
                      &ConfigFormEntry->AuthConfigData
                      );
      if (EFI_ERROR (Status)) {
        ZeroMem (&ConfigFormEntry->AuthConfigData, sizeof (ConfigFormEntry->AuthConfigData));
      }
      //
      // Compose the Port string and create a new EFI_STRING_ID.
      //
      UnicodeSPrint (PortString, sizeof (PortString), L"Port %s", ConfigFormEntry->MacString);
      ConfigFormEntry->PortTitleToken = HiiSetString (mCallbackInfo->RegisteredHandle, 0, PortString, NULL);

      //
      // Compose the help string of this port and create a new EFI_STRING_ID.
      //
      UnicodeSPrint (PortString, sizeof (PortString), L"Set the iSCSI parameters on port %s", ConfigFormEntry->MacString);
      ConfigFormEntry->PortTitleHelpToken = HiiSetString (mCallbackInfo->RegisteredHandle, 0, PortString, NULL);

      InsertTailList (&mIScsiConfigFormList, &ConfigFormEntry->Link);
      mNumberOfIScsiDevices++;
    }
  } else {
    ASSERT (EntryExisted);

    mNumberOfIScsiDevices--;
    RemoveEntryList (&ConfigFormEntry->Link);
    FreePool (ConfigFormEntry);
  }
  //
  // Allocate space for creation of Buffer
  //

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
  NET_LIST_FOR_EACH (Entry, &mIScsiConfigFormList) {
    ConfigFormEntry = NET_LIST_USER_STRUCT (Entry, ISCSI_CONFIG_FORM_ENTRY, Link);

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
    &gIp4IScsiConfigGuid,
    FORMID_MAIN_FORM,
    StartOpCodeHandle, // Label DEVICE_ENTRY_LABEL
    EndOpCodeHandle    // LABEL_END
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return EFI_SUCCESS;
}

/**
  Initialize the iSCSI configuration form.

  @param[in]  DriverBindingHandle  The iSCSI driverbinding handle.

  @retval EFI_SUCCESS              The iSCSI configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.
**/
EFI_STATUS
IScsiConfigFormInit (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_HII_DATABASE_PROTOCOL   *HiiDatabase;
  ISCSI_FORM_CALLBACK_INFO    *CallbackInfo;

  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&HiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CallbackInfo = (ISCSI_FORM_CALLBACK_INFO *) AllocateZeroPool (sizeof (ISCSI_FORM_CALLBACK_INFO));
  if (CallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CallbackInfo->Signature   = ISCSI_FORM_CALLBACK_INFO_SIGNATURE;
  CallbackInfo->HiiDatabase = HiiDatabase;
  CallbackInfo->Current     = NULL;

  CallbackInfo->ConfigAccess.ExtractConfig = IScsiFormExtractConfig;
  CallbackInfo->ConfigAccess.RouteConfig = IScsiFormRouteConfig;
  CallbackInfo->ConfigAccess.Callback = IScsiFormCallback;

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&CallbackInfo->ConfigRouting);
  if (EFI_ERROR (Status)) {
    FreePool(CallbackInfo);
    return Status;
  }

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &CallbackInfo->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mIScsiHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &CallbackInfo->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  
  //
  // Publish our HII data
  //
  CallbackInfo->RegisteredHandle = HiiAddPackages (
                                     &gIp4IScsiConfigGuid,
                                     CallbackInfo->DriverHandle,
                                     IScsi4DxeStrings,
                                     IScsiConfigDxeBin,
                                     NULL
                                     );
  if (CallbackInfo->RegisteredHandle == NULL) {
    FreePool(CallbackInfo);
    return EFI_OUT_OF_RESOURCES;
  }

  mCallbackInfo = CallbackInfo;

  return Status;
}

/**
  Unload the iSCSI configuration form, this includes: delete all the iSCSI
  device configuration entries, uninstall the form callback protocol and
  free the resources used.

  @param[in]  DriverBindingHandle The iSCSI driverbinding handle.
  
  @retval EFI_SUCCESS             The iSCSI configuration form is unloaded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
IScsiConfigFormUnload (
  IN EFI_HANDLE  DriverBindingHandle
  )
{
  ISCSI_CONFIG_FORM_ENTRY     *ConfigFormEntry;

  while (!IsListEmpty (&mIScsiConfigFormList)) {
    //
    // Uninstall the device forms as the iSCSI driver instance may fail to
    // control the controller but still install the device configuration form.
    // In such case, upon driver unloading, the driver instance's driverbinding.
    // stop () won't be called, so we have to take this chance here to uninstall
    // the device form.
    //
    ConfigFormEntry = NET_LIST_USER_STRUCT (mIScsiConfigFormList.ForwardLink, ISCSI_CONFIG_FORM_ENTRY, Link);
    IScsiConfigUpdateForm (DriverBindingHandle, ConfigFormEntry->Controller, FALSE);
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
         &mIScsiHiiVendorDevicePath,
         &gEfiHiiConfigAccessProtocolGuid,
         &mCallbackInfo->ConfigAccess,
         NULL
         );
  FreePool (mCallbackInfo);

  return EFI_SUCCESS;
}
