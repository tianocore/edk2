/** @file
  Header file for IP4Config driver.

Copyright (c) 2006 - 2008, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_IP4CONFIG_H_
#define _EFI_IP4CONFIG_H_

#include <PiDxe.h>

#include <Protocol/Dhcp4.h>
#include <Protocol/Ip4Config.h>
#include <Protocol/ManagedNetwork.h>

#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include "NicIp4Variable.h"

typedef struct _IP4_CONFIG_INSTANCE IP4_CONFIG_INSTANCE;

typedef enum {
  IP4_CONFIG_STATE_IDLE         = 0,
  IP4_CONFIG_STATE_STARTED,
  IP4_CONFIG_STATE_CONFIGURED
} IP4_CONFIG_STATE;

#define IP4_PROTO_ICMP                 0x01
#define IP4_CONFIG_INSTANCE_SIGNATURE  SIGNATURE_32 ('I', 'P', '4', 'C')

typedef enum {
  DHCP_TAG_PARA_LIST            = 55,
  DHCP_TAG_NETMASK              = 1,
  DHCP_TAG_ROUTER               = 3
} DHCP_TAGS;

//
// Configure the DHCP to request the routers and netmask
// from server. The DHCP_TAG_NETMASK is included in Head.
//
#pragma pack(1)
typedef struct {
  EFI_DHCP4_PACKET_OPTION Head;
  UINT8                   Route;
} IP4_CONFIG_DHCP4_OPTION;
#pragma pack()

struct _IP4_CONFIG_INSTANCE {
  UINT32                        Signature;
  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;

  EFI_IP4_CONFIG_PROTOCOL       Ip4ConfigProtocol;
  EFI_NIC_IP4_CONFIG_PROTOCOL   NicIp4Protocol;

  //
  // NicConfig's state, such as IP4_CONFIG_STATE_IDLE
  //
  INTN                          State;

  //
  // Mnp child to keep the connection with MNP.
  //
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;
  EFI_HANDLE                    MnpHandle;

  //
  // User's requests data
  //
  EFI_EVENT                     DoneEvent;
  EFI_EVENT                     ReconfigEvent;
  EFI_STATUS                    Result;

  //
  // Identity of this interface and some configuration info.
  //
  NIC_ADDR                      NicAddr;
  UINT16                        NicName[IP4_NIC_NAME_LENGTH];
  UINT32                        NicIndex;
  NIC_IP4_CONFIG_INFO           *NicConfig;

  //
  // DHCP handles to access DHCP
  //
  EFI_DHCP4_PROTOCOL            *Dhcp4;
  EFI_HANDLE                    Dhcp4Handle;
  EFI_EVENT                     Dhcp4Event;
};

#define IP4_CONFIG_INSTANCE_FROM_IP4CONFIG(this) \
  CR (this, IP4_CONFIG_INSTANCE, Ip4ConfigProtocol, IP4_CONFIG_INSTANCE_SIGNATURE)

#define IP4_CONFIG_INSTANCE_FROM_NIC_IP4CONFIG(this) \
  CR (this, IP4_CONFIG_INSTANCE, NicIp4Protocol, IP4_CONFIG_INSTANCE_SIGNATURE)

extern EFI_DRIVER_BINDING_PROTOCOL   gIp4ConfigDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gIp4ConfigComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gIp4ConfigComponentName2;
extern IP4_CONFIG_INSTANCE           *mIp4ConfigNicList[MAX_IP4_CONFIG_IN_VARIABLE];
extern EFI_IP4_CONFIG_PROTOCOL       mIp4ConfigProtocolTemplate;
extern EFI_NIC_IP4_CONFIG_PROTOCOL   mNicIp4ConfigProtocolTemplate;

/**
  Release all the DHCP related resources.

  @param  This                   The IP4 configure instance

  @return None

**/
VOID
Ip4ConfigCleanDhcp4 (
  IN IP4_CONFIG_INSTANCE        *This
  );

/**
  Clean up all the configuration parameters.

  @param  Instance               The IP4 configure instance

  @return None

**/
VOID
Ip4ConfigCleanConfig (
  IN IP4_CONFIG_INSTANCE        *Instance
  );
#endif
