/** @file
  This file defines NicIp4Config Protocol.
  EFI_NIC_IP4_CONFIG_PROTOCOL is a proprietary protocol, not defined by UEFI2.0.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __NIC_IP4_CONFIG_H__
#define __NIC_IP4_CONFIG_H__

#include <Protocol/Ip4Config.h>


#define EFI_NIC_IP4_CONFIG_PROTOCOL_GUID \
  { \
    0xdca3d4d, 0x12da, 0x4728, { 0xbf, 0x7e, 0x86, 0xce, 0xb9, 0x28, 0xd0, 0x67 } \
  }

#define EFI_NIC_IP4_CONFIG_VARIABLE_GUID \
  { \
    0xd8944553, 0xc4dd, 0x41f4, { 0x9b, 0x30, 0xe1, 0x39, 0x7c, 0xfb, 0x26, 0x7b } \
  }

#define EFI_NIC_IP4_CONFIG_VARIABLE          L"EfiNicIp4ConfigVariable"


typedef struct _EFI_NIC_IP4_CONFIG_PROTOCOL EFI_NIC_IP4_CONFIG_PROTOCOL;

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
  NIC_ADDR                  NicAddr;    ///< Link layer address to identify the NIC
  UINT32                    Source;     ///< Static or DHCP
  BOOLEAN                   Perment;    ///< Survive the reboot or not
  EFI_IP4_IPCONFIG_DATA     Ip4Info;    ///< IP addresses
} NIC_IP4_CONFIG_INFO;

///
/// IP4_CONFIG_VARIABLE is the EFI variable to
/// save the configuration. IP4_CONFIG_VARIABLE is
/// of variable length.
///
typedef struct {
  UINT32                    Len;        ///< Total length of the variable
  UINT16                    CheckSum;   ///< CheckSum, the same as IP4 head checksum
  UINT32                    Count;      ///< Number of NIC_IP4_CONFIG_INFO follows
  NIC_IP4_CONFIG_INFO       ConfigInfo;
} IP4_CONFIG_VARIABLE;

/**
  Get the configure parameter for this NIC.

  @param  This                   The NIC IP4 CONFIG protocol.
  @param  Len                    The length of the NicConfig buffer.
  @param  NicConfig              The buffer to receive the NIC's configure
                                 parameter.

  @retval EFI_SUCCESS            The configure parameter for this NIC was 
                                 obtained successfully .
  @retval EFI_INVALID_PARAMETER  This or ConfigLen is NULL.
  @retval EFI_NOT_FOUND          There is no configure parameter for the NIC in
                                 NVRam.
  @retval EFI_BUFFER_TOO_SMALL   The ConfigLen is too small or the NicConfig is 
                                 NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_NIC_IP4_CONFIG_GET_INFO)(
  IN EFI_NIC_IP4_CONFIG_PROTOCOL  *This,
  IN OUT UINTN                    *Len,
  OUT NIC_IP4_CONFIG_INFO         *NicConfig     OPTIONAL
  );

/**
  Set the IP configure parameters for this NIC. 

  If Reconfig is TRUE, the IP driver will be informed to discard current 
  auto configure parameter and restart the auto configuration process. 
  If current there is a pending auto configuration, EFI_ALREADY_STARTED is
  returned. You can only change the configure setting when either
  the configure has finished or not started yet. If NicConfig, the
  NIC's configure parameter is removed from the variable.

  @param  This                   The NIC IP4 CONFIG protocol
  @param  NicConfig              The new NIC IP4 configure parameter
  @param  Reconfig               Inform the IP4 driver to restart the auto
                                 configuration
                                 
  @retval EFI_SUCCESS            The configure parameter for this NIC was 
                                 set successfully .
  @retval EFI_INVALID_PARAMETER  This is NULL or the configure parameter is
                                 invalid.
  @retval EFI_ALREADY_STARTED    There is a pending auto configuration.
  @retval EFI_NOT_FOUND          No auto configure parameter is found

**/
typedef
EFI_STATUS
(EFIAPI *EFI_NIC_IP4_CONFIG_SET_INFO)(
  IN EFI_NIC_IP4_CONFIG_PROTOCOL  *This,
  IN NIC_IP4_CONFIG_INFO          *NicConfig     OPTIONAL,
  IN BOOLEAN                      ReConfig
  );

/**
  Return the name and MAC address for the NIC. The Name, if not NULL,
  has at least IP4_NIC_NAME_LENGTH bytes.

  @param  This                   The NIC IP4 CONFIG protocol
  @param  Name                   The buffer to return the name
  @param  NicAddr                The buffer to return the MAC addr

  @retval EFI_INVALID_PARAMETER  This is NULL
  @retval EFI_SUCCESS            The name or address of the NIC are returned.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_NIC_IP4_CONFIG_GET_NAME)(
  IN  EFI_NIC_IP4_CONFIG_PROTOCOL *This,
  OUT  UINT16                     *Name          OPTIONAL,
  OUT  NIC_ADDR                   *NicAddr       OPTIONAL
  );

struct _EFI_NIC_IP4_CONFIG_PROTOCOL {
  EFI_NIC_IP4_CONFIG_GET_NAME     GetName;
  EFI_NIC_IP4_CONFIG_GET_INFO     GetInfo;
  EFI_NIC_IP4_CONFIG_SET_INFO     SetInfo;
};

extern EFI_GUID gEfiNicIp4ConfigVariableGuid;
extern EFI_GUID gEfiNicIp4ConfigProtocolGuid;
#endif
