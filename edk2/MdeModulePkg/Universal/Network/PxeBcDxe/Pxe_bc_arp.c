/** @file

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  pxe_bc_arp.c

Abstract:


**/

#include "Bc.h"

//
// Definitions for ARP
// Per RFC 826
//
STATIC ARP_HEADER ArpHeader;

#pragma pack(1)
STATIC struct {
  UINT8       MediaHeader[14];
  ARP_HEADER  ArpHeader;
  UINT8       ArpData[64];
} ArpReplyPacket;
#pragma pack()

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @return none

**/
VOID
InitArpHeader (
  VOID
  )
{
  ArpHeader.HwType      = HTONS (ETHERNET_ADD_SPC);
  ArpHeader.ProtType    = HTONS (ETHER_TYPE_IP);
  ArpHeader.HwAddLen    = ENET_HWADDLEN;
  ArpHeader.ProtAddLen  = IPV4_PROTADDLEN;
  ArpHeader.OpCode      = HTONS (ARP_REQUEST);

  CopyMem (&ArpReplyPacket.ArpHeader, &ArpHeader, sizeof (ARP_HEADER));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
VOID
HandleArpReceive (
  IN PXE_BASECODE_DEVICE  *Private,
  IN ARP_PACKET           *ArpPacketPtr,
  IN VOID                 *MediaHeader
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  EFI_SIMPLE_NETWORK_MODE *SnpMode;
  EFI_MAC_ADDRESS         TmpMacAddr;
  UINTN                   Index;
  UINT8                   *SrcHwAddr;
  UINT8                   *SrcPrAddr;
  UINT8                   *DstHwAddr;
  UINT8                   *DstPrAddr;
  UINT8                   *TmpPtr;

  //
  //
  //
  PxeBcMode = Private->EfiBc.Mode;
  SnpMode   = Private->SimpleNetwork->Mode;

  //
  // For now only ethernet addresses are supported.
  // This will need to be updated when other media
  // layers are supported by PxeBc, Snp and UNDI.
  //
  if (ArpPacketPtr->ArpHeader.HwType != HTONS (ETHERNET_ADD_SPC)) {
    return ;
  }
  //
  // For now only IP protocol addresses are supported.
  // This will need to be updated when other protocol
  // types are supported by PxeBc, Snp and UNDI.
  //
  if (ArpPacketPtr->ArpHeader.ProtType != HTONS (ETHER_TYPE_IP)) {
    return ;
  }
  //
  // For now only SNP hardware address sizes are supported.
  //
  if (ArpPacketPtr->ArpHeader.HwAddLen != SnpMode->HwAddressSize) {
    return ;
  }
  //
  // For now only PxeBc protocol address sizes are supported.
  //
  if (ArpPacketPtr->ArpHeader.ProtAddLen != Private->IpLength) {
    return ;
  }
  //
  // Ignore out of range opcodes
  //
  switch (ArpPacketPtr->ArpHeader.OpCode) {
  case HTONS (ARP_REPLY):
  case HTONS (ARP_REQUEST):
    break;

  default:
    return ;
  }
  //
  // update entry in our ARP cache if we have it
  //
  SrcHwAddr = (UINT8 *) &ArpPacketPtr->SrcHardwareAddr;
  SrcPrAddr = SrcHwAddr + SnpMode->HwAddressSize;

  for (Index = 0; Index < PxeBcMode->ArpCacheEntries; ++Index) {
    if (CompareMem (
          &PxeBcMode->ArpCache[Index].IpAddr,
          SrcPrAddr,
          Private->IpLength
          )) {
      continue;
    }

    CopyMem (
      &PxeBcMode->ArpCache[Index].MacAddr,
      SrcHwAddr,
      SnpMode->HwAddressSize
      );

    break;
  }
  //
  // Done if ARP packet was not for us.
  //
  DstHwAddr = SrcPrAddr + Private->IpLength;
  DstPrAddr = DstHwAddr + SnpMode->HwAddressSize;

  if (CompareMem (DstPrAddr, &PxeBcMode->StationIp, Private->IpLength)) {
    return ;
    //
    // not for us
    //
  }
  //
  // for us - if we did not update entry, add it
  //
  if (Index == PxeBcMode->ArpCacheEntries) {
    //
    // if we have a full table, get rid of oldest
    //
    if (Index == PXE_ARP_CACHE_SIZE) {
      Index = Private->OldestArpEntry;

      if (++Private->OldestArpEntry == PXE_ARP_CACHE_SIZE) {
        Private->OldestArpEntry = 0;
      }
    } else {
      ++PxeBcMode->ArpCacheEntries;
    }

    CopyMem (
      &PxeBcMode->ArpCache[Index].MacAddr,
      SrcHwAddr,
      SnpMode->HwAddressSize
      );

    CopyMem (
      &PxeBcMode->ArpCache[Index].IpAddr,
      SrcPrAddr,
      Private->IpLength
      );
  }
  //
  // if this is not a request or we don't yet have an IP, finished
  //
  if (ArpPacketPtr->ArpHeader.OpCode != HTONS (ARP_REQUEST) || !Private->GoodStationIp) {
    return ;
  }
  //
  // Assemble ARP reply.
  //
  //
  // Create media header.  [ dest mac | src mac | prot ]
  //
  CopyMem (
    &ArpReplyPacket.MediaHeader[0],
    SrcHwAddr,
    SnpMode->HwAddressSize
    );

  CopyMem (
    &ArpReplyPacket.MediaHeader[SnpMode->HwAddressSize],
    &SnpMode->CurrentAddress,
    SnpMode->HwAddressSize
    );

  CopyMem (
    &ArpReplyPacket.MediaHeader[2 * SnpMode->HwAddressSize],
    &((UINT8 *) MediaHeader)[2 * SnpMode->HwAddressSize],
    sizeof (UINT16)
    );

  //
  // ARP reply header is almost filled in,
  // just insert the correct opcode.
  //
  ArpReplyPacket.ArpHeader.OpCode = HTONS (ARP_REPLY);

  //
  // Now fill in ARP data.  [ src mac | src prot | dest mac | dest prot ]
  //
  TmpPtr = ArpReplyPacket.ArpData;
  CopyMem (TmpPtr, &SnpMode->CurrentAddress, SnpMode->HwAddressSize);

  TmpPtr += SnpMode->HwAddressSize;
  CopyMem (TmpPtr, &PxeBcMode->StationIp, Private->IpLength);

  TmpPtr += Private->IpLength;
  CopyMem (TmpPtr, SrcHwAddr, SnpMode->HwAddressSize);

  TmpPtr += SnpMode->HwAddressSize;
  CopyMem (TmpPtr, SrcPrAddr, Private->IpLength);

  //
  // Now send out the ARP reply.
  //
  CopyMem (&TmpMacAddr, SrcHwAddr, sizeof (EFI_MAC_ADDRESS));

  SendPacket (
    Private,
    &ArpReplyPacket.MediaHeader,
    &ArpReplyPacket.ArpHeader,
    sizeof (ARP_HEADER) + 2 * (Private->IpLength + SnpMode->HwAddressSize),
    &TmpMacAddr,
    PXE_PROTOCOL_ETHERNET_ARP,
    EFI_PXE_BASE_CODE_FUNCTION_ARP
    );

  //
  // Give time (100 microseconds) for ARP reply to get onto wire.
  //
  gBS->Stall (1000);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @return TRUE := If IP address was found and MAC address was stored
  @return FALSE := If IP address was not found

**/
BOOLEAN
GetHwAddr (
  IN PXE_BASECODE_DEVICE  *Private,
  IN EFI_IP_ADDRESS       *ProtocolAddrPtr,
  OUT EFI_MAC_ADDRESS     *HardwareAddrPtr
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  UINTN                   HardwareAddrLength;
  UINTN                   Index;

  PxeBcMode           = Private->EfiBc.Mode;
  HardwareAddrLength  = Private->SimpleNetwork->Mode->HwAddressSize;

  for (Index = 0; Index < PxeBcMode->ArpCacheEntries; ++Index) {
    if (!CompareMem (
          ProtocolAddrPtr,
          &PxeBcMode->ArpCache[Index].IpAddr,
          Private->IpLength
          )) {
      CopyMem (
        HardwareAddrPtr,
        &PxeBcMode->ArpCache[Index].MacAddr,
        HardwareAddrLength
        );

      return TRUE;
    }
  }

  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @return EFI_SUCCESS := ARP request sent
  @return other := ARP request could not be sent

**/
STATIC
EFI_STATUS
SendRequest (
  IN PXE_BASECODE_DEVICE  *Private,
  IN EFI_IP_ADDRESS       *ProtocolAddrPtr,
  IN EFI_MAC_ADDRESS      *HardwareAddrPtr
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  EFI_SIMPLE_NETWORK_MODE *SnpMode;
  ARP_PACKET              *ArpPacket;
  EFI_STATUS              Status;
  UINTN                   HardwareAddrLength;
  UINT8                   *SrcProtocolAddrPtr;
  UINT8                   *DestHardwareAddrptr;
  UINT8                   *DestProtocolAddrPtr;

  //
  //
  //
  PxeBcMode           = Private->EfiBc.Mode;
  SnpMode             = Private->SimpleNetwork->Mode;
  HardwareAddrLength  = SnpMode->HwAddressSize;

  //
  // Allocate ARP buffer
  //
  if (Private->ArpBuffer == NULL) {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    SnpMode->MediaHeaderSize + sizeof (ARP_PACKET),
                    (VOID **) &Private->ArpBuffer
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  ArpPacket = (VOID *) (Private->ArpBuffer + SnpMode->MediaHeaderSize);

  //
  // for now, only handle one kind of hw and pr address
  //
  ArpPacket->ArpHeader            = ArpHeader;
  ArpPacket->ArpHeader.HwAddLen   = (UINT8) HardwareAddrLength;
  ArpPacket->ArpHeader.ProtAddLen = (UINT8) Private->IpLength;

  //
  // rest more generic
  //
  SrcProtocolAddrPtr  = (UINT8 *) (&ArpPacket->SrcHardwareAddr) + HardwareAddrLength;
  DestHardwareAddrptr = SrcProtocolAddrPtr + Private->IpLength;
  DestProtocolAddrPtr = DestHardwareAddrptr + HardwareAddrLength;

  CopyMem (DestProtocolAddrPtr, ProtocolAddrPtr, Private->IpLength);
  CopyMem (DestHardwareAddrptr, HardwareAddrPtr, HardwareAddrLength);
  CopyMem (SrcProtocolAddrPtr, &PxeBcMode->StationIp, Private->IpLength);
  CopyMem (
    &ArpPacket->SrcHardwareAddr,
    &SnpMode->CurrentAddress,
    HardwareAddrLength
    );

  return SendPacket (
          Private,
          Private->ArpBuffer,
          ArpPacket,
          sizeof (ARP_HEADER) + ((Private->IpLength + HardwareAddrLength) << 1),
          &SnpMode->BroadcastAddress,
          PXE_PROTOCOL_ETHERNET_ARP,
          EFI_PXE_BASE_CODE_FUNCTION_ARP
          );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// check for address - if not there, send ARP request, wait and check again
// not how it would be done in a full system
//
#define ARP_REQUEST_TIMEOUT_MS  500 // try for half a second

  ////////////////////////////////////////////////////////////
//
//  BC Arp Routine
//

/**


**/
EFI_STATUS
EFIAPI
BcArp (
  IN EFI_PXE_BASE_CODE_PROTOCOL * This,
  IN EFI_IP_ADDRESS             * ProtocolAddrPtr,
  OUT EFI_MAC_ADDRESS           * HardwareAddrPtr OPTIONAL
  )
{
  EFI_MAC_ADDRESS     Mac;
  EFI_STATUS          StatCode;
  PXE_BASECODE_DEVICE *Private;

  //
  // Lock the instance data and make sure started
  //
  StatCode = EFI_SUCCESS;

  if (This == NULL) {
    DEBUG ((DEBUG_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "PXE_BASECODE_DEVICE poiner == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  if (This->Mode == NULL || !This->Mode->Started) {
    DEBUG ((DEBUG_ERROR, "BC was not started."));
    EfiReleaseLock (&Private->Lock);
    return EFI_NOT_STARTED;
  }

  DEBUG ((DEBUG_INFO, "\nBcArp()"));

  //
  // Issue BC command
  //
  if (ProtocolAddrPtr == NULL) {
    DEBUG (
      (DEBUG_INFO,
      "\nBcArp()  Exit #1  %Xh (%r)",
      EFI_INVALID_PARAMETER,
      EFI_INVALID_PARAMETER)
      );

    EfiReleaseLock (&Private->Lock);
    return EFI_INVALID_PARAMETER;
  }

  if (HardwareAddrPtr == NULL) {
    HardwareAddrPtr = &Mac;
  }

  ZeroMem (HardwareAddrPtr, Private->SimpleNetwork->Mode->HwAddressSize);

  if (GetHwAddr (Private, ProtocolAddrPtr, HardwareAddrPtr)) {
    DEBUG (
      (DEBUG_INFO,
      "\nBcArp()  Exit #2  %Xh (%r)",
      EFI_SUCCESS,
      EFI_SUCCESS)
      );

    EfiReleaseLock (&Private->Lock);
    return EFI_SUCCESS;
  }

  StatCode = DoArp (Private, ProtocolAddrPtr, HardwareAddrPtr);

  DEBUG ((DEBUG_INFO, "\nBcArp()  Exit #3  %Xh (%r)", StatCode, StatCode));

  EfiReleaseLock (&Private->Lock);
  return StatCode;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @return EFI_SUCCESS := MAC address found
  @return other := MAC address could not be found

**/
EFI_STATUS
DoArp (
  IN PXE_BASECODE_DEVICE  *Private,
  IN EFI_IP_ADDRESS       *ProtocolAddrPtr,
  OUT EFI_MAC_ADDRESS     *HardwareAddrPtr
  )
{
  EFI_STATUS  StatCode;
  EFI_EVENT   TimeoutEvent;
  UINTN       HeaderSize;
  UINTN       BufferSize;
  UINT16      Protocol;

  DEBUG ((DEBUG_INFO, "\nDoArp()"));

  //
  //
  //
  StatCode = SendRequest (Private, ProtocolAddrPtr, HardwareAddrPtr);

  if (EFI_ERROR (StatCode)) {
    DEBUG ((DEBUG_INFO, "\nDoArp()  Exit #1  %Xh (%r)", StatCode, StatCode));
    return StatCode;
  }
  //
  //
  //
  StatCode = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &TimeoutEvent
                    );

  if (EFI_ERROR (StatCode)) {
    return StatCode;
  }

  StatCode = gBS->SetTimer (
                    TimeoutEvent,
                    TimerRelative,
                    ARP_REQUEST_TIMEOUT_MS * 10000
                    );

  if (EFI_ERROR (StatCode)) {
    gBS->CloseEvent (TimeoutEvent);
    return StatCode;
  }
  //
  //
  //
  for (;;) {
    StatCode = WaitForReceive (
                Private,
                EFI_PXE_BASE_CODE_FUNCTION_ARP,
                TimeoutEvent,
                &HeaderSize,
                &BufferSize,
                &Protocol
                );

    if (EFI_ERROR (StatCode)) {
      break;
    }

    if (Protocol != PXE_PROTOCOL_ETHERNET_ARP) {
      continue;
    }

    HandleArpReceive (
      Private,
      (ARP_PACKET *) (Private->ReceiveBufferPtr + HeaderSize),
      Private->ReceiveBufferPtr
      );

    if (GetHwAddr (Private, ProtocolAddrPtr, HardwareAddrPtr)) {
      break;
    }
  }

  DEBUG (
    (DEBUG_INFO,
    "\nDoArp()  Exit #2  %Xh, (%r)",
    StatCode,
    StatCode)
    );

  gBS->CloseEvent (TimeoutEvent);

  return StatCode;
}

/* eof - pxe_bc_arp.c */
