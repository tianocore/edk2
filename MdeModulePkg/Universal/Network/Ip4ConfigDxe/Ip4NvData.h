/** @file
  Routines used to operate the Ip4 configure variable.

Copyright (c) 2009, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _NIC_IP4_NV_DATA_H_
#define _NIC_IP4_NV_DATA_H_


//
// one copy from NicIp4ConfigNvData.h
//
#define EFI_NIC_IP4_CONFIG_VARIABLE_GUID \
  { \
    0xd8944553, 0xc4dd, 0x41f4, { 0x9b, 0x30, 0xe1, 0x39, 0x7c, 0xfb, 0x26, 0x7b } \
  }

#define FORMID_MAIN_FORM    1
#define FORMID_DEVICE_FORM  2

#define KEY_DHCP_ENABLE           0x101
#define KEY_LOCAL_IP              0x102
#define KEY_SUBNET_MASK           0x103
#define KEY_GATE_WAY              0x104
#define KEY_SAVE_CHANGES          0x105

#define DEVICE_ENTRY_LABEL        0x1234
#define LABEL_END                 0xffff

#define KEY_DEVICE_ENTRY_BASE     0x1000

#define IP_MIN_SIZE               7
#define IP_MAX_SIZE               15
#define IP4_STR_MAX_SIZE          16

///
/// NIC_IP4_CONFIG_INFO contains the IP4 configure
/// parameters for that NIC. NIC_IP4_CONFIG_INFO is
/// of variable length.
///
typedef struct {
  UINT16          NicAddr[3];         ///< NIC MAC address
  UINT8           Reserved;           ///< Reserved bits
  UINT8           DhcpEnable;         ///< Static or DHCP
  CHAR16          StationAddress[IP4_STR_MAX_SIZE];  ///< IP addresses
  CHAR16          SubnetMask[IP4_STR_MAX_SIZE];      ///< Subnet address
  CHAR16          GatewayAddress[IP4_STR_MAX_SIZE];  ///< Gateway address
} IP4_CONFIG_IFR_NVDATA;

#endif
