/** @file
  This file defines NIC_IP4_CONFIG_INFO structure.
  
Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

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


//
// Config source: dhcp or static
//
#define IP4_CONFIG_SOURCE_DHCP     0
#define IP4_CONFIG_SOURCE_STATIC   1
#define IP4_CONFIG_SOURCE_MAX      2

#define IP4_NIC_NAME_LENGTH        64
#define MAX_IP4_CONFIG_IN_VARIABLE 16

//
// The following structures are used by drivers/applications other
// than EFI_IP4_PROTOCOL, such as the ifconfig shell application, to
// communicate the IP configuration information to the EFI_IP4_CONFIG_PROTOCOL.
// The EFI_IP4_PROTOCOL uses the EFI_IP4_CONFIG_PROTOCOL to get
// the default IP4 configuration.
//

///
/// NIC_ADDR contains the interface's type and MAC address to identify
/// a specific NIC.
///
typedef struct {
  UINT16                    Type;       ///< Interface type.
  UINT8                     Len;        ///< Length of MAC address.
  EFI_MAC_ADDRESS           MacAddr;    ///< MAC address of interface.
} NIC_ADDR;

///
/// NIC_IP4_CONFIG_INFO contains the IP4 configure
/// parameters for that NIC. NIC_IP4_CONFIG_INFO is
/// of variable length.
///
typedef struct {
  NIC_ADDR                  NicAddr;      ///< Link layer address to identify the NIC.
  UINT32                    Source;       ///< Static or DHCP.
  BOOLEAN                   Perment;      ///< Survive the reboot or not.
  EFI_IP4_IPCONFIG_DATA     Ip4Info;      ///< IP addresses.
} NIC_IP4_CONFIG_INFO;

extern EFI_GUID gEfiNicIp4ConfigVariableGuid;

#endif
