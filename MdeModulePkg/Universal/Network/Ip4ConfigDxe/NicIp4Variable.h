/** @file
  Routines used to operate the Ip4 configure variable.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _NIC_IP4_VARIABLE_H_
#define _NIC_IP4_VARIABLE_H_

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

/**
  Check whether the configure parameter is valid.

  @param  NicConfig    The configure parameter to check

  @return TRUE if the parameter is valid for the interface, otherwise FALSE.

**/
BOOLEAN
Ip4ConfigIsValid (
  IN NIC_IP4_CONFIG_INFO    *NicConfig
  );

/**
  Read the ip4 configure variable from the EFI variable.

  @param  Instance     The IP4 CONFIG instance.

  @return The IP4 configure read if it is there and is valid, otherwise NULL.

**/
NIC_IP4_CONFIG_INFO *
Ip4ConfigReadVariable (
  IN  IP4_CONFIG_INSTANCE   *Instance
  );

/**
  Write the IP4 configure variable to the NVRAM. If Config
  is NULL, remove the variable.

  @param  Instance     The IP4 CONFIG instance.
  @param  NicConfig    The IP4 configure data to write.

  @retval EFI_SUCCESS  The variable is written to the NVRam.
  @retval Others       Failed to write the variable.

**/
EFI_STATUS
Ip4ConfigWriteVariable (
  IN IP4_CONFIG_INSTANCE    *Instance,
  IN NIC_IP4_CONFIG_INFO    *NicConfig OPTIONAL
  );

/**
  Reclaim Ip4Config Variables for NIC which has been removed from the platform.

**/
VOID
Ip4ConfigReclaimVariable (
  VOID
  );

/**
  Fix the RouteTable pointer in an EFI_IP4_IPCONFIG_DATA structure.

  The pointer is set to be immediately follow the ConfigData if there're entries
  in the RouteTable. Otherwise it is set to NULL.

  @param  ConfigData     The IP4 IP configure data.

**/
VOID
Ip4ConfigFixRouteTablePointer (
  IN OUT EFI_IP4_IPCONFIG_DATA  *ConfigData
  );

#endif

