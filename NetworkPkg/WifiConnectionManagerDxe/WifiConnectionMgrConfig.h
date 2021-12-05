/** @file
  Define network structure used by the WiFi Connection Manager.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _WIFI_MGR_CONFIG_H_
#define _WIFI_MGR_CONFIG_H_

#include "WifiConnectionMgrConfigNVDataStruct.h"

extern UINT8  WifiConnectionManagerDxeBin[];
extern UINT8  WifiConnectionManagerDxeStrings[];

typedef struct {
  UINT32               Signature;

  //
  // Link to the current profile list in NIC device data (WIFI_MGR_DEVICE_DATA)
  //
  LIST_ENTRY           Link;

  UINT32               NicIndex;
  UINT32               ProfileIndex;   // The unique identifier for network profile, starts from 1
  CHAR16               SSId[SSID_STORAGE_SIZE];
  CHAR16               Password[PASSWORD_STORAGE_SIZE];

  UINT8                SecurityType;
  UINT8                EapAuthMethod;

  CHAR16               CACertName[WIFI_FILENAME_STR_MAX_SIZE];
  VOID                 *CACertData;
  UINTN                CACertSize;
  CHAR16               ClientCertName[WIFI_FILENAME_STR_MAX_SIZE];
  VOID                 *ClientCertData;
  UINTN                ClientCertSize;
  CHAR16               PrivateKeyName[WIFI_FILENAME_STR_MAX_SIZE];
  VOID                 *PrivateKeyData;
  UINTN                PrivateKeyDataSize;
  CHAR16               PrivateKeyPassword[PASSWORD_STORAGE_SIZE];    // Password to protect private key file
  CHAR16               EapIdentity[EAP_IDENTITY_SIZE];
  CHAR16               EapPassword[PASSWORD_STORAGE_SIZE];
  UINT8                EapSecondAuthMethod;

  BOOLEAN              AKMSuiteSupported;
  BOOLEAN              CipherSuiteSupported;
  BOOLEAN              IsAvailable;
  EFI_80211_NETWORK    Network;
  UINT8                NetworkQuality;
  EFI_STRING_ID        TitleToken;
} WIFI_MGR_NETWORK_PROFILE;

#define WIFI_MGR_PROFILE_SIGNATURE  SIGNATURE_32 ('W','M','N','P')

#pragma pack(1)
///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;
#pragma pack()

#endif
