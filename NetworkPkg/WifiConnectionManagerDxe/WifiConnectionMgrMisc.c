/** @file
  The Miscellaneous Routines for WiFi Connection Manager.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "WifiConnectionMgrDxe.h"

/**
  Empty function for event process function.

  @param Event    The Event need to be process
  @param Context  The context of the event.

**/
VOID
EFIAPI
WifiMgrInternalEmptyFunction (
  IN  EFI_EVENT    Event,
  IN  VOID         *Context
  )
{
  return;
}

/**
  Convert the mac address into a hexadecimal encoded ":" seperated string.

  @param[in]  Mac     The mac address.
  @param[in]  StrSize The size, in bytes, of the output buffer specified by Str.
  @param[out] Str     The storage to return the mac string.

**/
VOID
WifiMgrMacAddrToStr (
  IN  EFI_80211_MAC_ADDRESS  *Mac,
  IN  UINT32                 StrSize,
  OUT CHAR16                 *Str
  )
{
  if (Mac == NULL || Str == NULL) {
    return;
  }

  UnicodeSPrint (
    Str,
    StrSize,
    L"%02X:%02X:%02X:%02X:%02X:%02X",
    Mac->Addr[0], Mac->Addr[1], Mac->Addr[2],
    Mac->Addr[3], Mac->Addr[4], Mac->Addr[5]
    );
}

/**
  Read private key file to buffer.

  @param[in]   FileContext           The file context of private key file.
  @param[out]  PrivateKeyDataAddr    The buffer address to restore private key file, should be
                                     freed by caller.
  @param[out]  PrivateKeyDataSize    The size of read private key file.

  @retval EFI_SUCCESS                Successfully read the private key file.
  @retval EFI_INVALID_PARAMETER      One or more of the parameters is invalid.

**/
EFI_STATUS
WifiMgrReadFileToBuffer (
  IN   WIFI_MGR_FILE_CONTEXT          *FileContext,
  OUT  VOID                           **DataAddr,
  OUT  UINTN                          *DataSize
  )
{
  EFI_STATUS    Status;

  if (FileContext != NULL && FileContext->FHandle != NULL) {

    Status = ReadFileContent (
               FileContext->FHandle,
               DataAddr,
               DataSize,
               0
               );

    if (FileContext->FHandle != NULL) {
      FileContext->FHandle->Close (FileContext->FHandle);
    }
    FileContext->FHandle = NULL;
    return Status;
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Get the Nic data by the NicIndex.

  @param[in]  Private        The pointer to the global private data structure.
  @param[in]  NicIndex       The index indicates the position of wireless NIC.

  @return     Pointer to the Nic data, or NULL if not found.

**/
WIFI_MGR_DEVICE_DATA *
WifiMgrGetNicByIndex (
  IN WIFI_MGR_PRIVATE_DATA   *Private,
  IN UINT32                  NicIndex
  )
{
  LIST_ENTRY             *Entry;
  WIFI_MGR_DEVICE_DATA   *Nic;

  if (Private == NULL) {
    return NULL;
  }

  NET_LIST_FOR_EACH (Entry, &Private->NicList) {
    Nic = NET_LIST_USER_STRUCT_S (Entry, WIFI_MGR_DEVICE_DATA,
            Link, WIFI_MGR_DEVICE_DATA_SIGNATURE);
    if (Nic->NicIndex == NicIndex) {
      return Nic;
    }
  }

  return NULL;
}

/**
  Find a network profile through its' SSId and securit type, and the SSId is an unicode string.

  @param[in]  SSId                   The target network's SSId.
  @param[in]  SecurityType           The target network's security type.
  @param[in]  ProfileList            The profile list on a Nic.

  @return Pointer to a network profile, or NULL if not found.

**/
WIFI_MGR_NETWORK_PROFILE *
WifiMgrGetProfileByUnicodeSSId (
  IN  CHAR16                         *SSId,
  IN  UINT8                          SecurityType,
  IN  LIST_ENTRY                     *ProfileList
  )
{
  LIST_ENTRY                         *Entry;
  WIFI_MGR_NETWORK_PROFILE           *Profile;

  if (SSId == NULL || ProfileList == NULL) {
    return NULL;
  }

  NET_LIST_FOR_EACH (Entry, ProfileList) {
    Profile = NET_LIST_USER_STRUCT_S (Entry, WIFI_MGR_NETWORK_PROFILE,
                Link, WIFI_MGR_PROFILE_SIGNATURE);
    if (StrCmp (SSId, Profile->SSId) == 0 && SecurityType == Profile->SecurityType) {
      return Profile;
    }
  }

  return NULL;
}

/**
  Find a network profile through its' SSId and securit type, and the SSId is an ascii string.

  @param[in]  SSId               The target network's SSId.
  @param[in]  SecurityType       The target network's security type.
  @param[in]  ProfileList        The profile list on a Nic.

  @return Pointer to a network profile, or NULL if not found.

**/
WIFI_MGR_NETWORK_PROFILE *
WifiMgrGetProfileByAsciiSSId (
  IN  CHAR8                      *SSId,
  IN  UINT8                      SecurityType,
  IN  LIST_ENTRY                 *ProfileList
  )
{
  CHAR16   SSIdUniCode[SSID_STORAGE_SIZE];

  if (SSId == NULL) {
    return NULL;
  }
  if (AsciiStrToUnicodeStrS (SSId, SSIdUniCode, SSID_STORAGE_SIZE) != RETURN_SUCCESS) {
    return NULL;
  }

  return WifiMgrGetProfileByUnicodeSSId (SSIdUniCode, SecurityType, ProfileList);
}

/**
  Find a network profile through its' profile index.

  @param[in]  ProfileIndex           The target network's profile index.
  @param[in]  ProfileList            The profile list on a Nic.

  @return Pointer to a network profile, or NULL if not found.

**/
WIFI_MGR_NETWORK_PROFILE *
WifiMgrGetProfileByProfileIndex (
  IN  UINT32                         ProfileIndex,
  IN  LIST_ENTRY                     *ProfileList
  )
{
  WIFI_MGR_NETWORK_PROFILE           *Profile;
  LIST_ENTRY                         *Entry;

  if (ProfileList == NULL) {
    return NULL;
  }
  NET_LIST_FOR_EACH (Entry, ProfileList) {
    Profile = NET_LIST_USER_STRUCT_S (Entry, WIFI_MGR_NETWORK_PROFILE,
                Link, WIFI_MGR_PROFILE_SIGNATURE);
    if (Profile->ProfileIndex == ProfileIndex) {
      return Profile;
    }
  }
  return NULL;
}

/**
  To test if the AKMSuite is in supported AKMSuite list.

  @param[in]  SupportedAKMSuiteCount       The count of the supported AKMSuites.
  @param[in]  SupportedAKMSuiteList        The supported AKMSuite list.
  @param[in]  AKMSuite                     The AKMSuite to be tested.

  @return True if this AKMSuite is supported, or False if not.

**/
BOOLEAN
WifiMgrSupportAKMSuite (
  IN  UINT16                               SupportedAKMSuiteCount,
  IN  UINT32                               *SupportedAKMSuiteList,
  IN  UINT32                               *AKMSuite
  )
{
  UINT16    Index;

  if (AKMSuite == NULL || SupportedAKMSuiteList == NULL ||
    SupportedAKMSuiteCount == 0) {
    return FALSE;
  }

  for (Index = 0; Index < SupportedAKMSuiteCount; Index ++) {
    if (SupportedAKMSuiteList[Index] == *AKMSuite) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  To check if the CipherSuite is in supported CipherSuite list.

  @param[in]  SupportedCipherSuiteCount       The count of the supported CipherSuites.
  @param[in]  SupportedCipherSuiteList        The supported CipherSuite list.
  @param[in]  CipherSuite                     The CipherSuite to be tested.

  @return True if this CipherSuite is supported, or False if not.

**/
BOOLEAN
WifiMgrSupportCipherSuite (
  IN  UINT16                                  SupportedCipherSuiteCount,
  IN  UINT32                                  *SupportedCipherSuiteList,
  IN  UINT32                                  *CipherSuite
  )
{
  UINT16  Index;

  if (CipherSuite == NULL || SupportedCipherSuiteCount == 0 ||
    SupportedCipherSuiteList == NULL) {
    return FALSE;
  }

  for (Index = 0; Index < SupportedCipherSuiteCount; Index ++) {
    if (SupportedCipherSuiteList[Index] == *CipherSuite) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Check an AKM suite list and a Cipher suite list to see if one or more AKM suites or Cipher suites
  are supported and find the matchable security type.

  @param[in]   AKMList                     The target AKM suite list to be checked.
  @param[in]   CipherList                  The target Cipher suite list to be checked
  @param[in]   Nic                         The Nic to operate, contains the supported AKMSuite list
                                           and supported CipherSuite list
  @param[out]  SecurityType                To identify a security type from the AKM suite list and
                                           Cipher suite list
  @param[out]  AKMSuiteSupported           To identify if this security type is supported. If it is
                                           NULL, overcome this field
  @param[out]  CipherSuiteSupported        To identify if this security type is supported. If it is
                                           NULL, overcome this field

  @retval EFI_SUCCESS                      This operation has completed successfully.
  @retval EFI_INVALID_PARAMETER            No Nic found or the suite list is null.

**/
EFI_STATUS
WifiMgrCheckRSN (
  IN    EFI_80211_AKM_SUITE_SELECTOR       *AKMList,
  IN    EFI_80211_CIPHER_SUITE_SELECTOR    *CipherList,
  IN    WIFI_MGR_DEVICE_DATA               *Nic,
  OUT   UINT8                              *SecurityType,
  OUT   BOOLEAN                            *AKMSuiteSupported,
  OUT   BOOLEAN                            *CipherSuiteSupported
  )
{
  EFI_80211_AKM_SUITE_SELECTOR             *SupportedAKMSuites;
  EFI_80211_CIPHER_SUITE_SELECTOR          *SupportedSwCipherSuites;
  EFI_80211_CIPHER_SUITE_SELECTOR          *SupportedHwCipherSuites;
  EFI_80211_SUITE_SELECTOR                 *AKMSuite;
  EFI_80211_SUITE_SELECTOR                 *CipherSuite;
  UINT16                                   AKMIndex;
  UINT16                                   CipherIndex;

  if (Nic == NULL || AKMList == NULL || CipherList == NULL|| SecurityType == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SupportedAKMSuites      = Nic->SupportedSuites.SupportedAKMSuites;
  SupportedSwCipherSuites = Nic->SupportedSuites.SupportedSwCipherSuites;
  SupportedHwCipherSuites = Nic->SupportedSuites.SupportedHwCipherSuites;

  *SecurityType = SECURITY_TYPE_UNKNOWN;
  if (AKMSuiteSupported != NULL && CipherSuiteSupported != NULL) {
    *AKMSuiteSupported    = FALSE;
    *CipherSuiteSupported = FALSE;
  }

  if (AKMList->AKMSuiteCount == 0) {
    if (CipherList->CipherSuiteCount == 0) {
      *SecurityType = SECURITY_TYPE_NONE;
      if (AKMSuiteSupported != NULL && CipherSuiteSupported != NULL) {
        *AKMSuiteSupported    = TRUE;
        *CipherSuiteSupported = TRUE;
      }
    }

    return EFI_SUCCESS;
  }

  for (AKMIndex = 0; AKMIndex < AKMList->AKMSuiteCount; AKMIndex ++) {

    AKMSuite = AKMList->AKMSuiteList + AKMIndex;
    if (WifiMgrSupportAKMSuite(SupportedAKMSuites->AKMSuiteCount,
      (UINT32*) SupportedAKMSuites->AKMSuiteList, (UINT32*) AKMSuite)) {

      if (AKMSuiteSupported != NULL && CipherSuiteSupported != NULL) {
        *AKMSuiteSupported = TRUE;
      }
      for (CipherIndex = 0; CipherIndex < CipherList->CipherSuiteCount; CipherIndex ++) {

        CipherSuite = CipherList->CipherSuiteList + CipherIndex;

        if (SupportedSwCipherSuites != NULL) {

          if (WifiMgrSupportCipherSuite(SupportedSwCipherSuites->CipherSuiteCount,
            (UINT32*) SupportedSwCipherSuites->CipherSuiteList, (UINT32*) CipherSuite)) {

            *SecurityType = WifiMgrGetSecurityType ((UINT32*) AKMSuite, (UINT32*) CipherSuite);

            if (*SecurityType != SECURITY_TYPE_UNKNOWN) {

              if (AKMSuiteSupported != NULL && CipherSuiteSupported != NULL) {
                *CipherSuiteSupported = TRUE;
              }
              return EFI_SUCCESS;
            }
          }
        }

        if (SupportedHwCipherSuites != NULL) {

          if (WifiMgrSupportCipherSuite(SupportedHwCipherSuites->CipherSuiteCount,
            (UINT32*) SupportedHwCipherSuites->CipherSuiteList, (UINT32*) CipherSuite)) {

            *SecurityType = WifiMgrGetSecurityType ((UINT32*) AKMSuite, (UINT32*) CipherSuite);

            if (*SecurityType != SECURITY_TYPE_UNKNOWN) {

              if (AKMSuiteSupported != NULL && CipherSuiteSupported != NULL) {
                *CipherSuiteSupported = TRUE;
              }
              return EFI_SUCCESS;
            }
          }
        }
      }
    }
  }

  *SecurityType = WifiMgrGetSecurityType ((UINT32*) AKMList->AKMSuiteList,
                    (UINT32*) CipherList->CipherSuiteList);

  return EFI_SUCCESS;
}

/**
  Get the security type for a certain AKMSuite and CipherSuite.

  @param[in]   AKMSuite             An certain AKMSuite.
  @param[in]   CipherSuite          An certain CipherSuite.

  @return a security type if found, or SECURITY_TYPE_UNKNOWN.

**/
UINT8
WifiMgrGetSecurityType (
  IN  UINT32    *AKMSuite,
  IN  UINT32    *CipherSuite
  )
{
  if (CipherSuite == NULL) {

    if (AKMSuite == NULL) {
      return SECURITY_TYPE_NONE;
    } else {
      return SECURITY_TYPE_UNKNOWN;
    }
  } else if (*CipherSuite == IEEE_80211_PAIRWISE_CIPHER_SUITE_USE_GROUP) {

    if (AKMSuite == NULL) {
      return SECURITY_TYPE_NONE;
    } else {
      return SECURITY_TYPE_UNKNOWN;
    }
  } else if (*CipherSuite == IEEE_80211_PAIRWISE_CIPHER_SUITE_WEP40 ||
    *CipherSuite == IEEE_80211_PAIRWISE_CIPHER_SUITE_WEP104) {

    return SECURITY_TYPE_WEP;
  } else if (*CipherSuite == IEEE_80211_PAIRWISE_CIPHER_SUITE_CCMP) {

    if (AKMSuite == NULL) {
      return SECURITY_TYPE_UNKNOWN;
    }

    if (*AKMSuite == IEEE_80211_AKM_SUITE_8021X_OR_PMKSA ||
      *AKMSuite == IEEE_80211_AKM_SUITE_8021X_OR_PMKSA_SHA256) {

      return SECURITY_TYPE_WPA2_ENTERPRISE;
    } else if (*AKMSuite == IEEE_80211_AKM_SUITE_PSK ||
        *AKMSuite == IEEE_80211_AKM_SUITE_PSK_SHA256){

      return SECURITY_TYPE_WPA2_PERSONAL;
    }else {
      return SECURITY_TYPE_UNKNOWN;
    }
  } else if (*CipherSuite == IEEE_80211_PAIRWISE_CIPHER_SUITE_TKIP) {

    if (AKMSuite == NULL) {
      return SECURITY_TYPE_UNKNOWN;
    }

    if (*AKMSuite == IEEE_80211_AKM_SUITE_8021X_OR_PMKSA ||
      *AKMSuite == IEEE_80211_AKM_SUITE_8021X_OR_PMKSA_SHA256) {

      return SECURITY_TYPE_WPA_ENTERPRISE;
    } else if (*AKMSuite == IEEE_80211_AKM_SUITE_PSK ||
        *AKMSuite == IEEE_80211_AKM_SUITE_PSK_SHA256){

      return SECURITY_TYPE_WPA_PERSONAL;
    }else {
      return SECURITY_TYPE_UNKNOWN;
    }
  } else {
    return SECURITY_TYPE_UNKNOWN;
  }
}

/**
  Get supported AKMSuites and CipherSuites from supplicant for a Nic.

  @param[in]   Nic                      The Nic to operate.

  @retval EFI_SUCCESS                   Get the supported suite list successfully.
  @retval EFI_INVALID_PARAMETER         No Nic found or supplicant is NULL.

**/
EFI_STATUS
WifiMgrGetSupportedSuites (
  IN    WIFI_MGR_DEVICE_DATA            *Nic
  )
{
  EFI_STATUS                            Status;
  EFI_SUPPLICANT_PROTOCOL               *Supplicant;
  EFI_80211_AKM_SUITE_SELECTOR          *SupportedAKMSuites;
  EFI_80211_CIPHER_SUITE_SELECTOR       *SupportedSwCipherSuites;
  EFI_80211_CIPHER_SUITE_SELECTOR       *SupportedHwCipherSuites;
  UINTN                                 DataSize;

  SupportedAKMSuites      = NULL;
  SupportedSwCipherSuites = NULL;
  SupportedHwCipherSuites = NULL;

  if (Nic == NULL || Nic->Supplicant == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Supplicant = Nic->Supplicant;

  DataSize  = 0;
  Status = Supplicant->GetData (Supplicant, EfiSupplicant80211SupportedAKMSuites, NULL, &DataSize);
  if (Status == EFI_BUFFER_TOO_SMALL && DataSize > 0) {

    SupportedAKMSuites = AllocateZeroPool(DataSize);
    if (SupportedAKMSuites == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = Supplicant->GetData (Supplicant, EfiSupplicant80211SupportedAKMSuites,
                           (UINT8 *) SupportedAKMSuites, &DataSize);
    if (!EFI_ERROR (Status)) {
      Nic->SupportedSuites.SupportedAKMSuites = SupportedAKMSuites;
    } else {
      FreePool (SupportedAKMSuites);
    }
  } else {
    SupportedAKMSuites = NULL;
  }

  DataSize  = 0;
  Status = Supplicant->GetData (Supplicant, EfiSupplicant80211SupportedSoftwareCipherSuites, NULL, &DataSize);
  if (Status == EFI_BUFFER_TOO_SMALL && DataSize > 0) {


    SupportedSwCipherSuites = AllocateZeroPool(DataSize);
    if (SupportedSwCipherSuites == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = Supplicant->GetData (Supplicant, EfiSupplicant80211SupportedSoftwareCipherSuites,
                           (UINT8 *) SupportedSwCipherSuites, &DataSize);
    if (!EFI_ERROR (Status)) {
      Nic->SupportedSuites.SupportedSwCipherSuites = SupportedSwCipherSuites;
    } else {
      FreePool (SupportedSwCipherSuites);
    }
  } else {
    SupportedSwCipherSuites = NULL;
  }

  DataSize  = 0;
  Status = Supplicant->GetData (Supplicant, EfiSupplicant80211SupportedHardwareCipherSuites, NULL, &DataSize);
  if (Status == EFI_BUFFER_TOO_SMALL && DataSize > 0) {

    SupportedHwCipherSuites = AllocateZeroPool(DataSize);
    if (SupportedHwCipherSuites == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = Supplicant->GetData (Supplicant, EfiSupplicant80211SupportedHardwareCipherSuites,
                           (UINT8 *) SupportedHwCipherSuites, &DataSize);
    if (!EFI_ERROR (Status)) {
      Nic->SupportedSuites.SupportedHwCipherSuites = SupportedHwCipherSuites;
    } else {
      FreePool (SupportedHwCipherSuites);
    }
  } else {
    SupportedHwCipherSuites = NULL;
  }

  return EFI_SUCCESS;
}

/**
  Clean secrets from a network profile.

  @param[in]   Profile               The profile to be cleanned.

**/
VOID
WifiMgrCleanProfileSecrets (
  IN  WIFI_MGR_NETWORK_PROFILE       *Profile
  )
{
  ZeroMem (Profile->Password, sizeof (CHAR16) * PASSWORD_STORAGE_SIZE);
  ZeroMem (Profile->EapPassword, sizeof (CHAR16) * PASSWORD_STORAGE_SIZE);
  ZeroMem (Profile->PrivateKeyPassword, sizeof (CHAR16) * PASSWORD_STORAGE_SIZE);

  if (Profile->CACertData != NULL) {

    ZeroMem (Profile->CACertData, Profile->CACertSize);
    FreePool (Profile->CACertData);
  }
  Profile->CACertData = NULL;
  Profile->CACertSize = 0;

  if (Profile->ClientCertData != NULL) {

    ZeroMem (Profile->ClientCertData, Profile->ClientCertSize);
    FreePool (Profile->ClientCertData);
  }
  Profile->ClientCertData = NULL;
  Profile->ClientCertSize = 0;

  if (Profile->PrivateKeyData != NULL) {

    ZeroMem (Profile->PrivateKeyData, Profile->PrivateKeyDataSize);
    FreePool (Profile->PrivateKeyData);
  }
  Profile->PrivateKeyData     = NULL;
  Profile->PrivateKeyDataSize = 0;
}

/**
  Free all network profiles in a profile list.

  @param[in]   ProfileList           The profile list to be freed.

**/
VOID
WifiMgrFreeProfileList (
  IN  LIST_ENTRY                     *ProfileList
  )
{
  WIFI_MGR_NETWORK_PROFILE           *Profile;
  LIST_ENTRY                         *Entry;
  LIST_ENTRY                         *NextEntry;

  if (ProfileList == NULL) {
    return;
  }

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, ProfileList) {

    Profile = NET_LIST_USER_STRUCT_S (Entry, WIFI_MGR_NETWORK_PROFILE,
                Link, WIFI_MGR_PROFILE_SIGNATURE);

    WifiMgrCleanProfileSecrets (Profile);

    if (Profile->Network.AKMSuite != NULL) {
      FreePool(Profile->Network.AKMSuite);
    }

    if (Profile->Network.CipherSuite != NULL) {
      FreePool(Profile->Network.CipherSuite);
    }

    FreePool (Profile);
  }
}

/**
  Free user configured hidden network list.

  @param[in]   HiddenList           The hidden network list to be freed.

**/
VOID
WifiMgrFreeHiddenList (
  IN  LIST_ENTRY                     *HiddenList
  )
{
  WIFI_HIDDEN_NETWORK_DATA           *HiddenNetwork;
  LIST_ENTRY                         *Entry;
  LIST_ENTRY                         *NextEntry;

  if (HiddenList == NULL) {
    return;
  }

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, HiddenList) {

    HiddenNetwork = NET_LIST_USER_STRUCT_S (Entry, WIFI_HIDDEN_NETWORK_DATA,
                      Link, WIFI_MGR_HIDDEN_NETWORK_SIGNATURE);
    FreePool (HiddenNetwork);
  }
}


/**
  Free the resources of a config token.

  @param[in]   ConfigToken          The config token to be freed.
**/
VOID
WifiMgrFreeToken (
  IN   WIFI_MGR_MAC_CONFIG_TOKEN    *ConfigToken
  )
{
  EFI_80211_GET_NETWORKS_RESULT     *Result;

  if (ConfigToken == NULL) {
    return;
  }

  switch (ConfigToken->Type) {

    case TokenTypeGetNetworksToken:

      if (ConfigToken->Token.GetNetworksToken != NULL) {

        gBS->CloseEvent (ConfigToken->Token.GetNetworksToken->Event);
        if (ConfigToken->Token.GetNetworksToken->Data != NULL) {
          FreePool(ConfigToken->Token.GetNetworksToken->Data);
        }

        Result = ConfigToken->Token.GetNetworksToken->Result;
        if (Result != NULL) {
          FreePool (Result);
        }

        FreePool(ConfigToken->Token.GetNetworksToken);
      }

      FreePool (ConfigToken);
      break;

    case TokenTypeConnectNetworkToken:

      if (ConfigToken->Token.ConnectNetworkToken != NULL) {

        gBS->CloseEvent (ConfigToken->Token.ConnectNetworkToken->Event);
        if (ConfigToken->Token.ConnectNetworkToken->Data != NULL) {
          FreePool(ConfigToken->Token.ConnectNetworkToken->Data);
        }
        FreePool(ConfigToken->Token.ConnectNetworkToken);
      }
      FreePool (ConfigToken);
      break;

    case TokenTypeDisconnectNetworkToken:

      if (ConfigToken->Token.DisconnectNetworkToken != NULL) {

        FreePool(ConfigToken->Token.DisconnectNetworkToken);
      }

      FreePool (ConfigToken);
      break;

    default :
      break;
  }
}
