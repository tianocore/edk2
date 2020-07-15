/** @file
  Define NVData structures used by the iSCSI configuration component.

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ISCSI_NVDATASTRUC_H_
#define _ISCSI_NVDATASTRUC_H_

#include <Guid/IScsiConfigHii.h>

#define VAR_EQ_TEST_NAME          0x100
#define CONFIGURATION_VARSTORE_ID 0x6666

#define FORMID_MAIN_FORM          1
#define FORMID_MAC_FORM           2
#define FORMID_ATTEMPT_FORM       3
#define FORMID_ORDER_FORM         4
#define FORMID_DELETE_FORM        5

#define ISCSI_MAX_ATTEMPTS_NUM    FixedPcdGet8 (PcdMaxIScsiAttemptNumber)

#define ISCSI_NAME_IFR_MIN_SIZE   4
#define ISCSI_NAME_IFR_MAX_SIZE   223
#define ISCSI_NAME_MAX_SIZE       224

#define ATTEMPT_NAME_LIST_SIZE    96
#define ATTEMPT_NAME_SIZE         12

#define CONNECT_MIN_RETRY         0
#define CONNECT_MAX_RETRY         16

#define CONNECT_MIN_TIMEOUT       100
#define CONNECT_MAX_TIMEOUT       20000
#define CONNECT_DEFAULT_TIMEOUT   1000

#define ISCSI_ACTIVE_DISABLED     0
#define ISCSI_ACTIVE_ENABLED      1

#define ISCSI_DISABLED            0
#define ISCSI_ENABLED             1
#define ISCSI_ENABLED_FOR_MPIO    2

#define IP_MODE_IP4               0
#define IP_MODE_IP6               1
#define IP_MODE_AUTOCONFIG        2

#define ISCSI_AUTH_TYPE_NONE      0
#define ISCSI_AUTH_TYPE_CHAP      1
#define ISCSI_AUTH_TYPE_KRB       2

#define IP4_MIN_SIZE              7
#define IP4_MAX_SIZE              15
#define IP4_STR_MAX_SIZE          16

//
// Macros used for an IPv4 or an IPv6 address.
//
#define IP_MIN_SIZE               2
#define IP_MAX_SIZE               39
#define IP_STR_MAX_SIZE           40

#define LUN_MIN_SIZE              1
#define LUN_MAX_SIZE              20
#define ISCSI_LUN_STR_MAX_LEN     21

#define ISCSI_CHAP_UNI            0
#define ISCSI_CHAP_MUTUAL         1

#define TARGET_PORT_MIN_NUM       0
#define TARGET_PORT_MAX_NUM       65535
#define LABEL_END                 0xffff

#define KEY_INITIATOR_NAME        0x101
#define KEY_DHCP_ENABLE           0x102
#define KEY_LOCAL_IP              0x103
#define KEY_SUBNET_MASK           0x104
#define KEY_GATE_WAY              0x105
#define KEY_TARGET_IP             0x106
#define KEY_CHAP_NAME             0x107
#define KEY_CHAP_SECRET           0x108
#define KEY_REVERSE_CHAP_NAME     0x109
#define KEY_REVERSE_CHAP_SECRET   0x10a
#define KEY_SAVE_CHANGES          0x10b
#define KEY_TARGET_NAME           0x10c
#define KEY_BOOT_LUN              0x10d

#define KEY_ADD_ATTEMPT           0x10e
#define KEY_SAVE_ATTEMPT_CONFIG   0x10f
#define KEY_ORDER_ATTEMPT_CONFIG  0x110
#define KEY_SAVE_ORDER_CHANGES    0x111
#define KEY_IGNORE_ORDER_CHANGES  0x112
#define KEY_ATTEMPT_NAME          0x113
#define KEY_SAVE_DELETE_ATTEMPT   0x114
#define KEY_IGNORE_DELETE_ATTEMPT 0x115
#define KEY_DELETE_ATTEMPT        0x116

#define KEY_IP_MODE               0x11c
#define KEY_AUTH_TYPE             0x11d
#define KEY_CONFIG_ISID           0x11e

#define ATTEMPT_ENTRY_LABEL       0x9000
#define KEY_ATTEMPT_ENTRY_BASE    0xa000
#define KEY_DE_ATTEMPT_ENTRY_BASE 0xb000

#define KEY_DEVICE_ENTRY_BASE     0x1000
#define KEY_MAC_ENTRY_BASE        0x2000
#define MAC_ENTRY_LABEL           0x3000
#define ORDER_ENTRY_LABEL         0x4000
#define DELETE_ENTRY_LABEL        0x5000
#define KEYWORD_ENTRY_LABEL       0x6000
#define CONFIG_OPTION_OFFSET      0x9000

#define ISCSI_CHAP_SECRET_MIN_LEN 12
#define ISCSI_CHAP_SECRET_MAX_LEN 16
//
// ISCSI_CHAP_SECRET_STORAGE = ISCSI_CHAP_SECRET_MAX_LEN + sizeof (NULL-Terminator)
//
#define ISCSI_CHAP_SECRET_STORAGE  17

#define ISCSI_CHAP_NAME_MAX_LEN    126
#define ISCSI_CHAP_NAME_STORAGE    127

#define KERBEROS_SECRET_MIN_LEN   12
#define KERBEROS_SECRET_MAX_LEN   16
#define KERBEROS_SECRET_STORAGE   17
#define KERBEROS_NAME_MAX_LEN     96
#define KERBEROS_KDC_PORT_MIN_NUM 0
#define KERBEROS_KDC_PORT_MAX_NUM 65535

#define ISID_CONFIGURABLE_MIN_LEN 6
#define ISID_CONFIGURABLE_MAX_LEN 12
#define ISID_CONFIGURABLE_STORAGE 13

//
// sizeof (EFI_MAC_ADDRESS) * 3
//
#define ISCSI_MAX_MAC_STRING_LEN            96

///
/// Macro used for target Url.
///
#define ISCSI_TARGET_URI_MIN_SIZE     0
#define ISCSI_TARGET_URI_MAX_SIZE     255

#pragma pack(1)

//
// Used by keyword.
//
typedef struct {
  CHAR16  ISCSIIsId[ISID_CONFIGURABLE_STORAGE];
  CHAR16  ISCSIInitiatorIpAddress[IP4_STR_MAX_SIZE];
  CHAR16  ISCSIInitiatorNetmask[IP4_STR_MAX_SIZE];
  CHAR16  ISCSIInitiatorGateway[IP4_STR_MAX_SIZE];
  CHAR16  ISCSITargetName[ISCSI_NAME_MAX_SIZE];
  CHAR16  ISCSITargetIpAddress[ISCSI_TARGET_URI_MAX_SIZE];
  CHAR16  ISCSILun[ISCSI_LUN_STR_MAX_LEN];
  CHAR16  ISCSIChapUsername[ISCSI_CHAP_NAME_STORAGE];
  CHAR16  ISCSIChapSecret[ISCSI_CHAP_SECRET_STORAGE];
  CHAR16  ISCSIReverseChapUsername[ISCSI_CHAP_NAME_STORAGE];
  CHAR16  ISCSIReverseChapSecret[ISCSI_CHAP_SECRET_STORAGE];
} KEYWORD_STR;

typedef struct _ISCSI_CONFIG_IFR_NVDATA {
  CHAR16  InitiatorName[ISCSI_NAME_MAX_SIZE];
  CHAR16  AttemptName[ATTEMPT_NAME_SIZE];
  UINT8   Enabled;
  UINT8   IpMode;

  UINT8   ConnectRetryCount;
  UINT8   Padding1;
  UINT16  ConnectTimeout; // Timeout value in milliseconds.

  UINT8   InitiatorInfoFromDhcp;
  UINT8   TargetInfoFromDhcp;
  CHAR16  LocalIp[IP4_STR_MAX_SIZE];
  CHAR16  SubnetMask[IP4_STR_MAX_SIZE];
  CHAR16  Gateway[IP4_STR_MAX_SIZE];

  CHAR16  TargetName[ISCSI_NAME_MAX_SIZE];
  CHAR16  TargetIp[ISCSI_TARGET_URI_MAX_SIZE];
  UINT16  TargetPort;
  CHAR16  BootLun[ISCSI_LUN_STR_MAX_LEN];

  UINT8   AuthenticationType;

  UINT8   CHAPType;
  CHAR16  CHAPName[ISCSI_CHAP_NAME_STORAGE];
  CHAR16  CHAPSecret[ISCSI_CHAP_SECRET_STORAGE];
  CHAR16  ReverseCHAPName[ISCSI_CHAP_NAME_STORAGE];
  CHAR16  ReverseCHAPSecret[ISCSI_CHAP_SECRET_STORAGE];

  BOOLEAN MutualRequired;
  UINT8   Padding2;
  CHAR16  KerberosUserName[KERBEROS_NAME_MAX_LEN];
  CHAR16  KerberosUserSecret[KERBEROS_SECRET_STORAGE];
  CHAR16  KerberosKDCName[KERBEROS_NAME_MAX_LEN];
  CHAR16  KerberosKDCRealm[KERBEROS_NAME_MAX_LEN];
  CHAR16  KerberosKDCIp[IP_STR_MAX_SIZE];
  UINT16  KerberosKDCPort;

  UINT8   DynamicOrderedList[ISCSI_MAX_ATTEMPTS_NUM];
  UINT8   DeleteAttemptList[ISCSI_MAX_ATTEMPTS_NUM];
  UINT8   AddAttemptList[ISCSI_MAX_ATTEMPTS_NUM];
  CHAR16  IsId[ISID_CONFIGURABLE_STORAGE];

  //
  // This will be used by keywords.
  //
  CHAR16  ISCSIMacAddr[ISCSI_MAX_MAC_STRING_LEN];
  CHAR16  ISCSIAttemptOrder[ATTEMPT_NAME_LIST_SIZE];
  CHAR16  ISCSIAddAttemptList[ATTEMPT_NAME_LIST_SIZE];
  CHAR16  ISCSIDeleteAttemptList[ATTEMPT_NAME_LIST_SIZE];
  CHAR16  ISCSIDisplayAttemptList[ATTEMPT_NAME_LIST_SIZE];
  CHAR16  ISCSIAttemptName[ATTEMPT_NAME_LIST_SIZE];
  UINT8   ISCSIBootEnableList[ISCSI_MAX_ATTEMPTS_NUM];
  UINT8   ISCSIIpAddressTypeList[ISCSI_MAX_ATTEMPTS_NUM];
  UINT8   ISCSIConnectRetry[ISCSI_MAX_ATTEMPTS_NUM];
  UINT16  ISCSIConnectTimeout[ISCSI_MAX_ATTEMPTS_NUM];
  UINT8   ISCSIInitiatorInfoViaDHCP[ISCSI_MAX_ATTEMPTS_NUM];
  UINT8   ISCSITargetInfoViaDHCP[ISCSI_MAX_ATTEMPTS_NUM];
  UINT16  ISCSITargetTcpPort[ISCSI_MAX_ATTEMPTS_NUM];
  UINT8   ISCSIAuthenticationMethod[ISCSI_MAX_ATTEMPTS_NUM];
  UINT8   ISCSIChapType[ISCSI_MAX_ATTEMPTS_NUM];
  KEYWORD_STR Keyword[ISCSI_MAX_ATTEMPTS_NUM];
} ISCSI_CONFIG_IFR_NVDATA;
#pragma pack()

#endif
