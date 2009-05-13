/** @file
  This file defines NIC_IP4_CONFIG_INFO structure.
  
Copyright (c) 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __NIC_IP4_CONFIG_NVDATA_H__
#define __NIC_IP4_CONFIG_NVDATA_H__

#include <Protocol/Ip4Config.h>

#define EFI_NIC_IP4_CONFIG_VARIABLE_GUID \
  { \
    0xd8944553, 0xc4dd, 0x41f4, { 0x9b, 0x30, 0xe1, 0x39, 0x7c, 0xfb, 0x26, 0x7b } \
  }

#define EFI_NIC_IP4_CONFIG_VARIABLE          L"EfiNicIp4ConfigVariable"

typedef enum {
  //
  // Config source: dhcp or static
  //
  IP4_CONFIG_SOURCE_DHCP     = 0,
  IP4_CONFIG_SOURCE_STATIC,
  IP4_CONFIG_SOURCE_MAX,

  IP4_NIC_NAME_LENGTH        = 64,
  MAX_IP4_CONFIG_IN_VARIABLE = 16
} IP4_CONFIG_TYPE;

//
// The following structures are used by drivers/applications other
// than EFI_IP4_PROTOCOL, such as ifconfig shell application, to
// communicate the IP configuration information to EFI_IP4_CONFIG_PROTOCOL.
// EFI_IP4_CONFIG_PROTOCOL in turn is used by EFI_IP4_PROTOCOL to get
// the default IP4 configuration. ifconfig can't use the EFI_IP4_PROTOCOL
// because it don't know how to configure the default IP address even
// it has got the address.
//

///
/// NIC_ADDR contains the interface's type and MAC address to identify
/// a specific NIC.
///
typedef struct {
  UINT16                    Type;       ///< Interface type
  UINT8                     Len;        ///< Length of MAC address
  EFI_MAC_ADDRESS           MacAddr;    ///< MAC address of interface
} NIC_ADDR;

///
/// NIC_IP4_CONFIG_INFO contains the IP4 configure
/// parameters for that NIC. NIC_IP4_CONFIG_INFO is
/// of variable length.
///
typedef struct {
  NIC_ADDR                  NicAddr;      ///< Link layer address to identify the NIC
  UINT32                    Source;       ///< Static or DHCP
  BOOLEAN                   Perment;      ///< Survive the reboot or not
  EFI_IP4_IPCONFIG_DATA     Ip4Info;      ///< IP addresses
} NIC_IP4_CONFIG_INFO;

extern EFI_GUID gEfiNicIp4ConfigVariableGuid;

#endif
