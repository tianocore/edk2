/** @file
  Define NVData structures used by the iSCSI configuration component

Copyright (c) 2004 - 2007, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_NVDATASTRUC_H_
#define _ISCSI_NVDATASTRUC_H_

#define ISCSI_CONFIG_GUID \
  { \
    0x6456ed61, 0x3579, 0x41c9, { 0x8a, 0x26, 0x0a, 0x0b, 0xd6, 0x2b, 0x78, 0xfc } \
  }

#define VAR_EQ_TEST_NAME    0x100

#define FORMID_MAIN_FORM    1
#define FORMID_DEVICE_FORM  2

#define ISCSI_NAME_MAX_SIZE 224

//
// Vfr has a limit on the size, it's 255 bytes.
//
#define ISCSI_NAME_IFR_MAX_SIZE   126

#define IP_MIN_SIZE               7
#define IP_MAX_SIZE               15
#define IP4_STR_MAX_SIZE          16

#define LUN_MIN_SIZE              1
#define LUN_MAX_SIZE              20

#define ISCSI_CHAP_NONE           0
#define ISCSI_CHAP_UNI            1
#define ISCSI_CHAP_MUTUAL         2

#define TARGET_PORT_MIN_NUM       0
#define TARGET_PORT_MAX_NUM       65535

#define DEVICE_ENTRY_LABEL        0x1234

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

#define KEY_DEVICE_ENTRY_BASE     0x1000

#define ISCSI_LUN_STR_MAX_LEN     21
#define ISCSI_CHAP_SECRET_MIN_LEN 13
#define ISCSI_CHAP_SECRET_MAX_LEN 17
#define ISCSI_CHAP_NAME_MAX_LEN   126

#pragma pack(1)
typedef struct {
  CHAR16  InitiatorName[ISCSI_NAME_IFR_MAX_SIZE];

  UINT8   Enabled;

  UINT8   InitiatorInfoFromDhcp;
  CHAR16  LocalIp[IP4_STR_MAX_SIZE];
  CHAR16  SubnetMask[IP4_STR_MAX_SIZE];
  CHAR16  Gateway[IP4_STR_MAX_SIZE];

  CHAR16  TargetName[ISCSI_NAME_IFR_MAX_SIZE];
  CHAR16  TargetIp[IP4_STR_MAX_SIZE];
  UINT16  TargetPort;
  CHAR16  BootLun[ISCSI_LUN_STR_MAX_LEN];
  UINT8   TargetInfoFromDhcp;

  UINT8   CHAPType;
  CHAR16  CHAPName[ISCSI_CHAP_NAME_MAX_LEN];
  CHAR16  CHAPSecret[ISCSI_CHAP_SECRET_MAX_LEN];
  CHAR16  ReverseCHAPName[ISCSI_CHAP_NAME_MAX_LEN];
  CHAR16  ReverseCHAPSecret[ISCSI_CHAP_SECRET_MAX_LEN];
} ISCSI_CONFIG_IFR_NVDATA;
#pragma pack()

#endif
