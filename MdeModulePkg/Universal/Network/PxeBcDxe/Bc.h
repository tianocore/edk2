/** @file

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  bc.h

Abstract:


**/

#ifndef _BC_H
#define _BC_H

#include <Uefi.h>

#include <Guid/SmBios.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Bis.h>
#include <Protocol/PxeBaseCode.h>
#include <Protocol/PxeBaseCodeCallBack.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/LoadFile.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Tcp.h>
#include <Protocol/LoadedImage.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>

#define CALLBACK_INTERVAL             100 // ten times a second
#define FILTER_BITS                   (EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP | \
                     EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST | \
                     EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS | \
                     EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST \
          )

#define WAIT_TX_TIMEOUT               1000

#define SUPPORT_IPV6                  0

#define PXE_BASECODE_DEVICE_SIGNATURE SIGNATURE_32 ('p', 'x', 'e', 'd')

//
// Determine the classes of IPv4 address
//
#define  IS_CLASSA_IPADDR(x)  ((((EFI_IP_ADDRESS*)x)->v4.Addr[0] & 0x80) == 0x00)
#define  IS_CLASSB_IPADDR(x)  ((((EFI_IP_ADDRESS*)x)->v4.Addr[0] & 0xc0) == 0x80)
#define  IS_CLASSC_IPADDR(x)  ((((EFI_IP_ADDRESS*)x)->v4.Addr[0] & 0xe0) == 0xc0)
#define  IS_INADDR_UNICAST(x) ((IS_CLASSA_IPADDR(x) || IS_CLASSB_IPADDR(x) || IS_CLASSC_IPADDR(x)) && (((EFI_IP_ADDRESS*)x)->Addr[0] != 0) )

//
// Definitions for internet group management protocol version 2 message
// structure
// Per RFC 2236, November 1997
//
#pragma pack(1)

typedef struct {
  UINT8   Type;
  UINT8   MaxRespTime;  // in tenths of a second
  UINT16  Checksum;     // ones complement of ones complement sum of
                        // 16 bit words of message
  UINT32  GroupAddress; // for general query, all systems group,
                        // for group specific, the group
} IGMPV2_MESSAGE;

#define IGMP_TYPE_QUERY                 0x11
#define IGMP_TYPE_REPORT                0x16
#define IGMP_TYPE_V1REPORT              0x12
#define IGMP_TYPE_LEAVE_GROUP           0x17

#define IGMP_DEFAULT_MAX_RESPONSE_TIME  10  // 10 second default
#pragma pack()

#define MAX_MCAST_GROUPS  8                 // most we allow ourselves to join at once
#define MAX_OFFERS        16

typedef struct {
  UINTN                                     Signature;
  EFI_LOCK                                  Lock;
  BOOLEAN                                   ShowErrorMessages;
  EFI_TCP_PROTOCOL                          Tcp;
  EFI_PXE_BASE_CODE_PROTOCOL                EfiBc;
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL       *CallbackProtocolPtr;
  EFI_HANDLE                                Handle;

  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *NiiPtr;
  EFI_SIMPLE_NETWORK_PROTOCOL               *SimpleNetwork;
  UINT8                                     *TransmitBufferPtr;
  UINT8                                     *ReceiveBufferPtr;
  EFI_PXE_BASE_CODE_FUNCTION                Function;

  UINTN                                     OldestArpEntry;
  UINTN                                     MCastGroupCount;
  EFI_EVENT                                 Igmpv1TimeoutEvent;
  BOOLEAN                                   UseIgmpv1Reporting;
  EFI_EVENT                                 IgmpGroupEvent[MAX_MCAST_GROUPS];
  UINT16                                    RandomPort;

#if SUPPORT_IPV6
  //
  // TBD
  //
#else
  UINT32          MCastGroup[MAX_MCAST_GROUPS];
#endif

  BOOLEAN         GoodStationIp;
  BOOLEAN         DidTransmit;
  UINTN           IpLength;
  VOID            *DhcpPacketBuffer;
  UINTN           FileSize;
  VOID            *BootServerReceiveBuffer;
  EFI_IP_ADDRESS  ServerIp;

  //
  // work area
  // for dhcp
  //
  VOID            *ReceiveBuffers;
  VOID            *TransmitBuffer;
  UINTN           NumOffersReceived;
  UINT16          TotalSeconds;

  //
  // arrays for different types of offers
  //
  UINT8           ServerCount[4];
  UINT8           OfferCount[4][MAX_OFFERS];
  UINT8           GotBootp;
  UINT8           GotProxy[4];
  UINT8           BinlProxies[MAX_OFFERS];

  UINT8           *ArpBuffer;
  UINT8           *TftpAckBuffer;
  UINT8           *TftpErrorBuffer;
  IGMPV2_MESSAGE  IgmpMessage;
  BOOLEAN         BigBlkNumFlag;
  UINT8           Timeout;
  UINT16          RandomSeed;
} PXE_BASECODE_DEVICE;

//
// type index
//
#define DHCP_ONLY_IX      0
#define PXE10_IX          1
#define WfM11a_IX         2
#define BINL_IX           3

#define PXE_RND_PORT_LOW  2070

//
//
//
#define LOADFILE_DEVICE_SIGNATURE SIGNATURE_32 ('p', 'x', 'e', 'l')

typedef struct {
  UINTN                   Signature;
  EFI_LOCK                Lock;
  EFI_LOAD_FILE_PROTOCOL  LoadFile;
  PXE_BASECODE_DEVICE     *Private;
} LOADFILE_DEVICE;

#define EFI_BASE_CODE_DEV_FROM_THIS(a)  CR (a, PXE_BASECODE_DEVICE, efi_bc, PXE_BASECODE_DEVICE_SIGNATURE);

#define EFI_BASE_CODE_DEV_FROM_TCP(a)   CR (a, PXE_BASECODE_DEVICE, Tcp, PXE_BASECODE_DEVICE_SIGNATURE);

#define EFI_LOAD_FILE_DEV_FROM_THIS(a)  CR (a, LOADFILE_DEVICE, LoadFile, LOADFILE_DEVICE_SIGNATURE)

EFI_BIS_PROTOCOL                    *
PxebcBisStart (
  PXE_BASECODE_DEVICE     *Private,
  BIS_APPLICATION_HANDLE  *BisAppHandle,
  EFI_BIS_DATA            **BisDataSigInfo
  );

VOID
PxebcBisStop (
  EFI_BIS_PROTOCOL        *Bis,
  BIS_APPLICATION_HANDLE  BisAppHandle,
  EFI_BIS_DATA            *BisDataSigInfo
  );

BOOLEAN
PxebcBisVerify (
  PXE_BASECODE_DEVICE     *Private,
  VOID                    *FileBuffer,
  UINTN                   FileBufferLength,
  VOID                    *CredentialBuffer,
  UINTN                   CredentialBufferLength
  );

BOOLEAN
PxebcBisDetect (
  PXE_BASECODE_DEVICE *Private
  );

//
// Global Variables
//
extern EFI_COMPONENT_NAME_PROTOCOL   gPxeBcComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gPxeBcComponentName2;

//
// //////////////////////////////////////////////////////////
//
// prototypes
//

/**
  Initialize the base code drivers and install the driver binding

  Standard EFI Image Entry

  @retval EFI_SUCCESS  This driver was successfully bound

**/
EFI_STATUS
EFIAPI
InitializeBCDriver (
  IN EFI_HANDLE                       ImageHandle,
  IN EFI_SYSTEM_TABLE                 *SystemTable
  );

EFI_STATUS
EFIAPI
BcStart (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN BOOLEAN                          UseIpv6
  );

EFI_STATUS
EFIAPI
BcStop (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This
  );

EFI_STATUS
EFIAPI
BcDhcp (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN BOOLEAN                          SortOffers
  );

EFI_STATUS
EFIAPI
BcDiscover (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN UINT16                           Type,
  IN UINT16                           *Layer,
  IN BOOLEAN                          UseBis,
  IN EFI_PXE_BASE_CODE_DISCOVER_INFO  * Info OPTIONAL
  );

EFI_STATUS
EFIAPI
BcMtftp (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN EFI_PXE_BASE_CODE_TFTP_OPCODE    Operation,
  IN OUT VOID                         *BufferPtr,
  IN BOOLEAN                          Overwrite,
  IN OUT UINT64                       *BufferSize,
  IN UINTN                            *BlockSize OPTIONAL,
  IN EFI_IP_ADDRESS                   * ServerIp,
  IN UINT8                            *Filename,
  IN EFI_PXE_BASE_CODE_MTFTP_INFO     * Info OPTIONAL,
  IN BOOLEAN                          DontUseBuffer
  );

EFI_STATUS
EFIAPI
BcUdpWrite (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN UINT16                           OpFlags,
  IN EFI_IP_ADDRESS                   *DestIp,
  IN EFI_PXE_BASE_CODE_UDP_PORT       *DestPort,
  IN EFI_IP_ADDRESS                   *GatewayIp, OPTIONAL
  IN EFI_IP_ADDRESS                   *SrcIp, OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT   *SrcPort, OPTIONAL
  IN UINTN                            *HeaderSize, OPTIONAL
  IN VOID                             *HeaderPtr, OPTIONAL
  IN UINTN                            *BufferSize,
  IN VOID                             *BufferPtr
  );

EFI_STATUS
EFIAPI
BcUdpRead (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN UINT16                           OpFlags,
  IN OUT EFI_IP_ADDRESS               *DestIp, OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT   *DestPort, OPTIONAL
  IN OUT EFI_IP_ADDRESS               *SrcIp, OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT   *SrcPort, OPTIONAL
  IN UINTN                            *HeaderSize, OPTIONAL
  IN VOID                             *HeaderPtr, OPTIONAL
  IN OUT UINTN                        *BufferSize,
  IN VOID                             *BufferPtr
  );

EFI_STATUS
EFIAPI
BcTcpWrite (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN UINT16                           OpFlags,
  IN UINT16                           *UrgentPointer,
  IN UINT32                           *SequenceNumber,
  IN UINT32                           *AckNumber,
  IN UINT16                           *HlenResCode,
  IN UINT16                           *Window,
  IN EFI_IP_ADDRESS                   *DestIp,
  IN EFI_PXE_BASE_CODE_TCP_PORT       *DestPort,
  IN EFI_IP_ADDRESS                   *GatewayIp, OPTIONAL
  IN EFI_IP_ADDRESS                   *SrcIp, OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_TCP_PORT   *SrcPort, OPTIONAL
  IN UINTN                            *HeaderSize, OPTIONAL
  IN VOID                             *HeaderPtr, OPTIONAL
  IN UINTN                            *BufferSize,
  IN VOID                             *BufferPtr
  );

EFI_STATUS
EFIAPI
BcTcpRead (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN UINT16                           OpFlags,
  IN OUT EFI_IP_ADDRESS               *DestIp, OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_TCP_PORT   *DestPort, OPTIONAL
  IN OUT EFI_IP_ADDRESS               *SrcIp, OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_TCP_PORT   *SrcPort, OPTIONAL
  IN UINTN                            *HeaderSize, OPTIONAL
  IN VOID                             *HeaderPtr, OPTIONAL
  IN OUT UINTN                        *BufferSize,
  IN VOID                             *BufferPtr
  );

EFI_STATUS
EFIAPI
BcArp (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN EFI_IP_ADDRESS                   * IpAddr,
  IN EFI_MAC_ADDRESS                  * MacAddr OPTIONAL
  );

EFI_STATUS
EFIAPI
BcIpFilter (
  IN EFI_PXE_BASE_CODE_PROTOCOL       *This,
  IN EFI_PXE_BASE_CODE_IP_FILTER      *NewFilter
  );

EFI_STATUS
EFIAPI
BcSetParameters (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN BOOLEAN                          *NewAutoArp, OPTIONAL
  IN BOOLEAN                          *NewSendGUID, OPTIONAL
  IN UINT8                            *NewTTL, OPTIONAL
  IN UINT8                            *NewToS, OPTIONAL
  IN BOOLEAN                          *NewMakeCallback OPTIONAL
  );

EFI_STATUS
EFIAPI
BcSetStationIP (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN EFI_IP_ADDRESS                   * NewStationIp, OPTIONAL
  IN EFI_IP_ADDRESS                   * NewSubnetMask OPTIONAL
  );

EFI_STATUS
EFIAPI
BcSetPackets (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  BOOLEAN                             *NewDhcpDiscoverValid, OPTIONAL
  BOOLEAN                             *NewDhcpAckReceived, OPTIONAL
  BOOLEAN                             *NewProxyOfferReceived, OPTIONAL
  BOOLEAN                             *NewPxeDiscoverValid, OPTIONAL
  BOOLEAN                             *NewPxeReplyReceived, OPTIONAL
  BOOLEAN                             *NewPxeBisReplyReceived, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET         * NewDhcpDiscover, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET         * NewDhcpAck, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET         * NewProxyOffer, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET         * NewPxeDiscover, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET         * NewPxeReply, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET         * NewPxeBisReply OPTIONAL
  );

EFI_STATUS
EFIAPI
LoadFile (
  IN EFI_LOAD_FILE_PROTOCOL           *This,
  IN EFI_DEVICE_PATH_PROTOCOL         *FilePath,
  IN BOOLEAN                          BootPolicy,
  IN OUT UINTN                        *BufferSize,
  IN VOID                             *Buffer
  );

EFI_STATUS
PxeBcLibGetSmbiosSystemGuidAndSerialNumber (
  IN  EFI_GUID    *SystemGuid,
  OUT CHAR8       **SystemSerialNumber
  );

#include "Ip.h"
#include "Dhcp.h"
#include "Tftp.h"

#endif /* _BC_H */

/* EOF - bc.h */
