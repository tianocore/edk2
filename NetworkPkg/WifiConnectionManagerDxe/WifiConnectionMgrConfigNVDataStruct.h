/** @file
  Define IFR NVData structures used by the WiFi Connection Manager.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _WIFI_NVDATASTRUC_H_
#define _WIFI_NVDATASTRUC_H_

#include <Guid/WifiConnectionManagerConfigHii.h>
#include "WifiConnectionMgrConfigHii.h"

#define MANAGER_VARSTORE_ID  0x0802

#define WIFI_STR_MAX_SIZE            224
#define WIFI_FILENAME_STR_MAX_SIZE   224
#define WIFI_MGR_MAX_MAC_STRING_LEN  96

#define SSID_MIN_LEN       1
#define SSID_MAX_LEN       32
#define SSID_STORAGE_SIZE  33

#define PASSWORD_MIN_LEN       8
#define PASSWORD_MAX_LEN       63
#define PASSWORD_STORAGE_SIZE  64

#define EAP_IDENTITY_LEN   63
#define EAP_IDENTITY_SIZE  64

#define FORMID_NONE_FORM            0
#define FORMID_MAC_SELECTION        1
#define FORMID_WIFI_MAINPAGE        2
#define FORMID_NETWORK_LIST         3
#define FORMID_CONNECT_NETWORK      4
#define FORMID_ENROLL_CERT          5
#define FORMID_CA_LIST              6
#define FORMID_ENROLL_PRIVATE_KEY   7
#define FORMID_PRIVATE_KEY_LIST     8
#define FORMID_WIFI_SETTINGS        9
#define FORMID_HIDDEN_NETWORK_LIST  10

//
// Mac List Form Key
//
#define KEY_MAC_LIST  0x100

//
// Main Form Key
//
#define KEY_REFRESH_TITLE_CONNECTION_STATUS  0x101

//
// Network List Form Key
//
#define KEY_NETWORK_LIST          0x102
#define KEY_REFRESH_NETWORK_LIST  0x103
#define KEY_WIFI_SETTINGS         0x104

//
// Connect Network Form Key
//
#define KEY_PASSWORD_CONNECT_NETWORK            0x201
#define KEY_CONNECT_ACTION                      0x202
#define KEY_REFRESH_CONNECT_CONFIGURATION       0x203
#define KEY_EAP_AUTH_METHOD_CONNECT_NETWORK     0x204
#define KEY_EAP_SEAUTH_METHOD_CONNECT_NETWORK   0x205
#define KEY_ENROLL_CA_CERT_CONNECT_NETWORK      0x206
#define KEY_ENROLL_CLIENT_CERT_CONNECT_NETWORK  0x207
#define KEY_ENROLL_PRIVATE_KEY_CONNECT_NETWORK  0x208
#define KEY_EAP_IDENTITY_CONNECT_NETWORK        0x209
#define KEY_EAP_PASSWORD_CONNECT_NETWORK        0x210

//
// Cert Form And Private Key Form
//
#define KEY_EAP_ENROLL_CERT_FROM_FILE         0x301
#define KEY_EAP_ENROLL_PRIVATE_KEY_FROM_FILE  0x302
#define KEY_SAVE_CERT_TO_MEM                  0x303
#define KEY_NO_SAVE_CERT_TO_MEM               0x304
#define KEY_SAVE_PRIVATE_KEY_TO_MEM           0x305
#define KEY_NO_SAVE_PRIVATE_KEY_TO_MEM        0x306
#define KEY_PRIVATE_KEY_PASSWORD              0x307
#define KEY_ENROLLED_CERT_NAME                0x308
#define KEY_ENROLLED_PRIVATE_KEY_NAME         0x309

//
// Hidden Network Configuration Form
//
#define KEY_HIDDEN_NETWORK         0x401
#define KEY_ADD_HIDDEN_NETWORK     0x402
#define KEY_REMOVE_HIDDEN_NETWORK  0x403

//
// Dynamic Lists
//
#define MAC_LIST_COUNT_MAX  255
#define LABEL_MAC_ENTRY     0x1000
#define KEY_MAC_ENTRY_BASE  0x1100

#define NETWORK_LIST_COUNT_MAX            4095
#define LABEL_NETWORK_LIST_ENTRY          0x2000
#define KEY_AVAILABLE_NETWORK_ENTRY_BASE  0x3000

#define HIDDEN_NETWORK_LIST_COUNT_MAX  255
#define LABEL_HIDDEN_NETWORK_ENTRY     0x4000
#define KEY_HIDDEN_NETWORK_ENTRY_BASE  0x4100

#define LABEL_END  0xffff

//
// Network Security Type
//
#define SECURITY_TYPE_NONE             0
#define SECURITY_TYPE_WPA_ENTERPRISE   1
#define SECURITY_TYPE_WPA2_ENTERPRISE  2
#define SECURITY_TYPE_WPA_PERSONAL     3
#define SECURITY_TYPE_WPA2_PERSONAL    4
#define SECURITY_TYPE_WEP              5
#define SECURITY_TYPE_UNKNOWN          6
#define SECURITY_TYPE_MAX              7

#define EAP_AUTH_METHOD_TTLS  0
#define EAP_AUTH_METHOD_PEAP  1
#define EAP_AUTH_METHOD_TLS   2
#define EAP_AUTH_METHOD_MAX   3

#define EAP_SEAUTH_METHOD_MSCHAPV2  0
#define EAP_SEAUTH_METHOD_MAX       1

#define HIDDEN_NETWORK_LIST_VAR_OFFSET  ((UINT16) OFFSET_OF (WIFI_MANAGER_IFR_NVDATA, HiddenNetworkList))

#pragma pack(1)
typedef struct _WIFI_MANAGER_IFR_NVDATA {
  UINT32    ProfileCount;
  CHAR16    SSId[SSID_STORAGE_SIZE];
  CHAR16    Password[PASSWORD_STORAGE_SIZE];
  CHAR16    PrivateKeyPassword[PASSWORD_STORAGE_SIZE];
  CHAR16    EapIdentity[EAP_IDENTITY_SIZE];
  CHAR16    EapPassword[PASSWORD_STORAGE_SIZE];
  UINT8     SecurityType;
  UINT8     EapAuthMethod;
  UINT8     EapSecondAuthMethod;
  UINT8     HiddenNetworkList[HIDDEN_NETWORK_LIST_COUNT_MAX];
} WIFI_MANAGER_IFR_NVDATA;
#pragma pack()

#endif
