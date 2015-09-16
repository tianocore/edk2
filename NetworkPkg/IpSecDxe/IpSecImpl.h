/** @file
  The definitions related to IPsec protocol implementation.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IP_SEC_IMPL_H_
#define _IP_SEC_IMPL_H_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/IpSec.h>
#include <Protocol/IpSecConfig.h>
#include <Protocol/Dpc.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>

typedef struct _IPSEC_PRIVATE_DATA IPSEC_PRIVATE_DATA;
typedef struct _IPSEC_SPD_ENTRY IPSEC_SPD_ENTRY;
typedef struct _IPSEC_PAD_ENTRY IPSEC_PAD_ENTRY;
typedef struct _IPSEC_SPD_DATA IPSEC_SPD_DATA;

#define IPSEC_PRIVATE_DATA_SIGNATURE        SIGNATURE_32 ('I', 'P', 'S', 'E')

#define IPSEC_PRIVATE_DATA_FROM_IPSEC(a)    CR (a, IPSEC_PRIVATE_DATA, IpSec, IPSEC_PRIVATE_DATA_SIGNATURE)
#define IPSEC_PRIVATE_DATA_FROM_UDP4LIST(a) CR (a, IPSEC_PRIVATE_DATA, Udp4List, IPSEC_PRIVATE_DATA_SIGNATURE)
#define IPSEC_PRIVATE_DATA_FROM_UDP6LIST(a) CR (a, IPSEC_PRIVATE_DATA, Udp6List, IPSEC_PRIVATE_DATA_SIGNATURE)
#define IPSEC_UDP_SERVICE_FROM_LIST(a)      BASE_CR (a, IKE_UDP_SERVICE, List)
#define IPSEC_SPD_ENTRY_FROM_LIST(a)        BASE_CR (a, IPSEC_SPD_ENTRY, List)
#define IPSEC_SAD_ENTRY_FROM_LIST(a)        BASE_CR (a, IPSEC_SAD_ENTRY, List)
#define IPSEC_PAD_ENTRY_FROM_LIST(a)        BASE_CR (a, IPSEC_PAD_ENTRY, List)
#define IPSEC_SAD_ENTRY_FROM_SPD(a)         BASE_CR (a, IPSEC_SAD_ENTRY, BySpd)

#define IPSEC_STATUS_DISABLED       0
#define IPSEC_STATUS_ENABLED        1
#define IPSEC_ESP_PROTOCOL          50
#define IPSEC_AH_PROTOCOL           51
#define IPSEC_DEFAULT_VARIABLE_SIZE 0x100

//
// Internal Structure Definition
//
#pragma pack(1)
typedef struct _EFI_AH_HEADER {
  UINT8   NextHeader;
  UINT8   PayloadLen;
  UINT16  Reserved;
  UINT32  Spi;
  UINT32  SequenceNumber;
} EFI_AH_HEADER;

typedef struct _EFI_ESP_HEADER {
  UINT32  Spi;
  UINT32  SequenceNumber;
} EFI_ESP_HEADER;

typedef struct _EFI_ESP_TAIL {
  UINT8 PaddingLength;
  UINT8 NextHeader;
} EFI_ESP_TAIL;
#pragma pack()

struct _IPSEC_SPD_DATA {
  CHAR16                    Name[100];
  UINT32                    PackageFlag;
  EFI_IPSEC_TRAFFIC_DIR     TrafficDirection;
  EFI_IPSEC_ACTION          Action;
  EFI_IPSEC_PROCESS_POLICY  *ProcessingPolicy;
  LIST_ENTRY                Sas;
};

struct _IPSEC_SPD_ENTRY {
  EFI_IPSEC_SPD_SELECTOR  *Selector;
  IPSEC_SPD_DATA          *Data;
  LIST_ENTRY              List;
};

typedef struct _IPSEC_SAD_DATA {
  EFI_IPSEC_MODE         Mode;
  UINT64                 SequenceNumber;
  UINT8                  AntiReplayWindowSize;
  UINT64                 AntiReplayBitmap[4];  // bitmap for received packet
  EFI_IPSEC_ALGO_INFO    AlgoInfo;
  EFI_IPSEC_SA_LIFETIME  SaLifetime;
  UINT32                 PathMTU;
  IPSEC_SPD_ENTRY        *SpdEntry;
  EFI_IPSEC_SPD_SELECTOR *SpdSelector;
  BOOLEAN                ESNEnabled;           // Extended (64-bit) SN enabled
  BOOLEAN                ManualSet;
  EFI_IP_ADDRESS         TunnelDestAddress;
  EFI_IP_ADDRESS         TunnelSourceAddress;
} IPSEC_SAD_DATA;

typedef struct _IPSEC_SAD_ENTRY {
  EFI_IPSEC_SA_ID  *Id;
  IPSEC_SAD_DATA  *Data;
  LIST_ENTRY      List;
  LIST_ENTRY      BySpd;                      // Linked on IPSEC_SPD_DATA.Sas
} IPSEC_SAD_ENTRY;

struct _IPSEC_PAD_ENTRY {
  EFI_IPSEC_PAD_ID    *Id;
  EFI_IPSEC_PAD_DATA  *Data;
  LIST_ENTRY          List;
};

typedef struct _IPSEC_RECYCLE_CONTEXT {
  EFI_IPSEC_FRAGMENT_DATA *FragmentTable;
  UINT8                   *PayloadBuffer;
} IPSEC_RECYCLE_CONTEXT;

//
// Struct used to store the Hash and its data.
//
typedef struct {
  UINTN DataSize;
  UINT8 *Data;
} HASH_DATA_FRAGMENT;

struct _IPSEC_PRIVATE_DATA {
  UINT32                    Signature;
  EFI_HANDLE                Handle;           // Virtual handle to install private prtocol
  EFI_HANDLE                ImageHandle;
  EFI_IPSEC2_PROTOCOL       IpSec;
  EFI_IPSEC_CONFIG_PROTOCOL IpSecConfig;
  BOOLEAN                   SetBySelf;
  LIST_ENTRY                Udp4List;
  UINTN                     Udp4Num;
  LIST_ENTRY                Udp6List;
  UINTN                     Udp6Num;
  LIST_ENTRY                Ikev1SessionList;
  LIST_ENTRY                Ikev1EstablishedList;
  LIST_ENTRY                Ikev2SessionList;
  LIST_ENTRY                Ikev2EstablishedList;
  BOOLEAN                   IsIPsecDisabling;
};

/**
  This function processes the inbound traffic with IPsec.

  It checks the received packet security property, trims the ESP/AH header, and then 
  returns without an IPsec protected IP Header and FragmentTable.
  
  @param[in]      IpVersion          The version of IP.
  @param[in, out] IpHead             Points to IP header containing the ESP/AH header 
                                     to be trimed on input, and without ESP/AH header
                                     on return.
  @param[in, out] LastHead           The Last Header in IP header on return.
  @param[in, out] OptionsBuffer      Pointer to the options buffer.
  @param[in, out] OptionsLength      Length of the options buffer.
  @param[in, out] FragmentTable      Pointer to a list of fragments in form of IPsec
                                     protected on input, and without IPsec protected
                                     on return.
  @param[in, out] FragmentCount      The number of fragments.
  @param[out]     SpdEntry           Pointer to contain the address of SPD entry on return.
  @param[out]     RecycleEvent       The event for recycling of resources.

  @retval EFI_SUCCESS              The operation was successful.
  @retval EFI_UNSUPPORTED          The IPSEC protocol is not supported.

**/
EFI_STATUS
IpSecProtectInboundPacket (
  IN     UINT8                       IpVersion,
  IN OUT VOID                        *IpHead,
  IN OUT UINT8                       *LastHead,
  IN OUT VOID                        **OptionsBuffer,
  IN OUT UINT32                      *OptionsLength,
  IN OUT EFI_IPSEC_FRAGMENT_DATA     **FragmentTable,
  IN OUT UINT32                      *FragmentCount,
     OUT EFI_IPSEC_SPD_SELECTOR      **SpdEntry,
     OUT EFI_EVENT                   *RecycleEvent
  );


/**
  This fucntion processes the output traffic with IPsec.

  It protected the sending packet by encrypting it payload and inserting ESP/AH header
  in the orginal IP header, then return the IpHeader and IPsec protected Fragmentable.

  @param[in]      IpVersion          The version of IP.
  @param[in, out] IpHead             Point to IP header containing the orginal IP header
                                     to be processed on input, and inserted ESP/AH header
                                     on return.
  @param[in, out] LastHead           The Last Header in IP header.
  @param[in, out] OptionsBuffer      Pointer to the options buffer.
  @param[in, out] OptionsLength      Length of the options buffer.
  @param[in, out] FragmentTable      Pointer to a list of fragments to be protected by
                                     IPsec on input, and with IPsec protected
                                     on return.
  @param[in, out] FragmentCount      Number of fragments.
  @param[in]      SadEntry           Related SAD entry.
  @param[out]     RecycleEvent       Event for recycling of resources.

  @retval EFI_SUCCESS              The operation is successful.
  @retval EFI_UNSUPPORTED          If the IPSEC protocol is not supported.

**/
EFI_STATUS
IpSecProtectOutboundPacket (
  IN     UINT8                       IpVersion,
  IN OUT VOID                        *IpHead,
  IN OUT UINT8                       *LastHead,
  IN OUT VOID                        **OptionsBuffer,
  IN OUT UINT32                      *OptionsLength,
  IN OUT EFI_IPSEC_FRAGMENT_DATA     **FragmentTable,
  IN OUT UINT32                      *FragmentCount,
  IN     IPSEC_SAD_ENTRY             *SadEntry,
     OUT EFI_EVENT                   *RecycleEvent
  );

/**
  Check if the IP Address in the address range of AddressInfos specified.

  @param[in]  IpVersion         The IP version.
  @param[in]  IpAddr            Points to EFI_IP_ADDRESS to be check.
  @param[in]  AddressInfo       A list of EFI_IP_ADDRESS_INFO that is used to check
                                the IP Address is matched.
  @param[in]  AddressCount      The total numbers of the AddressInfo.

  @retval   TRUE    If the Specified IP Address is in the range of the AddressInfos specified.
  @retval   FALSE   If the Specified IP Address is not in the range of the AddressInfos specified.

**/
BOOLEAN
IpSecMatchIpAddress (
  IN UINT8                                  IpVersion,
  IN EFI_IP_ADDRESS                         *IpAddr,
  IN EFI_IP_ADDRESS_INFO                    *AddressInfo,
  IN UINT32                                 AddressCount
  );

/**
  Find a PAD entry according to remote IP address.

  @param[in]  IpVersion         The version of IP.
  @param[in]  IpAddr            Point to remote IP address.

  @return The pointer of related PAD entry.

**/
IPSEC_PAD_ENTRY *
IpSecLookupPadEntry (
  IN UINT8                                  IpVersion,
  IN EFI_IP_ADDRESS                         *IpAddr
  );

/**
  Check if the specified IP packet can be serviced by this SPD entry.

  @param[in]  SpdEntry          Point to SPD entry.
  @param[in]  IpVersion         Version of IP.
  @param[in]  IpHead            Point to IP header.
  @param[in]  IpPayload         Point to IP payload.
  @param[in]  Protocol          The Last protocol of IP packet.
  @param[in]  IsOutbound        Traffic direction.
  @param[out] Action            The support action of SPD entry.

  @retval EFI_SUCCESS       Find the related SPD.
  @retval EFI_NOT_FOUND     Not find the related SPD entry;

**/
EFI_STATUS
IpSecLookupSpdEntry (
  IN     IPSEC_SPD_ENTRY         *SpdEntry,
  IN     UINT8                   IpVersion,
  IN     VOID                    *IpHead,
  IN     UINT8                   *IpPayload,
  IN     UINT8                   Protocol,
  IN     BOOLEAN                 IsOutbound, 
     OUT EFI_IPSEC_ACTION        *Action
  );

/**
  Look up if there is existing SAD entry for specified IP packet sending.

  This function is called by the IPsecProcess when there is some IP packet needed to
  send out. This function checks if there is an existing SAD entry that can be serviced
  to this IP packet sending. If no existing SAD entry could be used, this
  function will invoke an IPsec Key Exchange Negotiation.

  @param[in]  Private           Points to private data.
  @param[in]  NicHandle         Points to a NIC handle.
  @param[in]  IpVersion         The version of IP.
  @param[in]  IpHead            The IP Header of packet to be sent out.
  @param[in]  IpPayload         The IP Payload to be sent out.
  @param[in]  OldLastHead       The Last protocol of the IP packet.
  @param[in]  SpdEntry          Points to a related SPD entry.
  @param[out] SadEntry          Contains the Point of a related SAD entry.

  @retval EFI_DEVICE_ERROR  One of following conditions is TRUE:
                            - If don't find related UDP service.
                            - Sequence Number is used up.
                            - Extension Sequence Number is used up.
  @retval EFI_NOT_READY     No existing SAD entry could be used.
  @retval EFI_SUCCESS       Find the related SAD entry.

**/
EFI_STATUS
IpSecLookupSadEntry (
  IN IPSEC_PRIVATE_DATA      *Private,
  IN EFI_HANDLE              NicHandle,
  IN UINT8                   IpVersion,
  IN VOID                    *IpHead,
  IN UINT8                   *IpPayload,
  IN UINT8                   OldLastHead,
  IN IPSEC_SPD_ENTRY         *SpdEntry,
  OUT IPSEC_SAD_ENTRY        **SadEntry
  );

/**
  Find the SAD through whole SAD list.

  @param[in]  Spi               The SPI used to search the SAD entry.
  @param[in]  DestAddress       The destination used to search the SAD entry.
  @param[in]  IpVersion         The IP version. Ip4 or Ip6.

  @return  The pointer to a certain SAD entry.

**/
IPSEC_SAD_ENTRY *
IpSecLookupSadBySpi (
  IN UINT32                                 Spi,
  IN EFI_IP_ADDRESS                         *DestAddress,
  IN UINT8                                  IpVersion
  )
;

/**
  Handles IPsec packet processing for inbound and outbound IP packets.

  The EFI_IPSEC_PROCESS process routine handles each inbound or outbound packet.
  The behavior is that it can perform one of the following actions:
  bypass the packet, discard the packet, or protect the packet.

  @param[in]      This             Pointer to the EFI_IPSEC2_PROTOCOL instance.
  @param[in]      NicHandle        Instance of the network interface.
  @param[in]      IpVersion        IPV4 or IPV6.
  @param[in, out] IpHead           Pointer to the IP Header.
  @param[in, out] LastHead         The protocol of the next layer to be processed by IPsec.
  @param[in, out] OptionsBuffer    Pointer to the options buffer.
  @param[in, out] OptionsLength    Length of the options buffer.
  @param[in, out] FragmentTable    Pointer to a list of fragments.
  @param[in, out] FragmentCount    Number of fragments.
  @param[in]      TrafficDirection Traffic direction.
  @param[out]     RecycleSignal    Event for recycling of resources.

  @retval EFI_SUCCESS              The packet was bypassed and all buffers remain the same.
  @retval EFI_SUCCESS              The packet was protected.
  @retval EFI_ACCESS_DENIED        The packet was discarded.

**/
EFI_STATUS
EFIAPI
IpSecProcess (
  IN     EFI_IPSEC2_PROTOCOL              *This,
  IN     EFI_HANDLE                      NicHandle,
  IN     UINT8                           IpVersion,
  IN OUT VOID                            *IpHead,
  IN OUT UINT8                           *LastHead,
  IN OUT VOID                            **OptionsBuffer,
  IN OUT UINT32                          *OptionsLength,
  IN OUT EFI_IPSEC_FRAGMENT_DATA         **FragmentTable,
  IN OUT UINT32                          *FragmentCount,
  IN     EFI_IPSEC_TRAFFIC_DIR           TrafficDirection,
     OUT EFI_EVENT                       *RecycleSignal
  );

extern EFI_DPC_PROTOCOL    *mDpc;
extern EFI_IPSEC2_PROTOCOL  mIpSecInstance;

extern EFI_COMPONENT_NAME2_PROTOCOL gIpSecComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL  gIpSecComponentName;


#endif
