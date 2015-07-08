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
    ISCSI_CONFIG_GUID
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
  Convert the IP address into a dotted string.

  @param[in]  Ip        The IP address.
  @param[in]  Ipv6Flag  Indicates whether the IP address is version 4 or version 6.
  @param[out] Str       The formatted IP string.

**/
VOID
IScsiIpToStr (
  IN  EFI_IP_ADDRESS    *Ip,
  IN  BOOLEAN           Ipv6Flag,
  OUT CHAR16            *Str
  )
{
  EFI_IPv4_ADDRESS      *Ip4;
  EFI_IPv6_ADDRESS      *Ip6;
  UINTN                 Index;
  BOOLEAN               Short;
  UINTN                 Number;
  CHAR16                FormatString[8];

  if (!Ipv6Flag) {
    Ip4 = &Ip->v4;

    UnicodeSPrint (
      Str,
      (UINTN) 2 * IP4_STR_MAX_SIZE,
      L"%d.%d.%d.%d",
      (UINTN) Ip4->Addr[0],
      (UINTN) Ip4->Addr[1],
      (UINTN) Ip4->Addr[2],
      (UINTN) Ip4->Addr[3]
      );

    return ;
  }

  Ip6   = &Ip->v6;
  Short = FALSE;

  for (Index = 0; Index < 15; Index = Index + 2) {
    if (!Short &&
        Index % 2 == 0 &&
        Ip6->Addr[Index] == 0 &&
        Ip6->Addr[Index + 1] == 0
        ) {
      //
      // Deal with the case of ::.
      //
      if (Index == 0) {
        *Str       = L':';
        *(Str + 1) = L':';
        Str        = Str + 2;
      } else {
        *Str       = L':';
        Str        = Str + 1;
      }

      while ((Index < 15) && (Ip6->Addr[Index] == 0) && (Ip6->Addr[Index + 1] == 0)) {
        Index = Index + 2;
      }

      Short = TRUE;

      if (Index == 16) {
        //
        // :: is at the end of the address.
        //
        *Str = L'\0';
        break;
      }
    }

    ASSERT (Index < 15);

    if (Ip6->Addr[Index] == 0) {
      Number = UnicodeSPrint (Str, 2 * IP_STR_MAX_SIZE, L"%x:", (UINTN) Ip6->Addr[Index + 1]);
    } else {
      if (Ip6->Addr[Index + 1] < 0x10) {
        CopyMem (FormatString, L"%x0%x:", StrSize (L"%x0%x:"));
      } else {
        CopyMem (FormatString, L"%x%x:", StrSize (L"%x%x:"));
      }

      Number = UnicodeSPrint (
                 Str,
                 2 * IP_STR_MAX_SIZE,
                 (CONST CHAR16 *) FormatString,
                 (UINTN) Ip6->Addr[Index],
                 (UINTN) Ip6->Addr[Index + 1]
                 );
    }

    Str = Str + Number;

    if (Index + 2 == 16) {
      *Str = L'\0';
      if (*(Str - 1) == L':') {
        *(Str - 1) = L'\0';
      }
    }
  }
}

/**
  Check whether the input IP address is valid.

  @param[in]  Ip        The IP address.
  @param[in]  IpMode    Indicates iSCSI running on IP4 or IP6 stack.

  @retval     TRUE      The input IP address is valid.
  @retval     FALSE     Otherwise

**/
BOOLEAN
IpIsUnicast (
  IN EFI_IP_ADDRESS *Ip,
  IN  UINT8          IpMode
  )
{
  if (IpMode == IP_MODE_IP4) {
    return NetIp4IsUnicast (NTOHL (Ip->Addr[0]), 0);
  } else if (IpMode == IP_MODE_IP6) {
    return NetIp6IsValidUnicast (&Ip->v6);
  } else {
    DEBUG ((DEBUG_ERROR, "IpMode %d is invalid when configuring the iSCSI target IP!\n", IpMode));
    return FALSE;
  }
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
      (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
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
  Get the attempt config data from global structure by the ConfigIndex.

  @param[in]  AttemptConfigIndex     The unique index indicates the attempt.

  @return       Pointer to the attempt config data.
  @retval NULL  The attempt configuration data cannot be found.

**/
ISCSI_ATTEMPT_CONFIG_NVDATA *
IScsiConfigGetAttemptByConfigIndex (
  IN UINT8                     AttemptConfigIndex
  )
{
  LIST_ENTRY                   *Entry;
  ISCSI_ATTEMPT_CONFIG_NVDATA  *Attempt;

  NET_LIST_FOR_EACH (Entry, &mPrivate->AttemptConfigs) {
    Attempt = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
    if (Attempt->AttemptConfigIndex == AttemptConfigIndex) {
      return Attempt;
    }
  }

  return NULL;
}


/**
  Get the existing attempt config data from global structure by the NicIndex.

  @param[in]  NewAttempt         The created new attempt
  @param[in]  IScsiMode          The IScsi Mode of the new attempt, Enabled or
                                 Enabled for MPIO.

  @return                        Pointer to the existing attempt config data which
                                 has the same NICIndex as the new created attempt.
  @retval     NULL               The attempt with NicIndex does not exist.

**/
ISCSI_ATTEMPT_CONFIG_NVDATA *
IScsiConfigGetAttemptByNic (
  IN ISCSI_ATTEMPT_CONFIG_NVDATA *NewAttempt,
  IN UINT8                       IScsiMode
  )
{
  LIST_ENTRY                   *Entry;
  ISCSI_ATTEMPT_CONFIG_NVDATA  *Attempt;

  NET_LIST_FOR_EACH (Entry, &mPrivate->AttemptConfigs) {
    Attempt = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
    if (Attempt != NewAttempt && Attempt->NicIndex == NewAttempt->NicIndex &&
        Attempt->SessionConfigData.Enabled == IScsiMode) {
      return Attempt;
    }
  }

  return NULL;
}


/**
  Convert the iSCSI configuration data into the IFR data.

  @param[in]       Attempt                The iSCSI attempt config data.
  @param[in, out]  IfrNvData              The IFR nv data.

**/
VOID
IScsiConvertAttemptConfigDataToIfrNvData (
  IN ISCSI_ATTEMPT_CONFIG_NVDATA  *Attempt,
  IN OUT ISCSI_CONFIG_IFR_NVDATA  *IfrNvData
  )
{
  ISCSI_SESSION_CONFIG_NVDATA   *SessionConfigData;
  ISCSI_CHAP_AUTH_CONFIG_NVDATA *AuthConfigData;
  EFI_IP_ADDRESS                Ip;

  //
  // Normal session configuration parameters.
  //
  SessionConfigData                 = &Attempt->SessionConfigData;
  IfrNvData->Enabled                = SessionConfigData->Enabled;
  IfrNvData->IpMode                 = SessionConfigData->IpMode;

  IfrNvData->InitiatorInfoFromDhcp  = SessionConfigData->InitiatorInfoFromDhcp;
  IfrNvData->TargetInfoFromDhcp     = SessionConfigData->TargetInfoFromDhcp;
  IfrNvData->TargetPort             = SessionConfigData->TargetPort;

  if (IfrNvData->IpMode == IP_MODE_IP4) {
    CopyMem (&Ip.v4, &SessionConfigData->LocalIp, sizeof (EFI_IPv4_ADDRESS));
    IScsiIpToStr (&Ip, FALSE, IfrNvData->LocalIp);
    CopyMem (&Ip.v4, &SessionConfigData->SubnetMask, sizeof (EFI_IPv4_ADDRESS));
    IScsiIpToStr (&Ip, FALSE, IfrNvData->SubnetMask);
    CopyMem (&Ip.v4, &SessionConfigData->Gateway, sizeof (EFI_IPv4_ADDRESS));
    IScsiIpToStr (&Ip, FALSE, IfrNvData->Gateway);
    CopyMem (&Ip.v4, &SessionConfigData->TargetIp, sizeof (EFI_IPv4_ADDRESS));
    IScsiIpToStr (&Ip, FALSE, IfrNvData->TargetIp);
  } else if (IfrNvData->IpMode == IP_MODE_IP6) {
    ZeroMem (IfrNvData->TargetIp, sizeof (IfrNvData->TargetIp));
    IP6_COPY_ADDRESS (&Ip.v6, &SessionConfigData->TargetIp);
    IScsiIpToStr (&Ip, TRUE, IfrNvData->TargetIp);
  }

  AsciiStrToUnicodeStr (SessionConfigData->TargetName, IfrNvData->TargetName);
  IScsiLunToUnicodeStr (SessionConfigData->BootLun, IfrNvData->BootLun);
  IScsiConvertIsIdToString (IfrNvData->IsId, SessionConfigData->IsId);

  IfrNvData->ConnectRetryCount = SessionConfigData->ConnectRetryCount;
  IfrNvData->ConnectTimeout    = SessionConfigData->ConnectTimeout;

  //
  // Authentication parameters.
  //
  IfrNvData->AuthenticationType = Attempt->AuthenticationType;

  if (IfrNvData->AuthenticationType == ISCSI_AUTH_TYPE_CHAP) {
    AuthConfigData      = &Attempt->AuthConfigData.CHAP;
    IfrNvData->CHAPType = AuthConfigData->CHAPType;
    AsciiStrToUnicodeStr (AuthConfigData->CHAPName, IfrNvData->CHAPName);
    AsciiStrToUnicodeStr (AuthConfigData->CHAPSecret, IfrNvData->CHAPSecret);
    AsciiStrToUnicodeStr (AuthConfigData->ReverseCHAPName, IfrNvData->ReverseCHAPName);
    AsciiStrToUnicodeStr (AuthConfigData->ReverseCHAPSecret, IfrNvData->ReverseCHAPSecret);
  }

  //
  // Other parameters.
  //
  AsciiStrToUnicodeStr (Attempt->AttemptName, IfrNvData->AttemptName);
}

/**
  Convert the IFR data to iSCSI configuration data.

  @param[in]       IfrNvData              Point to ISCSI_CONFIG_IFR_NVDATA.
  @param[in, out]  Attempt                The iSCSI attempt config data.

  @retval EFI_INVALID_PARAMETER  Any input or configured parameter is invalid.
  @retval EFI_NOT_FOUND          Cannot find the corresponding variable.
  @retval EFI_OUT_OF_RESOURCES   The operation is failed due to lack of resources.
  @retval EFI_ABORTED            The operation is aborted.
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
IScsiConvertIfrNvDataToAttemptConfigData (
  IN ISCSI_CONFIG_IFR_NVDATA          *IfrNvData,
  IN OUT ISCSI_ATTEMPT_CONFIG_NVDATA  *Attempt
  )
{
  EFI_IP_ADDRESS              HostIp;
  EFI_IP_ADDRESS              SubnetMask;
  EFI_IP_ADDRESS              Gateway;
  CHAR16                      *MacString;
  CHAR16                      *AttemptName1;
  CHAR16                      *AttemptName2;
  ISCSI_ATTEMPT_CONFIG_NVDATA *ExistAttempt;
  ISCSI_ATTEMPT_CONFIG_NVDATA *SameNicAttempt;
  CHAR16                      IScsiMode[64];
  CHAR16                      IpMode[64];
  ISCSI_NIC_INFO              *NicInfo;
  EFI_INPUT_KEY               Key;
  UINT8                       *AttemptConfigOrder;
  UINTN                       AttemptConfigOrderSize;
  UINT8                       *AttemptOrderTmp;
  UINTN                       TotalNumber;
  EFI_STATUS                  Status;

  if (IfrNvData == NULL || Attempt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update those fields which don't have INTERACTIVE attribute.
  //
  Attempt->SessionConfigData.ConnectRetryCount     = IfrNvData->ConnectRetryCount;
  Attempt->SessionConfigData.ConnectTimeout        = IfrNvData->ConnectTimeout;
  Attempt->SessionConfigData.IpMode                = IfrNvData->IpMode;

  if (IfrNvData->IpMode < IP_MODE_AUTOCONFIG) {
    Attempt->SessionConfigData.InitiatorInfoFromDhcp = IfrNvData->InitiatorInfoFromDhcp;
    Attempt->SessionConfigData.TargetPort            = IfrNvData->TargetPort;

    if (Attempt->SessionConfigData.TargetPort == 0) {
      Attempt->SessionConfigData.TargetPort = ISCSI_WELL_KNOWN_PORT;
    }

    Attempt->SessionConfigData.TargetInfoFromDhcp = IfrNvData->TargetInfoFromDhcp;
  }

  Attempt->AuthenticationType = IfrNvData->AuthenticationType;

  if (Attempt->AuthenticationType == ISCSI_AUTH_TYPE_CHAP) {
    Attempt->AuthConfigData.CHAP.CHAPType = IfrNvData->CHAPType;
  }

  //
  // Only do full parameter validation if iSCSI is enabled on this device.
  //
  if (IfrNvData->Enabled != ISCSI_DISABLED) {
    if (Attempt->SessionConfigData.ConnectTimeout < CONNECT_MIN_TIMEOUT) {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Connection Establishing Timeout is less than minimum value 100ms.",
        NULL
        );
      
      return EFI_INVALID_PARAMETER;
    }

    //
    // Validate the address configuration of the Initiator if DHCP isn't
    // deployed.
    //
    if (!Attempt->SessionConfigData.InitiatorInfoFromDhcp) {
      CopyMem (&HostIp.v4, &Attempt->SessionConfigData.LocalIp, sizeof (HostIp.v4));
      CopyMem (&SubnetMask.v4, &Attempt->SessionConfigData.SubnetMask, sizeof (SubnetMask.v4));
      CopyMem (&Gateway.v4, &Attempt->SessionConfigData.Gateway, sizeof (Gateway.v4));

      if ((Gateway.Addr[0] != 0)) {
        if (SubnetMask.Addr[0] == 0) {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Gateway address is set but subnet mask is zero.",
            NULL
            );
          
          return EFI_INVALID_PARAMETER;
        } else if (!IP4_NET_EQUAL (HostIp.Addr[0], Gateway.Addr[0], SubnetMask.Addr[0])) {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Local IP and Gateway are not in the same subnet.",
            NULL
            );
          
          return EFI_INVALID_PARAMETER;
        }
      }
    }
    //
    // Validate target configuration if DHCP isn't deployed.
    //
    if (!Attempt->SessionConfigData.TargetInfoFromDhcp && Attempt->SessionConfigData.IpMode < IP_MODE_AUTOCONFIG) {
      if (!IpIsUnicast (&Attempt->SessionConfigData.TargetIp, IfrNvData->IpMode)) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Target IP is invalid!",
          NULL
          );
        return EFI_INVALID_PARAMETER;
      }

      //
      // Validate iSCSI target name configuration again:
      // The format of iSCSI target name is already verified in IScsiFormCallback() when
      // user input the name; here we only check the case user does not input the name.
      //
      if (Attempt->SessionConfigData.TargetName[0] == '\0') {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"iSCSI target name is NULL!",
          NULL
          );
        return EFI_INVALID_PARAMETER;
      }
    }


    //
    // Validate the authentication info.
    //
    if (IfrNvData->AuthenticationType == ISCSI_AUTH_TYPE_CHAP) {
      if ((IfrNvData->CHAPName[0] == '\0') || (IfrNvData->CHAPSecret[0] == '\0')) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"CHAP Name or CHAP Secret is invalid!",
          NULL
          );
        
        return EFI_INVALID_PARAMETER;
      }

      if ((IfrNvData->CHAPType == ISCSI_CHAP_MUTUAL) &&
          ((IfrNvData->ReverseCHAPName[0] == '\0') || (IfrNvData->ReverseCHAPSecret[0] == '\0'))
          ) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Reverse CHAP Name or Reverse CHAP Secret is invalid!",
          NULL
          );        
        return EFI_INVALID_PARAMETER;
      }
    }

    //
    // Check whether this attempt uses NIC which is already used by existing attempt.
    //
    SameNicAttempt = IScsiConfigGetAttemptByNic (Attempt, IfrNvData->Enabled);
    if (SameNicAttempt != NULL) {
      AttemptName1 = (CHAR16 *) AllocateZeroPool (ATTEMPT_NAME_MAX_SIZE * sizeof (CHAR16));
      if (AttemptName1 == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      AttemptName2 = (CHAR16 *) AllocateZeroPool (ATTEMPT_NAME_MAX_SIZE * sizeof (CHAR16));
      if (AttemptName2 == NULL) {
        FreePool (AttemptName1);
        return EFI_OUT_OF_RESOURCES;
      }      
      
      AsciiStrToUnicodeStr (Attempt->AttemptName, AttemptName1);
      if (StrLen (AttemptName1) > ATTEMPT_NAME_SIZE) {
        CopyMem (&AttemptName1[ATTEMPT_NAME_SIZE], L"...", 4 * sizeof (CHAR16));
      }

      AsciiStrToUnicodeStr (SameNicAttempt->AttemptName, AttemptName2);
      if (StrLen (AttemptName2) > ATTEMPT_NAME_SIZE) {
        CopyMem (&AttemptName2[ATTEMPT_NAME_SIZE], L"...", 4 * sizeof (CHAR16));
      }

      UnicodeSPrint (
        mPrivate->PortString,
        (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
        L"Warning! Attempt \"%s\" uses same NIC as Attempt \"%s\".",
        AttemptName1,
        AttemptName2
        );

      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        mPrivate->PortString,
        NULL
        );       

      FreePool (AttemptName1);
      FreePool (AttemptName2);
    }
  }

  //
  // Update the iSCSI Mode data and record it in attempt help info.
  //
  Attempt->SessionConfigData.Enabled = IfrNvData->Enabled;
  if (IfrNvData->Enabled == ISCSI_DISABLED) {
    UnicodeSPrint (IScsiMode, 64, L"Disabled");
  } else if (IfrNvData->Enabled == ISCSI_ENABLED) {
    UnicodeSPrint (IScsiMode, 64, L"Enabled");
  } else if (IfrNvData->Enabled == ISCSI_ENABLED_FOR_MPIO) {
    UnicodeSPrint (IScsiMode, 64, L"Enabled for MPIO");
  }

  if (IfrNvData->IpMode == IP_MODE_IP4) {
    UnicodeSPrint (IpMode, 64, L"IP4");
  } else if (IfrNvData->IpMode == IP_MODE_IP6) {
    UnicodeSPrint (IpMode, 64, L"IP6");
  } else if (IfrNvData->IpMode == IP_MODE_AUTOCONFIG) {
    UnicodeSPrint (IpMode, 64, L"Autoconfigure");
  }

  NicInfo = IScsiGetNicInfoByIndex (Attempt->NicIndex);
  if (NicInfo == NULL) {
    return EFI_NOT_FOUND;
  }

  MacString = (CHAR16 *) AllocateZeroPool (ISCSI_MAX_MAC_STRING_LEN * sizeof (CHAR16));
  if (MacString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrToUnicodeStr (Attempt->MacString, MacString);

  UnicodeSPrint (
    mPrivate->PortString,
    (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
    L"MAC: %s, PFA: Bus %d | Dev %d | Func %d, iSCSI mode: %s, IP version: %s",
    MacString,
    NicInfo->BusNumber,
    NicInfo->DeviceNumber,
    NicInfo->FunctionNumber,
    IScsiMode,
    IpMode
    );

  Attempt->AttemptTitleHelpToken = HiiSetString (
                                     mCallbackInfo->RegisteredHandle,
                                     Attempt->AttemptTitleHelpToken,
                                     mPrivate->PortString,
                                     NULL
                                     );
  if (Attempt->AttemptTitleHelpToken == 0) {
    FreePool (MacString);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Check whether this attempt is an existing one.
  //
  ExistAttempt = IScsiConfigGetAttemptByConfigIndex (Attempt->AttemptConfigIndex);
  if (ExistAttempt != NULL) {
    ASSERT (ExistAttempt == Attempt);

    if (IfrNvData->Enabled == ISCSI_DISABLED &&
        Attempt->SessionConfigData.Enabled != ISCSI_DISABLED) {

      //
      // User updates the Attempt from "Enabled"/"Enabled for MPIO" to "Disabled".
      //
      if (Attempt->SessionConfigData.Enabled == ISCSI_ENABLED_FOR_MPIO) {
        if (mPrivate->MpioCount < 1) {
          return EFI_ABORTED;
        }

        if (--mPrivate->MpioCount == 0) {
          mPrivate->EnableMpio = FALSE;
        }
      } else if (Attempt->SessionConfigData.Enabled == ISCSI_ENABLED) {
        if (mPrivate->SinglePathCount < 1) {
          return EFI_ABORTED;
        }
        mPrivate->SinglePathCount--;
      }

    } else if (IfrNvData->Enabled == ISCSI_ENABLED_FOR_MPIO &&
               Attempt->SessionConfigData.Enabled == ISCSI_ENABLED) {
      //
      // User updates the Attempt from "Enabled" to "Enabled for MPIO".
      //
      if (mPrivate->SinglePathCount < 1) {
        return EFI_ABORTED;
      }

      mPrivate->EnableMpio = TRUE;
      mPrivate->MpioCount++;
      mPrivate->SinglePathCount--;

    } else if (IfrNvData->Enabled == ISCSI_ENABLED &&
               Attempt->SessionConfigData.Enabled == ISCSI_ENABLED_FOR_MPIO) {
      //
      // User updates the Attempt from "Enabled for MPIO" to "Enabled".
      //
      if (mPrivate->MpioCount < 1) {
        return EFI_ABORTED;
      }

      if (--mPrivate->MpioCount == 0) {
        mPrivate->EnableMpio = FALSE;
      }
      mPrivate->SinglePathCount++;

    } else if (IfrNvData->Enabled != ISCSI_DISABLED &&
               Attempt->SessionConfigData.Enabled == ISCSI_DISABLED) {
      //
      // User updates the Attempt from "Disabled" to "Enabled"/"Enabled for MPIO".
      //
      if (IfrNvData->Enabled == ISCSI_ENABLED_FOR_MPIO) {
        mPrivate->EnableMpio = TRUE;
        mPrivate->MpioCount++;

      } else if (IfrNvData->Enabled == ISCSI_ENABLED) {
        mPrivate->SinglePathCount++;
      }
    }

  } else if (ExistAttempt == NULL) {
    //
    // When a new attempt is created, pointer of the attempt is saved to
    // mPrivate->NewAttempt, and also saved to mCallbackInfo->Current in
    // IScsiConfigProcessDefault. If input Attempt does not match any existing
    // attempt, it should be a new created attempt. Save it to system now.
    //    
    ASSERT (Attempt == mPrivate->NewAttempt);

    //
    // Save current order number for this attempt.
    //
    AttemptConfigOrder = IScsiGetVariableAndSize (
                           L"AttemptOrder",
                           &gIScsiConfigGuid,
                           &AttemptConfigOrderSize
                           );

    TotalNumber = AttemptConfigOrderSize / sizeof (UINT8);
    TotalNumber++;

    //
    // Append the new created attempt order to the end.
    //
    AttemptOrderTmp = AllocateZeroPool (TotalNumber * sizeof (UINT8));
    if (AttemptOrderTmp == NULL) {
      if (AttemptConfigOrder != NULL) {
        FreePool (AttemptConfigOrder);
      }
      return EFI_OUT_OF_RESOURCES;
    }

    if (AttemptConfigOrder != NULL) {
      CopyMem (AttemptOrderTmp, AttemptConfigOrder, AttemptConfigOrderSize);
      FreePool (AttemptConfigOrder);
    }

    AttemptOrderTmp[TotalNumber - 1] = Attempt->AttemptConfigIndex;
    AttemptConfigOrder               = AttemptOrderTmp;
    AttemptConfigOrderSize           = TotalNumber * sizeof (UINT8);

    Status = gRT->SetVariable (
                    L"AttemptOrder",
                    &gIScsiConfigGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    AttemptConfigOrderSize,
                    AttemptConfigOrder
                    );
    FreePool (AttemptConfigOrder);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Insert new created attempt to array.
    //
    InsertTailList (&mPrivate->AttemptConfigs, &Attempt->Link);
    mPrivate->AttemptCount++;
    //
    // Reset mPrivate->NewAttempt to NULL, which indicates none attempt is created
    // but not saved now.
    //
    mPrivate->NewAttempt = NULL;

    if (IfrNvData->Enabled == ISCSI_ENABLED_FOR_MPIO) {
      //
      // This new Attempt is enabled for MPIO; enable the multipath mode.
      //
      mPrivate->EnableMpio = TRUE;
      mPrivate->MpioCount++;
    } else if (IfrNvData->Enabled == ISCSI_ENABLED) {
      mPrivate->SinglePathCount++;
    }

    IScsiConfigUpdateAttempt ();
  }

  //
  // Record the user configuration information in NVR.
  //
  UnicodeSPrint (
    mPrivate->PortString,
    (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
    L"%s%d",
    MacString,
    (UINTN) Attempt->AttemptConfigIndex
    );

  FreePool (MacString);

  return gRT->SetVariable (
                mPrivate->PortString,
                &gEfiIScsiInitiatorNameProtocolGuid,
                ISCSI_CONFIG_VAR_ATTR,
                sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA),
                Attempt
                );
}

/**
  Create Hii Extend Label OpCode as the start opcode and end opcode. It is 
  a help function.

  @param[in]  StartLabelNumber   The number of start label.
  @param[out] StartOpCodeHandle  Points to the start opcode handle.
  @param[out] StartLabel         Points to the created start opcode.
  @param[out] EndOpCodeHandle    Points to the end opcode handle.
  @param[out] EndLabel           Points to the created end opcode.

  @retval EFI_OUT_OF_RESOURCES   Do not have sufficient resource to finish this
                                 operation.
  @retval EFI_INVALID_PARAMETER  Any input parameter is invalid.                                 
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
IScsiCreateOpCode (
  IN  UINT16                        StartLabelNumber,
  OUT VOID                          **StartOpCodeHandle,
  OUT EFI_IFR_GUID_LABEL            **StartLabel,
  OUT VOID                          **EndOpCodeHandle,
  OUT EFI_IFR_GUID_LABEL            **EndLabel
  )
{
  EFI_STATUS                        Status;
  EFI_IFR_GUID_LABEL                *InternalStartLabel;
  EFI_IFR_GUID_LABEL                *InternalEndLabel;

  if (StartOpCodeHandle == NULL || StartLabel == NULL || EndOpCodeHandle == NULL || EndLabel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *StartOpCodeHandle = NULL;
  *EndOpCodeHandle   = NULL;
  Status             = EFI_OUT_OF_RESOURCES;

  //
  // Initialize the container for dynamic opcodes.
  //
  *StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (*StartOpCodeHandle == NULL) {
    return Status;
  }

  *EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (*EndOpCodeHandle == NULL) {
    goto Exit;
  }

  //
  // Create Hii Extend Label OpCode as the start opcode.
  //
  InternalStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                                *StartOpCodeHandle,
                                                &gEfiIfrTianoGuid,
                                                NULL,
                                                sizeof (EFI_IFR_GUID_LABEL)
                                                );
  if (InternalStartLabel == NULL) {
    goto Exit;
  }
  
  InternalStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  InternalStartLabel->Number       = StartLabelNumber;

  //
  // Create Hii Extend Label OpCode as the end opcode.
  //
  InternalEndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                              *EndOpCodeHandle,
                                              &gEfiIfrTianoGuid,
                                              NULL,
                                              sizeof (EFI_IFR_GUID_LABEL)
                                              );
  if (InternalEndLabel == NULL) {
    goto Exit;
  }

  InternalEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  InternalEndLabel->Number       = LABEL_END;

  *StartLabel = InternalStartLabel;
  *EndLabel   = InternalEndLabel;

  return EFI_SUCCESS;

Exit:

  if (*StartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (*StartOpCodeHandle);
  }

  if (*EndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (*EndOpCodeHandle);
  }
  
  return Status;
}

/**
  Callback function when user presses "Add an Attempt".

  @retval EFI_OUT_OF_RESOURCES   Does not have sufficient resources to finish this
                                 operation.
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
IScsiConfigAddAttempt (
  VOID
  )
{
  LIST_ENTRY                    *Entry;
  ISCSI_NIC_INFO                *NicInfo;
  EFI_STRING_ID                 PortTitleToken;
  EFI_STRING_ID                 PortTitleHelpToken;
  CHAR16                        MacString[ISCSI_MAX_MAC_STRING_LEN];
  EFI_STATUS                    Status;
  VOID                          *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL            *StartLabel;
  VOID                          *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL            *EndLabel;

  Status = IScsiCreateOpCode (
             MAC_ENTRY_LABEL,
             &StartOpCodeHandle,
             &StartLabel,
             &EndOpCodeHandle,
             &EndLabel
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Ask user to select a MAC for this attempt.
  //
  NET_LIST_FOR_EACH (Entry, &mPrivate->NicInfoList) {
    NicInfo = NET_LIST_USER_STRUCT (Entry, ISCSI_NIC_INFO, Link);
    IScsiMacAddrToStr (
      &NicInfo->PermanentAddress,
      NicInfo->HwAddressSize,
      NicInfo->VlanId,
      MacString
      );

    UnicodeSPrint (mPrivate->PortString, (UINTN) ISCSI_NAME_IFR_MAX_SIZE, L"MAC %s", MacString);
    PortTitleToken = HiiSetString (
                       mCallbackInfo->RegisteredHandle,
                       0,
                       mPrivate->PortString,
                       NULL
                       );
    if (PortTitleToken == 0) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
      L"PFA: Bus %d | Dev %d | Func %d",
      NicInfo->BusNumber,
      NicInfo->DeviceNumber,
      NicInfo->FunctionNumber
      );
    PortTitleHelpToken = HiiSetString (mCallbackInfo->RegisteredHandle, 0, mPrivate->PortString, NULL);
    if (PortTitleHelpToken == 0) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;      
    }

    HiiCreateGotoOpCode (
      StartOpCodeHandle,                      // Container for dynamic created opcodes
      FORMID_ATTEMPT_FORM,
      PortTitleToken,
      PortTitleHelpToken,
      EFI_IFR_FLAG_CALLBACK,                  // Question flag
      (UINT16) (KEY_MAC_ENTRY_BASE + NicInfo->NicIndex)
      );
  }

  Status = HiiUpdateForm (
             mCallbackInfo->RegisteredHandle, // HII handle
             &gIScsiConfigGuid,               // Formset GUID
             FORMID_MAC_FORM,                 // Form ID
             StartOpCodeHandle,               // Label for where to insert opcodes
             EndOpCodeHandle                  // Replace data
             );

Exit:
  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  
  return Status;
}


/**
  Update the MAIN form to display the configured attempts.

**/
VOID
IScsiConfigUpdateAttempt (
  VOID
  )
{
  CHAR16                        AttemptName[ATTEMPT_NAME_MAX_SIZE];
  LIST_ENTRY                    *Entry;
  ISCSI_ATTEMPT_CONFIG_NVDATA   *AttemptConfigData;
  VOID                          *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL            *StartLabel;
  VOID                          *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL            *EndLabel;
  EFI_STATUS                    Status;

  Status = IScsiCreateOpCode (
             ATTEMPT_ENTRY_LABEL,
             &StartOpCodeHandle,
             &StartLabel,
             &EndOpCodeHandle,
             &EndLabel
             );
  if (EFI_ERROR (Status)) {
    return ;
  }

  NET_LIST_FOR_EACH (Entry, &mPrivate->AttemptConfigs) {
    AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);

    AsciiStrToUnicodeStr (AttemptConfigData->AttemptName, AttemptName);
    UnicodeSPrint (mPrivate->PortString, (UINTN) 128, L"Attempt %s", AttemptName);
    AttemptConfigData->AttemptTitleToken = HiiSetString (
                                             mCallbackInfo->RegisteredHandle,
                                             0,
                                             mPrivate->PortString,
                                             NULL
                                             );
    if (AttemptConfigData->AttemptTitleToken == 0) {
      return ;
    }

    HiiCreateGotoOpCode (
      StartOpCodeHandle,                         // Container for dynamic created opcodes
      FORMID_ATTEMPT_FORM,                       // Form ID
      AttemptConfigData->AttemptTitleToken,      // Prompt text
      AttemptConfigData->AttemptTitleHelpToken,  // Help text
      EFI_IFR_FLAG_CALLBACK,                     // Question flag
      (UINT16) (KEY_ATTEMPT_ENTRY_BASE + AttemptConfigData->AttemptConfigIndex)   // Question ID
      );
  }

  HiiUpdateForm (
    mCallbackInfo->RegisteredHandle, // HII handle
    &gIScsiConfigGuid,               // Formset GUID
    FORMID_MAIN_FORM,                // Form ID
    StartOpCodeHandle,               // Label for where to insert opcodes
    EndOpCodeHandle                  // Replace data
  );    

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}


/**
  Callback function when user presses "Commit Changes and Exit" in Delete Attempts.

  @param[in]  IfrNvData          The IFR NV data.

  @retval EFI_NOT_FOUND          Cannot find the corresponding variable.
  @retval EFI_SUCCESS            The operation is completed successfully.
  @retval EFI_ABOTRED            This operation is aborted cause of error
                                 configuration.
  @retval EFI_OUT_OF_RESOURCES   Fail to finish the operation due to lack of
                                 resources.

**/
EFI_STATUS
IScsiConfigDeleteAttempts (
  IN ISCSI_CONFIG_IFR_NVDATA  *IfrNvData
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  UINTN                       NewIndex;
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptConfigData;
  UINT8                       *AttemptConfigOrder;
  UINTN                       AttemptConfigOrderSize;
  UINT8                       *AttemptNewOrder;
  UINT32                      Attribute;
  UINTN                       Total;
  UINTN                       NewTotal;
  LIST_ENTRY                  *Entry;
  LIST_ENTRY                  *NextEntry;
  CHAR16                      MacString[ISCSI_MAX_MAC_STRING_LEN];

  AttemptConfigOrder = IScsiGetVariableAndSize (
                         L"AttemptOrder",
                         &gIScsiConfigGuid,
                         &AttemptConfigOrderSize
                         );
  if ((AttemptConfigOrder == NULL) || (AttemptConfigOrderSize == 0)) {
    return EFI_NOT_FOUND;
  }

  AttemptNewOrder = AllocateZeroPool (AttemptConfigOrderSize);
  if (AttemptNewOrder == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Total    = AttemptConfigOrderSize / sizeof (UINT8);
  NewTotal = Total;
  Index    = 0;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &mPrivate->AttemptConfigs) {
    if (IfrNvData->DeleteAttemptList[Index] == 0) {
      Index++;
      continue;
    }

    //
    // Delete the attempt.
    //

    AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
    if (AttemptConfigData == NULL) {
      Status = EFI_NOT_FOUND;
      goto Error;
    }

    //
    // Remove this attempt from UI configured attempt list.
    //
    RemoveEntryList (&AttemptConfigData->Link);
    mPrivate->AttemptCount--;

    if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED_FOR_MPIO) {
      if (mPrivate->MpioCount < 1) {
        Status = EFI_ABORTED;
        goto Error;
      }

      //
      // No more attempt is enabled for MPIO. Transit the iSCSI mode to single path.
      //
      if (--mPrivate->MpioCount == 0) {
        mPrivate->EnableMpio = FALSE;
      }
    } else if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED) {
      if (mPrivate->SinglePathCount < 1) {
        Status = EFI_ABORTED;
        goto Error;
      }

      mPrivate->SinglePathCount--;
    }

    AsciiStrToUnicodeStr (AttemptConfigData->MacString, MacString);

    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) 128,
      L"%s%d",
      MacString,
      (UINTN) AttemptConfigData->AttemptConfigIndex
      );

    gRT->SetVariable (
           mPrivate->PortString,
           &gEfiIScsiInitiatorNameProtocolGuid,
           0,
           0,
           NULL
           );

    //
    // Mark the attempt order in NVR to be deleted - 0.
    //
    for (NewIndex = 0; NewIndex < Total; NewIndex++) {
      if (AttemptConfigOrder[NewIndex] == AttemptConfigData->AttemptConfigIndex) {
        AttemptConfigOrder[NewIndex] = 0;
        break;
      }
    }

    NewTotal--;
    FreePool (AttemptConfigData);

    //
    // Check next Attempt.
    //
    Index++;
  }

  //
  // Construct AttemptNewOrder.
  //
  for (Index = 0, NewIndex = 0; Index < Total; Index++) {
    if (AttemptConfigOrder[Index] != 0) {
      AttemptNewOrder[NewIndex] = AttemptConfigOrder[Index];
      NewIndex++;
    }
  }

  Attribute = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE;

  //
  // Update AttemptOrder in NVR.
  //
  Status = gRT->SetVariable (
                  L"AttemptOrder",
                  &gIScsiConfigGuid,
                  Attribute,
                  NewTotal * sizeof (UINT8),
                  AttemptNewOrder
                  );

Error:
  if (AttemptConfigOrder != NULL) {
    FreePool (AttemptConfigOrder);
  }

  if (AttemptNewOrder != NULL) {
    FreePool (AttemptNewOrder);
  }
  
  return Status;
}


/**
  Callback function when user presses "Delete Attempts".

  @param[in]  IfrNvData          The IFR nv data.

  @retval EFI_INVALID_PARAMETER  Any parameter is invalid.
  @retval EFI_BUFFER_TOO_SMALL   The buffer in UpdateData is too small.
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
IScsiConfigDisplayDeleteAttempts (
  IN ISCSI_CONFIG_IFR_NVDATA  *IfrNvData
  )
{

  UINT8                       *AttemptConfigOrder;
  UINTN                       AttemptConfigOrderSize;
  LIST_ENTRY                  *Entry;
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptConfigData;
  UINT8                       Index;
  VOID                        *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *EndLabel;
  EFI_STATUS                  Status;

  Status = IScsiCreateOpCode (
             DELETE_ENTRY_LABEL,
             &StartOpCodeHandle,
             &StartLabel,
             &EndOpCodeHandle,
             &EndLabel
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AttemptConfigOrder = IScsiGetVariableAndSize (
                         L"AttemptOrder",
                         &gIScsiConfigGuid,
                         &AttemptConfigOrderSize
                         );
  if (AttemptConfigOrder != NULL) {
    //
    // Create the check box opcode to be deleted.
    //
    Index = 0;

    NET_LIST_FOR_EACH (Entry, &mPrivate->AttemptConfigs) {
      AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
      IfrNvData->DeleteAttemptList[Index] = 0x00;

      HiiCreateCheckBoxOpCode(
        StartOpCodeHandle,
        (EFI_QUESTION_ID) (ATTEMPT_DEL_QUESTION_ID + Index),
        CONFIGURATION_VARSTORE_ID,
        (UINT16) (ATTEMPT_DEL_VAR_OFFSET + Index),
        AttemptConfigData->AttemptTitleToken,
        AttemptConfigData->AttemptTitleHelpToken,
        0,
        0,
        NULL
        );

      Index++;

      if (Index == ISCSI_MAX_ATTEMPTS_NUM) {
        break;
      }
    }

    FreePool (AttemptConfigOrder);
  }

  Status = HiiUpdateForm (
             mCallbackInfo->RegisteredHandle, // HII handle
             &gIScsiConfigGuid,               // Formset GUID
             FORMID_DELETE_FORM,              // Form ID
             StartOpCodeHandle,               // Label for where to insert opcodes
             EndOpCodeHandle                  // Replace data
             );    

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return Status;
}


/**
  Callback function when user presses "Change Attempt Order".

  @retval EFI_INVALID_PARAMETER  Any parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Does not have sufficient resources to finish this
                                 operation.
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
IScsiConfigDisplayOrderAttempts (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINT8                       Index;
  LIST_ENTRY                  *Entry;
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptConfigData;
  VOID                        *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *EndLabel;
  VOID                        *OptionsOpCodeHandle;  
  
  Status = IScsiCreateOpCode (
             ORDER_ENTRY_LABEL,
             &StartOpCodeHandle,
             &StartLabel,
             &EndOpCodeHandle,
             &EndLabel
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (StartOpCodeHandle != NULL);

  OptionsOpCodeHandle = NULL;

  //
  // If no attempt to be ordered, update the original form and exit.
  //
  if (mPrivate->AttemptCount == 0) {
    goto Exit;
  }

  //
  // Create Option OpCode.
  //
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (OptionsOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Index = 0;

  NET_LIST_FOR_EACH (Entry, &mPrivate->AttemptConfigs) {
    AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      AttemptConfigData->AttemptTitleToken,
      0,
      EFI_IFR_NUMERIC_SIZE_1,
      AttemptConfigData->AttemptConfigIndex
      );
    Index++;
  }

  ASSERT (Index == mPrivate->AttemptCount);

  HiiCreateOrderedListOpCode (
    StartOpCodeHandle,                          // Container for dynamic created opcodes
    DYNAMIC_ORDERED_LIST_QUESTION_ID,           // Question ID
    CONFIGURATION_VARSTORE_ID,                  // VarStore ID
    DYNAMIC_ORDERED_LIST_VAR_OFFSET,            // Offset in Buffer Storage
    STRING_TOKEN (STR_ORDER_ATTEMPT_ENTRY),     // Question prompt text        
    STRING_TOKEN (STR_ORDER_ATTEMPT_ENTRY),     // Question help text       
    0,                                          // Question flag
    EFI_IFR_UNIQUE_SET,                         // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
    EFI_IFR_NUMERIC_SIZE_1,                     // Data type of Question value
    ISCSI_MAX_ATTEMPTS_NUM,                     // Maximum container
    OptionsOpCodeHandle,                        // Option Opcode list                        
    NULL                                        // Default Opcode is NULL                               
    );

Exit:
  Status = HiiUpdateForm (
             mCallbackInfo->RegisteredHandle, // HII handle
             &gIScsiConfigGuid,               // Formset GUID
             FORMID_ORDER_FORM,               // Form ID
             StartOpCodeHandle,               // Label for where to insert opcodes
             EndOpCodeHandle                  // Replace data
             );             

Error:
  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);  
  if (OptionsOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  }

  return Status;
}


/**
  Callback function when user presses "Commit Changes and Exit" in Change Attempt Order.

  @param[in]  IfrNvData          The IFR nv data.

  @retval EFI_OUT_OF_RESOURCES   Does not have sufficient resources to finish this
                                 operation.
  @retval EFI_NOT_FOUND          Cannot find the corresponding variable.
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
IScsiConfigOrderAttempts (
  IN ISCSI_CONFIG_IFR_NVDATA  *IfrNvData
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  UINTN                       Indexj;
  UINT8                       AttemptConfigIndex;
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptConfigData;
  UINT8                       *AttemptConfigOrder;
  UINT8                       *AttemptConfigOrderTmp;
  UINTN                       AttemptConfigOrderSize;

  AttemptConfigOrder = IScsiGetVariableAndSize (
                         L"AttemptOrder",
                         &gIScsiConfigGuid,
                         &AttemptConfigOrderSize
                         );
  if (AttemptConfigOrder == NULL) {
    return EFI_NOT_FOUND;
  }

  AttemptConfigOrderTmp = AllocateZeroPool (AttemptConfigOrderSize);
  if (AttemptConfigOrderTmp == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  for (Index = 0; Index < ISCSI_MAX_ATTEMPTS_NUM; Index++) {
    //
    // The real content ends with 0.
    //
    if (IfrNvData->DynamicOrderedList[Index] == 0) {
      break;
    }

    AttemptConfigIndex = IfrNvData->DynamicOrderedList[Index];
    AttemptConfigData  = IScsiConfigGetAttemptByConfigIndex (AttemptConfigIndex);
    if (AttemptConfigData == NULL) {
      Status = EFI_NOT_FOUND;
      goto Exit;
    }

    //
    // Reorder the Attempt List.
    //
    RemoveEntryList (&AttemptConfigData->Link);
    InsertTailList (&mPrivate->AttemptConfigs, &AttemptConfigData->Link);

    AttemptConfigOrderTmp[Index] = AttemptConfigIndex;

    //
    // Mark it to be deleted - 0.
    //
    for (Indexj = 0; Indexj < AttemptConfigOrderSize / sizeof (UINT8); Indexj++) {
      if (AttemptConfigOrder[Indexj] == AttemptConfigIndex) {
        AttemptConfigOrder[Indexj] = 0;
        break;
      }
    }
  }

  //
  // Adjust the attempt order in NVR.
  //
  for (; Index < AttemptConfigOrderSize / sizeof (UINT8); Index++) {
    for (Indexj = 0; Indexj < AttemptConfigOrderSize / sizeof (UINT8); Indexj++) {
      if (AttemptConfigOrder[Indexj] != 0) {
        AttemptConfigOrderTmp[Index] = AttemptConfigOrder[Indexj];
        AttemptConfigOrder[Indexj]   = 0;
        continue;
      }
    }
  }

  Status = gRT->SetVariable (
                  L"AttemptOrder",
                  &gIScsiConfigGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  AttemptConfigOrderSize,
                  AttemptConfigOrderTmp
                  );

Exit:
  if (AttemptConfigOrderTmp != NULL) {
    FreePool (AttemptConfigOrderTmp);
  }

  FreePool (AttemptConfigOrder);
  return Status;
}


/**
  Callback function when a user presses "Attempt *" or when a user selects a NIC to
  create the new attempt.

  @param[in]  KeyValue           A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param[in]  IfrNvData          The IFR nv data.

  @retval EFI_OUT_OF_RESOURCES   Does not have sufficient resources to finish this
                                 operation.
  @retval EFI_NOT_FOUND          Cannot find the corresponding variable.
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
IScsiConfigProcessDefault (
  IN  EFI_QUESTION_ID              KeyValue,
  IN  ISCSI_CONFIG_IFR_NVDATA      *IfrNvData
  )
{
  BOOLEAN                     NewAttempt;
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptConfigData;
  ISCSI_SESSION_CONFIG_NVDATA *ConfigData;
  UINT8                       CurrentAttemptConfigIndex;
  ISCSI_NIC_INFO              *NicInfo;
  UINT8                       NicIndex;
  CHAR16                      MacString[ISCSI_MAX_MAC_STRING_LEN];
  UINT8                       *AttemptConfigOrder;
  UINTN                       AttemptConfigOrderSize;
  UINTN                       TotalNumber;
  UINTN                       Index;

  //
  // Is User creating a new attempt?
  //
  NewAttempt = FALSE;

  if ((KeyValue >= KEY_MAC_ENTRY_BASE) &&
      (KeyValue <= (UINT16) (mPrivate->MaxNic + KEY_MAC_ENTRY_BASE))) {
    //
    // User has pressed "Add an Attempt" and then selects a NIC.
    //
    NewAttempt = TRUE;
  } else if ((KeyValue >= KEY_ATTEMPT_ENTRY_BASE) &&
             (KeyValue < (ISCSI_MAX_ATTEMPTS_NUM + KEY_ATTEMPT_ENTRY_BASE))) {

    //
    // User has pressed "Attempt *".
    //
    NewAttempt = FALSE;
  } else {
    //
    // Don't process anything.
    //
    return EFI_SUCCESS;
  }
  
  //
  // Free any attempt that is previously created but not saved to system.
  //
  if (mPrivate->NewAttempt != NULL) {
    FreePool (mPrivate->NewAttempt);
    mPrivate->NewAttempt = NULL;
  }

  if (NewAttempt) {
    //
    // Determine which NIC user has selected for the new created attempt.
    //
    NicIndex = (UINT8) (KeyValue - KEY_MAC_ENTRY_BASE);
    NicInfo = IScsiGetNicInfoByIndex (NicIndex);
    if (NicInfo == NULL) {
      return EFI_NOT_FOUND;
    }
    
    //
    // Create new attempt.
    //

    AttemptConfigData = AllocateZeroPool (sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA));
    if (AttemptConfigData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ConfigData                    = &AttemptConfigData->SessionConfigData;
    ConfigData->TargetPort        = ISCSI_WELL_KNOWN_PORT;
    ConfigData->ConnectTimeout    = CONNECT_DEFAULT_TIMEOUT;
    ConfigData->ConnectRetryCount = CONNECT_MIN_RETRY;

    AttemptConfigData->AuthenticationType           = ISCSI_AUTH_TYPE_CHAP;
    AttemptConfigData->AuthConfigData.CHAP.CHAPType = ISCSI_CHAP_UNI;

    //
    // Get current order number for this attempt.
    //
    AttemptConfigOrder = IScsiGetVariableAndSize (
                           L"AttemptOrder",
                           &gIScsiConfigGuid,
                           &AttemptConfigOrderSize
                           );

    TotalNumber = AttemptConfigOrderSize / sizeof (UINT8);

    if (AttemptConfigOrder == NULL) {
      CurrentAttemptConfigIndex = 1;
    } else {
      //
      // Get the max attempt config index.
      //
      CurrentAttemptConfigIndex = AttemptConfigOrder[0];
      for (Index = 1; Index < TotalNumber; Index++) {
        if (CurrentAttemptConfigIndex < AttemptConfigOrder[Index]) {
          CurrentAttemptConfigIndex = AttemptConfigOrder[Index];
        }
      }

      CurrentAttemptConfigIndex++;
    }

    TotalNumber++;

    //
    // Record the mapping between attempt order and attempt's configdata.
    //
    AttemptConfigData->AttemptConfigIndex  = CurrentAttemptConfigIndex;

    if (AttemptConfigOrder != NULL) {
      FreePool (AttemptConfigOrder);
    }

    //
    // Record the MAC info in Config Data.
    //
    IScsiMacAddrToStr (
      &NicInfo->PermanentAddress,
      NicInfo->HwAddressSize,
      NicInfo->VlanId,
      MacString
      );

    UnicodeStrToAsciiStr (MacString, AttemptConfigData->MacString);
    AttemptConfigData->NicIndex = NicIndex;

    //
    // Generate OUI-format ISID based on MAC address.
    //
    CopyMem (AttemptConfigData->SessionConfigData.IsId, &NicInfo->PermanentAddress, 6);
    AttemptConfigData->SessionConfigData.IsId[0] = 
      (UINT8) (AttemptConfigData->SessionConfigData.IsId[0] & 0x3F);

    //
    // Add the help info for the new attempt.
    //
    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
      L"MAC: %s, PFA: Bus %d | Dev %d | Func %d",
      MacString,
      NicInfo->BusNumber,
      NicInfo->DeviceNumber,
      NicInfo->FunctionNumber
      );

    AttemptConfigData->AttemptTitleHelpToken  = HiiSetString (
                                                  mCallbackInfo->RegisteredHandle,
                                                  0,
                                                  mPrivate->PortString,
                                                  NULL
                                                  );
    if (AttemptConfigData->AttemptTitleHelpToken == 0) {
      FreePool (AttemptConfigData);
      return EFI_INVALID_PARAMETER;
    }

    //
    // Set the attempt name to default.
    //
    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) 128,
      L"%d",
      (UINTN) AttemptConfigData->AttemptConfigIndex
      );
    UnicodeStrToAsciiStr (mPrivate->PortString, AttemptConfigData->AttemptName);

    //
    // Save the created Attempt temporarily. If user does not save the attempt
    // by press 'KEY_SAVE_ATTEMPT_CONFIG' later, iSCSI driver would know that
    // and free resources.
    //
    mPrivate->NewAttempt = (VOID *) AttemptConfigData;

  } else {
    //
    // Determine which Attempt user has selected to configure.
    // Get the attempt configuration data.
    //
    CurrentAttemptConfigIndex = (UINT8) (KeyValue - KEY_ATTEMPT_ENTRY_BASE);

    AttemptConfigData = IScsiConfigGetAttemptByConfigIndex (CurrentAttemptConfigIndex);
    if (AttemptConfigData == NULL) {
      DEBUG ((DEBUG_ERROR, "Corresponding configuration data can not be retrieved!\n"));
      return EFI_NOT_FOUND;
    }
  }

  //
  // Clear the old IFR data to avoid sharing it with other attempts.
  //
  if (IfrNvData->AuthenticationType == ISCSI_AUTH_TYPE_CHAP) {
    ZeroMem (IfrNvData->CHAPName, sizeof (IfrNvData->CHAPName));
    ZeroMem (IfrNvData->CHAPSecret, sizeof (IfrNvData->CHAPSecret));
    ZeroMem (IfrNvData->ReverseCHAPName, sizeof (IfrNvData->ReverseCHAPName));
    ZeroMem (IfrNvData->ReverseCHAPSecret, sizeof (IfrNvData->ReverseCHAPSecret));
  }
  
  IScsiConvertAttemptConfigDataToIfrNvData (AttemptConfigData, IfrNvData);

  //
  // Update current attempt to be a new created attempt or an existing attempt.
  //
  mCallbackInfo->Current = AttemptConfigData;

  return EFI_SUCCESS;
}


/**
   
  This function allows the caller to request the current
  configuration for one or more named elements. The resulting
  string is in <ConfigAltResp> format. Also, any and all alternative
  configuration strings shall be appended to the end of the
  current configuration string. If they are, they must appear
  after the current configuration. They must contain the same
  routing (GUID, NAME, PATH) as the current configuration string.
  They must have an additional description indicating the type of
  alternative configuration the string represents,
  "ALTCFG=<StringToken>". That <StringToken> (when
  converted from Hex UNICODE to binary) is a reference to a
  string in the associated string pack.

  @param[in]  This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param[in]  Request    A null-terminated Unicode string in
                         <ConfigRequest> format. Note that this
                         includes the routing information as well as
                         the configurable name / value pairs. It is
                         invalid for this string to be in
                         <MultiConfigRequest> format.

  @param[out] Progress   On return, points to a character in the
                         Request string. Points to the string's null
                         terminator if request was successful. Points
                         to the most recent "&" before the first
                         failing name / value pair (or the beginning
                         of the string if the failure is in the first
                         name / value pair) if the request was not successful.                        

  @param[out] Results    A null-terminated Unicode string in
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
                                  to most recent "&" before the
                                  error or the beginning of the
                                  string.

  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question.

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
  CHAR8                            *InitiatorName;
  UINTN                            BufferSize;
  ISCSI_CONFIG_IFR_NVDATA          *IfrNvData;
  ISCSI_FORM_CALLBACK_INFO         *Private;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;

  if (This == NULL || Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gIScsiConfigGuid, mVendorStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  Private = ISCSI_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK (This);
  IfrNvData = AllocateZeroPool (sizeof (ISCSI_CONFIG_IFR_NVDATA));
  if (IfrNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  if (Private->Current != NULL) {
    IScsiConvertAttemptConfigDataToIfrNvData (Private->Current, IfrNvData);
  }

  BufferSize    = ISCSI_NAME_MAX_SIZE;
  InitiatorName = (CHAR8 *) AllocateZeroPool (BufferSize);
  if (InitiatorName == NULL) {
    FreePool (IfrNvData);
    return EFI_OUT_OF_RESOURCES;
  }
    
  Status = gIScsiInitiatorName.Get (&gIScsiInitiatorName, &BufferSize, InitiatorName);
  if (EFI_ERROR (Status)) {
    IfrNvData->InitiatorName[0] = L'\0';
  } else {
    AsciiStrToUnicodeStr (InitiatorName, IfrNvData->InitiatorName);
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig().
  //
  BufferSize = sizeof (ISCSI_CONFIG_IFR_NVDATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gIScsiConfigGuid, mVendorStorageName, Private->DriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) IfrNvData,
                                BufferSize,
                                Results,
                                Progress
                                );
  FreePool (IfrNvData);
  FreePool (InitiatorName);

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
  simplify the job.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param[in]  Configuration  A null-terminated Unicode string in
                             <ConfigString> format. 
  
  @param[out] Progress       A pointer to a string filled in with the
                             offset of the most recent '&' before the
                             first failing name / value pair (or the
                             beginning of the string if the failure
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
  if (This == NULL || Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gIScsiConfigGuid, mVendorStorageName)) {
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

  @param[in]       This          Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]       Action        Specifies the type of action taken by the browser.
  @param[in]       QuestionId    A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect. The format of the data tends to 
                                 vary based on the opcode that generated the callback.
  @param[in]       Type          The type of value for the question.
  @param[in, out]  Value         A pointer to the data being sent to the original
                                 exporting driver.
  @param[out]      ActionRequest On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.
**/
EFI_STATUS
EFIAPI
IScsiFormCallback (
  IN CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN        EFI_BROWSER_ACTION               Action,
  IN        EFI_QUESTION_ID                  QuestionId,
  IN        UINT8                            Type,
  IN OUT    EFI_IFR_TYPE_VALUE               *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST       *ActionRequest
  )
{
  ISCSI_FORM_CALLBACK_INFO    *Private;
  UINTN                       BufferSize;
  CHAR8                       *IScsiName;
  CHAR8                       IpString[IP_STR_MAX_SIZE];
  CHAR8                       LunString[ISCSI_LUN_STR_MAX_LEN];
  UINT64                      Lun;
  EFI_IP_ADDRESS              HostIp;
  EFI_IP_ADDRESS              SubnetMask;
  EFI_IP_ADDRESS              Gateway;
  ISCSI_CONFIG_IFR_NVDATA     *IfrNvData;
  ISCSI_CONFIG_IFR_NVDATA     OldIfrNvData;
  EFI_STATUS                  Status;
  CHAR16                      AttemptName[ATTEMPT_NAME_SIZE + 4];
  EFI_INPUT_KEY               Key;

  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) || (Action == EFI_BROWSER_ACTION_FORM_CLOSE)) {
    //
    // Do nothing for UEFI OPEN/CLOSE Action
    //
    return EFI_SUCCESS;
  }

  if ((Action != EFI_BROWSER_ACTION_CHANGING) && (Action != EFI_BROWSER_ACTION_CHANGED)) {
    //
    // All other type return unsupported.
    //
    return EFI_UNSUPPORTED;
  }

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = ISCSI_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK (This);
  
  //
  // Retrieve uncommitted data from Browser
  //
  
  BufferSize = sizeof (ISCSI_CONFIG_IFR_NVDATA);
  IfrNvData = AllocateZeroPool (BufferSize);
  if (IfrNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  IScsiName = (CHAR8 *) AllocateZeroPool (ISCSI_NAME_MAX_SIZE);
  if (IScsiName == NULL) {
    FreePool (IfrNvData);
    return EFI_OUT_OF_RESOURCES;
  }
  
  Status = EFI_SUCCESS;
  
  ZeroMem (&OldIfrNvData, BufferSize);
  
  HiiGetBrowserData (NULL, NULL, BufferSize, (UINT8 *) IfrNvData);
  
  CopyMem (&OldIfrNvData, IfrNvData, BufferSize);

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (QuestionId) {
    case KEY_ADD_ATTEMPT:
      //
      // Check whether iSCSI initiator name is configured already.
      //
      mPrivate->InitiatorNameLength = ISCSI_NAME_MAX_SIZE;
      Status = gIScsiInitiatorName.Get (
                                     &gIScsiInitiatorName,
                                     &mPrivate->InitiatorNameLength,
                                     mPrivate->InitiatorName
                                     );
      if (EFI_ERROR (Status)) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Error: please configure iSCSI initiator name first!",
          NULL
          );        
        break;
      }
      
      Status = IScsiConfigAddAttempt ();
      break;

    case KEY_DELETE_ATTEMPT:
      CopyMem (
        OldIfrNvData.DeleteAttemptList,
        IfrNvData->DeleteAttemptList,
        sizeof (IfrNvData->DeleteAttemptList)
        );
      Status = IScsiConfigDisplayDeleteAttempts (IfrNvData);
      break;

    case KEY_ORDER_ATTEMPT_CONFIG:
      //
      // Order the attempt according to user input.
      //
      CopyMem (
        OldIfrNvData.DynamicOrderedList,
        IfrNvData->DynamicOrderedList,
        sizeof (IfrNvData->DynamicOrderedList)
        );
      IScsiConfigDisplayOrderAttempts ();
      break;
    
    default:
      Status = IScsiConfigProcessDefault (QuestionId, IfrNvData);
      break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {  
    switch (QuestionId) {
    case KEY_INITIATOR_NAME:
      UnicodeStrToAsciiStr (IfrNvData->InitiatorName, IScsiName);
      BufferSize  = AsciiStrSize (IScsiName);

      Status      = gIScsiInitiatorName.Set (&gIScsiInitiatorName, &BufferSize, IScsiName);
      if (EFI_ERROR (Status)) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid iSCSI Name!",
          NULL
          );      
      }

      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
      break;
    case KEY_ATTEMPT_NAME:
      if (StrLen (IfrNvData->AttemptName) > ATTEMPT_NAME_SIZE) {
        CopyMem (AttemptName, IfrNvData->AttemptName, ATTEMPT_NAME_SIZE * sizeof (CHAR16));
        CopyMem (&AttemptName[ATTEMPT_NAME_SIZE], L"...", 4 * sizeof (CHAR16));
      } else {
        CopyMem (
          AttemptName,
          IfrNvData->AttemptName,
          (StrLen (IfrNvData->AttemptName) + 1) * sizeof (CHAR16)
          );
      }

      UnicodeStrToAsciiStr (IfrNvData->AttemptName, Private->Current->AttemptName);

      IScsiConfigUpdateAttempt ();

      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
      break;
      
    case KEY_SAVE_ATTEMPT_CONFIG:
      Status = IScsiConvertIfrNvDataToAttemptConfigData (IfrNvData, Private->Current);
      if (EFI_ERROR (Status)) {
        break;
      }

      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
      break;

    case KEY_SAVE_ORDER_CHANGES:
      //
      // Sync the Attempt Order to NVR.
      //
      Status = IScsiConfigOrderAttempts (IfrNvData);
      if (EFI_ERROR (Status)) {
        break;
      }

      IScsiConfigUpdateAttempt ();
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
      break;

    case KEY_IGNORE_ORDER_CHANGES:
      CopyMem (
        IfrNvData->DynamicOrderedList,
        OldIfrNvData.DynamicOrderedList,
        sizeof (IfrNvData->DynamicOrderedList)
        );
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
      break;

    case KEY_SAVE_DELETE_ATTEMPT:
      //
      // Delete the Attempt Order from NVR
      //
      Status = IScsiConfigDeleteAttempts (IfrNvData);
      if (EFI_ERROR (Status)) {
        break;
      }

      IScsiConfigUpdateAttempt ();
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
      break;

    case KEY_IGNORE_DELETE_ATTEMPT:
      CopyMem (
        IfrNvData->DeleteAttemptList,
        OldIfrNvData.DeleteAttemptList,
        sizeof (IfrNvData->DeleteAttemptList)
        );
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
      break;

    case KEY_IP_MODE:
      switch (Value->u8) {
      case IP_MODE_IP6:
        ZeroMem (IfrNvData->TargetIp, sizeof (IfrNvData->TargetIp));
        IScsiIpToStr (&Private->Current->SessionConfigData.TargetIp, TRUE, IfrNvData->TargetIp);
        Private->Current->AutoConfigureMode = 0;
        break;

      case IP_MODE_IP4:
        ZeroMem (IfrNvData->TargetIp, sizeof (IfrNvData->TargetIp));
        IScsiIpToStr (&Private->Current->SessionConfigData.TargetIp, FALSE, IfrNvData->TargetIp);
        Private->Current->AutoConfigureMode = 0;

        break;
      }

      break;

    case KEY_LOCAL_IP:
      Status = NetLibStrToIp4 (IfrNvData->LocalIp, &HostIp.v4);
      if (EFI_ERROR (Status) || !NetIp4IsUnicast (NTOHL (HostIp.Addr[0]), 0)) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid IP address!",
          NULL
          ); 
        
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Private->Current->SessionConfigData.LocalIp, &HostIp.v4, sizeof (HostIp.v4));
      }

      break;

    case KEY_SUBNET_MASK:
      Status = NetLibStrToIp4 (IfrNvData->SubnetMask, &SubnetMask.v4);
      if (EFI_ERROR (Status) || ((SubnetMask.Addr[0] != 0) && (IScsiGetSubnetMaskPrefixLength (&SubnetMask.v4) == 0))) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid Subnet Mask!",
          NULL
          ); 
        
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Private->Current->SessionConfigData.SubnetMask, &SubnetMask.v4, sizeof (SubnetMask.v4));
      }

      break;

    case KEY_GATE_WAY:
      Status = NetLibStrToIp4 (IfrNvData->Gateway, &Gateway.v4);
      if (EFI_ERROR (Status) || ((Gateway.Addr[0] != 0) && !NetIp4IsUnicast (NTOHL (Gateway.Addr[0]), 0))) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid Gateway!",
          NULL
          );       
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Private->Current->SessionConfigData.Gateway, &Gateway.v4, sizeof (Gateway.v4));
      }

      break;

    case KEY_TARGET_IP:
      UnicodeStrToAsciiStr (IfrNvData->TargetIp, IpString);
      Status = IScsiAsciiStrToIp (IpString, IfrNvData->IpMode, &HostIp);
      if (EFI_ERROR (Status) || !IpIsUnicast (&HostIp, IfrNvData->IpMode)) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid IP address!",
          NULL
          );       
        Status = EFI_INVALID_PARAMETER;
      } else {
        CopyMem (&Private->Current->SessionConfigData.TargetIp, &HostIp, sizeof (HostIp));
      }

      break;

    case KEY_TARGET_NAME:
      UnicodeStrToAsciiStr (IfrNvData->TargetName, IScsiName);
      Status = IScsiNormalizeName (IScsiName, AsciiStrLen (IScsiName));
      if (EFI_ERROR (Status)) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid iSCSI Name!",
          NULL
          );
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
      UnicodeStrToAsciiStr (IfrNvData->BootLun, LunString);
      Status = IScsiAsciiStrToLun (LunString, (UINT8 *) &Lun);
      if (EFI_ERROR (Status)) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid LUN string!",
          NULL
          );       
      } else {
        CopyMem (Private->Current->SessionConfigData.BootLun, &Lun, sizeof (Lun));
      }

      break;

    case KEY_AUTH_TYPE:
      switch (Value->u8) {
      case ISCSI_AUTH_TYPE_CHAP:
        IfrNvData->CHAPType = ISCSI_CHAP_UNI;
        break;
      default:
        break;
      }

      break;

    case KEY_CHAP_NAME:
      UnicodeStrToAsciiStr (
        IfrNvData->CHAPName,
        Private->Current->AuthConfigData.CHAP.CHAPName
        );
      break;

    case KEY_CHAP_SECRET:
      UnicodeStrToAsciiStr (
        IfrNvData->CHAPSecret,
        Private->Current->AuthConfigData.CHAP.CHAPSecret
        );
      break;

    case KEY_REVERSE_CHAP_NAME:
      UnicodeStrToAsciiStr (
        IfrNvData->ReverseCHAPName,
        Private->Current->AuthConfigData.CHAP.ReverseCHAPName
        );
      break;

    case KEY_REVERSE_CHAP_SECRET:
      UnicodeStrToAsciiStr (
        IfrNvData->ReverseCHAPSecret,
        Private->Current->AuthConfigData.CHAP.ReverseCHAPSecret
        );
      break;

    case KEY_CONFIG_ISID:
      IScsiParseIsIdFromString (IfrNvData->IsId, Private->Current->SessionConfigData.IsId);
      IScsiConvertIsIdToString (IfrNvData->IsId, Private->Current->SessionConfigData.IsId);

      break;

    default:
      break;
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // Pass changed uncommitted data back to Form Browser.
    //
    BufferSize = sizeof (ISCSI_CONFIG_IFR_NVDATA);
    HiiSetBrowserData (NULL, NULL, BufferSize, (UINT8 *) IfrNvData, NULL);
  }

  FreePool (IfrNvData);
  FreePool (IScsiName);

  return Status;
}


/**
  Initialize the iSCSI configuration form.

  @param[in]  DriverBindingHandle The iSCSI driverbinding handle.

  @retval EFI_SUCCESS             The iSCSI configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.

**/
EFI_STATUS
IScsiConfigFormInit (
  IN EFI_HANDLE  DriverBindingHandle
  )
{
  EFI_STATUS                  Status;
  ISCSI_FORM_CALLBACK_INFO    *CallbackInfo;

  CallbackInfo = (ISCSI_FORM_CALLBACK_INFO *) AllocateZeroPool (sizeof (ISCSI_FORM_CALLBACK_INFO));
  if (CallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CallbackInfo->Signature   = ISCSI_FORM_CALLBACK_INFO_SIGNATURE;
  CallbackInfo->Current     = NULL;

  CallbackInfo->ConfigAccess.ExtractConfig = IScsiFormExtractConfig;
  CallbackInfo->ConfigAccess.RouteConfig   = IScsiFormRouteConfig;
  CallbackInfo->ConfigAccess.Callback      = IScsiFormCallback;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle.
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
  // Publish our HII data.
  //
  CallbackInfo->RegisteredHandle = HiiAddPackages (
                                     &gIScsiConfigGuid,
                                     CallbackInfo->DriverHandle,
                                     IScsiDxeStrings,
                                     IScsiConfigVfrBin,
                                     NULL
                                     );
  if (CallbackInfo->RegisteredHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           &CallbackInfo->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mIScsiHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &CallbackInfo->ConfigAccess,
           NULL
           );
    FreePool(CallbackInfo);
    return EFI_OUT_OF_RESOURCES;
  }

  mCallbackInfo = CallbackInfo;

  return EFI_SUCCESS;
}


/**
  Unload the iSCSI configuration form, this includes: delete all the iSCSI
  configuration entries, uninstall the form callback protocol, and
  free the resources used.

  @param[in]  DriverBindingHandle The iSCSI driverbinding handle.

  @retval EFI_SUCCESS             The iSCSI configuration form is unloaded.
  @retval Others                  Failed to unload the form.

**/
EFI_STATUS
IScsiConfigFormUnload (
  IN EFI_HANDLE  DriverBindingHandle
  )
{
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptConfigData;
  ISCSI_NIC_INFO              *NicInfo;
  LIST_ENTRY                  *Entry;
  EFI_STATUS                  Status;

  while (!IsListEmpty (&mPrivate->AttemptConfigs)) {
    Entry = NetListRemoveHead (&mPrivate->AttemptConfigs);
    AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
    FreePool (AttemptConfigData);
    mPrivate->AttemptCount--;
  }

  ASSERT (mPrivate->AttemptCount == 0);

  while (!IsListEmpty (&mPrivate->NicInfoList)) {
    Entry = NetListRemoveHead (&mPrivate->NicInfoList);
    NicInfo = NET_LIST_USER_STRUCT (Entry, ISCSI_NIC_INFO, Link);
    FreePool (NicInfo);
    mPrivate->NicCount--;
  }

  ASSERT (mPrivate->NicCount == 0);

  //
  // Free attempt is created but not saved to system.
  //
  if (mPrivate->NewAttempt != NULL) {
    FreePool (mPrivate->NewAttempt);
  }

  FreePool (mPrivate);
  mPrivate = NULL;

  //
  // Remove HII package list.
  //
  HiiRemovePackages (mCallbackInfo->RegisteredHandle);

  //
  // Uninstall Device Path Protocol and Config Access protocol.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  mCallbackInfo->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mIScsiHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mCallbackInfo->ConfigAccess,
                  NULL
                  );

  FreePool (mCallbackInfo);

  return Status;
}
