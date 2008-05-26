/** @file

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  NicIp4Variable.h

Abstract:

  Routines used to operate the Ip4 configure variable


**/

#ifndef _NIC_IP4_VARIABLE_H_
#define _NIC_IP4_VARIABLE_H_


#include <Protocol/NicIp4Config.h>

//
// Return the size of NIC_IP4_CONFIG_INFO and EFI_IP4_IPCONFIG_DATA.
// They are of variable size
//
#define SIZEOF_IP4_CONFIG_INFO(Ip4Config) \
  (sizeof (EFI_IP4_IPCONFIG_DATA) + \
   sizeof (EFI_IP4_ROUTE_TABLE) * (Ip4Config)->RouteTableSize)

#define SIZEOF_NIC_IP4_CONFIG_INFO(NicConfig) \
  (sizeof (NIC_IP4_CONFIG_INFO) + \
   sizeof (EFI_IP4_ROUTE_TABLE) * (NicConfig)->Ip4Info.RouteTableSize)

//
// Compare whether two NIC address are equal includes their type and length.
//
#define NIC_ADDR_EQUAL(Nic1, Nic2) \
  (((Nic1)->Type == (Nic2)->Type) && ((Nic1)->Len == (Nic2)->Len) && \
   NET_MAC_EQUAL (&(Nic1)->MacAddr, &(Nic2)->MacAddr, (Nic1)->Len))

BOOLEAN
Ip4ConfigIsValid (
  IN NIC_IP4_CONFIG_INFO    *NicConfig
  );

IP4_CONFIG_VARIABLE *
Ip4ConfigReadVariable (
  VOID
  );

EFI_STATUS
Ip4ConfigWriteVariable (
  IN IP4_CONFIG_VARIABLE    *Config       OPTIONAL
  );

NIC_IP4_CONFIG_INFO *
Ip4ConfigFindNicVariable (
  IN IP4_CONFIG_VARIABLE    *Variable,
  IN NIC_ADDR               *NicAddr
  );

IP4_CONFIG_VARIABLE *
Ip4ConfigModifyVariable (
  IN IP4_CONFIG_VARIABLE    *Variable,    OPTIONAL
  IN NIC_ADDR               *NicAddr,
  IN NIC_IP4_CONFIG_INFO    *Config       OPTIONAL
  );

VOID
Ip4ConfigFixRouteTablePointer (
  IN EFI_IP4_IPCONFIG_DATA  *ConfigData
  );

#endif
