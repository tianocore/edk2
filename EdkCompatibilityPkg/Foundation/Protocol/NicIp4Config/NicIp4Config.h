/*++

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  NicIp4Config.h

Abstract:

--*/

#ifndef _NIC_IP4_CONFIG_H_
#define _NIC_IP4_CONFIG_H_

#include EFI_PROTOCOL_DEFINITION (Ip4Config)

#define EFI_NIC_IP4_CONFIG_PROTOCOL_GUID \
  {0xdca3d4d, 0x12da, 0x4728, {0xbf, 0x7e, 0x86, 0xce, 0xb9, 0x28, 0xd0, 0x67}}

#define EFI_NIC_IP4_CONFIG_VARIABLE_GUID  \
  {0xd8944553, 0xc4dd, 0x41f4, {0x9b, 0x30, 0xe1, 0x39, 0x7c, 0xfb, 0x26, 0x7b}}

#define EFI_NIC_IP4_CONFIG_VARIABLE          L"EfiNicIp4ConfigVariable"


typedef struct _EFI_NIC_IP4_CONFIG_PROTOCOL EFI_NIC_IP4_CONFIG_PROTOCOL;

enum {
  //
  // Config source: dhcp or static
  //
  IP4_CONFIG_SOURCE_DHCP     = 0,
  IP4_CONFIG_SOURCE_STATIC,
  IP4_CONFIG_SOURCE_MAX,
  
  IP4_NIC_NAME_LENGTH        = 64,
  MAX_IP4_CONFIG_IN_VARIABLE = 128
};

//
// The following structures are used by drivers/applications other
// than EFI_IP4_PROTOCOL, such as ifconfig shell application, to 
// communicate the IP configuration information to EFI_IP4_CONFIG_PROTOCOL. 
// EFI_IP4_CONFIG_PROTOCOL in turn is used by EFI_IP4_PROTOCOL to get 
// the default IP4 configuration. ifconfig can't use the EFI_IP4_PROTOCOL
// because it don't know how to configure the default IP address even
// it has got the address. 
//
// NIC_ADDR contains the interface's type and MAC address to identify
// a specific NIC. NIC_IP4_CONFIG_INFO contains the IP4 configure 
// parameters for that NIC. IP4_CONFIG_VARIABLE is the EFI variable to 
// save the configuration. NIC_IP4_CONFIG_INFO and IP4_CONFIG_VARIABLE 
// is of variable length.
//
// EFI_NIC_IP4_CONFIG_PROTOCOL is a priority protocol, not defined by UEFI2.0
//
typedef struct {
  UINT16                    Type;
  UINT8                     Len;
  EFI_MAC_ADDRESS           MacAddr;
} NIC_ADDR;

typedef struct {
  NIC_ADDR                  NicAddr;    // Link layer address to identify the NIC
  UINT32                    Source;     // Static or DHCP
  BOOLEAN                   Perment;    // Survive the reboot or not
  EFI_IP4_IPCONFIG_DATA     Ip4Info;    // IP addresses
} NIC_IP4_CONFIG_INFO;

typedef struct {
  UINT32                    Len;        // Total length of the variable
  UINT16                    CheckSum;   // CheckSum, the same as IP4 head checksum
  UINT32                    Count;      // Number of NIC_IP4_CONFIG_INFO follows
  NIC_IP4_CONFIG_INFO       ConfigInfo;
} IP4_CONFIG_VARIABLE;

typedef
EFI_STATUS
(EFIAPI *EFI_NIC_IP4_CONFIG_GET_INFO) (
  IN EFI_NIC_IP4_CONFIG_PROTOCOL  *This,
  IN OUT UINTN                    *Len,
  OUT NIC_IP4_CONFIG_INFO         *NicConfig     OPTIONAL 
  );

typedef
EFI_STATUS
(EFIAPI *EFI_NIC_IP4_CONFIG_SET_INFO) (
  IN EFI_NIC_IP4_CONFIG_PROTOCOL  *This,
  IN NIC_IP4_CONFIG_INFO          *NicConfig,    OPTIONAL
  IN BOOLEAN                      ReConfig 
  );

typedef
EFI_STATUS
(EFIAPI *EFI_NIC_IP4_CONFIG_GET_NAME) (
  IN  EFI_NIC_IP4_CONFIG_PROTOCOL *This,
  IN  UINT16                      *Name,         OPTIONAL
  IN  NIC_ADDR                    *NicAddr       OPTIONAL 
  );

struct _EFI_NIC_IP4_CONFIG_PROTOCOL {
  EFI_NIC_IP4_CONFIG_GET_NAME     GetName;
  EFI_NIC_IP4_CONFIG_GET_INFO     GetInfo;
  EFI_NIC_IP4_CONFIG_SET_INFO     SetInfo;
};

extern EFI_GUID gEfiNicIp4ConfigVariableGuid;
extern EFI_GUID gEfiNicIp4ConfigProtocolGuid;
#endif
