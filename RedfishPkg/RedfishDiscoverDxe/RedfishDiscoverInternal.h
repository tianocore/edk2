/** @file
  This file defines the EFI Redfish Discover Protocol interface.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022, AMD Incorporated. All rights reserved.
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EFI_REDFISH_DISCOVER_INTERNAL_H_
#define EFI_REDFISH_DISCOVER_INTERNAL_H_

#include <Uefi.h>

#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/RedfishDiscover.h>
#include <Protocol/Smbios.h>
#include <Protocol/Tcp4.h>
#include <Protocol/Tcp6.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>
#include <Library/RedfishDebugLib.h>
#include <Library/RestExLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include <IndustryStandard/RedfishHostInterface.h>

#define REDFISH_DISCOVER_VERSION                    0x00010000
#define EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_TPL  TPL_NOTIFY

#define MAC_COMPARE(This, Target)  (CompareMem ((VOID *)&(This)->MacAddress, &(Target)->MacAddress, (This)->HwAddressSize) == 0)
#define VALID_TCP6(Target, This)   ((Target)->IsIpv6 && ((This)->NetworkProtocolType == ProtocolTypeTcp6))
#define VALID_TCP4(Target, This)   (!(Target)->IsIpv6 && ((This)->NetworkProtocolType == ProtocolTypeTcp4))
#define REDFISH_HI_ITERFACE_SPECIFIC_DATA_LENGTH_OFFSET  ((UINT16)(UINTN)(&((SMBIOS_TABLE_TYPE42 *)0)->InterfaceTypeSpecificDataLength))
#define REDFISH_HI_PROTOCOL_HOSTNAME_LENGTH_OFFSET       ((UINT16)(UINTN)(&((REDFISH_OVER_IP_PROTOCOL_DATA *)0)->RedfishServiceHostnameLength))

//
// GUID definitions
//

#define EFI_REDFISH_DISCOVER_TCP4_INSTANCE_GUID \
  { \
    0xfbab97a4, 0x4c6a, 0xf8e8, { 0xf2, 0x25, 0x42, 0x8a, 0x80, 0x3f, 0xb6, 0xaa } \
  }

#define EFI_REDFISH_DISCOVER_TCP6_INSTANCE_GUID \
  { \
    0xbe513b6d, 0x41c1, 0x96Ed, { 0x8d, 0xaf, 0x3e, 0x89, 0xc5, 0xf5, 0x02, 0x25 } \
  }

#define EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_GUID \
  { \
    0xc44a6076, 0xd42a, 0x4d54, { 0x85, 0x6d, 0x98, 0x8a, 0x85, 0x8f, 0xa1, 0x11 } \
  }

extern EFI_COMPONENT_NAME_PROTOCOL   gRedfishDiscoverComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gRedfishDiscoverComponentName2;
extern EFI_UNICODE_STRING_TABLE      *gRedfishDiscoverControllerNameTable;

//
// Enumeration of network protocols
// required for the Redfish service discovery.
//
typedef enum {
  ProtocolTypeTcp4 = 0, ///< Network protocol TCPv4.
  ProtocolTypeTcp6,     ///< Network protocol TCCv6.
  ProtocolTypeRestEx,   ///< REST EX over network protocol.
  MaxProtocolType
} NETWORK_INTERFACE_PROTOCOL_TYPE;

//
// Network protocol information installed on
// the network interface.
//
typedef struct {
  EFI_GUID      ProtocolGuid;              ///< Network protocol GUID.
  EFI_GUID      ProtocolServiceGuid;       ///< Network protocol service GUID.
  UINT32        ProtocolDiscoverId;        ///< The identifier installed on network protocol handle.
  EFI_HANDLE    ProtocolControllerHandle;  ///< The controller handle on network protocol.
  VOID          *NetworkProtocolInterface; ///< The protocol interface of network protocol.
} REDFISH_DISCOVER_NETWORK_INTERFACE_PROTOCOL;

//
// Internal structure used to maintain network
// interface properties.
//
typedef struct {
  LIST_ENTRY                                     Entry;                      ///< Link list entry.
  EFI_HANDLE                                     OpenDriverAgentHandle;      ///< The agent to open network protocol.
  EFI_HANDLE                                     OpenDriverControllerHandle; ///< The controller handle to open network protocol.
  UINTN                                          HwAddressSize;              ///< The size of network interface hardware address.
  EFI_MAC_ADDRESS                                MacAddress;                 ///< MAC address of network interface.
  CHAR16                                         *StrMacAddr;                ///< String to MAC address of network interface.
  BOOLEAN                                        GotSubnetInfo;              ///< Indicates sub net information is retrieved.
  EFI_IP_ADDRESS                                 SubnetAddr;                 ///< Subnet ID.
  EFI_IP_ADDRESS                                 SubnetMask;                 ///< Subnet mask (IPv4 only)
  UINT8                                          SubnetPrefixLength;         ///< Subnet prefix.
  UINT16                                         VlanId;                     ///< VLAN ID
  UINT32                                         SubnetAddrInfoIPv6Number;   ///< IPv6 address info number.
  EFI_IP6_ADDRESS_INFO                           *SubnetAddrInfoIPv6;        ///< IPv6 address info.
  //
  // Network interface protocol and REST EX info.
  //
  UINT32                                         NetworkProtocolType;          ///< Network protocol type. Refer to
                                                                               ///< NETWORK_INTERFACE_PROTOCOL_TYPE.
  REDFISH_DISCOVER_NETWORK_INTERFACE_PROTOCOL    NetworkInterfaceProtocolInfo; ///< Network interface protocol information.
  EFI_HANDLE                                     RestExHandle;                 ///< REST EX handle associated with this network interface.
  //
  // EFI_REDFISH_DISCOVER_PROTOCOL instance installed
  // on this network interface.
  //
  EFI_HANDLE                                     EfiRedfishDiscoverProtocolHandle; ///< EFI_REDFISH_DISCOVER_PROTOCOL instance installed
                                                                                   ///< on this network interface.
} EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL;

//
// Redfish Discover Instance signature
//

#define EFI_REDFISH_DISCOVER_DATA_SIGNATURE  SIGNATURE_32 ('E', 'R', 'D', 'D')

#define EFI_REDFISH_DISOVER_DATA_FROM_DISCOVER_PROTOCOL(a) \
  CR (a, EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_INTERNAL, RedfishDiscoverProtocol, EFI_REDFISH_DISCOVER_DATA_SIGNATURE)

//
// Internal structure used to maintain REST EX properties.
//
typedef struct {
  LIST_ENTRY                                Entry;                      ///< Link list entry.
  UINT32                                    Signature;                  ///< Instance signature.
  EFI_HANDLE                                OpenDriverAgentHandle;      ///< The agent to open network protocol.
  EFI_HANDLE                                OpenDriverControllerHandle; ///< The controller handle to open network protocol.
  EFI_HANDLE                                RestExChildHandle;          ///< The child handle created through REST EX Service Protocol.
  EFI_HANDLE                                RestExControllerHandle;     ///< The controller handle which provide REST EX protocol.
  EFI_REST_EX_PROTOCOL                      *RestExProtocolInterface;   ///< Pointer to EFI_REST_EX_PROTOCOL.
  UINT32                                    RestExId;                   ///< The identifier installed on REST EX controller handle.
  UINTN                                     NumberOfNetworkInterfaces;  ///< Number of network interfaces can do Redfish service discovery.
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE    *NetworkInterfaceInstances; ///< Network interface instances. It's an array of instances. The number of entries
                                                                        ///< in array is indicated by NumberOfNetworkInterfaces.
  EFI_REDFISH_DISCOVER_PROTOCOL             RedfishDiscoverProtocol;    ///< EFI_REDFISH_DISCOVER_PROTOCOL protocol.
} EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_INTERNAL;

/**
  This function to get subnet information.

  @param[in]            ImageHandle  EFI handle with this image.
  @param[in]            Instance  Instance of Network interface.
  @retval EFI_STATUS    Get subnet information successfully.
  @retval Otherwise     Fail to get subnet information.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_REDFISH_DISCOVER_GET_SUBNET_INFO)(
  IN EFI_HANDLE ImageHandle,
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *Instance
  );

//
// The require network protocol matrix.
//
typedef struct {
  UINT32                                  ProtocolType;                        ///< Network protocol type,
                                                                               ///< Refer to NETWORK_INTERFACE_PROTOCOL_TYPE.
  CHAR16                                  *ProtocolName;                       ///< Protocol name.
  EFI_GUID                                *RequiredProtocolGuid;               ///< Network protocol interface GUID.
  EFI_GUID                                *RequiredServiceBindingProtocolGuid; ///< Network protocol service GUID.
  EFI_GUID                                *DiscoveredProtocolGuid;             ///< Protocol interface GUID use to install identifier.
  EFI_REDFISH_DISCOVER_GET_SUBNET_INFO    GetSubnetInfo;                       ///< Function of getting subnet information.
} REDFISH_DISCOVER_REQUIRED_PROTOCOL;

//
// Link list of Redfish discover instance.
//
typedef struct {
  LIST_ENTRY                         NextInstance;      ///< Next list.
  EFI_REDFISH_DISCOVERED_INSTANCE    *Instance;         ///< Pointer to EFI_REDFISH_DISCOVERED_INSTANCE.
} EFI_REDFISH_DISCOVERED_INTERNAL_LIST;

//
// Internal structure of Redfish discover instance.
//
typedef struct {
  LIST_ENTRY                                         Entry;             ///< Link list entry.
  EFI_HANDLE                                         Owner;             ///< The owner owns this Redfish service discovery.
                                                                        ///< It's the EFI image handle of driver uses
                                                                        ///< EFI Redfish Discover Protocol.
  EFI_REDFISH_DISCOVER_FLAG                          DiscoverFlags;     ///< EFI_REDFISH_DISCOVER_FLAG
  EFI_REDFISH_DISCOVERED_TOKEN                       *DiscoverToken;    ///< Token used to signal when Redfish service is discovered.
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL    *NetworkInterface; ///< EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL
                                                                        ///< instance used to discover Redfish service.
  //
  // Below for Host interface discovery.
  //
  BOOLEAN                                            HostIntfValidation; ///< Indicates whether to validate Redfish Host interface.
  EFI_IP_ADDRESS                                     TargetIpAddress;    ///< Target IP address reported in Redfish Host interface.
  UINT8                                              HostAddrFormat;     ///< Unknown=00h, Ipv4=01h, Ipv6=02h.
  EFI_IP_ADDRESS                                     HostIpAddress;      ///< Host IP address reported in Redfish Host interface.
  EFI_IP_ADDRESS                                     HostSubnetMask;     ///< Host subnet mask address reported in Redfish Host interface.
} EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE;

/**
  The function adds a new found Redfish service to internal list and
  notify client.

  It simply frees the packet.

  @param[in]  Instance              EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE.
  @param[in]  RedfishVersion        Redfish version.
  @param[in]  RedfishLocation       Redfish location.
  @param[in]  Uuid                  Service UUID string.
  @param[in]  Os                    OS string.
  @param[in]  OsVer                 OS version string.
  @param[in]  Product               Product string.
  @param[in]  ProductVer            Product version string.
  @param[in]  UseHttps              Redfish service requires secured connection.
  @retval EFI_SUCCESS               Redfish service is added to list successfully.

**/
EFI_STATUS
AddAndSignalNewRedfishService (
  IN EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE  *Instance,
  IN UINTN                                     *RedfishVersion OPTIONAL,
  IN CHAR8                                     *RedfishLocation OPTIONAL,
  IN CHAR8                                     *Uuid  OPTIONAL,
  IN CHAR8                                     *Os  OPTIONAL,
  IN CHAR8                                     *OsVer  OPTIONAL,
  IN CHAR8                                     *Product  OPTIONAL,
  IN CHAR8                                     *ProductVer  OPTIONAL,
  IN BOOLEAN                                   UseHttps
  );

/**
  The function gets information reported in Redfish Host Interface.

  It simply frees the packet.

  @param[in]  Smbios           SMBIOS protocol.
  @param[out] DeviceDescriptor Pointer to REDFISH_INTERFACE_DATA.
  @param[out] ProtocolData     Pointer to REDFISH_OVER_IP_PROTOCOL_DATA.

  @retval EFI_SUCCESS    Get host interface successfully.
  @retval Otherwise      Fail to tet host interface.

**/
EFI_STATUS
RedfishGetHostInterfaceProtocolData (
  IN EFI_SMBIOS_PROTOCOL             *Smbios,
  OUT REDFISH_INTERFACE_DATA         **DeviceDescriptor,
  OUT REDFISH_OVER_IP_PROTOCOL_DATA  **ProtocolData
  );

extern EFI_GUID  gRedfishDiscoverTcp4Instance;
extern EFI_GUID  gRedfishDiscoverTcp6Instance;
extern EFI_GUID  gRedfishDiscoverRestEXInstance;
#endif
