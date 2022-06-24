/** @file
  The Hii functions for WiFi Connection Manager.

  Copyright (c) 2019 - 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "WifiConnectionMgrDxe.h"

CHAR16  mVendorStorageName[] = L"WIFI_MANAGER_IFR_NVDATA";

HII_VENDOR_DEVICE_PATH  mWifiMgrDxeHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    WIFI_CONNECTION_MANAGER_CONFIG_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

//
// HII Config Access Protocol instance
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_HII_CONFIG_ACCESS_PROTOCOL  gWifiMgrDxeHiiConfigAccess = {
  WifiMgrDxeHiiConfigAccessExtractConfig,
  WifiMgrDxeHiiConfigAccessRouteConfig,
  WifiMgrDxeHiiConfigAccessCallback
};

CHAR16  *mSecurityType[] = {
  L"OPEN           ",
  L"WPA-Enterprise ",
  L"WPA2-Enterprise",
  L"WPA-Personal   ",
  L"WPA2-Personal  ",
  L"WEP            ",
  L"WPA3-Personal  ",
  L"WPA3-Enterprise",
  L"UnKnown        "
};

CHAR16  *mSignalStrengthBar[] = {
  L"[-----]",
  L"[*----]",
  L"[**---]",
  L"[***--]",
  L"[****-]",
  L"[*****]"
};

#define  RSSI_TO_SIGNAL_STRENGTH_BAR(Rssi)  mSignalStrengthBar[((Rssi + 19)/20)]

#define  NET_LIST_FOR_EACH_FROM_NODE(Entry, Node, ListHead) \
  for(Entry = Node->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

extern EFI_GUID  gWifiConfigFormSetGuid;

/**
  Create Hii Extend Label OpCode as the start opcode and end opcode.
  The caller is responsible for freeing the OpCode with HiiFreeOpCodeHandle().

  @param[in]  StartLabelNumber   The number of start label.
  @param[out] StartOpCodeHandle  Points to the start opcode handle.
  @param[out] EndOpCodeHandle    Points to the end opcode handle.

  @retval EFI_OUT_OF_RESOURCES   Do not have sufficient resource to finish this
                                 operation.
  @retval EFI_INVALID_PARAMETER  Any input parameter is invalid.
  @retval EFI_SUCCESS            The operation is completed successfully.
  @retval Other Errors           Returned errors when updating the HII form.

**/
EFI_STATUS
WifiMgrCreateOpCode (
  IN  UINT16  StartLabelNumber,
  OUT VOID    **StartOpCodeHandle,
  OUT VOID    **EndOpCodeHandle
  )
{
  EFI_STATUS          Status;
  EFI_IFR_GUID_LABEL  *InternalStartLabel;
  EFI_IFR_GUID_LABEL  *InternalEndLabel;

  if ((StartOpCodeHandle == NULL) || (EndOpCodeHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status             = EFI_OUT_OF_RESOURCES;
  *StartOpCodeHandle = NULL;
  *EndOpCodeHandle   = NULL;

  //
  // Initialize the container for dynamic opcodes.
  //
  *StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (*StartOpCodeHandle == NULL) {
    goto Exit;
  }

  *EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (*EndOpCodeHandle == NULL) {
    goto Exit;
  }

  //
  // Create Hii Extend Label OpCode as the start opcode.
  //
  InternalStartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
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
  InternalEndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
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
  Display the Nic list contains all available Nics.

  @param[in]  Private            The pointer to the global private data structure.

  @retval EFI_INVALID_PARAMETER  Any input parameter is invalid.
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
WifiMgrShowNicList (
  IN  WIFI_MGR_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS            Status;
  CHAR16                MacString[WIFI_MGR_MAX_MAC_STRING_LEN];
  CHAR16                PortString[WIFI_STR_MAX_SIZE];
  EFI_STRING_ID         PortTitleToken;
  EFI_STRING_ID         PortTitleHelpToken;
  WIFI_MGR_DEVICE_DATA  *Nic;
  LIST_ENTRY            *Entry;
  VOID                  *StartOpCodeHandle;
  VOID                  *EndOpCodeHandle;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = WifiMgrCreateOpCode (
             LABEL_MAC_ENTRY,
             &StartOpCodeHandle,
             &EndOpCodeHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NET_LIST_FOR_EACH (Entry, &Private->NicList) {
    Nic = NET_LIST_USER_STRUCT_S (Entry, WIFI_MGR_DEVICE_DATA, Link, WIFI_MGR_DEVICE_DATA_SIGNATURE);
    WifiMgrMacAddrToStr (&Nic->MacAddress, sizeof (MacString), MacString);
    UnicodeSPrint (PortString, sizeof (PortString), L"MAC %s", MacString);
    PortTitleToken = HiiSetString (
                       Private->RegisteredHandle,
                       0,
                       PortString,
                       NULL
                       );
    if (PortTitleToken == 0) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    UnicodeSPrint (PortString, sizeof (PortString), L"MAC Address");
    PortTitleHelpToken = HiiSetString (
                           Private->RegisteredHandle,
                           0,
                           PortString,
                           NULL
                           );
    if (PortTitleHelpToken == 0) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    HiiCreateGotoOpCode (
      StartOpCodeHandle,
      FORMID_WIFI_MAINPAGE,
      PortTitleToken,
      PortTitleHelpToken,
      EFI_IFR_FLAG_CALLBACK,
      (UINT16)(KEY_MAC_ENTRY_BASE + Nic->NicIndex)
      );
  }

  Status = HiiUpdateForm (
             Private->RegisteredHandle,       // HII handle
             &gWifiConfigFormSetGuid,         // Formset GUID
             FORMID_MAC_SELECTION,            // Form ID
             StartOpCodeHandle,               // Label for where to insert opcodes
             EndOpCodeHandle                  // Replace data
             );

Exit:

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  return Status;
}

/**
  Retreive the unicode string of the AKM Suite list of a profile.
  The caller is responsible for freeing the string with FreePool().

  @param[in]  Profile           The network profile contains a AKM suite list.

  @return the unicode string of AKM suite list or "None".

**/
CHAR16 *
WifiMgrGetStrAKMList (
  IN  WIFI_MGR_NETWORK_PROFILE  *Profile
  )
{
  UINT8   Index;
  UINT16  AKMSuiteCount;
  CHAR16  *AKMListDisplay;
  UINTN   Length;

  AKMListDisplay = NULL;
  if ((Profile == NULL) || (Profile->Network.AKMSuite == NULL)) {
    goto Exit;
  }

  AKMSuiteCount = Profile->Network.AKMSuite->AKMSuiteCount;
  if (AKMSuiteCount != 0) {
    //
    // Current AKM Suite is between 1-18
    //
    AKMListDisplay = (CHAR16 *)AllocateZeroPool (sizeof (CHAR16) * (AKMSuiteCount * 3 + 1));
    Length         = 0;
    if (AKMListDisplay != NULL) {
      for (Index = 0; Index < AKMSuiteCount; Index++) {
        //
        // The size of buffer should be 4 CHAR16 for Null-terminated Unicode string.
        //
        UnicodeSPrint (
          AKMListDisplay + Length,
          sizeof (CHAR16) * 4,
          L"%d ",
          Profile->Network.AKMSuite->AKMSuiteList[Index].SuiteType
          );
        Length = StrLen (AKMListDisplay + Length) + Length;
        if (Index == AKMSuiteCount - 1) {
          *(AKMListDisplay + (Length - 1)) = L'\0';
        }
      }
    }
  }

Exit:

  if (AKMListDisplay == NULL) {
    AKMListDisplay = AllocateCopyPool (sizeof (L"None"), L"None");
  }

  return AKMListDisplay;
}

/**
  Retreive the unicode string of the Cipher Suite list of a profile.
  The caller is responsible for freeing the string with FreePool().

  @param[in]  Profile           The network profile contains a Cipher suite list.

  @return the unicode string of Cipher suite list or "None".

**/
CHAR16 *
WifiMgrGetStrCipherList (
  IN  WIFI_MGR_NETWORK_PROFILE  *Profile
  )
{
  UINT8   Index;
  UINT16  CipherSuiteCount;
  CHAR16  *CipherListDisplay;

  CipherListDisplay = NULL;
  if ((Profile == NULL) || (Profile->Network.CipherSuite == NULL)) {
    goto Exit;
  }

  CipherSuiteCount = Profile->Network.CipherSuite->CipherSuiteCount;
  if (CipherSuiteCount != 0) {
    //
    // Current Cipher Suite is between 1-9
    //
    CipherListDisplay = (CHAR16 *)AllocateZeroPool (sizeof (CHAR16) * (CipherSuiteCount * 2 + 1));
    if (CipherListDisplay != NULL) {
      for (Index = 0; Index < CipherSuiteCount; Index++) {
        //
        // The size of buffer should be 3 CHAR16 for Null-terminated Unicode string.
        // The first char is the Cipher Suite number, the second char is ' ', the third char is '\0'.
        //
        UnicodeSPrint (
          CipherListDisplay + (Index * 2),
          sizeof (CHAR16) * 3,
          L"%d ",
          Profile->Network.CipherSuite->CipherSuiteList[Index].SuiteType
          );
        if (Index == CipherSuiteCount - 1) {
          *(CipherListDisplay + (Index * 2 + 1)) = L'\0';
        }
      }
    }
  }

Exit:

  if (CipherListDisplay == NULL) {
    CipherListDisplay = AllocateCopyPool (sizeof (L"None"), L"None");
  }

  return CipherListDisplay;
}

/**
  Refresh the network list display of the current Nic.

  @param[in]   Private           The pointer to the global private data structure.
  @param[out]  IfrNvData         The IFR NV data.

  @retval EFI_SUCCESS            The operation is completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Other Errors           Returned errors when creating Opcodes or updating the
                                 Hii form.

**/
EFI_STATUS
WifiMgrRefreshNetworkList (
  IN    WIFI_MGR_PRIVATE_DATA    *Private,
  OUT   WIFI_MANAGER_IFR_NVDATA  *IfrNvData
  )
{
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  UINT32                    AvailableCount;
  EFI_STRING_ID             PortPromptToken;
  EFI_STRING_ID             PortTextToken;
  EFI_STRING_ID             PortHelpToken;
  WIFI_MGR_NETWORK_PROFILE  *Profile;
  LIST_ENTRY                *Entry;
  VOID                      *StartOpCodeHandle;
  VOID                      *EndOpCodeHandle;
  CHAR16                    *AKMListDisplay;
  CHAR16                    *CipherListDisplay;
  CHAR16                    PortString[WIFI_STR_MAX_SIZE];
  UINTN                     PortStringSize;
  WIFI_MGR_NETWORK_PROFILE  *ConnectedProfile;

  if (Private->CurrentNic == NULL) {
    return EFI_SUCCESS;
  }

  Status = WifiMgrCreateOpCode (
             LABEL_NETWORK_LIST_ENTRY,
             &StartOpCodeHandle,
             &EndOpCodeHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OldTpl            = gBS->RaiseTPL (TPL_CALLBACK);
  AvailableCount    = 0;
  PortStringSize    = sizeof (PortString);
  ConnectedProfile  = NULL;
  AKMListDisplay    = NULL;
  CipherListDisplay = NULL;

  if (Private->CurrentNic->ConnectState == WifiMgrConnectedToAp) {
    //
    // Display the current connected network.
    // Find the current operate network under connected status.
    //
    if ((Private->CurrentNic->CurrentOperateNetwork != NULL) &&
        Private->CurrentNic->CurrentOperateNetwork->IsAvailable)
    {
      Profile = Private->CurrentNic->CurrentOperateNetwork;
      AvailableCount++;

      AKMListDisplay = WifiMgrGetStrAKMList (Profile);
      if (AKMListDisplay == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      CipherListDisplay = WifiMgrGetStrCipherList (Profile);
      if (CipherListDisplay == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      UnicodeSPrint (PortString, PortStringSize, L"%s (Connected)", Profile->SSId);
      PortPromptToken = HiiSetString (Private->RegisteredHandle, 0, PortString, NULL);

      if (Profile->SecurityType == SECURITY_TYPE_NONE) {
        PortHelpToken = 0;
      } else {
        UnicodeSPrint (PortString, PortStringSize, L"AKMSuite: %s CipherSuite: %s", AKMListDisplay, CipherListDisplay);
        PortHelpToken = HiiSetString (Private->RegisteredHandle, 0, PortString, NULL);
      }

      FreePool (AKMListDisplay);
      FreePool (CipherListDisplay);
      AKMListDisplay    = NULL;
      CipherListDisplay = NULL;

      HiiCreateGotoOpCode (
        StartOpCodeHandle,
        FORMID_CONNECT_NETWORK,
        PortPromptToken,
        PortHelpToken,
        EFI_IFR_FLAG_CALLBACK,
        (UINT16)(KEY_AVAILABLE_NETWORK_ENTRY_BASE + Profile->ProfileIndex)
        );

      UnicodeSPrint (
        PortString,
        PortStringSize,
        L"%s       %s %s",
        (Profile->SecurityType != SECURITY_TYPE_NONE ? L"Secured" : L"Open   "),
        mSecurityType[Profile->SecurityType],
        RSSI_TO_SIGNAL_STRENGTH_BAR (Profile->NetworkQuality)
        );
      PortTextToken = HiiSetString (Private->RegisteredHandle, 0, PortString, NULL);

      HiiCreateTextOpCode (
        StartOpCodeHandle,
        PortTextToken,
        0,
        0
        );

      ConnectedProfile = Profile;
    } else {
      Private->CurrentNic->HasDisconnectPendingNetwork = TRUE;
    }
  }

  //
  // Display all supported available networks.
  //
  NET_LIST_FOR_EACH (Entry, &Private->CurrentNic->ProfileList) {
    Profile = NET_LIST_USER_STRUCT_S (
                Entry,
                WIFI_MGR_NETWORK_PROFILE,
                Link,
                WIFI_MGR_PROFILE_SIGNATURE
                );
    if (ConnectedProfile == Profile) {
      continue;
    }

    if (Profile->IsAvailable && Profile->CipherSuiteSupported) {
      AvailableCount++;

      AKMListDisplay = WifiMgrGetStrAKMList (Profile);
      if (AKMListDisplay == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      CipherListDisplay = WifiMgrGetStrCipherList (Profile);
      if (CipherListDisplay == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      PortPromptToken = HiiSetString (Private->RegisteredHandle, 0, Profile->SSId, NULL);
      if (PortPromptToken == 0) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      if (Profile->SecurityType == SECURITY_TYPE_NONE) {
        PortHelpToken = 0;
      } else {
        UnicodeSPrint (
          PortString,
          PortStringSize,
          L"AKMSuite: %s CipherSuite: %s",
          AKMListDisplay,
          CipherListDisplay
          );
        PortHelpToken = HiiSetString (Private->RegisteredHandle, 0, PortString, NULL);
        if (PortHelpToken == 0) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Exit;
        }
      }

      FreePool (AKMListDisplay);
      FreePool (CipherListDisplay);
      AKMListDisplay    = NULL;
      CipherListDisplay = NULL;

      HiiCreateGotoOpCode (
        StartOpCodeHandle,
        FORMID_CONNECT_NETWORK,
        PortPromptToken,
        PortHelpToken,
        EFI_IFR_FLAG_CALLBACK,
        (UINT16)(KEY_AVAILABLE_NETWORK_ENTRY_BASE + Profile->ProfileIndex)
        );

      UnicodeSPrint (
        PortString,
        PortStringSize,
        L"%s       %s %s",
        (Profile->SecurityType != SECURITY_TYPE_NONE ? L"Secured" : L"Open   "),
        mSecurityType[Profile->SecurityType],
        RSSI_TO_SIGNAL_STRENGTH_BAR (Profile->NetworkQuality)
        );
      PortTextToken = HiiSetString (Private->RegisteredHandle, 0, PortString, NULL);
      if (PortTextToken == 0) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      HiiCreateTextOpCode (
        StartOpCodeHandle,
        PortTextToken,
        0,
        0
        );
    }
  }

  //
  // Display all Unsupported available networks.
  //
  NET_LIST_FOR_EACH (Entry, &Private->CurrentNic->ProfileList) {
    Profile = NET_LIST_USER_STRUCT_S (
                Entry,
                WIFI_MGR_NETWORK_PROFILE,
                Link,
                WIFI_MGR_PROFILE_SIGNATURE
                );
    if (ConnectedProfile == Profile) {
      continue;
    }

    if (Profile->IsAvailable && !Profile->CipherSuiteSupported) {
      AvailableCount++;

      AKMListDisplay = WifiMgrGetStrAKMList (Profile);
      if (AKMListDisplay == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      CipherListDisplay = WifiMgrGetStrCipherList (Profile);
      if (CipherListDisplay == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      PortPromptToken = HiiSetString (Private->RegisteredHandle, 0, Profile->SSId, NULL);

      if (Profile->AKMSuiteSupported) {
        UnicodeSPrint (
          PortString,
          PortStringSize,
          L"AKMSuite: %s CipherSuite(UnSupported): %s",
          AKMListDisplay,
          CipherListDisplay
          );
      } else {
        UnicodeSPrint (
          PortString,
          PortStringSize,
          L"AKMSuite(UnSupported): %s CipherSuite(UnSupported): %s",
          AKMListDisplay,
          CipherListDisplay
          );
      }

      FreePool (AKMListDisplay);
      FreePool (CipherListDisplay);
      AKMListDisplay    = NULL;
      CipherListDisplay = NULL;

      PortHelpToken = HiiSetString (Private->RegisteredHandle, 0, PortString, NULL);

      HiiCreateGotoOpCode (
        StartOpCodeHandle,
        FORMID_CONNECT_NETWORK,
        PortPromptToken,
        PortHelpToken,
        EFI_IFR_FLAG_CALLBACK,
        (UINT16)(KEY_AVAILABLE_NETWORK_ENTRY_BASE + Profile->ProfileIndex)
        );

      UnicodeSPrint (
        PortString,
        PortStringSize,
        L"%s       %s %s",
        L"UnSupported",
        mSecurityType[Profile->SecurityType],
        RSSI_TO_SIGNAL_STRENGTH_BAR (Profile->NetworkQuality)
        );
      PortTextToken = HiiSetString (Private->RegisteredHandle, 0, PortString, NULL);

      HiiCreateTextOpCode (
        StartOpCodeHandle,
        PortTextToken,
        0,
        0
        );
    }
  }

  Status = HiiUpdateForm (
             Private->RegisteredHandle,       // HII handle
             &gWifiConfigFormSetGuid,         // Formset GUID
             FORMID_NETWORK_LIST,             // Form ID
             StartOpCodeHandle,               // Label for where to insert opcodes
             EndOpCodeHandle                  // Replace data
             );

Exit:

  gBS->RestoreTPL (OldTpl);

  if (AKMListDisplay != NULL) {
    FreePool (AKMListDisplay);
  }

  if (CipherListDisplay != NULL) {
    FreePool (CipherListDisplay);
  }

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  DEBUG ((DEBUG_INFO, "[WiFi Connection Manager] Network List is Refreshed!\n"));
  return Status;
}

/**
  Refresh the hidden network list configured by user.

  @param[in]   Private           The pointer to the global private data structure.

  @retval EFI_SUCCESS            The operation is completed successfully.
  @retval Other Errors           Returned errors when creating Opcodes or updating the
                                 Hii form.
**/
EFI_STATUS
WifiMgrRefreshHiddenList (
  IN    WIFI_MGR_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  UINTN                     Index;
  EFI_STRING_ID             StringId;
  VOID                      *StartOpCodeHandle;
  VOID                      *EndOpCodeHandle;
  WIFI_HIDDEN_NETWORK_DATA  *HiddenNetwork;
  LIST_ENTRY                *Entry;

  if (Private == NULL) {
    return EFI_SUCCESS;
  }

  Status = WifiMgrCreateOpCode (
             LABEL_HIDDEN_NETWORK_ENTRY,
             &StartOpCodeHandle,
             &EndOpCodeHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  Index  = 0;

  NET_LIST_FOR_EACH (Entry, &Private->HiddenNetworkList) {
    HiddenNetwork = NET_LIST_USER_STRUCT_S (
                      Entry,
                      WIFI_HIDDEN_NETWORK_DATA,
                      Link,
                      WIFI_MGR_HIDDEN_NETWORK_SIGNATURE
                      );
    StringId = HiiSetString (Private->RegisteredHandle, 0, HiddenNetwork->SSId, NULL);

    HiiCreateCheckBoxOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID)(KEY_HIDDEN_NETWORK_ENTRY_BASE + Index),
      MANAGER_VARSTORE_ID,
      (UINT16)(HIDDEN_NETWORK_LIST_VAR_OFFSET + Index),
      StringId,
      0,
      0,
      0,
      NULL
      );
    Index++;
  }

  Status = HiiUpdateForm (
             Private->RegisteredHandle,       // HII handle
             &gWifiConfigFormSetGuid,         // Formset GUID
             FORMID_HIDDEN_NETWORK_LIST,      // Form ID
             StartOpCodeHandle,               // Label for where to insert opcodes
             EndOpCodeHandle                  // Replace data
             );

  gBS->RestoreTPL (OldTpl);
  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  return Status;
}

/**
  Callback function for user to select a Nic.

  @param[in]  Private            The pointer to the global private data structure.
  @param[in]  KeyValue           The key value received from HII input.

  @retval EFI_NOT_FOUND          The corresponding Nic is not found.
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
WifiMgrSelectNic (
  IN     WIFI_MGR_PRIVATE_DATA  *Private,
  IN     EFI_QUESTION_ID        KeyValue
  )
{
  WIFI_MGR_DEVICE_DATA  *Nic;
  UINT32                NicIndex;
  CHAR16                MacString[WIFI_MGR_MAX_MAC_STRING_LEN];

  NicIndex = KeyValue - KEY_MAC_ENTRY_BASE;
  Nic      = WifiMgrGetNicByIndex (Private, NicIndex);
  if (Nic == NULL) {
    return EFI_NOT_FOUND;
  }

  Private->CurrentNic = Nic;

  WifiMgrMacAddrToStr (&Nic->MacAddress, sizeof (MacString), MacString);
  HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_MAC_ADDRESS), MacString, NULL);
  return EFI_SUCCESS;
}

/**
  Restore the NV data to be default.

  @param[in]  Private             The pointer to the global private data structure.
  @param[out] IfrNvData           The IFR NV data.

**/
VOID
WifiMgrCleanUserInput (
  IN  WIFI_MGR_PRIVATE_DATA  *Private
  )
{
  Private->SecurityType        = SECURITY_TYPE_NONE;
  Private->EapAuthMethod       = EAP_AUTH_METHOD_TTLS;
  Private->EapSecondAuthMethod = EAP_SEAUTH_METHOD_MSCHAPV2;
  Private->FileType            = FileTypeMax;
}

/**
  UI handle function when user select a network to connect.

  @param[in]  Private             The pointer to the global private data structure.
  @param[in]  ProfileIndex        The profile index user selected to connect.

  @retval EFI_INVALID_PARAMETER   Nic is null.
  @retval EFI_NOT_FOUND           Profile could not be found.
  @retval EFI_SUCCESS             The operation is completed successfully.

**/
EFI_STATUS
WifiMgrUserSelectProfileToConnect (
  IN     WIFI_MGR_PRIVATE_DATA  *Private,
  IN     UINT32                 ProfileIndex
  )
{
  WIFI_MGR_NETWORK_PROFILE  *Profile;
  WIFI_MGR_DEVICE_DATA      *Nic;

  Nic = Private->CurrentNic;
  if (Nic == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the connection page
  //
  WifiMgrCleanUserInput (Private);

  Profile = WifiMgrGetProfileByProfileIndex (ProfileIndex, &Nic->ProfileList);
  if (Profile == NULL) {
    return EFI_NOT_FOUND;
  }

  Private->CurrentNic->UserSelectedProfile = Profile;

  return EFI_SUCCESS;
}

/**
  Record password from a HII input string.

  @param[in]  Private             The pointer to the global private data structure.
  @param[in]  StringId            The QuestionId received from HII input.
  @param[in]  StringBuffer        The unicode string buffer to store password.
  @param[in]  StringBufferLen     The len of unicode string buffer.

  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.
  @retval EFI_NOT_FOUND           The password string is not found or invalid.
  @retval EFI_SUCCESS             The operation is completed successfully.

**/
EFI_STATUS
WifiMgrRecordPassword (
  IN   WIFI_MGR_PRIVATE_DATA  *Private,
  IN   EFI_STRING_ID          StringId,
  IN   CHAR16                 *StringBuffer,
  IN   UINTN                  StringBufferLen
  )
{
  CHAR16  *Password;

  if ((StringId == 0) || (StringBuffer == NULL) || (StringBufferLen <= 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Password = HiiGetString (Private->RegisteredHandle, StringId, NULL);
  if (Password == NULL) {
    return EFI_NOT_FOUND;
  }

  if (StrLen (Password) > StringBufferLen) {
    FreePool (Password);
    return EFI_NOT_FOUND;
  }

  StrnCpyS (StringBuffer, StringBufferLen, Password, StrLen (Password));
  ZeroMem (Password, (StrLen (Password) + 1) * sizeof (CHAR16));
  FreePool (Password);

  //
  // Clean password in string package
  //
  HiiSetString (Private->RegisteredHandle, StringId, L"", NULL);
  return EFI_SUCCESS;
}

/**
  Update connection message on connect configuration page, and trigger related form refresh.

  @param[in]   Nic                        The related Nic for updating message.
  @param[in]   ConnectStateChanged        The tag to tell if the connection state has been changed, only
                                          when the connection changes from "Connected" or "Disconnecting"
                                          to "Disconnected", or from "Disconnected" or "Connecting" to
                                          "Connected", this tag can be set as TRUE.
  @param[in]   ConnectStatusMessage       The message to show on connected status bar, if NULL, will
                                          use default message.

**/
VOID
WifiMgrUpdateConnectMessage (
  IN  WIFI_MGR_DEVICE_DATA  *Nic,
  IN  BOOLEAN               ConnectStateChanged,
  IN  EFI_STRING            ConnectStatusMessage
  )
{
  CHAR16                 ConnectStatusStr[WIFI_STR_MAX_SIZE];
  WIFI_MGR_PRIVATE_DATA  *Private;

  Private = Nic->Private;
  if ((Private == NULL) || (Private->CurrentNic != Nic)) {
    return;
  }

  //
  // Update Connection Status Bar
  //
  if (ConnectStatusMessage != NULL) {
    HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_CONNECT_STATUS), ConnectStatusMessage, NULL);
  } else {
    if (Nic->ConnectState == WifiMgrConnectedToAp) {
      UnicodeSPrint (
        ConnectStatusStr,
        sizeof (ConnectStatusStr),
        L"Connected to %s",
        Nic->CurrentOperateNetwork->SSId
        );
      HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_CONNECT_STATUS), ConnectStatusStr, NULL);
    } else if (Nic->ConnectState == WifiMgrDisconnected) {
      HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_CONNECT_STATUS), L"Disconnected", NULL);
    } else if (Nic->ConnectState == WifiMgrConnectingToAp) {
      UnicodeSPrint (
        ConnectStatusStr,
        sizeof (ConnectStatusStr),
        L"Connecting to %s ...",
        Nic->CurrentOperateNetwork->SSId
        );
      HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_CONNECT_STATUS), ConnectStatusStr, NULL);
    } else if (Nic->ConnectState == WifiMgrDisconnectingToAp) {
      UnicodeSPrint (
        ConnectStatusStr,
        sizeof (ConnectStatusStr),
        L"Disconnecting from %s ...",
        Nic->CurrentOperateNetwork->SSId
        );
      HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_CONNECT_STATUS), ConnectStatusStr, NULL);
    } else {
      return;
    }
  }

  //
  // Update Connect Button
  //
  if ((Nic->ConnectState == WifiMgrConnectedToAp) && (Nic->UserSelectedProfile == Nic->CurrentOperateNetwork)) {
    HiiSetString (
      Private->RegisteredHandle,
      STRING_TOKEN (STR_CONNECT_NOW),
      L"Disconnect from this Network",
      NULL
      );
  } else {
    HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_CONNECT_NOW), L"Connect to this Network", NULL);
  }

  gBS->SignalEvent (Private->ConnectFormRefreshEvent);

  //
  // Update Main Page and Network List
  //
  if (ConnectStateChanged) {
    if (Nic->ConnectState == WifiMgrConnectedToAp) {
      HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_CONNECTION_INFO), L"Connected to", NULL);
      HiiSetString (
        Private->RegisteredHandle,
        STRING_TOKEN (STR_CONNECTED_SSID),
        Nic->CurrentOperateNetwork->SSId,
        NULL
        );
    } else {
      HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_CONNECTION_INFO), L"Disconnected", NULL);
      HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_CONNECTED_SSID), L"", NULL);
    }

    gBS->SignalEvent (Private->NetworkListRefreshEvent);
    gBS->SignalEvent (Private->MainPageRefreshEvent);
  }
}

/**
  Convert the driver configuration data into the IFR data.

  @param[in]   Private            The pointer to the global private data structure.
  @param[out]  IfrNvData          The IFR NV data.

  @retval EFI_SUCCESS             The operation is completed successfully.

**/
EFI_STATUS
WifiMgrConvertConfigDataToIfrNvData (
  IN   WIFI_MGR_PRIVATE_DATA    *Private,
  OUT  WIFI_MANAGER_IFR_NVDATA  *IfrNvData
  )
{
  //
  // Private shouldn't be NULL here, assert if Private is NULL.
  //
  ASSERT (Private != NULL);

  if (Private->CurrentNic != NULL) {
    IfrNvData->ProfileCount = Private->CurrentNic->AvailableCount;
  } else {
    IfrNvData->ProfileCount = 0;
  }

  return EFI_SUCCESS;
}

/**
  Convert the IFR data into the driver configuration data.

  @param[in]       Private             The pointer to the global private data structure.
  @param[in, out]  IfrNvData           The IFR NV data.

  @retval EFI_SUCCESS                  The operation is completed successfully.

**/
EFI_STATUS
WifiMgrConvertIfrNvDataToConfigData (
  IN     WIFI_MGR_PRIVATE_DATA    *Private,
  IN OUT WIFI_MANAGER_IFR_NVDATA  *IfrNvData
  )
{
  return EFI_SUCCESS;
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

  @param This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param Request    A null-terminated Unicode string in
                    <ConfigRequest> format. Note that this
                    includes the routing information as well as
                    the configurable name / value pairs. It is
                    invalid for this string to be in
                    <MultiConfigRequest> format.
                    If a NULL is passed in for the Request field,
                    all of the settings being abstracted by this function
                    will be returned in the Results field.  In addition,
                    if a ConfigHdr is passed in with no request elements,
                    all of the settings being abstracted for that particular
                    ConfigHdr reference will be returned in the Results Field.

  @param Progress   On return, points to a character in the
                    Request string. Points to the string's null
                    terminator if request was successful. Points
                    to the most recent "&" before the first
                    failing name / value pair (or the beginning
                    of the string if the failure is in the first
                    name / value pair) if the request was not
                    successful.

  @param Results    A null-terminated Unicode string in
                    <MultiConfigAltResp> format which has all values
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
                                  to most recent "&" before the
                                  error or the beginning of the
                                  string.

  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question.

**/
EFI_STATUS
EFIAPI
WifiMgrDxeHiiConfigAccessExtractConfig (
  IN CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST  EFI_STRING                      Request,
  OUT       EFI_STRING                      *Progress,
  OUT       EFI_STRING                      *Results
  )
{
  WIFI_MGR_PRIVATE_DATA    *Private;
  WIFI_MANAGER_IFR_NVDATA  *IfrNvData;
  EFI_STRING               ConfigRequestHdr;
  EFI_STRING               ConfigRequest;
  UINTN                    Size;
  BOOLEAN                  AllocatedRequest;
  UINTN                    BufferSize;
  EFI_STATUS               Status;

  if ((This == NULL) || (Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) &&
      !HiiIsConfigHdrMatch (Request, &gWifiConfigFormSetGuid, mVendorStorageName))
  {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  Private = WIFI_MGR_PRIVATE_DATA_FROM_CONFIG_ACCESS (This);

  BufferSize = sizeof (WIFI_MANAGER_IFR_NVDATA);
  IfrNvData  = AllocateZeroPool (BufferSize);
  if (IfrNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  WifiMgrConvertConfigDataToIfrNvData (Private, IfrNvData);

  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator.
    //
    ConfigRequestHdr = HiiConstructConfigHdr (
                         &gWifiConfigFormSetGuid,
                         mVendorStorageName,
                         Private->DriverHandle
                         );
    if (ConfigRequestHdr == NULL) {
      FreePool (IfrNvData);
      return EFI_OUT_OF_RESOURCES;
    }

    Size          = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    if (ConfigRequest == NULL) {
      FreePool (IfrNvData);
      FreePool (ConfigRequestHdr);
      return EFI_OUT_OF_RESOURCES;
    }

    AllocatedRequest = TRUE;
    UnicodeSPrint (
      ConfigRequest,
      Size,
      L"%s&OFFSET=0&WIDTH=%016LX",
      ConfigRequestHdr,
      (UINT64)BufferSize
      );
    FreePool (ConfigRequestHdr);
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *)IfrNvData,
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
  simplify the job.

  @param This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param Configuration  A null-terminated Unicode string in
                        <ConfigString> format.

  @param Progress       A pointer to a string filled in with the
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
                                  was not found

**/
EFI_STATUS
EFIAPI
WifiMgrDxeHiiConfigAccessRouteConfig (
  IN CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST  EFI_STRING                      Configuration,
  OUT       EFI_STRING                      *Progress
  )
{
  EFI_STATUS               Status;
  UINTN                    BufferSize;
  WIFI_MGR_PRIVATE_DATA    *Private;
  WIFI_MANAGER_IFR_NVDATA  *IfrNvData;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  IfrNvData  = NULL;
  *Progress  = Configuration;
  BufferSize = sizeof (WIFI_MANAGER_IFR_NVDATA);
  Private    = WIFI_MGR_PRIVATE_DATA_FROM_CONFIG_ACCESS (This);

  if (!HiiIsConfigHdrMatch (Configuration, &gWifiConfigFormSetGuid, mVendorStorageName)) {
    return EFI_NOT_FOUND;
  }

  IfrNvData = AllocateZeroPool (BufferSize);
  if (IfrNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  WifiMgrConvertConfigDataToIfrNvData (Private, IfrNvData);

  Status = gHiiConfigRouting->ConfigToBlock (
                                gHiiConfigRouting,
                                Configuration,
                                (UINT8 *)IfrNvData,
                                &BufferSize,
                                Progress
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WifiMgrConvertIfrNvDataToConfigData (Private, IfrNvData);
  ZeroMem (IfrNvData, sizeof (WIFI_MANAGER_IFR_NVDATA));
  FreePool (IfrNvData);

  return Status;
}

/**
  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect. The format of the data tends to
                                 vary based on the opcode that generated the callback.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
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
WifiMgrDxeHiiConfigAccessCallback (
  IN     CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN     EFI_BROWSER_ACTION                    Action,
  IN     EFI_QUESTION_ID                       QuestionId,
  IN     UINT8                                 Type,
  IN OUT EFI_IFR_TYPE_VALUE                    *Value,
  OUT    EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  EFI_STATUS                Status;
  EFI_INPUT_KEY             Key;
  UINTN                     BufferSize;
  WIFI_MGR_PRIVATE_DATA     *Private;
  WIFI_MANAGER_IFR_NVDATA   *IfrNvData;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  WIFI_MGR_NETWORK_PROFILE  *Profile;
  WIFI_MGR_NETWORK_PROFILE  *ProfileToConnect;
  WIFI_HIDDEN_NETWORK_DATA  *HiddenNetwork;
  UINTN                     TempDataSize;
  VOID                      *TempData;
  LIST_ENTRY                *Entry;
  UINT32                    Index;
  UINT32                    RemoveCount;
  CHAR16                    *TempPassword;
  CHAR16                    *ErrorMessage;

  if ((Action != EFI_BROWSER_ACTION_FORM_OPEN) &&
      (Action != EFI_BROWSER_ACTION_FORM_CLOSE) &&
      (Action != EFI_BROWSER_ACTION_CHANGING) &&
      (Action != EFI_BROWSER_ACTION_CHANGED) &&
      (Action != EFI_BROWSER_ACTION_RETRIEVE))
  {
    return EFI_UNSUPPORTED;
  }

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status  = EFI_SUCCESS;
  Private = WIFI_MGR_PRIVATE_DATA_FROM_CONFIG_ACCESS (This);
  if (Private->CurrentNic == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Retrieve uncommitted data from Browser
  //
  BufferSize = sizeof (WIFI_MANAGER_IFR_NVDATA);
  IfrNvData  = AllocateZeroPool (BufferSize);
  if (IfrNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HiiGetBrowserData (&gWifiConfigFormSetGuid, mVendorStorageName, BufferSize, (UINT8 *)IfrNvData);

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    switch (QuestionId) {
      case KEY_MAC_LIST:

        Status = WifiMgrShowNicList (Private);
        break;

      case KEY_REFRESH_NETWORK_LIST:

        if (Private->CurrentNic->UserSelectedProfile != NULL) {
          Profile = Private->CurrentNic->UserSelectedProfile;

          //
          // Erase secrets since user has left Connection Page
          // Connection Page may direct to Network List Page or Eap Configuration Page,
          // secrets only need to be erased when head to Network List Page
          //
          WifiMgrCleanProfileSecrets (Profile);

          Private->CurrentNic->UserSelectedProfile = NULL;
        }

        break;

      case KEY_CONNECT_ACTION:

        if (Private->CurrentNic->UserSelectedProfile == NULL) {
          break;
        }

        Profile = Private->CurrentNic->UserSelectedProfile;

        //
        // Enter the network connection configuration page
        // Recovery from restored data
        //
        if (HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_SSID), Profile->SSId, NULL) == 0) {
          return EFI_OUT_OF_RESOURCES;
        }

        IfrNvData->SecurityType = Profile->SecurityType;
        if (HiiSetString (
              Private->RegisteredHandle,
              STRING_TOKEN (STR_SECURITY_TYPE),
              mSecurityType[IfrNvData->SecurityType],
              NULL
              ) == 0)
        {
          return EFI_OUT_OF_RESOURCES;
        }

        if ((IfrNvData->SecurityType == SECURITY_TYPE_WPA2_ENTERPRISE) ||
            (IfrNvData->SecurityType == SECURITY_TYPE_WPA3_ENTERPRISE))
        {
          IfrNvData->EapAuthMethod       = Profile->EapAuthMethod;
          IfrNvData->EapSecondAuthMethod = Profile->EapSecondAuthMethod;
          StrCpyS (IfrNvData->EapIdentity, EAP_IDENTITY_SIZE, Profile->EapIdentity);
        }

        break;

      case KEY_ENROLLED_CERT_NAME:

        if (Private->CurrentNic->UserSelectedProfile == NULL) {
          break;
        }

        Profile = Private->CurrentNic->UserSelectedProfile;

        //
        // Enter the key enrollment page
        // For TTLS and PEAP, only CA cert needs to be cared
        //
        if (Private->FileType == FileTypeCACert) {
          if (Profile->CACertData != NULL) {
            HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_EAP_ENROLLED_CERT_NAME), Profile->CACertName, NULL);
          } else {
            HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_EAP_ENROLLED_CERT_NAME), L"", NULL);
          }
        } else if (Private->FileType == FileTypeClientCert) {
          if (Profile->ClientCertData != NULL) {
            HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_EAP_ENROLLED_CERT_NAME), Profile->ClientCertName, NULL);
          } else {
            HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_EAP_ENROLLED_CERT_NAME), L"", NULL);
          }
        }

        break;

      case KEY_ENROLLED_PRIVATE_KEY_NAME:

        if (Private->CurrentNic->UserSelectedProfile == NULL) {
          break;
        }

        Profile = Private->CurrentNic->UserSelectedProfile;

        if (Profile->PrivateKeyData != NULL) {
          HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_EAP_ENROLLED_PRIVATE_KEY_NAME), Profile->PrivateKeyName, NULL);
        } else {
          HiiSetString (Private->RegisteredHandle, STRING_TOKEN (STR_EAP_ENROLLED_PRIVATE_KEY_NAME), L"", NULL);
        }

        break;

      default:
        break;
    }
  } else if (Action == EFI_BROWSER_ACTION_FORM_CLOSE) {
    switch (QuestionId) {
      case KEY_CONNECT_ACTION:

        if (Private->CurrentNic->UserSelectedProfile == NULL) {
          break;
        }

        Profile = Private->CurrentNic->UserSelectedProfile;

        //
        // Restore User Config Data for Page recovery
        //
        if ((IfrNvData->SecurityType == SECURITY_TYPE_WPA2_ENTERPRISE) ||
            (IfrNvData->SecurityType == SECURITY_TYPE_WPA3_ENTERPRISE))
        {
          Profile->EapAuthMethod       = IfrNvData->EapAuthMethod;
          Profile->EapSecondAuthMethod = IfrNvData->EapSecondAuthMethod;
          StrCpyS (Profile->EapIdentity, EAP_IDENTITY_SIZE, IfrNvData->EapIdentity);
        }

        break;

      default:
        break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (QuestionId) {
      case KEY_NETWORK_LIST:

        //
        // User triggered a scan process.
        //
        Private->CurrentNic->OneTimeScanRequest = TRUE;
        break;

      case KEY_PASSWORD_CONNECT_NETWORK:
      case KEY_EAP_PASSWORD_CONNECT_NETWORK:
      case KEY_PRIVATE_KEY_PASSWORD:

        if (Private->CurrentNic->UserSelectedProfile == NULL) {
          break;
        }

        Profile = Private->CurrentNic->UserSelectedProfile;

        if (QuestionId == KEY_PASSWORD_CONNECT_NETWORK) {
          TempPassword = Profile->Password;
        } else if (QuestionId == KEY_EAP_PASSWORD_CONNECT_NETWORK) {
          TempPassword = Profile->EapPassword;
        } else {
          TempPassword = Profile->PrivateKeyPassword;
        }

        Status = WifiMgrRecordPassword (Private, Value->string, TempPassword, PASSWORD_STORAGE_SIZE);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "[WiFi Connection Manager] Error: Failed to input password!"));
          break;
        }

        //
        // This password is not a new created password, so no need to confirm.
        //
        Status = EFI_NOT_FOUND;
        break;

      case KEY_CONNECT_ACTION:

        ErrorMessage     = NULL;
        ProfileToConnect = NULL;

        if (Private->CurrentNic->UserSelectedProfile == NULL) {
          break;
        }

        Profile = Private->CurrentNic->UserSelectedProfile;

        if ((Private->CurrentNic->ConnectState == WifiMgrDisconnected) ||
            (Profile != Private->CurrentNic->CurrentOperateNetwork))
        {
          //
          // When this network is not currently connected, pend it to connect.
          //
          if (Profile->AKMSuiteSupported && Profile->CipherSuiteSupported) {
            if ((Profile->SecurityType == SECURITY_TYPE_NONE) ||
                (Profile->SecurityType == SECURITY_TYPE_WPA2_PERSONAL) ||
                (Profile->SecurityType == SECURITY_TYPE_WPA3_PERSONAL))
            {
              //
              // For Open network, connect directly.
              //
              ProfileToConnect = Profile;
            } else if ((Profile->SecurityType == SECURITY_TYPE_WPA2_ENTERPRISE) ||
                       (Profile->SecurityType == SECURITY_TYPE_WPA3_ENTERPRISE))
            {
              //
              // For WPA/WPA2-Enterprise network, conduct eap configuration first.
              // Only EAP-TLS, TTLS and PEAP is supported now!
              //
              Profile->EapAuthMethod = IfrNvData->EapAuthMethod;
              StrCpyS (Profile->EapIdentity, EAP_IDENTITY_SIZE, IfrNvData->EapIdentity);

              if ((IfrNvData->EapAuthMethod == EAP_AUTH_METHOD_TTLS) || (IfrNvData->EapAuthMethod == EAP_AUTH_METHOD_PEAP)) {
                Profile->EapSecondAuthMethod = IfrNvData->EapSecondAuthMethod;
                ProfileToConnect             = Profile;
              } else if (IfrNvData->EapAuthMethod == EAP_AUTH_METHOD_TLS) {
                ProfileToConnect = Profile;
              } else {
                ErrorMessage = L"ERROR: Only EAP-TLS, TTLS or PEAP is supported now!";
              }
            } else {
              ErrorMessage = L"ERROR: Can't connect to this network!";
            }
          } else {
            ErrorMessage = L"ERROR: This network is not supported!";
          }

          if (ErrorMessage != NULL) {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              ErrorMessage,
              NULL
              );
          }

          if (ProfileToConnect != NULL) {
            Private->CurrentNic->OneTimeConnectRequest = TRUE;
            Private->CurrentNic->ConnectPendingNetwork = ProfileToConnect;
          }
        } else if (Private->CurrentNic->ConnectState == WifiMgrConnectedToAp) {
          //
          // This network is currently connected, just disconnect from it.
          //
          Private->CurrentNic->OneTimeDisconnectRequest    = TRUE;
          Private->CurrentNic->HasDisconnectPendingNetwork = TRUE;
        }

        break;

      case KEY_ENROLL_CA_CERT_CONNECT_NETWORK:

        Private->FileType = FileTypeCACert;
        break;

      case KEY_ENROLL_CLIENT_CERT_CONNECT_NETWORK:

        Private->FileType = FileTypeClientCert;
        break;

      case KEY_EAP_ENROLL_PRIVATE_KEY_FROM_FILE:

        FilePath = NULL;
        ChooseFile (NULL, NULL, NULL, &FilePath);

        if (FilePath != NULL) {
          UpdatePrivateKeyFromFile (Private, FilePath);
          FreePool (FilePath);
        }

        break;

      case KEY_EAP_ENROLL_CERT_FROM_FILE:

        //
        // User will select a cert file from File Explore
        //
        FilePath = NULL;
        ChooseFile (NULL, NULL, NULL, &FilePath);

        if (FilePath != NULL) {
          UpdateCAFromFile (Private, FilePath);
          FreePool (FilePath);
        }

        break;

      case KEY_SAVE_PRIVATE_KEY_TO_MEM:

        if ((Private->FileContext != NULL) && (Private->FileContext->FHandle != NULL) &&
            (Private->CurrentNic->UserSelectedProfile != NULL))
        {
          //
          // Read Private Key file to Buffer
          //
          Profile = Private->CurrentNic->UserSelectedProfile;
          if (Profile->PrivateKeyData != NULL) {
            ZeroMem (Profile->PrivateKeyData, Profile->PrivateKeyDataSize);
            FreePool (Profile->PrivateKeyData);
            Profile->PrivateKeyData = NULL;
          }

          Status = WifiMgrReadFileToBuffer (
                     Private->FileContext,
                     &TempData,
                     &TempDataSize
                     );
          if (EFI_ERROR (Status)) {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"ERROR: Can't read this private key file!",
              NULL
              );
          } else {
            ASSERT (Private->FileContext->FileName != NULL);

            Profile->PrivateKeyData     = TempData;
            Profile->PrivateKeyDataSize = TempDataSize;
            StrCpyS (Profile->PrivateKeyName, WIFI_FILENAME_STR_MAX_SIZE, Private->FileContext->FileName);

            DEBUG ((
              DEBUG_INFO,
              "[WiFi Connection Manager] Private Key: %s has been enrolled! Size: %d\n",
              Profile->PrivateKeyName,
              Profile->PrivateKeyDataSize
              ));
          }
        }

        break;

      case KEY_SAVE_CERT_TO_MEM:

        if ((Private->FileContext != NULL) && (Private->FileContext->FHandle != NULL) &&
            (Private->CurrentNic->UserSelectedProfile != NULL))
        {
          //
          // Read Cert file to Buffer
          //
          Profile = Private->CurrentNic->UserSelectedProfile;

          if (Private->FileType == FileTypeCACert) {
            if (Profile->CACertData != NULL) {
              ZeroMem (Profile->CACertData, Profile->CACertSize);
              FreePool (Profile->CACertData);
              Profile->CACertData = NULL;
            }
          } else if (Private->FileType == FileTypeClientCert) {
            if (Profile->ClientCertData != NULL) {
              ZeroMem (Profile->ClientCertData, Profile->ClientCertSize);
              FreePool (Profile->ClientCertData);
              Profile->ClientCertData = NULL;
            }
          } else {
            break;
          }

          Status = WifiMgrReadFileToBuffer (
                     Private->FileContext,
                     &TempData,
                     &TempDataSize
                     );
          if (EFI_ERROR (Status)) {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"ERROR: Can't read this certificate file!",
              NULL
              );
          } else {
            ASSERT (Private->FileContext->FileName != NULL);
            if (Private->FileType == FileTypeCACert) {
              Profile->CACertData = TempData;
              Profile->CACertSize = TempDataSize;
              StrCpyS (Profile->CACertName, WIFI_FILENAME_STR_MAX_SIZE, Private->FileContext->FileName);
              DEBUG ((
                DEBUG_INFO,
                "[WiFi Connection Manager] CA Cert: %s has been enrolled! Size: %d\n",
                Profile->CACertName,
                Profile->CACertSize
                ));
            } else {
              Profile->ClientCertData = TempData;
              Profile->ClientCertSize = TempDataSize;
              StrCpyS (Profile->ClientCertName, WIFI_FILENAME_STR_MAX_SIZE, Private->FileContext->FileName);
              DEBUG ((
                DEBUG_INFO,
                "[WiFi Connection Manager] Client Cert: %s has been enrolled! Size: %d\n",
                Profile->ClientCertName,
                Profile->ClientCertSize
                ));
            }
          }
        }

        break;

      case KEY_ADD_HIDDEN_NETWORK:

        //
        // Add a Hidden Network
        //
        if ((StrLen (IfrNvData->SSId) < SSID_MIN_LEN) ||
            (Private->HiddenNetworkCount >= HIDDEN_NETWORK_LIST_COUNT_MAX))
        {
          Status = EFI_ABORTED;
          break;
        } else {
          //
          // Check if this SSId is already in Hidden Network List
          //
          NET_LIST_FOR_EACH (Entry, &Private->HiddenNetworkList) {
            HiddenNetwork = NET_LIST_USER_STRUCT_S (
                              Entry,
                              WIFI_HIDDEN_NETWORK_DATA,
                              Link,
                              WIFI_MGR_HIDDEN_NETWORK_SIGNATURE
                              );
            if (StrCmp (HiddenNetwork->SSId, IfrNvData->SSId) == 0) {
              Status = EFI_ABORTED;
              break;
            }
          }
        }

        HiddenNetwork = (WIFI_HIDDEN_NETWORK_DATA *)AllocateZeroPool (sizeof (WIFI_HIDDEN_NETWORK_DATA));
        if (HiddenNetwork == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        HiddenNetwork->Signature = WIFI_MGR_HIDDEN_NETWORK_SIGNATURE;
        StrCpyS (HiddenNetwork->SSId, SSID_STORAGE_SIZE, IfrNvData->SSId);

        InsertTailList (&Private->HiddenNetworkList, &HiddenNetwork->Link);
        Private->HiddenNetworkCount++;

        WifiMgrRefreshHiddenList (Private);
        break;

      case KEY_REMOVE_HIDDEN_NETWORK:

        //
        // Remove Hidden Networks
        //
        Entry       = GetFirstNode (&Private->HiddenNetworkList);
        RemoveCount = 0;
        for (Index = 0; Index < Private->HiddenNetworkCount; Index++) {
          if (IfrNvData->HiddenNetworkList[Index] != 0) {
            HiddenNetwork = NET_LIST_USER_STRUCT_S (Entry, WIFI_HIDDEN_NETWORK_DATA, Link, WIFI_MGR_HIDDEN_NETWORK_SIGNATURE);
            Entry         = RemoveEntryList (Entry);
            RemoveCount++;

            FreePool (HiddenNetwork);
          } else {
            Entry = GetNextNode (&Private->HiddenNetworkList, Entry);
          }
        }

        Private->HiddenNetworkCount -= RemoveCount;
        WifiMgrRefreshHiddenList (Private);
        break;

      default:

        if ((QuestionId >= KEY_MAC_ENTRY_BASE) && (QuestionId < KEY_MAC_ENTRY_BASE + Private->NicCount)) {
          //
          // User selects a wireless NIC.
          //
          Status = WifiMgrSelectNic (Private, QuestionId);
          if (EFI_ERROR (Status)) {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"ERROR: Fail to operate the wireless NIC!",
              NULL
              );
          }
        } else if (Private->CurrentNic != NULL) {
          if ((QuestionId >= KEY_AVAILABLE_NETWORK_ENTRY_BASE) &&
              (QuestionId <= KEY_AVAILABLE_NETWORK_ENTRY_BASE + Private->CurrentNic->MaxProfileIndex))
          {
            Status = WifiMgrUserSelectProfileToConnect (Private, QuestionId - KEY_AVAILABLE_NETWORK_ENTRY_BASE);
            if (!EFI_ERROR (Status)) {
              WifiMgrUpdateConnectMessage (Private->CurrentNic, FALSE, NULL);
            }
          }

          if (EFI_ERROR (Status)) {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"ERROR: Fail to operate this profile!",
              NULL
              );
          }
        }

        break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    switch (QuestionId) {
      case KEY_SAVE_CERT_TO_MEM:
      case KEY_SAVE_PRIVATE_KEY_TO_MEM:

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
        break;

      case KEY_NO_SAVE_CERT_TO_MEM:
      case KEY_NO_SAVE_PRIVATE_KEY_TO_MEM:

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
        break;

      default:

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        break;
    }
  } else if (Action == EFI_BROWSER_ACTION_RETRIEVE) {
    switch (QuestionId) {
      case KEY_REFRESH_NETWORK_LIST:

        WifiMgrRefreshNetworkList (Private, IfrNvData);
        break;

      default:
        break;
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // Pass changed uncommitted data back to Form Browser.
    //
    BufferSize = sizeof (WIFI_MANAGER_IFR_NVDATA);
    HiiSetBrowserData (&gWifiConfigFormSetGuid, mVendorStorageName, BufferSize, (UINT8 *)IfrNvData, NULL);
  }

  ZeroMem (IfrNvData, sizeof (WIFI_MANAGER_IFR_NVDATA));
  FreePool (IfrNvData);
  return Status;
}

/**
  Initialize the WiFi configuration form.

  @param[in]  Private             The pointer to the global private data structure.

  @retval EFI_SUCCESS             The configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.
  @retval Other Erros             Returned Errors when installing protocols.

**/
EFI_STATUS
WifiMgrDxeConfigFormInit (
  WIFI_MGR_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private->ConfigAccess.ExtractConfig = WifiMgrDxeHiiConfigAccessExtractConfig;
  Private->ConfigAccess.RouteConfig   = WifiMgrDxeHiiConfigAccessRouteConfig;
  Private->ConfigAccess.Callback      = WifiMgrDxeHiiConfigAccessCallback;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mWifiMgrDxeHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &Private->ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Publish our HII data.
  //
  Private->RegisteredHandle = HiiAddPackages (
                                &gWifiConfigFormSetGuid,
                                Private->DriverHandle,
                                WifiConnectionManagerDxeStrings,
                                WifiConnectionManagerDxeBin,
                                NULL
                                );
  if (Private->RegisteredHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           Private->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mWifiMgrDxeHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &Private->ConfigAccess,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  Private->FileContext = AllocateZeroPool (sizeof (WIFI_MGR_FILE_CONTEXT));
  if (Private->FileContext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Unload the WiFi configuration form.

  @param[in]  Private             The pointer to the global private data structure.

  @retval EFI_SUCCESS             The configuration form is unloaded successfully.
  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.
  @retval Other Errors            Returned Erros when uninstalling protocols.

**/
EFI_STATUS
WifiMgrDxeConfigFormUnload (
  WIFI_MGR_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Private->FileContext != NULL) {
    if (Private->FileContext->FHandle != NULL) {
      Private->FileContext->FHandle->Close (Private->FileContext->FHandle);
    }

    if (Private->FileContext->FileName != NULL) {
      FreePool (Private->FileContext->FileName);
    }

    FreePool (Private->FileContext);
  }

  HiiRemovePackages (Private->RegisteredHandle);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Private->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mWifiMgrDxeHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &Private->ConfigAccess,
                  NULL
                  );

  return Status;
}
