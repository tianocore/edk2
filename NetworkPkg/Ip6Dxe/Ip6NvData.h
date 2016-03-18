/** @file
  NVData structure used by the IP6 configuration component.

  Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IP6_NV_DATA_H_
#define _IP6_NV_DATA_H_

#include <Guid/Ip6ConfigHii.h>

#define FORMID_MAIN_FORM          1
#define FORMID_MANUAL_CONFIG_FORM 2
#define FORMID_HEAD_FORM          3

#define IP6_POLICY_AUTO           0
#define IP6_POLICY_MANUAL         1
#define DAD_MAX_TRANSMIT_COUNT    10

#define KEY_INTERFACE_ID          0x101
#define KEY_MANUAL_ADDRESS        0x102
#define KEY_GATEWAY_ADDRESS       0x103
#define KEY_DNS_ADDRESS           0x104
#define KEY_SAVE_CHANGES          0x105
#define KEY_SAVE_CONFIG_CHANGES   0x106
#define KEY_IGNORE_CONFIG_CHANGES 0x107
#define KEY_GET_CURRENT_SETTING   0x108

#define HOST_ADDRESS_LABEL        0x9000
#define ROUTE_TABLE_LABEL         0xa000
#define GATEWAY_ADDRESS_LABEL     0xb000
#define DNS_ADDRESS_LABEL         0xc000
#define LABEL_END                 0xffff

#define INTERFACE_ID_STR_MIN_SIZE 1
#define INTERFACE_ID_STR_MAX_SIZE 23
#define INTERFACE_ID_STR_STORAGE  25
#define IP6_STR_MAX_SIZE          40
#define ADDRESS_STR_MIN_SIZE      2
#define ADDRESS_STR_MAX_SIZE      255

///
/// IP6_CONFIG_IFR_NVDATA contains the IP6 configure
/// parameters for that NIC.
///
#pragma pack(1)
typedef struct {
  UINT8           IfType;                                 ///< interface type
  UINT8           Padding[3];
  UINT32          Policy;                                 ///< manual or automatic
  UINT32          DadTransmitCount;                       ///< dad transmits count
  CHAR16          InterfaceId[INTERFACE_ID_STR_STORAGE];  ///< alternative interface id
  CHAR16          ManualAddress[ADDRESS_STR_MAX_SIZE];    ///< IP addresses
  CHAR16          GatewayAddress[ADDRESS_STR_MAX_SIZE];   ///< Gateway address
  CHAR16          DnsAddress[ADDRESS_STR_MAX_SIZE];       ///< DNS server address
} IP6_CONFIG_IFR_NVDATA;
#pragma pack()

#endif

