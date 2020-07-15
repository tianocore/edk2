/** @file
  The miscellaneous structure definitions for WiFi connection driver.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_WIFI_MGR_DXE_H__
#define __EFI_WIFI_MGR_DXE_H__

#include <Uefi.h>

//
// Libraries
//
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/FileExplorerLib.h>

//
// UEFI Driver Model Protocols
//
#include <Protocol/DriverBinding.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/ComponentName.h>

//
// Consumed Protocols
//
#include <Protocol/WiFi2.h>
#include <Protocol/AdapterInformation.h>
#include <Protocol/Supplicant.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/EapConfiguration.h>

//
// Produced Protocols
//
#include <Protocol/HiiConfigAccess.h>

//
// Guids
//
#include <Guid/ImageAuthentication.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/WifiConnectionManagerConfigHii.h>

//
// NvData struct definition
//
#include "WifiConnectionMgrConfigNVDataStruct.h"
#include "WifiConnectionMgrConfig.h"
#include "EapContext.h"
#include "WifiConnectionMgrConfigHii.h"

//
// Driver Version
//
#define WIFI_MGR_DXE_VERSION  0xb

#define OUI_IEEE_80211I 0xAC0F00

typedef enum {
  Ieee80211PairwiseCipherSuiteUseGroupCipherSuite = 0,
  Ieee80211PairwiseCipherSuiteWEP40  = 1,
  Ieee80211PairwiseCipherSuiteTKIP   = 2,
  Ieee80211PairwiseCipherSuiteCCMP   = 4,
  Ieee80211PairwiseCipherSuiteWEP104 = 5,
  Ieee80211PairwiseCipherSuiteBIP    = 6,
  //...
} IEEE_80211_PAIRWISE_CIPHER_SUITE;

#define IEEE_80211_PAIRWISE_CIPHER_SUITE_USE_GROUP   (OUI_IEEE_80211I | (Ieee80211PairwiseCipherSuiteUseGroupCipherSuite << 24))
#define IEEE_80211_PAIRWISE_CIPHER_SUITE_WEP40       (OUI_IEEE_80211I | (Ieee80211PairwiseCipherSuiteWEP40 << 24))
#define IEEE_80211_PAIRWISE_CIPHER_SUITE_TKIP        (OUI_IEEE_80211I | (Ieee80211PairwiseCipherSuiteTKIP << 24))
#define IEEE_80211_PAIRWISE_CIPHER_SUITE_CCMP        (OUI_IEEE_80211I | (Ieee80211PairwiseCipherSuiteCCMP << 24))
#define IEEE_80211_PAIRWISE_CIPHER_SUITE_WEP104      (OUI_IEEE_80211I | (Ieee80211PairwiseCipherSuiteWEP104 << 24))
#define IEEE_80211_PAIRWISE_CIPHER_SUITE_BIP         (OUI_IEEE_80211I | (Ieee80211PairwiseCipherSuiteBIP << 24))

typedef enum {
  Ieee80211AkmSuite8021XOrPMKSA       = 1,
  Ieee80211AkmSuitePSK                = 2,
  Ieee80211AkmSuite8021XOrPMKSASHA256 = 5,
  Ieee80211AkmSuitePSKSHA256          = 6
  //...
} IEEE_80211_AKM_SUITE;

#define IEEE_80211_AKM_SUITE_8021X_OR_PMKSA         (OUI_IEEE_80211I | (Ieee80211AkmSuite8021XOrPMKSA << 24))
#define IEEE_80211_AKM_SUITE_PSK                    (OUI_IEEE_80211I | (Ieee80211AkmSuitePSK << 24))
#define IEEE_80211_AKM_SUITE_8021X_OR_PMKSA_SHA256  (OUI_IEEE_80211I | (Ieee80211AkmSuite8021XOrPMKSASHA256 << 24))
#define IEEE_80211_AKM_SUITE_PSK_SHA256             (OUI_IEEE_80211I | (Ieee80211AkmSuitePSKSHA256 << 24))

//
// Protocol instances
//
extern EFI_DRIVER_BINDING_PROTOCOL       gWifiMgrDxeDriverBinding;
extern EFI_COMPONENT_NAME2_PROTOCOL      gWifiMgrDxeComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL       gWifiMgrDxeComponentName;
extern EFI_HII_CONFIG_ACCESS_PROTOCOL    gWifiMgrDxeHiiConfigAccess;

//
// Private Context Data Structure
//
typedef enum {
  WifiMgrDisconnected,
  WifiMgrConnectingToAp,
  WifiMgrConnectedToAp,
  WifiMgrDisconnectingToAp,
  WifiMgrConnectStateMaximum
} WIFI_MGR_CONNECT_STATE;

typedef enum {
  WifiMgrScanFinished,
  WifiMgrScanning,
  WifiMgrScanStateMaximum
} WIFI_MGR_SCAN_STATE;

#define  WIFI_SCAN_FREQUENCY    30

typedef struct _WIFI_MGR_SUPPORTED_SUITES {
  EFI_80211_AKM_SUITE_SELECTOR     *SupportedAKMSuites;
  EFI_80211_CIPHER_SUITE_SELECTOR  *SupportedSwCipherSuites;
  EFI_80211_CIPHER_SUITE_SELECTOR  *SupportedHwCipherSuites;
} WIFI_MGR_SUPPORTED_SUITES;

#define EFI_WIFIMGR_PRIVATE_GUID \
  { \
    0x99b7c019, 0x4789, 0x4829, { 0xa7, 0xbd, 0x0d, 0x4b, 0xaa, 0x62, 0x28, 0x72 } \
  }

typedef struct _WIFI_MGR_PRIVATE_DATA  WIFI_MGR_PRIVATE_DATA;

typedef struct _WIFI_MGR_PRIVATE_PROTOCOL {
  UINT32  Reserved;
} WIFI_MGR_PRIVATE_PROTOCOL;

typedef struct _WIFI_MGR_FILE_CONTEXT {
  EFI_FILE_HANDLE                   FHandle;
  UINT16                            *FileName;
} WIFI_MGR_FILE_CONTEXT;

typedef enum {
  FileTypeCACert,
  FileTypeClientCert,
  FileTypeMax
} WIFI_MGR_FILE_TYPE;

typedef struct {
  UINT32                                     Signature;
  EFI_HANDLE                                 DriverHandle;
  EFI_HANDLE                                 ControllerHandle;
  EFI_EVENT                                  TickTimer;
  WIFI_MGR_PRIVATE_DATA                      *Private;

  //
  // Pointers to consumed protocols
  //
  EFI_WIRELESS_MAC_CONNECTION_II_PROTOCOL    *Wmp;
  EFI_SUPPLICANT_PROTOCOL                    *Supplicant;
  EFI_EAP_CONFIGURATION_PROTOCOL             *EapConfig;

  //
  // Produced protocols
  //
  WIFI_MGR_PRIVATE_PROTOCOL                   WifiMgrIdentifier;

  //
  // Private functions and data fields
  //
  LIST_ENTRY                                  Link;  // Link to the NicList in global private data structure.
  UINT32                                      NicIndex;
  EFI_80211_MAC_ADDRESS                       MacAddress;
  WIFI_MGR_SUPPORTED_SUITES                   SupportedSuites;
  EFI_ADAPTER_INFO_MEDIA_STATE                LastLinkState;

  //
  // The network is currently connected, connecting or disconnecting.
  // Only one network can be operated at one time.
  //
  WIFI_MGR_NETWORK_PROFILE                    *CurrentOperateNetwork;
  WIFI_MGR_NETWORK_PROFILE                    *ConnectPendingNetwork;
  BOOLEAN                                     HasDisconnectPendingNetwork;

  //
  //Profile related data fields
  //
  LIST_ENTRY                                  ProfileList; // List of WIFI_MGR_NETWORK_PROFILE
  UINT32                                      AvailableCount;
  UINT32                                      MaxProfileIndex;
  WIFI_MGR_NETWORK_PROFILE                    *UserSelectedProfile;

  //
  // Data fields for Hii functionlity
  //
  BOOLEAN                                     OneTimeScanRequest;
  BOOLEAN                                     OneTimeConnectRequest;
  BOOLEAN                                     OneTimeDisconnectRequest;
  WIFI_MGR_SCAN_STATE                         ScanState;
  UINTN                                       ScanTickTime;
  WIFI_MGR_CONNECT_STATE                      ConnectState;
  BOOLEAN                                     ConnectStateChanged;
} WIFI_MGR_DEVICE_DATA;

#define WIFI_MGR_DEVICE_DATA_SIGNATURE  SIGNATURE_32 ('W','M','D','D')

#define WIFI_MGR_DEVICE_DATA_FROM_IDENTIFIER(Identifier) \
  CR ( \
    Identifier, \
    WIFI_MGR_DEVICE_DATA, \
    WifiMgrIdentifier, \
    WIFI_MGR_DEVICE_DATA_SIGNATURE \
    )

typedef struct {
  UINT32                                     Signature;
  LIST_ENTRY                                 Link;
  CHAR16                                     SSId[SSID_STORAGE_SIZE];
} WIFI_HIDDEN_NETWORK_DATA;

#define WIFI_MGR_HIDDEN_NETWORK_SIGNATURE  SIGNATURE_32 ('W','M','H','N')

#define WIFI_MGR_HIDDEN_NETWORK_FROM_IDENTIFIER(Identifier) \
  CR ( \
    Identifier, \
    WIFI_HIDDEN_NETWORK_DATA, \
    WifiMgrIdentifier, \
    WIFI_MGR_HIDDEN_NETWORK_SIGNATURE \
    )

//
// Global private data struct
//
struct _WIFI_MGR_PRIVATE_DATA {

  UINT32                            Signature;
  EFI_HANDLE                        DriverHandle;
  EFI_HII_HANDLE                    RegisteredHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;

  UINT32                            NicCount;
  LIST_ENTRY                        NicList;
  WIFI_MGR_DEVICE_DATA              *CurrentNic;

  //
  // Data fields for Hii functionlity
  //
  EFI_EVENT                         NetworkListRefreshEvent;        // Event to refresh the network list form
  EFI_EVENT                         ConnectFormRefreshEvent;        // Event to refresh the connect form
  EFI_EVENT                         MainPageRefreshEvent;           // Event to refresh the main page

  //
  //User Input Record
  //
  UINT8                             SecurityType;
  UINT8                             EapAuthMethod;
  UINT8                             EapSecondAuthMethod;
  CHAR16                            EapIdentity[EAP_IDENTITY_SIZE];

  WIFI_MGR_FILE_CONTEXT             *FileContext;
  WIFI_MGR_FILE_TYPE                FileType;

  UINT32                            HiddenNetworkCount;
  LIST_ENTRY                        HiddenNetworkList;
};

#define WIFI_MGR_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('W','M','P','D')

#define WIFI_MGR_PRIVATE_DATA_FROM_CONFIG_ACCESS(This) \
  CR ( \
    This, \
    WIFI_MGR_PRIVATE_DATA, \
    ConfigAccess, \
    WIFI_MGR_PRIVATE_DATA_SIGNATURE \
    )
extern    WIFI_MGR_PRIVATE_DATA    *mPrivate;

typedef enum {
  TokenTypeGetNetworksToken,
  TokenTypeConnectNetworkToken,
  TokenTypeDisconnectNetworkToken,
  TokenTypeMax,
} WIFI_MGR_MAC_CONFIG_TOKEN_TYPE;

typedef union {
  EFI_80211_GET_NETWORKS_TOKEN         *GetNetworksToken;
  EFI_80211_CONNECT_NETWORK_TOKEN      *ConnectNetworkToken;
  EFI_80211_DISCONNECT_NETWORK_TOKEN   *DisconnectNetworkToken;
} MAC_CONNECTION2_ADAPTER_TOKEN;

typedef struct {
  WIFI_MGR_DEVICE_DATA            *Nic;
  WIFI_MGR_MAC_CONFIG_TOKEN_TYPE  Type;
  MAC_CONNECTION2_ADAPTER_TOKEN   Token;
} WIFI_MGR_MAC_CONFIG_TOKEN;

//
// Include files with function prototypes
//
#include "WifiConnectionMgrDriverBinding.h"
#include "WifiConnectionMgrImpl.h"
#include "WifiConnectionMgrComponentName.h"
#include "WifiConnectionMgrHiiConfigAccess.h"
#include "WifiConnectionMgrMisc.h"
#include "WifiConnectionMgrFileUtil.h"

#endif
