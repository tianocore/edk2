/** @file

Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_PXEBC_IMPL_H__
#define __EFI_PXEBC_IMPL_H__


typedef struct _PXEBC_PRIVATE_DATA  PXEBC_PRIVATE_DATA;

#include <Uefi.h>

#include <Guid/SmBios.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/PxeBaseCode.h>
#include <Protocol/Mtftp4.h>
#include <Protocol/Udp4.h>
#include <Protocol/LoadFile.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/PxeBaseCodeCallBack.h>
#include <Protocol/Arp.h>
#include <Protocol/Ip4.h>
#include <Protocol/Ip4Config2.h>

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/NetLib.h>
#include <Library/DpcLib.h>
#include <Library/PcdLib.h>

#include "PxeBcDriver.h"
#include "PxeBcDhcp.h"
#include "PxeBcMtftp.h"
#include "PxeBcSupport.h"

#define PXEBC_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('P', 'X', 'E', 'P')
#define PXEBC_MTFTP_TIMEOUT                4
#define PXEBC_MTFTP_RETRIES                6
#define PXEBC_DEFAULT_UDP_OVERHEAD_SIZE    8
#define PXEBC_DEFAULT_TFTP_OVERHEAD_SIZE   4
#define PXEBC_DEFAULT_PACKET_SIZE          1480
#define PXEBC_DEFAULT_LIFETIME             50000  // 50ms, unit is microsecond

struct _PXEBC_PRIVATE_DATA {
  UINT32                                    Signature;
  EFI_HANDLE                                Controller;
  EFI_HANDLE                                Image;
  EFI_HANDLE                                ArpChild;
  EFI_HANDLE                                Dhcp4Child;
  EFI_HANDLE                                Ip4Child;
  EFI_HANDLE                                Mtftp4Child;
  EFI_HANDLE                                Udp4ReadChild;
  EFI_HANDLE                                Udp4WriteChild;

  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *Nii;

  EFI_PXE_BASE_CODE_PROTOCOL                PxeBc;
  EFI_LOAD_FILE_PROTOCOL                    LoadFile;
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL       LoadFileCallback;
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL       *PxeBcCallback;
  EFI_ARP_PROTOCOL                          *Arp;
  EFI_DHCP4_PROTOCOL                        *Dhcp4;
  EFI_IP4_PROTOCOL                          *Ip4;
  EFI_IP4_CONFIG2_PROTOCOL                  *Ip4Config2;
  EFI_IP4_CONFIG_DATA                       Ip4ConfigData;
  EFI_MTFTP4_PROTOCOL                       *Mtftp4;
  EFI_UDP4_PROTOCOL                         *Udp4Read;
  EFI_UDP4_PROTOCOL                         *Udp4Write;
  UINT16                                    CurrentUdpSrcPort;
  EFI_UDP4_CONFIG_DATA                      Udp4CfgData;


  EFI_PXE_BASE_CODE_MODE                    Mode;
  EFI_PXE_BASE_CODE_FUNCTION                Function;

  CHAR8                                     *BootFileName;

  EFI_IP_ADDRESS                            StationIp;
  EFI_IP_ADDRESS                            SubnetMask;
  EFI_IP_ADDRESS                            GatewayIp;
  EFI_IP_ADDRESS                            ServerIp;
  BOOLEAN                                   AddressIsOk;
  UINT32                                    Ip4MaxPacketSize;
  UINTN                                     BlockSize;
  UINTN                                     FileSize;

  UINT8                                     OptionBuffer[PXEBC_DHCP4_MAX_OPTION_SIZE];
  EFI_DHCP4_PACKET                          SeedPacket;
  EFI_MAC_ADDRESS                           Mac;
  UINT8                                     MacLen;

  BOOLEAN                                   SortOffers;
  BOOLEAN                                   GotProxyOffer;
  UINT32                                    NumOffers;
  UINT32                                    SelectedOffer;
  UINT32                                    ProxyOfferType;

  //
  // Cached packets as complements of pxe mode data
  //
  PXEBC_CACHED_DHCP4_PACKET                 ProxyOffer;
  PXEBC_CACHED_DHCP4_PACKET                 Dhcp4Ack;
  PXEBC_CACHED_DHCP4_PACKET                 PxeReply;
  PXEBC_CACHED_DHCP4_PACKET                 Dhcp4Offers[PXEBC_MAX_OFFER_NUM];

  //
  // Arrays for different types of offers:
  //   ServerCount records the count of the servers we got the offers,
  //   OfferIndex records the index of the offer sent by the server indexed by ServerCount.
  //
  UINT32                                    ServerCount[DHCP4_PACKET_TYPE_MAX];
  UINT32                                    OfferIndex[DHCP4_PACKET_TYPE_MAX][PXEBC_MAX_OFFER_NUM];
  UINT32                                    BootpIndex;
  UINT32                                    ProxyIndex[DHCP4_PACKET_TYPE_MAX];
  UINT32                                    BinlIndex[PXEBC_MAX_OFFER_NUM];

  EFI_EVENT                                 GetArpCacheEvent;
  //
  // token and event used to get ICMP error data from IP
  //
  EFI_IP4_COMPLETION_TOKEN                  IcmpErrorRcvToken;
};

#define PXEBC_PRIVATE_DATA_FROM_PXEBC(a)          CR (a, PXEBC_PRIVATE_DATA, PxeBc, PXEBC_PRIVATE_DATA_SIGNATURE)

#define PXEBC_PRIVATE_DATA_FROM_LOADFILE(a)       CR (a, PXEBC_PRIVATE_DATA, LoadFile, PXEBC_PRIVATE_DATA_SIGNATURE)

#define PXEBC_PRIVATE_DATA_FROM_PXEBCCALLBACK(a)  CR (a, PXEBC_PRIVATE_DATA, PxeBcCallback, PXEBC_PRIVATE_DATA_SIGNATURE)

extern EFI_PXE_BASE_CODE_PROTOCOL mPxeBcProtocolTemplate;
extern EFI_LOAD_FILE_PROTOCOL     mLoadFileProtocolTemplate;

/**
  Causes the driver to load a specified file.

  @param  This                  Protocol instance pointer.
  @param  FilePath              The device specific path of the file to load.
  @param  BootPolicy            If TRUE, indicates that the request originates from the
                                boot manager is attempting to load FilePath as a boot
                                selection. If FALSE, then FilePath must match as exact file
                                to be loaded.
  @param  BufferSize            On input the size of Buffer in bytes. On output with a return
                                code of EFI_SUCCESS, the amount of data transferred to
                                Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                                the size of Buffer required to retrieve the requested file.
  @param  Buffer                The memory buffer to transfer the file to. IF Buffer is NULL,
                                then no the size of the requested file is returned in
                                BufferSize.

  @retval EFI_SUCCESS                 The file was loaded.
  @retval EFI_UNSUPPORTED             The device does not support the provided BootPolicy
  @retval EFI_INVALID_PARAMETER       FilePath is not a valid device path, or
                                      BufferSize is NULL.
  @retval EFI_NO_MEDIA                No medium was present to load the file.
  @retval EFI_DEVICE_ERROR            The file was not loaded due to a device error.
  @retval EFI_NO_RESPONSE             The remote system did not respond.
  @retval EFI_NOT_FOUND               The file was not found.
  @retval EFI_ABORTED                 The file load process was manually cancelled.

**/
EFI_STATUS
EFIAPI
EfiPxeLoadFile (
  IN EFI_LOAD_FILE_PROTOCOL           * This,
  IN EFI_DEVICE_PATH_PROTOCOL         * FilePath,
  IN BOOLEAN                          BootPolicy,
  IN OUT UINTN                        *BufferSize,
  IN VOID                             *Buffer OPTIONAL
  );

#endif
