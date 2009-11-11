/** @file
  EFI IPSEC Protocol Definition
  The EFI_IPSEC_PROTOCOL is used to abstract the ability to deal with the individual
  packets sent and received by the host and provide packet-level security for IP datagram.

  Copyright (c) 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:          
  This Protocol is introduced in UEFI Specification 2.3

**/

#ifndef __EFI_IPSEC_PROTOCOL_H__
#define __EFI_IPSEC_PROTOCOL_H__

#include <Protocol/IpSecConfig.h>

#define EFI_IPSEC_PROTOCOL_GUID \
  { \
    0xdfb386f7, 0xe100, 0x43ad, {0x9c, 0x9a, 0xed, 0x90, 0xd0, 0x8a, 0x5e, 0x12 } \
  }

typedef struct _EFI_IPSEC_PROTOCOL  EFI_IPSEC_PROTOCOL;

///
/// EFI_IPSEC_FRAGMENT_DATA 
/// defines the instances of packet fragments.
///
typedef struct _EFI_IPSEC_FRAGMENT_DATA { 
  UINT32  FragmentLength;
  VOID    *FragmentBuffer;
} EFI_IPSEC_FRAGMENT_DATA; 


/**
  Handles IPsec packet processing for inbound and outbound IP packets. 

  The EFI_IPSEC_PROCESS process routine handles each inbound or outbound packet.
  The behavior is that it can perform one of the following actions: 
  bypass the packet, discard the packet, or protect the packet.       

  @param[in]      This             Pointer to the EFI_IPSEC_PROTOCOL instance.
  @param[in]      NicHandle        Instance of the network interface.
  @param[in]      IpVer            IPV4 or IPV6.
  @param[in, out] IpHead           Pointer to the IP Header.
  @param[in]      LastHead         The protocol of the next layer to be processed by IPsec.
  @param[in]      OptionsBuffer    Pointer to the options buffer. 
  @param[in]      OptionsLength    Length of the options buffer.
  @param[in, out] FragmentTable    Pointer to a list of fragments. 
  @param[in]      FragmentCount    Number of fragments.
  @param[in]      TrafficDirection Traffic direction.
  @param[out]     RecycleSignal    Event for recycling of resources.
 
  @retval EFI_SUCCESS              The packet was bypassed and all buffers remain the same.
  @retval EFI_SUCCESS              The packet was protected.
  @retval EFI_ACCESS_DENIED        The packet was discarded.

**/
typedef
EFI_STATUS
(EFIAPI  *EFI_IPSEC_PROCESS) (
  IN     EFI_IPSEC_PROTOCOL      *This,
  IN     EFI_HANDLE              NicHandle,
  IN     UINT8                   IpVer,
  IN OUT VOID                    *IpHead,
  IN     UINT8                   *LastHead,
  IN     VOID                    *OptionsBuffer,
  IN     UINT32                  OptionsLength,
  IN OUT EFI_IPSEC_FRAGMENT_DATA **FragmentTable,
  IN     UINT32                  *FragmentCount,
  IN     EFI_IPSEC_TRAFFIC_DIR   TrafficDirection,
     OUT EFI_EVENT               *RecycleSignal
  );

///
/// EFI_IPSEC_PROTOCOL 
/// provides the ability for  securing IP communications by authenticating
/// and/or encrypting each IP packet in a data stream. 
//  EFI_IPSEC_PROTOCOL can be consumed by both the IPv4 and IPv6 stack.
//  A user can employ this protocol for IPsec package handling in both IPv4
//  and IPv6 environment.
///
struct _EFI_IPSEC_PROTOCOL {
  EFI_IPSEC_PROCESS      Process;           ///< Handle the IPsec message.
  EFI_EVENT              DisabledEvent;     ///< Event signaled when the interface is disabled.
  BOOLEAN                DisabledFlag;      ///< State of the interface.
};

extern EFI_GUID gEfiIpSecProtocolGuid;

#endif
