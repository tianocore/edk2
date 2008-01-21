/*++

Copyright (c)  2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IScsiConfig.c

Abstract:

--*/

#include "IScsiImpl.h"

EFI_GUID        mVendorGuid             = ISCSI_CONFIG_GUID;
BOOLEAN         mIScsiDeviceListUpdated = FALSE;
UINTN           mNumberOfIScsiDevices   = 0;
ISCSI_FORM_CALLBACK_INFO  *mCallbackInfo;

NET_LIST_ENTRY  mIScsiConfigFormList = {
  &mIScsiConfigFormList,
  &mIScsiConfigFormList
};

STATIC
VOID
IScsiIpToStr (
  IN  EFI_IPv4_ADDRESS  *Ip,
  OUT CHAR16            *Str
  )
/*++

Routine Description:

  Convert the IPv4 address into a dotted string.

Arguments:

  Ip  - The IPv4 address.
  Str - The dotted IP string.

Returns:

  None.

--*/
{
  UnicodeSPrint ( Str, 2 * IP4_STR_MAX_SIZE, L"%d.%d.%d.%d", Ip->Addr[0], Ip->Addr[1], Ip->Addr[2], Ip->Addr[3]);
}

VOID
PopUpInvalidNotify (
  IN CHAR16 *Warning
  )
/*++

Routine Description:

  Pop up an invalid notify which displays the message in Warning.

Arguments:

  Warning - The warning message.

Returns:

  None.

--*/
{
  EFI_INPUT_KEY             Key;

  IfrLibCreatePopUp (1, &Key, Warning);
}

EFI_STATUS
IScsiUpdateDeviceList (
  VOID
  )
/*++

Routine Description:

  Update the list of iSCSI devices the iSCSI driver is controlling.

Arguments:

  None.

Returns:

  None.

--*/
{
  EFI_STATUS                  Status;
  ISCSI_DEVICE_LIST           *DeviceList;
  UINTN                       DataSize;
  UINTN                       NumHandles;
  EFI_HANDLE                  *Handles;
  UINTN                       HandleIndex;
  UINTN                       Index;
  UINTN                       LastDeviceIndex;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_SIMPLE_NETWORK_MODE     *Mode;
  ISCSI_MAC_INFO              *CurMacInfo;
  ISCSI_MAC_INFO              TempMacInfo;
  CHAR16                      MacString[65];
  UINTN                       DeviceListSize;

  //
  // Dump all the handles the Simple Network Protocol is installed on.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleNetworkProtocolGuid,
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
                  &mVendorGuid,
                  NULL,
                  &DataSize,
                  NULL
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    DeviceList = (ISCSI_DEVICE_LIST *) NetAllocatePool (DataSize);

    gRT->GetVariable (
          L"iSCSIDeviceList",
          &mVendorGuid,
          NULL,
          &DataSize,
          DeviceList
          );

    LastDeviceIndex = 0;

    for (HandleIndex = 0; HandleIndex < NumHandles; HandleIndex++) {
      gBS->HandleProtocol (Handles[HandleIndex], &gEfiSimpleNetworkProtocolGuid, (VOID **)&Snp);

      Mode = Snp->Mode;

      for (Index = LastDeviceIndex; Index < DeviceList->NumDevice; Index++) {
        CurMacInfo = &DeviceList->MacInfo[Index];
        if ((CurMacInfo->Len == Mode->HwAddressSize) &&
            (NET_MAC_EQUAL (&CurMacInfo->Mac, &Mode->PermanentAddress, Mode->HwAddressSize))
            ) {
          //
          // The previous configured NIC is still here.
          //
          if (Index != LastDeviceIndex) {
            //
            // Swap the current MAC address entry with the one indexed by
            // LastDeviceIndex.
            //
            NetCopyMem (&TempMacInfo, CurMacInfo, sizeof (ISCSI_MAC_INFO));
            NetCopyMem (CurMacInfo, &DeviceList->MacInfo[LastDeviceIndex], sizeof (ISCSI_MAC_INFO));
            NetCopyMem (&DeviceList->MacInfo[LastDeviceIndex], &TempMacInfo, sizeof (ISCSI_MAC_INFO));
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
      IScsiMacAddrToStr (&CurMacInfo->Mac, CurMacInfo->Len, MacString);
      gRT->SetVariable (MacString, &gEfiIScsiInitiatorNameProtocolGuid, 0, 0, NULL);
      gRT->SetVariable (MacString, &mIScsiCHAPAuthInfoGuid, 0, 0, NULL);
    }

    NetFreePool (DeviceList);
  } else if (Status != EFI_NOT_FOUND) {
    NetFreePool (Handles);
    return Status;
  }
  //
  // Construct the new iSCSI device list.
  //
  DeviceListSize        = sizeof (ISCSI_DEVICE_LIST) + (NumHandles - 1) * sizeof (ISCSI_MAC_INFO);
  DeviceList            = (ISCSI_DEVICE_LIST *) NetAllocatePool (DeviceListSize);
  DeviceList->NumDevice = (UINT8) NumHandles;

  for (Index = 0; Index < NumHandles; Index++) {
    gBS->HandleProtocol (Handles[Index], &gEfiSimpleNetworkProtocolGuid, (VOID **)&Snp);
    Mode        = Snp->Mode;

    CurMacInfo  = &DeviceList->MacInfo[Index];
    NetCopyMem (&CurMacInfo->Mac, &Mode->PermanentAddress, Mode->HwAddressSize);
    CurMacInfo->Len = (UINT8) Mode->HwAddressSize;
  }

  gRT->SetVariable (
        L"iSCSIDeviceList",
        &mVendorGuid,
        ISCSI_CONFIG_VAR_ATTR,
        DeviceListSize,
        DeviceList
        );

  NetFreePool (DeviceList);

  return Status;
}

STATIC
ISCSI_CONFIG_FORM_ENTRY *
IScsiGetConfigFormEntryByIndex (
  IN UINT32 Index
  )
/*++

Routine Description:

  Get the iSCSI configuration form entry by the index of the goto opcode actived.

Arguments:

  Index - The 0-based index of the goto opcode actived.

Returns:

  The iSCSI configuration form entry found.

--*/
{
  UINT32                  CurrentIndex;
  NET_LIST_ENTRY          *Entry;
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

STATIC
VOID
IScsiConvertDeviceConfigDataToIfrNvData (
  IN ISCSI_CONFIG_FORM_ENTRY  *ConfigFormEntry,
  IN ISCSI_CONFIG_IFR_NVDATA  *IfrNvData
  )
/*++

Routine Description:

  Convert the iSCSI configuration data into the IFR data.

Arguments:

  ConfigFormEntry - The iSCSI configuration form entry.
  IfrNvData       - The IFR nv data.

Returns:

  None.

--*/
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


EFI_STATUS
EFIAPI
IScsiFormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
/*++

  Routine Description:
    This function allows a caller to extract the current configuration for one
    or more named elements from the target driver.

  Arguments:
    This       - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
    Request    - A null-terminated Unicode string in <ConfigRequest> format.
    Progress   - On return, points to a character in the Request string.
                 Points to the string's null terminator if request was successful.
                 Points to the most recent '&' before the first failing name/value
                 pair (or the beginning of the string if the failure is in the
                 first name/value pair) if the request was not successful.
    Results    - A null-terminated Unicode string in <ConfigAltResp> format which
                 has all values filled in for the names in the Request string.
                 String to be allocated by the called function.

  Returns:
    EFI_SUCCESS           - The Results is filled with the requested values.
    EFI_OUT_OF_RESOURCES  - Not enough memory to store the results.
    EFI_INVALID_PARAMETER - Request is NULL, illegal syntax, or unknown name.
    EFI_NOT_FOUND         - Routing data doesn't match any storage in this driver.

--*/
{
  EFI_STATUS                       Status;
  CHAR8                            InitiatorName[ISCSI_NAME_IFR_MAX_SIZE];
  UINTN                            BufferSize;
  ISCSI_CONFIG_IFR_NVDATA          *IfrNvData;
  ISCSI_FORM_CALLBACK_INFO         *Private;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

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

  BufferSize  = ISCSI_NAME_IFR_MAX_SIZE;
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
  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               Request,
                               (UINT8 *) IfrNvData,
                               BufferSize,
                               Results,
                               Progress
                               );
  NetFreePool (IfrNvData);
  return Status;
}

EFI_STATUS
EFIAPI
IScsiFormRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
/*++

  Routine Description:
    This function processes the results of changes in configuration.

  Arguments:
    This          - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
    Configuration - A null-terminated Unicode string in <ConfigResp> format.
    Progress      - A pointer to a string filled in with the offset of the most
                    recent '&' before the first failing name/value pair (or the
                    beginning of the string if the failure is in the first
                    name/value pair) or the terminating NULL if all was successful.

  Returns:
    EFI_SUCCESS           - The Results is processed successfully.
    EFI_INVALID_PARAMETER - Configuration is NULL.
    EFI_NOT_FOUND         - Routing data doesn't match any storage in this driver.

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IScsiFormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        KeyValue,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
/*++

  Routine Description:
    This function processes the results of changes in configuration.

  Arguments:
    This          - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
    Action        - Specifies the type of action taken by the browser.
    QuestionId    - A unique value which is sent to the original exporting driver
                    so that it can identify the type of data to expect.
    Type          - The type of value for the question.
    Value         - A pointer to the data being sent to the original exporting driver.
    ActionRequest - On return, points to the action requested by the callback function.

  Returns:
    EFI_SUCCESS          - The callback successfully handled the action.
    EFI_OUT_OF_RESOURCES - Not enough storage is available to hold the variable and its data.
    EFI_DEVICE_ERROR     - The variable could not be saved.
    EFI_UNSUPPORTED      - The specified Action is not supported by the callback.

--*/
{
  ISCSI_FORM_CALLBACK_INFO  *Private;
  UINTN                     BufferSize;
  CHAR8                     IScsiName[ISCSI_NAME_IFR_MAX_SIZE];
  CHAR16                    PortString[128];
  CHAR8                     Ip4String[IP4_STR_MAX_SIZE];
  CHAR8                     LunString[ISCSI_LUN_STR_MAX_LEN];
  UINT64                    Lun;
  STRING_REF                DeviceFormTitleToken;
  ISCSI_CONFIG_IFR_NVDATA   *IfrNvData;
  ISCSI_CONFIG_FORM_ENTRY   *ConfigFormEntry;
  EFI_IP_ADDRESS            HostIp;
  EFI_IP_ADDRESS            SubnetMask;
  EFI_IP_ADDRESS            Gateway;
  EFI_STATUS                Status;

  Private   = ISCSI_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK (This);

  //
  // Retrive uncommitted data from Browser
  //
  BufferSize = sizeof (ISCSI_CONFIG_IFR_NVDATA);
  IfrNvData = AllocateZeroPool (BufferSize);
  ASSERT (IfrNvData != NULL);
  Status = GetBrowserData (NULL, NULL, &BufferSize, (UINT8 *) IfrNvData);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (IfrNvData);
    return Status;
  }

  switch (KeyValue) {
  case KEY_INITIATOR_NAME:
    IScsiUnicodeStrToAsciiStr (IfrNvData->InitiatorName, IScsiName);
    BufferSize  = AsciiStrLen (IScsiName) + 1;

    Status      = gIScsiInitiatorName.Set (&gIScsiInitiatorName, &BufferSize, IScsiName);
    if (EFI_ERROR (Status)) {
      PopUpInvalidNotify (L"Invalid iSCSI Name!");
    }

    break;

  case KEY_LOCAL_IP:
    IScsiUnicodeStrToAsciiStr (IfrNvData->LocalIp, Ip4String);
    Status = IScsiAsciiStrToIp (Ip4String, &HostIp.v4);
    if (EFI_ERROR (Status) || !Ip4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
      PopUpInvalidNotify (L"Invalid IP address!");
      Status = EFI_INVALID_PARAMETER;
    } else {
      NetCopyMem (&Private->Current->SessionConfigData.LocalIp, &HostIp.v4, sizeof (HostIp.v4));
    }

    break;

  case KEY_SUBNET_MASK:
    IScsiUnicodeStrToAsciiStr (IfrNvData->SubnetMask, Ip4String);
    Status = IScsiAsciiStrToIp (Ip4String, &SubnetMask.v4);
    if (EFI_ERROR (Status) || ((SubnetMask.Addr[0] != 0) && (IScsiGetSubnetMaskPrefixLength (&SubnetMask.v4) == 0))) {
      PopUpInvalidNotify (L"Invalid Subnet Mask!");
      Status = EFI_INVALID_PARAMETER;
    } else {
      NetCopyMem (&Private->Current->SessionConfigData.SubnetMask, &SubnetMask.v4, sizeof (SubnetMask.v4));
    }

    break;

  case KEY_GATE_WAY:
    IScsiUnicodeStrToAsciiStr (IfrNvData->Gateway, Ip4String);
    Status = IScsiAsciiStrToIp (Ip4String, &Gateway.v4);
    if (EFI_ERROR (Status) || ((Gateway.Addr[0] != 0) && !Ip4IsUnicast (NTOHL (Gateway.Addr[0]), 0))) {
      PopUpInvalidNotify (L"Invalid Gateway!");
      Status = EFI_INVALID_PARAMETER;
    } else {
      NetCopyMem (&Private->Current->SessionConfigData.Gateway, &Gateway.v4, sizeof (Gateway.v4));
    }

    break;

  case KEY_TARGET_IP:
    IScsiUnicodeStrToAsciiStr (IfrNvData->TargetIp, Ip4String);
    Status = IScsiAsciiStrToIp (Ip4String, &HostIp.v4);
    if (EFI_ERROR (Status) || !Ip4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
      PopUpInvalidNotify (L"Invalid IP address!");
      Status = EFI_INVALID_PARAMETER;
    } else {
      NetCopyMem (&Private->Current->SessionConfigData.TargetIp, &HostIp.v4, sizeof (HostIp.v4));
    }

    break;

  case KEY_TARGET_NAME:
    IScsiUnicodeStrToAsciiStr (IfrNvData->TargetName, IScsiName);
    Status = IScsiNormalizeName (IScsiName, AsciiStrLen (IScsiName));
    if (EFI_ERROR (Status)) {
      PopUpInvalidNotify (L"Invalid iSCSI Name!");
    } else {
      AsciiStrCpy (Private->Current->SessionConfigData.TargetName, IScsiName);
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
      PopUpInvalidNotify (L"Invalid LUN string!");
    } else {
      NetCopyMem (Private->Current->SessionConfigData.BootLun, &Lun, sizeof (Lun));
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
        NetCopyMem (&HostIp.v4, &Private->Current->SessionConfigData.LocalIp, sizeof (HostIp.v4));
        NetCopyMem (&SubnetMask.v4, &Private->Current->SessionConfigData.SubnetMask, sizeof (SubnetMask.v4));
        NetCopyMem (&Gateway.v4, &Private->Current->SessionConfigData.Gateway, sizeof (Gateway.v4));

        if ((Gateway.Addr[0] != 0)) {
          if (SubnetMask.Addr[0] == 0) {
            PopUpInvalidNotify (L"Gateway address is set but subnet mask is zero.");
            Status = EFI_INVALID_PARAMETER;
            break;
          } else if (!IP4_NET_EQUAL (HostIp.Addr[0], Gateway.Addr[0], SubnetMask.Addr[0])) {
            PopUpInvalidNotify (L"Local IP and Gateway are not in the same subnet.");
            Status = EFI_INVALID_PARAMETER;
            break;
          }
        }
      }
      //
      // Validate target configuration if DHCP isn't deployed.
      //
      if (!Private->Current->SessionConfigData.TargetInfoFromDhcp) {
        NetCopyMem (&HostIp.v4, &Private->Current->SessionConfigData.TargetIp, sizeof (HostIp.v4));
        if (!Ip4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
          PopUpInvalidNotify (L"Target IP is invalid!");
          Status = EFI_INVALID_PARAMETER;
          break;
        }
      }

      if (IfrNvData->CHAPType != ISCSI_CHAP_NONE) {
        if ((IfrNvData->CHAPName[0] == '\0') || (IfrNvData->CHAPSecret[0] == '\0')) {
          PopUpInvalidNotify (L"CHAP Name or CHAP Secret is invalid!");
          Status = EFI_INVALID_PARAMETER;
          break;
        }

        if ((IfrNvData->CHAPType == ISCSI_CHAP_MUTUAL) &&
            ((IfrNvData->ReverseCHAPName[0] == '\0') || (IfrNvData->ReverseCHAPSecret[0] == '\0'))
            ) {
          PopUpInvalidNotify (L"Reverse CHAP Name or Reverse CHAP Secret is invalid!");
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
          &mIScsiCHAPAuthInfoGuid,
          ISCSI_CONFIG_VAR_ATTR,
          BufferSize,
          &Private->Current->AuthConfigData
          );
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
    break;

  default:
    if ((KeyValue >= KEY_DEVICE_ENTRY_BASE) && (KeyValue < (mNumberOfIScsiDevices + KEY_DEVICE_ENTRY_BASE))) {
      //
      // In case goto the device configuration form, update the device form title.
      //
      ConfigFormEntry = IScsiGetConfigFormEntryByIndex ((UINT32) (KeyValue - KEY_DEVICE_ENTRY_BASE));
      ASSERT (ConfigFormEntry != NULL);

      UnicodeSPrint (PortString, (UINTN) 128, L"Port %s", ConfigFormEntry->MacString);
      DeviceFormTitleToken = (STRING_REF) STR_ISCSI_DEVICE_FORM_TITLE;
      IfrLibSetString (Private->RegisteredHandle, DeviceFormTitleToken, PortString);

      IScsiConvertDeviceConfigDataToIfrNvData (ConfigFormEntry, IfrNvData);

      Private->Current = ConfigFormEntry;
    }

    break;
  }

  if (!EFI_ERROR (Status)) {
    //
    // Pass changed uncommitted data back to Form Browser
    //
    BufferSize = sizeof (ISCSI_CONFIG_IFR_NVDATA);
    Status = SetBrowserData (NULL, NULL, BufferSize, (UINT8 *) IfrNvData, NULL);
  }

  NetFreePool (IfrNvData);
  return Status;
}


EFI_STATUS
IScsiConfigUpdateForm (
  IN EFI_HANDLE  DriverBindingHandle,
  IN EFI_HANDLE  Controller,
  IN BOOLEAN     AddForm
  )
/*++

Routine Description:

  Updates the iSCSI configuration form to add/delete an entry for the iSCSI
  device specified by the Controller.

Arguments:

  DriverBindingHandle - The driverbinding handle.
  Controller          - The controller handle of the iSCSI device.
  AddForm             - Whether to add or delete a form entry.

Returns:

  EFI_SUCCESS          - The iSCSI configuration form is updated.
  EFI_OUT_OF_RESOURCES - Failed to allocate memory.

--*/
{
  NET_LIST_ENTRY              *Entry;
  ISCSI_CONFIG_FORM_ENTRY     *ConfigFormEntry;
  BOOLEAN                     EntryExisted;
  EFI_STATUS                  Status;
  EFI_HII_UPDATE_DATA         UpdateData;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  CHAR16                      PortString[128];
  UINT16                      FormIndex;
  UINTN                       BufferSize;


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
      ConfigFormEntry = (ISCSI_CONFIG_FORM_ENTRY *) NetAllocateZeroPool (sizeof (ISCSI_CONFIG_FORM_ENTRY));
      if (ConfigFormEntry == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      NetListInit (&ConfigFormEntry->Link);
      ConfigFormEntry->Controller = Controller;

      //
      // Get the simple network protocol and convert the MAC address into
      // the formatted string.
      //
      Status = gBS->HandleProtocol (
                      Controller,
                      &gEfiSimpleNetworkProtocolGuid,
                      (VOID **)&Snp
                      );
      ASSERT (Status == EFI_SUCCESS);

      IScsiMacAddrToStr (&Snp->Mode->PermanentAddress, Snp->Mode->HwAddressSize, ConfigFormEntry->MacString);

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
        NetZeroMem (&ConfigFormEntry->SessionConfigData, sizeof (ConfigFormEntry->SessionConfigData));
      }
      //
      // Get the CHAP authentication configuration data.
      //
      BufferSize = sizeof (ConfigFormEntry->AuthConfigData);
      Status = gRT->GetVariable (
                      ConfigFormEntry->MacString,
                      &mIScsiCHAPAuthInfoGuid,
                      NULL,
                      &BufferSize,
                      &ConfigFormEntry->AuthConfigData
                      );
      if (EFI_ERROR (Status)) {
        NetZeroMem (&ConfigFormEntry->AuthConfigData, sizeof (ConfigFormEntry->AuthConfigData));
      }
      //
      // Compose the Port string and create a new STRING_REF.
      //
      UnicodeSPrint (PortString, 128, L"Port %s", ConfigFormEntry->MacString);
      IfrLibNewString (mCallbackInfo->RegisteredHandle, &ConfigFormEntry->PortTitleToken, PortString);

      //
      // Compose the help string of this port and create a new STRING_REF.
      //
      UnicodeSPrint (PortString, 128, L"Set the iSCSI parameters on port %s", ConfigFormEntry->MacString);
      IfrLibNewString (mCallbackInfo->RegisteredHandle, &ConfigFormEntry->PortTitleHelpToken, PortString);

      NetListInsertTail (&mIScsiConfigFormList, &ConfigFormEntry->Link);
      mNumberOfIScsiDevices++;
    }
  } else {
    ASSERT (EntryExisted);

    mNumberOfIScsiDevices--;
    NetListRemoveEntry (&ConfigFormEntry->Link);
    NetFreePool (ConfigFormEntry);
  }
  //
  // Allocate space for creation of Buffer
  //
  UpdateData.BufferSize = 0x1000;
  UpdateData.Data = NetAllocateZeroPool (0x1000);
  UpdateData.Offset = 0;

  FormIndex = 0;
  NET_LIST_FOR_EACH (Entry, &mIScsiConfigFormList) {
    ConfigFormEntry = NET_LIST_USER_STRUCT (Entry, ISCSI_CONFIG_FORM_ENTRY, Link);

    CreateGotoOpCode (
      FORMID_DEVICE_FORM,
      ConfigFormEntry->PortTitleToken,
      ConfigFormEntry->PortTitleHelpToken,
      EFI_IFR_FLAG_CALLBACK,
      KEY_DEVICE_ENTRY_BASE + FormIndex,
      &UpdateData
      );

    FormIndex++;
  }

  IfrLibUpdateForm (
    mCallbackInfo->RegisteredHandle,
    &mVendorGuid,
    FORMID_MAIN_FORM,
    DEVICE_ENTRY_LABEL,
    FALSE,
    &UpdateData
    );

  NetFreePool (UpdateData.Data);

  return EFI_SUCCESS;
}

EFI_STATUS
IScsiConfigFormInit (
  IN EFI_HANDLE  DriverBindingHandle
  )
/*++

Routine Description:

  Initialize the iSCSI configuration form.

Arguments:

  DriverBindingHandle - The iSCSI driverbinding handle.

Returns:

  EFI_SUCCESS          - The iSCSI configuration form is initialized.
  EFI_OUT_OF_RESOURCES - Failed to allocate memory.

--*/
{
  EFI_STATUS                  Status;
  EFI_HII_DATABASE_PROTOCOL   *HiiDatabase;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;
  ISCSI_FORM_CALLBACK_INFO    *CallbackInfo;

  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, &HiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CallbackInfo = (ISCSI_FORM_CALLBACK_INFO *) NetAllocatePool (sizeof (ISCSI_FORM_CALLBACK_INFO));
  if (CallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CallbackInfo->Signature             = ISCSI_FORM_CALLBACK_INFO_SIGNATURE;
  CallbackInfo->HiiDatabase = HiiDatabase;
  CallbackInfo->Current               = NULL;

  CallbackInfo->ConfigAccess.ExtractConfig = IScsiFormExtractConfig;
  CallbackInfo->ConfigAccess.RouteConfig = IScsiFormRouteConfig;
  CallbackInfo->ConfigAccess.Callback = IScsiFormCallback;

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, &CallbackInfo->ConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create driver handle used by HII database
  //
  Status = HiiLibCreateHiiDriverHandle (&CallbackInfo->DriverHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Install Config Access protocol to driver handle
  //
  Status = gBS->InstallProtocolInterface (
                  &CallbackInfo->DriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &CallbackInfo->ConfigAccess
                  );
  ASSERT_EFI_ERROR (Status);
  
  //
  // Publish our HII data
  //
  PackageList = PreparePackageList (2, &mVendorGuid, iSCSIStrings, IScsiConfigDxeBin);
  ASSERT (PackageList != NULL);
  
  Status = HiiDatabase->NewPackageList (
                           HiiDatabase,
                           PackageList,
                           CallbackInfo->DriverHandle,
                           &CallbackInfo->RegisteredHandle
                           );
  NetFreePool (PackageList);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mCallbackInfo = CallbackInfo;

  return Status;
}

EFI_STATUS
IScsiConfigFormUnload (
  IN EFI_HANDLE  DriverBindingHandle
  )
/*++

Routine Description:

  Unload the iSCSI configuration form, this includes: delete all the iSCSI
  device configuration entries, uninstall the form callback protocol and
  free the resources used.

Arguments:

  DriverBindingHandle - The iSCSI driverbinding handle.

Returns:

  EFI_SUCCESS          - The iSCSI configuration form is unloaded.
  EFI_OUT_OF_RESOURCES - Failed to allocate memory.

--*/
{
  ISCSI_CONFIG_FORM_ENTRY     *ConfigFormEntry;

  while (!NetListIsEmpty (&mIScsiConfigFormList)) {
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
  gBS->UninstallProtocolInterface (
        mCallbackInfo->DriverHandle,
        &gEfiHiiConfigAccessProtocolGuid,
        &mCallbackInfo->ConfigAccess
        );
  HiiLibDestroyHiiDriverHandle (mCallbackInfo->DriverHandle);

  NetFreePool (mCallbackInfo);

  return EFI_SUCCESS;
}
