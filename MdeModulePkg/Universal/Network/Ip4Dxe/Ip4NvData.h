/** @file
  Routines used to operate the Ip4Dxe.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IP4_NV_DATA_H_
#define _IP4_NV_DATA_H_

#include <Guid/Ip4Config2Hii.h>

#define FORMID_MAIN_FORM    1
#define FORMID_DEVICE_FORM  2

#define KEY_ENABLE                0x100
#define KEY_DHCP_ENABLE           0x101
#define KEY_LOCAL_IP              0x102
#define KEY_SUBNET_MASK           0x103
#define KEY_GATE_WAY              0x104
#define KEY_DNS                   0x105
#define KEY_SAVE_CHANGES          0x106

#define IP_MIN_SIZE               7
#define IP_MAX_SIZE               15
#define IP4_STR_MAX_SIZE          16
#define ADDRESS_STR_MAX_SIZE      255
#define MAX_IP4_CONFIG_DNS        16

///
/// IP4_CONFIG2_IFR_NVDATA contains the IP4 configure
/// parameters for that NIC.
///
typedef struct {
  UINT8           Configure;                         ///< NIC configure status
  UINT8           DhcpEnable;                        ///< Static or DHCP
  CHAR16          StationAddress[IP4_STR_MAX_SIZE];  ///< IP addresses
  CHAR16          SubnetMask[IP4_STR_MAX_SIZE];      ///< Subnet address
  CHAR16          GatewayAddress[IP4_STR_MAX_SIZE];  ///< Gateway address
  CHAR16          DnsAddress[ADDRESS_STR_MAX_SIZE];  ///< DNS server address
} IP4_CONFIG2_IFR_NVDATA;

#endif

