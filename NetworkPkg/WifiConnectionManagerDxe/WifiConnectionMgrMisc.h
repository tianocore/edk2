/** @file
  The Miscellaneous Routines for WiFi Connection Manager.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_WIFI_MGR_MISC_H__
#define __EFI_WIFI_MGR_MISC_H__

/**
  Empty function for event process function.

  @param[in] Event    The Event needs to be processed
  @param[in] Context  The context of the event

**/
VOID
EFIAPI
WifiMgrInternalEmptyFunction (
  IN  EFI_EVENT   Event,
  IN  VOID        *Context
  );

/**
  Convert the mac address into a hexadecimal encoded ":" seperated string.

  @param[in]  Mac     The mac address
  @param[in]  StrSize The size, in bytes, of the output buffer specified by Str
  @param[out] Str     The storage to return the mac string

**/
VOID
WifiMgrMacAddrToStr (
  IN  EFI_80211_MAC_ADDRESS     *Mac,
  IN  UINT32                    StrSize,
  OUT CHAR16                    *Str
  );

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
  OUT  VOID                           **PrivateKeyDataAddr,
  OUT  UINTN                          *PrivateKeyDataSize
  );


/**
  Get the Nic data by the NicIndex.

  @param[in]  Private        The pointer to the global private data structure.
  @param[in]  NicIndex       The index indicates the position of wireless NIC.

  @return     Pointer to the Nic data, or NULL if not found.

**/
WIFI_MGR_DEVICE_DATA *
WifiMgrGetNicByIndex (
  IN WIFI_MGR_PRIVATE_DATA      *Private,
  IN UINT32                     NicIndex
  );

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
  );

/**
  Find a network profile through its' SSId and securit type, and the SSId is an ascii string.

  @param[in]  SSId                   The target network's SSId.
  @param[in]  SecurityType           The target network's security type.
  @param[in]  ProfileList            The profile list on a Nic.

  @return Pointer to a network profile, or NULL if not found.

**/
WIFI_MGR_NETWORK_PROFILE *
WifiMgrGetProfileByAsciiSSId (
  IN  CHAR8                          *SSId,
  IN  UINT8                          SecurityType,
  IN  LIST_ENTRY                     *ProfileList
  );

/**
  Find a network profile through its' profile index.

  @param[in]  ProfileIndex           The target network's profile index.
  @param[in]  ProfileList            The profile list on a Nic.

  @return Pointer to a network profile, or NULL if not found.

**/
WIFI_MGR_NETWORK_PROFILE *
WifiMgrGetProfileByProfileIndex (
  IN UINT32                     ProfileIndex,
  IN LIST_ENTRY                 *ProfileList
  );

/**
  To test if the AKMSuite is in supported AKMSuite list.

  @param[in]  SupportedAKMSuiteCount      The count of the supported AKMSuites.
  @param[in]  SupportedAKMSuiteList       The supported AKMSuite list.
  @param[in]  AKMSuite                    The AKMSuite to be tested.

  @return True if this AKMSuite is supported, or False if not.

**/
BOOLEAN
WifiMgrSupportAKMSuite (
  IN  UINT16                              SupportedAKMSuiteCount,
  IN  UINT32                              *SupportedAKMSuiteList,
  IN  UINT32                              *AKMSuite
  );

/**
  To check if the CipherSuite is in supported CipherSuite list.

  @param[in]  SupportedCipherSuiteCount   The count of the supported CipherSuites.
  @param[in]  SupportedCipherSuiteList    The supported CipherSuite list.
  @param[in]  CipherSuite                 The CipherSuite to be tested.

  @return True if this CipherSuite is supported, or False if not.

**/
BOOLEAN
WifiMgrSupportCipherSuite (
  IN  UINT16                              SupportedCipherSuiteCount,
  IN  UINT32                              *SupportedCipherSuiteList,
  IN  UINT32                              *CipherSuite
  );

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
  IN    EFI_80211_AKM_SUITE_SELECTOR      *AKMList,
  IN    EFI_80211_CIPHER_SUITE_SELECTOR   *CipherList,
  IN    WIFI_MGR_DEVICE_DATA              *Nic,
  OUT   UINT8                             *SecurityType,
  OUT   BOOLEAN                           *AKMSuiteSupported,
  OUT   BOOLEAN                           *CipherSuiteSupported
  );

/**
  To get the security type for a certain AKMSuite and CipherSuite.

  @param[in]   AKMSuite             An certain AKMSuite.
  @param[in]   CipherSuite          An certain CipherSuite.

  @return a security type if found, or SECURITY_TYPE_UNKNOWN.

**/
UINT8
WifiMgrGetSecurityType (
  IN    UINT32                          *AKMSuite,
  IN    UINT32                          *CipherSuite
  );

/**
  Get supported AKMSuites and CipherSuites from supplicant.

  @param[in]   Nic                      The Nic to operate.

  @retval EFI_SUCCESS                   Get the supported suite list successfully.
  @retval EFI_INVALID_PARAMETER         No Nic found or supplicant is NULL.

**/
EFI_STATUS
WifiMgrGetSupportedSuites (
  IN    WIFI_MGR_DEVICE_DATA            *Nic
  );

/**
  Clean secrets from a network profile.

  @param[in]   Profile               The profile to be cleanned.

**/
VOID
WifiMgrCleanProfileSecrets (
  IN  WIFI_MGR_NETWORK_PROFILE       *Profile
  );

/**
  Free all network profiles in a profile list.

  @param[in]   ProfileList           The profile list to be freed.

**/
VOID
WifiMgrFreeProfileList (
  IN  LIST_ENTRY               *ProfileList
  );

/**
  Free user configured hidden network list.

  @param[in]   HiddenList           The hidden network list to be freed.

**/
VOID
WifiMgrFreeHiddenList (
  IN  LIST_ENTRY                     *HiddenList
  );

/**
  Free the resources of a config token.

  @param[in]   ConfigToken          The config token to be freed.

**/
VOID
WifiMgrFreeToken (
  IN   WIFI_MGR_MAC_CONFIG_TOKEN        *ConfigToken
  );

#endif
