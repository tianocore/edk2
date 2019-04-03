/** @file
  The Mac Connection2 Protocol adapter functions for WiFi Connection Manager.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "WifiConnectionMgrDxe.h"

EFI_EAP_TYPE mEapAuthMethod[] = {
  EFI_EAP_TYPE_TTLS,
  EFI_EAP_TYPE_PEAP,
  EFI_EAP_TYPE_EAPTLS
};

EFI_EAP_TYPE mEapSecondAuthMethod[] = {
  EFI_EAP_TYPE_MSCHAPV2
};

/**
  The callback function for scan operation. This function updates networks
  according to the latest scan result, and trigger UI refresh.

  ASSERT when errors occur in config token.

  @param[in]  Event                 The GetNetworks token receive event.
  @param[in]  Context               The context of the GetNetworks token.

**/
VOID
EFIAPI
WifiMgrOnScanFinished (
  IN  EFI_EVENT                     Event,
  IN  VOID                          *Context
  )
{
  EFI_STATUS                        Status;
  WIFI_MGR_MAC_CONFIG_TOKEN         *ConfigToken;
  WIFI_MGR_DEVICE_DATA              *Nic;
  WIFI_MGR_NETWORK_PROFILE          *Profile;
  EFI_80211_NETWORK                 *Network;
  UINTN                             DataSize;
  EFI_80211_NETWORK_DESCRIPTION     *NetworkDescription;
  EFI_80211_GET_NETWORKS_RESULT     *Result;
  LIST_ENTRY                        *Entry;
  UINT8                             SecurityType;
  BOOLEAN                           AKMSuiteSupported;
  BOOLEAN                           CipherSuiteSupported;
  CHAR8                             *AsciiSSId;
  UINTN                             Index;

  ASSERT (Context != NULL);

  ConfigToken = (WIFI_MGR_MAC_CONFIG_TOKEN *) Context;
  ASSERT (ConfigToken->Nic != NULL);
  ASSERT (ConfigToken->Type == TokenTypeGetNetworksToken);

  //
  // It is the GetNetworks token, set scan state to "ScanFinished"
  //
  ConfigToken->Nic->ScanState = WifiMgrScanFinished;

  ASSERT (ConfigToken->Token.GetNetworksToken != NULL);
  Result = ConfigToken->Token.GetNetworksToken->Result;
  Nic    = ConfigToken->Nic;

  //
  // Clean previous result, and update network list according to the scan result
  //
  Nic->AvailableCount    = 0;

  NET_LIST_FOR_EACH (Entry, &Nic->ProfileList) {
    Profile = NET_LIST_USER_STRUCT_S (Entry, WIFI_MGR_NETWORK_PROFILE,
                Link, WIFI_MGR_PROFILE_SIGNATURE);
    Profile->IsAvailable = FALSE;
  }

  if (Result == NULL) {
    gBS->SignalEvent (Nic->Private->NetworkListRefreshEvent);
    WifiMgrFreeToken(ConfigToken);
    return;
  }

  for (Index = 0; Index < Result->NumOfNetworkDesc; Index ++) {

    NetworkDescription = Result->NetworkDesc + Index;
    if (NetworkDescription == NULL) {
      continue;
    }

    Network = &NetworkDescription->Network;
    if (Network == NULL || Network->SSId.SSIdLen == 0) {
      continue;
    }

    Status = WifiMgrCheckRSN (
               Network->AKMSuite,
               Network->CipherSuite,
               Nic,
               &SecurityType,
               &AKMSuiteSupported,
               &CipherSuiteSupported
               );
    if (EFI_ERROR (Status)) {

      SecurityType          = SECURITY_TYPE_UNKNOWN;
      AKMSuiteSupported     = FALSE;
      CipherSuiteSupported  = FALSE;
    }

    AsciiSSId = (CHAR8*) AllocateZeroPool(sizeof (CHAR8) * (Network->SSId.SSIdLen + 1));
    if (AsciiSSId == NULL) {
      continue;
    }
    CopyMem(AsciiSSId, (CHAR8 *) Network->SSId.SSId, sizeof (CHAR8) * Network->SSId.SSIdLen);
    *(AsciiSSId + Network->SSId.SSIdLen) = '\0';

    Profile = WifiMgrGetProfileByAsciiSSId (AsciiSSId, SecurityType, &Nic->ProfileList);
    if (Profile == NULL) {

      if (Nic->MaxProfileIndex >= NETWORK_LIST_COUNT_MAX) {
        FreePool (AsciiSSId);
        continue;
      }

      //
      // Create a new profile
      //
      Profile = AllocateZeroPool (sizeof (WIFI_MGR_NETWORK_PROFILE));
      if (Profile == NULL) {
        FreePool (AsciiSSId);
        continue;
      }
      Profile->Signature    = WIFI_MGR_PROFILE_SIGNATURE;
      Profile->NicIndex     = Nic->NicIndex;
      Profile->ProfileIndex = Nic->MaxProfileIndex + 1;
      AsciiStrToUnicodeStrS (AsciiSSId, Profile->SSId, SSID_STORAGE_SIZE);
      InsertTailList (&Nic->ProfileList, &Profile->Link);
      Nic->MaxProfileIndex ++;
    }
    FreePool (AsciiSSId);

    //
    //May receive duplicate networks in scan results, check if it has already
    //been processed.
    //
    if (!Profile->IsAvailable) {

      Profile->IsAvailable          = TRUE;
      Profile->SecurityType         = SecurityType;
      Profile->AKMSuiteSupported    = AKMSuiteSupported;
      Profile->CipherSuiteSupported = CipherSuiteSupported;
      Profile->NetworkQuality       = NetworkDescription->NetworkQuality;
      Nic->AvailableCount ++;

      //
      //Copy BSSType and SSId
      //
      CopyMem(&Profile->Network, Network, sizeof (EFI_80211_NETWORK));

      //
      //Copy AKMSuite list
      //
      if (Network->AKMSuite != NULL) {

        if (Network->AKMSuite->AKMSuiteCount == 0) {
          DataSize = sizeof (EFI_80211_AKM_SUITE_SELECTOR);
        } else {
          DataSize = sizeof (EFI_80211_AKM_SUITE_SELECTOR) + sizeof (EFI_80211_SUITE_SELECTOR)
                       * (Network->AKMSuite->AKMSuiteCount - 1);
        }
        Profile->Network.AKMSuite = (EFI_80211_AKM_SUITE_SELECTOR *) AllocateZeroPool (DataSize);
        if (Profile->Network.AKMSuite == NULL) {
          continue;
        }
        CopyMem (Profile->Network.AKMSuite, Network->AKMSuite, DataSize);
      }

      //
      //Copy CipherSuite list
      //
      if (Network->CipherSuite != NULL) {

        if (Network->CipherSuite->CipherSuiteCount == 0) {
          DataSize = sizeof (EFI_80211_CIPHER_SUITE_SELECTOR);
        } else {
          DataSize = sizeof (EFI_80211_CIPHER_SUITE_SELECTOR) + sizeof (EFI_80211_SUITE_SELECTOR)
                       * (Network->CipherSuite->CipherSuiteCount - 1);
        }
        Profile->Network.CipherSuite = (EFI_80211_CIPHER_SUITE_SELECTOR *) AllocateZeroPool (DataSize);
        if (Profile->Network.CipherSuite == NULL) {
          continue;
        }
        CopyMem (Profile->Network.CipherSuite, Network->CipherSuite, DataSize);
      }
    } else {
      //
      // A duplicate network, update signal quality
      //
      if (Profile->NetworkQuality < NetworkDescription->NetworkQuality) {
        Profile->NetworkQuality = NetworkDescription->NetworkQuality;
      }
      continue;
    }
  }

  gBS->SignalEvent (Nic->Private->NetworkListRefreshEvent);

  //
  // The current connected network should always be available until disconnection
  // happens in Wifi FW layer, even when it is not in this time's scan result.
  //
  if (Nic->ConnectState == WifiMgrConnectedToAp && Nic->CurrentOperateNetwork != NULL) {
    if (!Nic->CurrentOperateNetwork->IsAvailable) {
      Nic->CurrentOperateNetwork->IsAvailable = TRUE;
      Nic->AvailableCount ++;
    }
  }

  WifiMgrFreeToken(ConfigToken);
}

/**
  Start scan operation, and send out a token to collect available networks.

  @param[in]  Nic                 Pointer to the device data of the selected NIC.

  @retval EFI_SUCCESS             The operation is completed.
  @retval EFI_ALREADY_STARTED     A former scan operation is already ongoing.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval Other Errors            Return errors when getting networks from low layer.

**/
EFI_STATUS
WifiMgrStartScan (
  IN      WIFI_MGR_DEVICE_DATA        *Nic
  )
{
  EFI_STATUS                          Status;
  EFI_TPL                             OldTpl;
  WIFI_MGR_MAC_CONFIG_TOKEN           *ConfigToken;
  EFI_80211_GET_NETWORKS_TOKEN        *GetNetworksToken;
  UINT32                              HiddenSSIdIndex;
  UINT32                              HiddenSSIdCount;
  EFI_80211_SSID                      *HiddenSSIdList;
  WIFI_HIDDEN_NETWORK_DATA            *HiddenNetwork;
  LIST_ENTRY                          *Entry;

  if (Nic == NULL || Nic->Wmp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Nic->ScanState == WifiMgrScanning) {
    return EFI_ALREADY_STARTED;
  }

  Nic->ScanState  = WifiMgrScanning;
  OldTpl          = gBS->RaiseTPL (TPL_CALLBACK);
  Status          = EFI_SUCCESS;
  HiddenSSIdList  = NULL;
  HiddenSSIdCount = Nic->Private->HiddenNetworkCount;
  HiddenSSIdIndex = 0;

  //
  //create a new get network token
  //
  ConfigToken     = AllocateZeroPool (sizeof (WIFI_MGR_MAC_CONFIG_TOKEN));
  if (ConfigToken == NULL) {
    gBS->RestoreTPL (OldTpl);
    return EFI_OUT_OF_RESOURCES;
  }

  ConfigToken->Type      = TokenTypeGetNetworksToken;
  ConfigToken->Nic       = Nic;
  ConfigToken->Token.GetNetworksToken = AllocateZeroPool (sizeof (EFI_80211_GET_NETWORKS_TOKEN));
  if (ConfigToken->Token.GetNetworksToken == NULL) {
    WifiMgrFreeToken(ConfigToken);
    gBS->RestoreTPL (OldTpl);
    return EFI_OUT_OF_RESOURCES;
  }
  GetNetworksToken = ConfigToken->Token.GetNetworksToken;

  //
  // There are some hidden networks to scan, add them into scan list
  //
  if (HiddenSSIdCount > 0) {
    HiddenSSIdList = AllocateZeroPool(HiddenSSIdCount * sizeof (EFI_80211_SSID));
    if (HiddenSSIdList == NULL) {
      WifiMgrFreeToken(ConfigToken);
      gBS->RestoreTPL (OldTpl);
      return EFI_OUT_OF_RESOURCES;
    }

    HiddenSSIdIndex = 0;
    NET_LIST_FOR_EACH (Entry, &Nic->Private->HiddenNetworkList) {

      HiddenNetwork = NET_LIST_USER_STRUCT_S (Entry, WIFI_HIDDEN_NETWORK_DATA,
                        Link, WIFI_MGR_HIDDEN_NETWORK_SIGNATURE);
      HiddenSSIdList[HiddenSSIdIndex].SSIdLen = (UINT8) StrLen (HiddenNetwork->SSId);
      UnicodeStrToAsciiStrS(HiddenNetwork->SSId,
        (CHAR8 *) HiddenSSIdList[HiddenSSIdIndex].SSId, SSID_STORAGE_SIZE);
      HiddenSSIdIndex ++;
    }
    GetNetworksToken->Data = AllocateZeroPool (sizeof (EFI_80211_GET_NETWORKS_DATA) +
                               (HiddenSSIdCount - 1) * sizeof (EFI_80211_SSID));
    if (GetNetworksToken->Data == NULL) {
      FreePool (HiddenSSIdList);
      WifiMgrFreeToken(ConfigToken);
      gBS->RestoreTPL (OldTpl);
      return EFI_OUT_OF_RESOURCES;
    }
    GetNetworksToken->Data->NumOfSSID = HiddenSSIdCount;
    CopyMem(GetNetworksToken->Data->SSIDList, HiddenSSIdList, HiddenSSIdCount * sizeof (EFI_80211_SSID));
    FreePool(HiddenSSIdList);
  } else {

    GetNetworksToken->Data = AllocateZeroPool (sizeof (EFI_80211_GET_NETWORKS_DATA));
    if (GetNetworksToken->Data == NULL) {
      WifiMgrFreeToken(ConfigToken);
      gBS->RestoreTPL (OldTpl);
      return EFI_OUT_OF_RESOURCES;
    }

    GetNetworksToken->Data->NumOfSSID = 0;
  }

  //
  //Create a handle when scan process ends
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  WifiMgrOnScanFinished,
                  ConfigToken,
                  &GetNetworksToken->Event
                  );
  if (EFI_ERROR (Status)) {
    WifiMgrFreeToken(ConfigToken);
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  //
  //Start scan ...
  //
  Status = Nic->Wmp->GetNetworks (Nic->Wmp, GetNetworksToken);
  if (EFI_ERROR (Status)) {
    Nic->ScanState  = WifiMgrScanFinished;
    WifiMgrFreeToken(ConfigToken);
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

/**
  Configure password to supplicant before connecting to a secured network.

  @param[in]  Nic                 Pointer to the device data of the selected NIC.
  @param[in]  Profile             The target network to be connected.

  @retval EFI_SUCCESS             The operation is completed.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_NOT_FOUND           No valid password is found to configure.
  @retval Other Errors            Returned errors when setting data to supplicant.

**/
EFI_STATUS
WifiMgrConfigPassword (
  IN    WIFI_MGR_DEVICE_DATA              *Nic,
  IN    WIFI_MGR_NETWORK_PROFILE          *Profile
  )
{
  EFI_STATUS                 Status;
  EFI_SUPPLICANT_PROTOCOL    *Supplicant;
  EFI_80211_SSID             SSId;
  UINT8                      *AsciiPassword;

  if (Nic == NULL || Nic->Supplicant == NULL || Profile == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Supplicant = Nic->Supplicant;
  //
  //Set SSId to supplicant
  //
  SSId.SSIdLen = Profile->Network.SSId.SSIdLen;
  CopyMem(SSId.SSId, Profile->Network.SSId.SSId, sizeof (Profile->Network.SSId.SSId));
  Status = Supplicant->SetData(Supplicant,EfiSupplicant80211TargetSSIDName,
                         (VOID *)&SSId, sizeof(EFI_80211_SSID));
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  //Set password to supplicant
  //
  if (StrLen (Profile->Password) < PASSWORD_MIN_LEN) {
    return EFI_NOT_FOUND;
  }
  AsciiPassword = AllocateZeroPool ((StrLen(Profile->Password) + 1) * sizeof (UINT8));
  if (AsciiPassword == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  UnicodeStrToAsciiStrS (Profile->Password, (CHAR8 *) AsciiPassword, PASSWORD_STORAGE_SIZE);
  Status = Supplicant->SetData (Supplicant, EfiSupplicant80211PskPassword,
                         AsciiPassword, (StrLen(Profile->Password) + 1) * sizeof (UINT8));
  ZeroMem (AsciiPassword, AsciiStrLen ((CHAR8 *) AsciiPassword) + 1);
  FreePool(AsciiPassword);

  return Status;
}

/**
  Conduct EAP configuration to supplicant before connecting to a EAP network.
  Current WiFi Connection Manager only supports three kinds of EAP networks:
  1). EAP-TLS (Two-Way Authentication is required in our implementation)
  2). EAP-TTLS/MSCHAPv2 (One-Way Authentication is required in our implementation)
  3). PEAPv0/MSCHAPv2 (One-Way Authentication is required in our implementation)

  @param[in]  Nic                 Pointer to the device data of the selected NIC.
  @param[in]  Profile             The target network to be connected.

  @retval EFI_SUCCESS             The operation is completed.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_UNSUPPORTED         The expected EAP method is not supported.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval Other Errors            Returned errors when setting data to supplicant.

**/
EFI_STATUS
WifiMgrConfigEap (
  IN    WIFI_MGR_DEVICE_DATA              *Nic,
  IN    WIFI_MGR_NETWORK_PROFILE          *Profile
  )
{
  EFI_STATUS                        Status;
  EFI_EAP_CONFIGURATION_PROTOCOL    *EapConfig;
  EFI_EAP_TYPE                      EapAuthMethod;
  EFI_EAP_TYPE                      EapSecondAuthMethod;
  EFI_EAP_TYPE                      *AuthMethodList;
  CHAR8                             *Identity;
  UINTN                             IdentitySize;
  CHAR16                            *Password;
  UINTN                             PasswordSize;
  UINTN                             EncryptPasswordLen;
  CHAR8                             *AsciiEncryptPassword;
  UINTN                             AuthMethodListSize;
  UINTN                             Index;

  if (Nic == NULL || Nic->EapConfig == NULL || Profile == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  EapConfig = Nic->EapConfig;

  if (Profile->EapAuthMethod >= EAP_AUTH_METHOD_MAX) {
    return EFI_INVALID_PARAMETER;
  }
  EapAuthMethod = mEapAuthMethod[Profile->EapAuthMethod];

  if (EapAuthMethod != EFI_EAP_TYPE_EAPTLS) {
    if (Profile->EapSecondAuthMethod >= EAP_SEAUTH_METHOD_MAX) {
      return EFI_INVALID_PARAMETER;
    }
    EapSecondAuthMethod = mEapSecondAuthMethod[Profile->EapSecondAuthMethod];
  }

  //
  //The first time to get Supported Auth Method list, return the size.
  //
  AuthMethodListSize  = 0;
  AuthMethodList      = NULL;
  Status = EapConfig->GetData (EapConfig, EFI_EAP_TYPE_ATTRIBUTE, EfiEapConfigEapSupportedAuthMethod,
                        (VOID *) AuthMethodList, &AuthMethodListSize);
  if (Status == EFI_SUCCESS) {
    //
    //No Supported Eap Auth Method
    //
    return EFI_UNSUPPORTED;
  } else if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  //
  // The second time to get Supported Auth Method list, return the list.
  // In current design, only EAPTLS, TTLS and PEAP are supported
  //
  AuthMethodList = (EFI_EAP_TYPE *) AllocateZeroPool(AuthMethodListSize);
  if (AuthMethodList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = EapConfig->GetData (EapConfig, EFI_EAP_TYPE_ATTRIBUTE, EfiEapConfigEapSupportedAuthMethod,
                        (VOID *) AuthMethodList, &AuthMethodListSize);
  if (EFI_ERROR (Status)) {
    FreePool (AuthMethodList);
    return Status;
  }

  //
  //Check if EapAuthMethod is in supported Auth Method list, if found, skip the loop.
  //
  for (Index = 0; Index < AuthMethodListSize / sizeof (EFI_EAP_TYPE); Index ++) {
    if (EapAuthMethod == AuthMethodList[Index]) {
      break;
    }
  }
  if (Index == AuthMethodListSize / sizeof (EFI_EAP_TYPE)) {
    FreePool (AuthMethodList);
    return EFI_UNSUPPORTED;
  }
  FreePool (AuthMethodList);

  //
  // Set Identity to Eap peer, Mandatory field for PEAP and TTLS
  //
  if (StrLen (Profile->EapIdentity) > 0) {

    IdentitySize = sizeof(CHAR8) * (StrLen(Profile->EapIdentity) + 1);
    Identity = AllocateZeroPool (IdentitySize);
    if (Identity == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    UnicodeStrToAsciiStrS(Profile->EapIdentity, Identity, IdentitySize);
    Status = EapConfig->SetData (EapConfig, EFI_EAP_TYPE_IDENTITY, EfiEapConfigIdentityString,
                          (VOID *) Identity, IdentitySize - 1);
    if (EFI_ERROR(Status)) {
      FreePool (Identity);
      return Status;
    }
    FreePool (Identity);
  } else {
    if (EapAuthMethod != EFI_EAP_TYPE_EAPTLS) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  //Set Auth Method to Eap peer, Mandatory field
  //
  Status = EapConfig->SetData (EapConfig, EFI_EAP_TYPE_ATTRIBUTE, EfiEapConfigEapAuthMethod,
                        (VOID *) &EapAuthMethod, sizeof (EapAuthMethod));
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (EapAuthMethod == EFI_EAP_TYPE_TTLS || EapAuthMethod == EFI_EAP_TYPE_PEAP) {

    Status = EapConfig->SetData (EapConfig, EapAuthMethod, EfiEapConfigEap2ndAuthMethod,
                          (VOID *) &EapSecondAuthMethod, sizeof (EapSecondAuthMethod));
    if (EFI_ERROR(Status)) {
      return Status;
    }

    //
    // Set Password to Eap peer
    //
    if (StrLen (Profile->EapPassword) < PASSWORD_MIN_LEN) {

      DEBUG ((DEBUG_ERROR, "[WiFi Connection Manager] Error: No Eap Password for Network: %s.\n", Profile->SSId));
      return EFI_INVALID_PARAMETER;
    }

    PasswordSize = sizeof (CHAR16) * (StrLen (Profile->EapPassword) + 1);
    Password = AllocateZeroPool (PasswordSize);
    if (Password == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    StrCpyS (Password, PasswordSize, Profile->EapPassword);;
    Status = EapConfig->SetData (EapConfig, EFI_EAP_TYPE_MSCHAPV2, EfiEapConfigEapMSChapV2Password,
               (VOID *) Password, PasswordSize);
    ZeroMem (Password, PasswordSize);
    FreePool (Password);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    //If CA cert is required, set it to Eap peer
    //
    if (Profile->CACertData != NULL) {

      Status = EapConfig->SetData (EapConfig, EapAuthMethod, EfiEapConfigEapTlsCACert,
                 Profile->CACertData, Profile->CACertSize);
      if (EFI_ERROR(Status)) {
        return Status;
      }
    } else {
      return EFI_INVALID_PARAMETER;
    }
  } else if (EapAuthMethod == EFI_EAP_TYPE_EAPTLS) {

    //
    //Set CA cert to Eap peer
    //
    if (Profile->CACertData == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    Status = EapConfig->SetData (EapConfig, EFI_EAP_TYPE_EAPTLS, EfiEapConfigEapTlsCACert,
               Profile->CACertData, Profile->CACertSize);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    //
    //Set Client cert to Eap peer
    //
    if (Profile->ClientCertData == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    Status = EapConfig->SetData (EapConfig, EFI_EAP_TYPE_EAPTLS, EfiEapConfigEapTlsClientCert,
               Profile->ClientCertData, Profile->ClientCertSize);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    //
    //Set Private key to Eap peer
    //
    if (Profile->PrivateKeyData == NULL) {

      DEBUG ((DEBUG_ERROR, "[WiFi Connection Manager]  Error: No Private Key for Network: %s.\n", Profile->SSId));
      return EFI_INVALID_PARAMETER;
    }

    Status = EapConfig->SetData (EapConfig, EFI_EAP_TYPE_EAPTLS, EfiEapConfigEapTlsClientPrivateKeyFile,
               Profile->PrivateKeyData, Profile->PrivateKeyDataSize);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    if (StrLen (Profile->PrivateKeyPassword) > 0) {

      EncryptPasswordLen = StrLen (Profile->PrivateKeyPassword);
      AsciiEncryptPassword = AllocateZeroPool(EncryptPasswordLen + 1);
      if (AsciiEncryptPassword == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      UnicodeStrToAsciiStrS(Profile->PrivateKeyPassword, AsciiEncryptPassword, EncryptPasswordLen + 1);
      Status = EapConfig->SetData(EapConfig, EFI_EAP_TYPE_EAPTLS,
                                    EfiEapConfigEapTlsClientPrivateKeyFilePassword,
                                    (VOID *) AsciiEncryptPassword, EncryptPasswordLen + 1);
      if (EFI_ERROR(Status)) {

        ZeroMem (AsciiEncryptPassword, EncryptPasswordLen + 1);
        FreePool (AsciiEncryptPassword);
        return Status;
      }

      ZeroMem (AsciiEncryptPassword, EncryptPasswordLen + 1);
      FreePool (AsciiEncryptPassword);
    }
  } else {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Get current link state from low layer.

  @param[in]   Nic                Pointer to the device data of the selected NIC.
  @param[out]  LinkState          The pointer to buffer to retrieve link state.

  @retval EFI_SUCCESS             The operation is completed.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_UNSUPPORTED         Adapter information protocol is not supported.
  @retval Other Errors            Returned errors when retrieving link state from low layer.

**/
EFI_STATUS
WifiMgrGetLinkState (
  IN   WIFI_MGR_DEVICE_DATA            *Nic,
  OUT  EFI_ADAPTER_INFO_MEDIA_STATE    *LinkState
  )
{
  EFI_STATUS                           Status;
  EFI_TPL                              OldTpl;
  UINTN                                DataSize;
  EFI_ADAPTER_INFO_MEDIA_STATE         *UndiState;
  EFI_ADAPTER_INFORMATION_PROTOCOL     *Aip;

  if (Nic == NULL || LinkState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  Status = gBS->OpenProtocol (
                  Nic->ControllerHandle,
                  &gEfiAdapterInformationProtocolGuid,
                  (VOID**) &Aip,
                  Nic->DriverHandle,
                  Nic->ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return EFI_UNSUPPORTED;
  }

  Status = Aip->GetInformation(
                  Aip,
                  &gEfiAdapterInfoMediaStateGuid,
                  (VOID **) &UndiState,
                  &DataSize
                  );
  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return Status;
  }
  gBS->RestoreTPL (OldTpl);

  CopyMem (LinkState, UndiState, sizeof (EFI_ADAPTER_INFO_MEDIA_STATE));
  FreePool (UndiState);
  return EFI_SUCCESS;
}

/**
  Prepare configuration work before connecting to the target network.
  For WPA2 Personal networks, password should be checked; and for EAP networks, parameters
  are different for different networks.

  @param[in]  Nic                 Pointer to the device data of the selected NIC.
  @param[in]  Profile             The target network to be connected.

  @retval EFI_SUCCESS             The operation is completed.
  @retval EFI_UNSUPPORTED         This network is not supported.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.

**/
EFI_STATUS
WifiMgrPrepareConnection (
  IN    WIFI_MGR_DEVICE_DATA              *Nic,
  IN    WIFI_MGR_NETWORK_PROFILE          *Profile
  )
{
  EFI_STATUS           Status;
  UINT8                SecurityType;
  BOOLEAN              AKMSuiteSupported;
  BOOLEAN              CipherSuiteSupported;

  if (Profile == NULL || Nic == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = WifiMgrCheckRSN (Profile->Network.AKMSuite, Profile->Network.CipherSuite,
             Nic, &SecurityType, &AKMSuiteSupported, &CipherSuiteSupported);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (AKMSuiteSupported && CipherSuiteSupported) {
    switch (SecurityType) {
      case SECURITY_TYPE_WPA2_PERSONAL:

        Status = WifiMgrConfigPassword (Nic, Profile);
        if (EFI_ERROR (Status)) {
          if (Status == EFI_NOT_FOUND) {
            if (Nic->OneTimeConnectRequest) {
              WifiMgrUpdateConnectMessage (Nic, FALSE, L"Connect Failed: Invalid Password!");
            }
          }
          return Status;
        }
        break;

      case SECURITY_TYPE_WPA2_ENTERPRISE:

        Status = WifiMgrConfigEap (Nic, Profile);
        if (EFI_ERROR (Status)) {
          if (Status == EFI_INVALID_PARAMETER) {
            if (Nic->OneTimeConnectRequest) {
              WifiMgrUpdateConnectMessage (Nic, FALSE, L"Connect Failed: Invalid Configuration!");
            }
          }
          return Status;
        }
        break;

      case SECURITY_TYPE_NONE:
        break;

      default:
        return EFI_UNSUPPORTED;
    }
  } else {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  The callback function for connect operation.

  ASSERT when errors occur in config token.

  @param[in]  Event                 The Connect token receive event.
  @param[in]  Context               The context of the connect token.

**/
VOID
EFIAPI
WifiMgrOnConnectFinished (
  IN  EFI_EVENT              Event,
  IN  VOID                   *Context
  )
{
  EFI_STATUS                         Status;
  WIFI_MGR_MAC_CONFIG_TOKEN          *ConfigToken;
  WIFI_MGR_NETWORK_PROFILE           *ConnectedProfile;
  UINT8                              SecurityType;
  UINT8                              SSIdLen;
  CHAR8                              *AsciiSSId;

  ASSERT (Context != NULL);

  ConnectedProfile = NULL;
  ConfigToken = (WIFI_MGR_MAC_CONFIG_TOKEN*) Context;
  ASSERT (ConfigToken->Nic != NULL);

  ConfigToken->Nic->ConnectState = WifiMgrDisconnected;
  ASSERT (ConfigToken->Type == TokenTypeConnectNetworkToken);

  ASSERT (ConfigToken->Token.ConnectNetworkToken != NULL);
  if (ConfigToken->Token.ConnectNetworkToken->Status != EFI_SUCCESS) {

    if (ConfigToken->Nic->OneTimeConnectRequest) {
      //
      // Only update message for user triggered connection
      //
      if (ConfigToken->Token.ConnectNetworkToken->Status == EFI_ACCESS_DENIED) {

        WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, L"Connect Failed: Permission Denied!");
      } else {
        WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, L"Connect Failed!");
      }
      ConfigToken->Nic->OneTimeConnectRequest = FALSE;
    }
    ConfigToken->Nic->CurrentOperateNetwork = NULL;
    return;
  }

  if (ConfigToken->Token.ConnectNetworkToken->ResultCode != ConnectSuccess) {

    if (ConfigToken->Nic->OneTimeConnectRequest) {

      if (ConfigToken->Token.ConnectNetworkToken->ResultCode == ConnectFailedReasonUnspecified) {
        WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, L"Connect Failed: Wrong Password or Unexpected Error!");
      } else {
        WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, L"Connect Failed!");
      }
    }
    goto Exit;
  }

  if (ConfigToken->Token.ConnectNetworkToken->Data == NULL ||
    ConfigToken->Token.ConnectNetworkToken->Data->Network == NULL) {

    //
    // An unexpected error occurs, tell low layer to perform a disconnect
    //
    ConfigToken->Nic->HasDisconnectPendingNetwork = TRUE;
    WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, NULL);
    goto Exit;
  }

  //
  // A correct connect token received, terminate the connection process
  //
  Status = WifiMgrCheckRSN(ConfigToken->Token.ConnectNetworkToken->Data->Network->AKMSuite,
             ConfigToken->Token.ConnectNetworkToken->Data->Network->CipherSuite,
             ConfigToken->Nic, &SecurityType, NULL, NULL);
  if (EFI_ERROR(Status)) {
    SecurityType = SECURITY_TYPE_UNKNOWN;
  }

  SSIdLen   = ConfigToken->Token.ConnectNetworkToken->Data->Network->SSId.SSIdLen;
  AsciiSSId = (CHAR8*) AllocateZeroPool(sizeof (CHAR8) * (SSIdLen + 1));
  if (AsciiSSId == NULL) {
    ConfigToken->Nic->HasDisconnectPendingNetwork = TRUE;
    WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, NULL);
    goto Exit;
  }

  CopyMem(AsciiSSId, ConfigToken->Token.ConnectNetworkToken->Data->Network->SSId.SSId, SSIdLen);
  *(AsciiSSId + SSIdLen) = '\0';

  ConnectedProfile = WifiMgrGetProfileByAsciiSSId(AsciiSSId, SecurityType, &ConfigToken->Nic->ProfileList);
  FreePool(AsciiSSId);
  if (ConnectedProfile == NULL) {
    ConfigToken->Nic->HasDisconnectPendingNetwork = TRUE;
    WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, NULL);
    goto Exit;
  }

  ConfigToken->Nic->ConnectState = WifiMgrConnectedToAp;
  WifiMgrUpdateConnectMessage (ConfigToken->Nic, TRUE, NULL);

Exit:

  if (ConfigToken->Nic->ConnectState == WifiMgrDisconnected) {
    ConfigToken->Nic->CurrentOperateNetwork = NULL;
  }
  ConfigToken->Nic->OneTimeConnectRequest = FALSE;
  WifiMgrFreeToken(ConfigToken);
}

/**
  Start connect operation, and send out a token to connect to a target network.

  @param[in]  Nic                 Pointer to the device data of the selected NIC.
  @param[in]  Profile             The target network to be connected.

  @retval EFI_SUCCESS             The operation is completed.
  @retval EFI_ALREADY_STARTED     Already in "connected" state, need to perform a disconnect
                                  operation first.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval Other Errors            Return errors when connecting network on low layer.

**/
EFI_STATUS
WifiMgrConnectToNetwork (
  IN    WIFI_MGR_DEVICE_DATA              *Nic,
  IN    WIFI_MGR_NETWORK_PROFILE          *Profile
  )
{
  EFI_STATUS                             Status;
  EFI_TPL                                OldTpl;
  EFI_ADAPTER_INFO_MEDIA_STATE           LinkState;
  WIFI_MGR_MAC_CONFIG_TOKEN              *ConfigToken;
  EFI_80211_CONNECT_NETWORK_TOKEN        *ConnectToken;

  if (Nic == NULL || Nic->Wmp == NULL || Profile == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = WifiMgrGetLinkState (Nic, &LinkState);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (LinkState.MediaState == EFI_SUCCESS) {
    return EFI_ALREADY_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  Status = WifiMgrPrepareConnection (Nic, Profile);
  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  //
  // Create a new connect token
  //
  ConfigToken = AllocateZeroPool (sizeof (WIFI_MGR_MAC_CONFIG_TOKEN));
  if (ConfigToken == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  ConfigToken->Type      = TokenTypeConnectNetworkToken;
  ConfigToken->Nic       = Nic;
  ConfigToken->Token.ConnectNetworkToken  = AllocateZeroPool (sizeof (EFI_80211_CONNECT_NETWORK_TOKEN));
  if (ConfigToken->Token.ConnectNetworkToken == NULL) {
    goto Exit;
  }

  ConnectToken           = ConfigToken->Token.ConnectNetworkToken;
  ConnectToken->Data     = AllocateZeroPool (sizeof (EFI_80211_CONNECT_NETWORK_DATA));
  if (ConnectToken->Data == NULL) {
    goto Exit;
  }

  ConnectToken->Data->Network = AllocateZeroPool (sizeof (EFI_80211_NETWORK));
  if (ConnectToken->Data->Network == NULL) {
    goto Exit;
  }
  CopyMem(ConnectToken->Data->Network, &Profile->Network, sizeof (EFI_80211_NETWORK));

  //
  // Add event handle and start to connect
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  WifiMgrOnConnectFinished,
                  ConfigToken,
                  &ConnectToken->Event
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Nic->ConnectState = WifiMgrConnectingToAp;
  Nic->CurrentOperateNetwork = Profile;
  WifiMgrUpdateConnectMessage (Nic, FALSE, NULL);

  //
  //Start Connecting ...
  //
  Status = Nic->Wmp->ConnectNetwork (Nic->Wmp, ConnectToken);

  //
  // Erase secrets after connection is triggered
  //
  WifiMgrCleanProfileSecrets (Profile);

  if (EFI_ERROR (Status)) {
    if (Status == EFI_ALREADY_STARTED) {
      Nic->ConnectState = WifiMgrConnectedToAp;
      WifiMgrUpdateConnectMessage (Nic, TRUE, NULL);
    } else {

      Nic->ConnectState          = WifiMgrDisconnected;
      Nic->CurrentOperateNetwork = NULL;

      if (Nic->OneTimeConnectRequest) {
        if (Status == EFI_NOT_FOUND) {
          WifiMgrUpdateConnectMessage (Nic, FALSE, L"Connect Failed: Not Available!");
        } else {
          WifiMgrUpdateConnectMessage (Nic, FALSE, L"Connect Failed: Unexpected Error!");
        }
      }
    }
    goto Exit;
  }

Exit:

  if (EFI_ERROR (Status)) {
    WifiMgrFreeToken (ConfigToken);
  }
  gBS->RestoreTPL (OldTpl);

  DEBUG ((DEBUG_INFO, "[WiFi Connection Manager] WifiMgrConnectToNetwork: %r\n", Status));
  return Status;
}

/**
  The callback function for disconnect operation.

  ASSERT when errors occur in config token.

  @param[in]  Event                 The Disconnect token receive event.
  @param[in]  Context               The context of the Disconnect token.

**/
VOID
EFIAPI
WifiMgrOnDisconnectFinished (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  WIFI_MGR_MAC_CONFIG_TOKEN         *ConfigToken;

  ASSERT (Context != NULL);

  ConfigToken = (WIFI_MGR_MAC_CONFIG_TOKEN*) Context;
  ASSERT (ConfigToken->Nic != NULL);
  ASSERT (ConfigToken->Type == TokenTypeDisconnectNetworkToken);

  ASSERT (ConfigToken->Token.DisconnectNetworkToken != NULL);
  if (ConfigToken->Token.DisconnectNetworkToken->Status != EFI_SUCCESS) {
    ConfigToken->Nic->ConnectState          = WifiMgrConnectedToAp;
    WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, NULL);
    ConfigToken->Nic->OneTimeDisconnectRequest = FALSE;
    goto Exit;
  }

  ConfigToken->Nic->ConnectState          = WifiMgrDisconnected;
  ConfigToken->Nic->CurrentOperateNetwork = NULL;
  WifiMgrUpdateConnectMessage (ConfigToken->Nic, TRUE, NULL);
  ConfigToken->Nic->OneTimeDisconnectRequest = FALSE;

  //
  // Disconnected network may not be in network list now, trigger a scan again!
  //
  ConfigToken->Nic->OneTimeScanRequest       = TRUE;

  Exit:
    WifiMgrFreeToken(ConfigToken);
    return;
}

/**
  Start disconnect operation, and send out a token to disconnect from current connected
  network.

  @param[in]  Nic                 Pointer to the device data of the selected NIC.

  @retval EFI_SUCCESS             The operation is completed.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval Other Errors            Return errors when disconnecting a network on low layer.

**/
EFI_STATUS
WifiMgrDisconnectToNetwork (
  IN    WIFI_MGR_DEVICE_DATA             *Nic
  )
{
  EFI_STATUS                             Status;
  EFI_TPL                                OldTpl;
  WIFI_MGR_MAC_CONFIG_TOKEN              *ConfigToken;
  EFI_80211_DISCONNECT_NETWORK_TOKEN     *DisconnectToken;

  if (Nic == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl      = gBS->RaiseTPL (TPL_CALLBACK);
  Status      = EFI_SUCCESS;
  ConfigToken = AllocateZeroPool (sizeof (WIFI_MGR_MAC_CONFIG_TOKEN));
  if (ConfigToken == NULL) {
    gBS->RestoreTPL (OldTpl);
    return EFI_OUT_OF_RESOURCES;
  }

  ConfigToken->Type      = TokenTypeDisconnectNetworkToken;
  ConfigToken->Nic       = Nic;
  ConfigToken->Token.DisconnectNetworkToken = AllocateZeroPool (sizeof (EFI_80211_DISCONNECT_NETWORK_TOKEN));
  if (ConfigToken->Token.DisconnectNetworkToken == NULL) {
    WifiMgrFreeToken(ConfigToken);
    gBS->RestoreTPL (OldTpl);
    return EFI_OUT_OF_RESOURCES;
  }

  DisconnectToken = ConfigToken->Token.DisconnectNetworkToken;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  WifiMgrOnDisconnectFinished,
                  ConfigToken,
                  &DisconnectToken->Event
                  );
  if (EFI_ERROR (Status)) {
    WifiMgrFreeToken(ConfigToken);
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  Nic->ConnectState = WifiMgrDisconnectingToAp;
  WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, NULL);

  Status = Nic->Wmp->DisconnectNetwork (Nic->Wmp, DisconnectToken);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {

      Nic->ConnectState          = WifiMgrDisconnected;
      Nic->CurrentOperateNetwork = NULL;

      //
      // This network is not in network list now, trigger a scan again!
      //
      Nic->OneTimeScanRequest    = TRUE;

      //
      // State has been changed from Connected to Disconnected
      //
      WifiMgrUpdateConnectMessage (ConfigToken->Nic, TRUE, NULL);
      Status                     = EFI_SUCCESS;
    } else {
      if (Nic->OneTimeDisconnectRequest) {

        WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, L"Disconnect Failed: Unexpected Error!");
      }

      Nic->ConnectState     = WifiMgrConnectedToAp;
      WifiMgrUpdateConnectMessage (ConfigToken->Nic, FALSE, NULL);
    }
    WifiMgrFreeToken(ConfigToken);
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  The state machine of the connection manager, periodically check the state and
  perform a corresponding operation.

  @param[in]  Event                   The timer event to be triggered.
  @param[in]  Context                 The context of the Nic device data.

**/
VOID
EFIAPI
WifiMgrOnTimerTick (
  IN EFI_EVENT                        Event,
  IN VOID                             *Context
  )
{
  WIFI_MGR_DEVICE_DATA                *Nic;
  EFI_STATUS                          Status;
  EFI_ADAPTER_INFO_MEDIA_STATE        LinkState;
  WIFI_MGR_NETWORK_PROFILE            *Profile;

  if (Context == NULL) {
    return;
  }

  Nic = (WIFI_MGR_DEVICE_DATA*) Context;
  NET_CHECK_SIGNATURE (Nic, WIFI_MGR_DEVICE_DATA_SIGNATURE);

  Status = WifiMgrGetLinkState (Nic, &LinkState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[WiFi Connection Manager] Error: Failed to get link state!\n"));
    return;
  }

  if (Nic->LastLinkState.MediaState != LinkState.MediaState) {
    if (Nic->LastLinkState.MediaState == EFI_SUCCESS && LinkState.MediaState == EFI_NO_MEDIA) {
      Nic->HasDisconnectPendingNetwork = TRUE;
    }
    Nic->LastLinkState.MediaState = LinkState.MediaState;
  }

  Nic->ScanTickTime ++;
  if ((Nic->ScanTickTime > WIFI_SCAN_FREQUENCY || Nic->OneTimeScanRequest) &&
    Nic->ScanState == WifiMgrScanFinished) {

    Nic->OneTimeScanRequest = FALSE;
    Nic->ScanTickTime = 0;

    DEBUG ((DEBUG_INFO, "[WiFi Connection Manager] Scan is triggered.\n"));
    WifiMgrStartScan (Nic);
  }

  if (Nic->AvailableCount > 0 && Nic->ScanState == WifiMgrScanFinished) {

    switch (Nic->ConnectState) {
    case WifiMgrDisconnected:

      if (Nic->HasDisconnectPendingNetwork) {
        Nic->HasDisconnectPendingNetwork = FALSE;
      }

      if (Nic->ConnectPendingNetwork != NULL) {

        Profile   = Nic->ConnectPendingNetwork;
        Status    = WifiMgrConnectToNetwork(Nic, Profile);
        Nic->ConnectPendingNetwork = NULL;
        if (EFI_ERROR (Status)) {
          //
          // Some error happened, don't wait for a return connect token!
          //
          Nic->OneTimeConnectRequest = FALSE;
        }
      }
      break;

    case WifiMgrConnectingToAp:
      break;

    case WifiMgrDisconnectingToAp:
      break;

    case WifiMgrConnectedToAp:

      if (Nic->ConnectPendingNetwork != NULL || Nic->HasDisconnectPendingNetwork) {

        Status    = WifiMgrDisconnectToNetwork(Nic);
        if (EFI_ERROR (Status)) {
          //
          // Some error happened, don't wait for a return disconnect token!
          //
          Nic->OneTimeDisconnectRequest = FALSE;
        }
      }
      break;

    default:
      break;
    }
  }
}
