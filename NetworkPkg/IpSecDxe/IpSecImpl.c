/** @file
  The implementation of IPsec.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecImpl.h"
#include "IkeService.h"
#include "IpSecDebug.h"
#include "IpSecCryptIo.h"
#include "IpSecConfigImpl.h"

/**
  Check if the specified Address is the Valid Address Range.

  This function checks if the bytes after prefixed length are all Zero in this
  Address. This Address is supposed to point to a range address. That means it
  should gives the correct prefixed address and the bytes outside the prefixed are
  zero.

  @param[in]  IpVersion         The IP version.
  @param[in]  Address           Points to EFI_IP_ADDRESS to be checked.
  @param[in]  PrefixLength      The PrefixeLength of this address.

  @retval     TRUE      The address is a vaild address range.
  @retval     FALSE     The address is not a vaild address range.

**/
BOOLEAN
IpSecValidAddressRange (
  IN UINT8                     IpVersion,
  IN EFI_IP_ADDRESS            *Address,
  IN UINT8                     PrefixLength
  )
{
  UINT8           Div;
  UINT8           Mod;
  UINT8           Mask;
  UINT8           AddrLen;
  UINT8           *Addr;
  EFI_IP_ADDRESS  ZeroAddr;

  if (PrefixLength == 0) {
    return TRUE;
  }

  AddrLen = (UINT8) ((IpVersion == IP_VERSION_4) ? 32 : 128);

  if (AddrLen <= PrefixLength) {
    return FALSE;
  }

  Div   = (UINT8) (PrefixLength / 8);
  Mod   = (UINT8) (PrefixLength % 8);
  Addr  = (UINT8 *) Address;
  ZeroMem (&ZeroAddr, sizeof (EFI_IP_ADDRESS));

  //
  // Check whether the mod part of host scope is zero or not.
  //
  if (Mod > 0) {
    Mask = (UINT8) (0xFF << (8 - Mod));

    if ((Addr[Div] | Mask) != Mask) {
      return FALSE;
    }

    Div++;
  }
  //
  // Check whether the div part of host scope is zero or not.
  //
  if (CompareMem (
        &Addr[Div],
        &ZeroAddr,
        sizeof (EFI_IP_ADDRESS) - Div
        ) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Extrct the Address Range from a Address.

  This function keep the prefix address and zero other part address.

  @param[in]  Address           Point to a specified address.
  @param[in]  PrefixLength      The prefix length.
  @param[out] Range             Contain the return Address Range.

**/
VOID
IpSecExtractAddressRange (
  IN EFI_IP_ADDRESS            *Address,
  IN UINT8                     PrefixLength,
  OUT EFI_IP_ADDRESS           *Range
  )
{
  UINT8 Div;
  UINT8 Mod;
  UINT8 Mask;
  UINT8 *Addr;

  if (PrefixLength == 0) {
    return ;
  }

  Div   = (UINT8) (PrefixLength / 8);
  Mod   = (UINT8) (PrefixLength % 8);
  Addr  = (UINT8 *) Range;

  CopyMem (Range, Address, sizeof (EFI_IP_ADDRESS));

  //
  // Zero the mod part of host scope.
  //
  if (Mod > 0) {
    Mask      = (UINT8) (0xFF << (8 - Mod));
    Addr[Div] = (UINT8) (Addr[Div] & Mask);
    Div++;
  }
  //
  // Zero the div part of host scope.
  //
  ZeroMem (&Addr[Div], sizeof (EFI_IP_ADDRESS) - Div);

}

/**
  Checks if the IP Address in the address range of AddressInfos specified.

  @param[in]  IpVersion         The IP version.
  @param[in]  IpAddr            Point to EFI_IP_ADDRESS to be check.
  @param[in]  AddressInfo       A list of EFI_IP_ADDRESS_INFO that is used to check
                                the IP Address is matched.
  @param[in]  AddressCount      The total numbers of the AddressInfo.

  @retval   TRUE    If the Specified IP Address is in the range of the AddressInfos specified.
  @retval   FALSE   If the Specified IP Address is not in the range of the AddressInfos specified.

**/
BOOLEAN
IpSecMatchIpAddress (
  IN UINT8                     IpVersion,
  IN EFI_IP_ADDRESS            *IpAddr,
  IN EFI_IP_ADDRESS_INFO       *AddressInfo,
  IN UINT32                    AddressCount
  )
{
  EFI_IP_ADDRESS  Range;
  UINT32          Index;
  BOOLEAN         IsMatch;

  IsMatch = FALSE;

  for (Index = 0; Index < AddressCount; Index++) {
    //
    // Check whether the target address is in the address range
    // if it's a valid range of address.
    //
    if (IpSecValidAddressRange (
          IpVersion,
          &AddressInfo[Index].Address,
          AddressInfo[Index].PrefixLength
          )) {
      //
      // Get the range of the target address belongs to.
      //
      ZeroMem (&Range, sizeof (EFI_IP_ADDRESS));
      IpSecExtractAddressRange (
        IpAddr,
        AddressInfo[Index].PrefixLength,
        &Range
        );

      if (CompareMem (
            &Range,
            &AddressInfo[Index].Address,
            sizeof (EFI_IP_ADDRESS)
            ) == 0) {
        //
        // The target address is in the address range.
        //
        IsMatch = TRUE;
        break;
      }
    }

    if (CompareMem (
          IpAddr,
          &AddressInfo[Index].Address,
          sizeof (EFI_IP_ADDRESS)
          ) == 0) {
      //
      // The target address is exact same as the address.
      //
      IsMatch = TRUE;
      break;
    }
  }
  return IsMatch;
}

/**
  Check if the specified Protocol and Prot is supported by the specified SPD Entry.

  This function is the subfunction of IPsecLookUpSpdEntry() that is used to
  check if the sent/received IKE packet has the related SPD entry support.

  @param[in]  Protocol          The Protocol to be checked.
  @param[in]  IpPayload         Point to IP Payload to be check.
  @param[in]  SpdProtocol       The Protocol supported by SPD.
  @param[in]  SpdLocalPort      The Local Port in SPD.
  @param[in]  SpdRemotePort     The Remote Port in SPD.
  @param[in]  IsOutbound        Flag to indicate the is for IKE Packet sending or recieving.

  @retval     TRUE      The Protocol and Port are supported by the SPD Entry.
  @retval     FALSE     The Protocol and Port are not supported by the SPD Entry.

**/
BOOLEAN
IpSecMatchNextLayerProtocol (
  IN UINT8                     Protocol,
  IN UINT8                     *IpPayload,
  IN UINT16                    SpdProtocol,
  IN UINT16                    SpdLocalPort,
  IN UINT16                    SpdRemotePort,
  IN BOOLEAN                   IsOutbound
  )
{
  BOOLEAN IsMatch;

  if (SpdProtocol == EFI_IPSEC_ANY_PROTOCOL) {
    return TRUE;
  }

  IsMatch = FALSE;

  if (SpdProtocol == Protocol) {
    switch (Protocol) {
    case EFI_IP_PROTO_UDP:
    case EFI_IP_PROTO_TCP:
      //
      // For udp and tcp, (0, 0) means no need to check local and remote
      // port. The payload is passed from upper level, which means it should
      // be in network order.
      //
      IsMatch = (BOOLEAN) (SpdLocalPort == 0 && SpdRemotePort == 0);
      IsMatch = (BOOLEAN) (IsMatch ||
                           (IsOutbound &&
                           (BOOLEAN)(
                              NTOHS (((EFI_UDP_HEADER *) IpPayload)->SrcPort) == SpdLocalPort &&
                              NTOHS (((EFI_UDP_HEADER *) IpPayload)->DstPort) == SpdRemotePort
                              )
                            ));

      IsMatch = (BOOLEAN) (IsMatch ||
                           (!IsOutbound &&
                           (BOOLEAN)(
                              NTOHS (((EFI_UDP_HEADER *) IpPayload)->DstPort) == SpdLocalPort &&
                              NTOHS (((EFI_UDP_HEADER *) IpPayload)->SrcPort) == SpdRemotePort
                              )
                           ));
      break;

    case EFI_IP_PROTO_ICMP:
      //
      // For icmpv4, type code is replaced with local port and remote port,
      // and (0, 0) means no need to check.
      //
      IsMatch = (BOOLEAN) (SpdLocalPort == 0 && SpdRemotePort == 0);
      IsMatch = (BOOLEAN) (IsMatch ||
                           (BOOLEAN) (((IP4_ICMP_HEAD *) IpPayload)->Type == SpdLocalPort &&
                                      ((IP4_ICMP_HEAD *) IpPayload)->Code == SpdRemotePort
                                      )
                           );
      break;

    case IP6_ICMP:
      //
      // For icmpv6, type code is replaced with local port and remote port,
      // and (0, 0) means no need to check.
      //
      IsMatch = (BOOLEAN) (SpdLocalPort == 0 && SpdRemotePort == 0);

      IsMatch = (BOOLEAN) (IsMatch ||
                           (BOOLEAN) (((IP6_ICMP_HEAD *) IpPayload)->Type == SpdLocalPort &&
                                      ((IP6_ICMP_HEAD *) IpPayload)->Code == SpdRemotePort
                                      )
                          );
      break;

    default:
      IsMatch = TRUE;
      break;
    }
  }

  return IsMatch;
}

/**
  Find the SAD through a specified SPD's SAD list.

  @param[in]  SadList           SAD list related to a specified SPD entry.
  @param[in]  DestAddress       The destination address used to find the SAD entry.
  @param[in]  IpVersion         The IP version. Ip4 or Ip6.

  @return  The pointer to a certain SAD entry.

**/
IPSEC_SAD_ENTRY *
IpSecLookupSadBySpd (
  IN LIST_ENTRY                 *SadList,
  IN EFI_IP_ADDRESS             *DestAddress,
  IN UINT8                      IpVersion
  )
{
  LIST_ENTRY      *Entry;
  IPSEC_SAD_ENTRY *SadEntry;

  NET_LIST_FOR_EACH (Entry, SadList) {

    SadEntry = IPSEC_SAD_ENTRY_FROM_SPD (Entry);
    //
    // Find the right SAD entry which contains the appointed dest address.
    //
    if (IpSecMatchIpAddress (
          IpVersion,
          DestAddress,
          SadEntry->Data->SpdSelector->RemoteAddress,
          SadEntry->Data->SpdSelector->RemoteAddressCount
          )){
      return SadEntry;
    }
  }

  return NULL;
}

/**
  Find the SAD through whole SAD list.

  @param[in]  Spi               The SPI used to search the SAD entry.
  @param[in]  DestAddress       The destination used to search the SAD entry.
  @param[in]  IpVersion         The IP version. Ip4 or Ip6.

  @return  the pointer to a certain SAD entry.

**/
IPSEC_SAD_ENTRY *
IpSecLookupSadBySpi (
  IN UINT32                   Spi,
  IN EFI_IP_ADDRESS           *DestAddress,
  IN UINT8                    IpVersion
  )
{
  LIST_ENTRY      *Entry;
  LIST_ENTRY      *SadList;
  IPSEC_SAD_ENTRY *SadEntry;

  SadList = &mConfigData[IPsecConfigDataTypeSad];

  NET_LIST_FOR_EACH (Entry, SadList) {

    SadEntry = IPSEC_SAD_ENTRY_FROM_LIST (Entry);

    //
    // Find the right SAD entry which contain the appointed spi and dest addr.
    //
    if (SadEntry->Id->Spi == Spi) {
      if (SadEntry->Data->Mode == EfiIPsecTunnel) {
        if (CompareMem (
              &DestAddress,
              &SadEntry->Data->TunnelDestAddress,
              sizeof (EFI_IP_ADDRESS)
              )) {
          return SadEntry;
        }
      } else {
        if (SadEntry->Data->SpdSelector != NULL &&
            IpSecMatchIpAddress (
              IpVersion,
              DestAddress,
              SadEntry->Data->SpdSelector->RemoteAddress,
              SadEntry->Data->SpdSelector->RemoteAddressCount
              )
            ) {
          return SadEntry;
        }
      }
    }
  }
  return NULL;
}

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
  )
{
  IKE_UDP_SERVICE *UdpService;
  IPSEC_SAD_ENTRY *Entry;
  IPSEC_SAD_DATA  *Data;
  EFI_IP_ADDRESS  DestIp;
  UINT32          SeqNum32;

  *SadEntry   = NULL;
  UdpService  = IkeLookupUdp (Private, NicHandle, IpVersion);

  if (UdpService == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Parse the destination address from ip header.
  //
  ZeroMem (&DestIp, sizeof (EFI_IP_ADDRESS));
  if (IpVersion == IP_VERSION_4) {
    CopyMem (
      &DestIp,
      &((IP4_HEAD *) IpHead)->Dst,
      sizeof (IP4_ADDR)
      );
  } else {
    CopyMem (
      &DestIp,
      &((EFI_IP6_HEADER *) IpHead)->DestinationAddress,
      sizeof (EFI_IP_ADDRESS)
      );
  }

  //
  // Find the SAD entry in the spd.sas list according to the dest address.
  //
  Entry = IpSecLookupSadBySpd (&SpdEntry->Data->Sas, &DestIp, IpVersion);

  if (Entry == NULL) {
    if (OldLastHead != IP6_ICMP ||
        (OldLastHead == IP6_ICMP && *IpPayload == ICMP_V6_ECHO_REQUEST)
        ) {
      //
      // Start ike negotiation process except the request packet of ping.
      //
      if (SpdEntry->Data->ProcessingPolicy->Mode == EfiIPsecTunnel) {
        IkeNegotiate (
          UdpService,
          SpdEntry,
          &SpdEntry->Data->ProcessingPolicy->TunnelOption->RemoteTunnelAddress
          );
      } else {
        IkeNegotiate (
          UdpService,
          SpdEntry,
          &DestIp
        );
      }

    }

    return EFI_NOT_READY;
  }

  Data = Entry->Data;

  if (!Data->ManualSet) {
    if (Data->ESNEnabled) {
      //
      // Validate the 64bit sn number if 64bit sn enabled.
      //
      if ((UINT64) (Data->SequenceNumber + 1) == 0) {
        //
        // TODO: Re-negotiate SA
        //
        return EFI_DEVICE_ERROR;
      }
    } else {
      //
      // Validate the 32bit sn number if 64bit sn disabled.
      //
      SeqNum32 = (UINT32) Data->SequenceNumber;
      if ((UINT32) (SeqNum32 + 1) == 0) {
        //
        // TODO: Re-negotiate SA
        //
        return EFI_DEVICE_ERROR;
      }
    }
  }

  *SadEntry = Entry;

  return EFI_SUCCESS;
}

/**
  Find a PAD entry according to a remote IP address.

  @param[in]  IpVersion         The version of IP.
  @param[in]  IpAddr            Points to remote IP address.

  @return the pointer of related PAD entry.

**/
IPSEC_PAD_ENTRY *
IpSecLookupPadEntry (
  IN UINT8                   IpVersion,
  IN EFI_IP_ADDRESS          *IpAddr
  )
{
  LIST_ENTRY          *PadList;
  LIST_ENTRY          *Entry;
  EFI_IP_ADDRESS_INFO *IpAddrInfo;
  IPSEC_PAD_ENTRY     *PadEntry;

  PadList = &mConfigData[IPsecConfigDataTypePad];

  for (Entry = PadList->ForwardLink; Entry != PadList; Entry = Entry->ForwardLink) {

    PadEntry    = IPSEC_PAD_ENTRY_FROM_LIST (Entry);
    IpAddrInfo  = &PadEntry->Id->Id.IpAddress;
    //
    // Find the right pad entry which contain the appointed dest addr.
    //
    if (IpSecMatchIpAddress (IpVersion, IpAddr, IpAddrInfo, 1)) {
      return PadEntry;
    }
  }

  return NULL;
}

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
  )
{
  EFI_IPSEC_SPD_SELECTOR  *SpdSel;
  IP4_HEAD                *Ip4;
  EFI_IP6_HEADER          *Ip6;
  EFI_IP_ADDRESS          SrcAddr;
  EFI_IP_ADDRESS          DstAddr;
  BOOLEAN                 SpdMatch;

  ASSERT (SpdEntry != NULL);
  SpdSel  = SpdEntry->Selector;
  Ip4     = (IP4_HEAD *) IpHead;
  Ip6     = (EFI_IP6_HEADER *) IpHead;

  ZeroMem (&SrcAddr, sizeof (EFI_IP_ADDRESS));
  ZeroMem (&DstAddr, sizeof (EFI_IP_ADDRESS));

  //
  // Parse the source and destination address from ip header.
  //
  if (IpVersion == IP_VERSION_4) {
    CopyMem (&SrcAddr, &Ip4->Src, sizeof (IP4_ADDR));
    CopyMem (&DstAddr, &Ip4->Dst, sizeof (IP4_ADDR));
  } else {
    CopyMem (&SrcAddr, &Ip6->SourceAddress, sizeof (EFI_IPv6_ADDRESS));
    CopyMem (&DstAddr, &Ip6->DestinationAddress, sizeof (EFI_IPv6_ADDRESS));
  }
  //
  // Check the local and remote addresses for outbound traffic
  //
  SpdMatch = (BOOLEAN)(IsOutbound &&
                       IpSecMatchIpAddress (
                         IpVersion,
                         &SrcAddr,
                         SpdSel->LocalAddress,
                         SpdSel->LocalAddressCount
                         ) &&
                       IpSecMatchIpAddress (
                         IpVersion,
                         &DstAddr,
                         SpdSel->RemoteAddress,
                         SpdSel->RemoteAddressCount
                         )
                       );

  //
  // Check the local and remote addresses for inbound traffic
  //
  SpdMatch = (BOOLEAN) (SpdMatch ||
                        (!IsOutbound &&
                        IpSecMatchIpAddress (
                          IpVersion,
                          &DstAddr,
                          SpdSel->LocalAddress,
                          SpdSel->LocalAddressCount
                          ) &&
                        IpSecMatchIpAddress (
                          IpVersion,
                          &SrcAddr,
                          SpdSel->RemoteAddress,
                          SpdSel->RemoteAddressCount
                          )
                        ));

  //
  // Check the next layer protocol and local and remote ports.
  //
  SpdMatch = (BOOLEAN) (SpdMatch &&
                        IpSecMatchNextLayerProtocol (
                          Protocol,
                          IpPayload,
                          SpdSel->NextLayerProtocol,
                          SpdSel->LocalPort,
                          SpdSel->RemotePort,
                          IsOutbound
                          )
                        );

  if (SpdMatch) {
    //
    // Find the right SPD entry if match the 5 key elements.
    //
    *Action = SpdEntry->Data->Action;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  The call back function of NetbufFromExt.

  @param[in]  Arg            The argument passed from the caller.

**/
VOID
EFIAPI
IpSecOnRecyclePacket (
  IN VOID                            *Arg
  )
{
}

/**
  This is a Notification function. It is called when the related IP6_TXTOKEN_WRAP
  is released.

  @param[in]  Event              The related event.
  @param[in]  Context            The data passed by the caller.

**/
VOID
EFIAPI
IpSecRecycleCallback (
  IN EFI_EVENT                       Event,
  IN VOID                            *Context
  )
{
  IPSEC_RECYCLE_CONTEXT *RecycleContext;

  RecycleContext = (IPSEC_RECYCLE_CONTEXT *) Context;

  if (RecycleContext->FragmentTable != NULL) {
    FreePool (RecycleContext->FragmentTable);
  }

  if (RecycleContext->PayloadBuffer != NULL) {
    FreePool (RecycleContext->PayloadBuffer);
  }

  FreePool (RecycleContext);
  gBS->CloseEvent (Event);

}

/**
  Calculate the extension hader of IP. The return length only doesn't contain
  the fixed IP header length.

  @param[in]  IpHead             Points to an IP head to be calculated.
  @param[in]  LastHead           Points to the last header of the IP header.

  @return The length of the extension header.

**/
UINT16
IpSecGetPlainExtHeadSize (
  IN VOID                             *IpHead,
  IN UINT8                            *LastHead
  )
{
  UINT16  Size;

  Size = (UINT16) (LastHead - (UINT8 *) IpHead);

  if (Size > sizeof (EFI_IP6_HEADER)) {
    //
    // * (LastHead+1) point the last header's length but not include the first
    // 8 octers, so this formluation add 8 at the end.
    //
    Size = (UINT16) (Size - sizeof (EFI_IP6_HEADER) + *(LastHead + 1) + 8);
  } else {
    Size = 0;
  }

  return Size;
}

/**
  Verify if the Authentication payload is correct.

  @param[in]  EspBuffer          Points to the ESP wrapped buffer.
  @param[in]  EspSize            The size of the ESP wrapped buffer.
  @param[in]  SadEntry           The related SAD entry to store the authentication
                                 algorithm key.
  @param[in]  IcvSize            The length of ICV.

  @retval EFI_SUCCESS        The authentication data is correct.
  @retval EFI_ACCESS_DENIED  The authentication data is not correct.

**/
EFI_STATUS
IpSecEspAuthVerifyPayload (
  IN UINT8                           *EspBuffer,
  IN UINTN                           EspSize,
  IN IPSEC_SAD_ENTRY                 *SadEntry,
  IN UINTN                           IcvSize
  )
{
  EFI_STATUS           Status;
  UINTN                AuthSize;
  UINT8                IcvBuffer[12];
  HASH_DATA_FRAGMENT   HashFragment[1];

  //
  // Calculate the size of authentication payload.
  //
  AuthSize  = EspSize - IcvSize;

  //
  // Calculate the icv buffer and size of the payload.
  //
  HashFragment[0].Data     = EspBuffer;
  HashFragment[0].DataSize = AuthSize;

  Status = IpSecCryptoIoHmac (
             SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId,
             SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKey,
             SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKeyLength,
             HashFragment,
             1,
             IcvBuffer,
             IcvSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Compare the calculated icv and the appended original icv.
  //
  if (CompareMem (EspBuffer + AuthSize, IcvBuffer, IcvSize) == 0) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_ERROR, "Error auth verify payload\n"));
  return EFI_ACCESS_DENIED;
}

/**
  Search the related SAD entry by the input .

  @param[in]  IpHead       The pointer to IP header.
  @param[in]  IpVersion    The version of IP (IP4 or IP6).
  @param[in]  Spi          The SPI used to search the related SAD entry.


  @retval     NULL             Not find the related SAD entry.
  @retval     IPSEC_SAD_ENTRY  Return the related SAD entry.

**/
IPSEC_SAD_ENTRY *
IpSecFoundSadFromInboundPacket (
   UINT8   *IpHead,
   UINT8   IpVersion,
   UINT32  Spi
   )
{
  EFI_IP_ADDRESS   DestIp;

  //
  // Parse destination address from ip header.
  //
  ZeroMem (&DestIp, sizeof (EFI_IP_ADDRESS));
  if (IpVersion == IP_VERSION_4) {
    CopyMem (
      &DestIp,
      &((IP4_HEAD *) IpHead)->Dst,
      sizeof (IP4_ADDR)
      );
  } else {
    CopyMem (
      &DestIp,
      &((EFI_IP6_HEADER *) IpHead)->DestinationAddress,
      sizeof (EFI_IPv6_ADDRESS)
      );
  }

  //
  // Lookup SAD entry according to the spi and dest address.
  //
  return IpSecLookupSadBySpi (Spi, &DestIp, IpVersion);
}

/**
  Validate the IP6 extension header format for both the packets we received
  and that we will transmit.

  @param[in]  NextHeader    The next header field in IPv6 basic header.
  @param[in]  ExtHdrs       The first bye of the option.
  @param[in]  ExtHdrsLen    The length of the whole option.
  @param[out] LastHeader    The pointer of NextHeader of the last extension
                            header processed by IP6.
  @param[out] RealExtsLen   The length of extension headers processed by IP6 layer.
                            This is an optional parameter that may be NULL.

  @retval     TRUE          The option is properly formated.
  @retval     FALSE         The option is malformated.

**/
BOOLEAN
IpSecIsIp6ExtsValid (
  IN UINT8                  *NextHeader,
  IN UINT8                  *ExtHdrs,
  IN UINT32                 ExtHdrsLen,
  OUT UINT8                 **LastHeader,
  OUT UINT32                *RealExtsLen    OPTIONAL
  )
{
  UINT32                     Pointer;
  UINT8                      *Option;
  UINT8                      OptionLen;
  UINT8                      CountD;
  UINT8                      CountF;
  UINT8                      CountA;

  if (RealExtsLen != NULL) {
    *RealExtsLen = 0;
  }

  *LastHeader = NextHeader;

  if (ExtHdrs == NULL && ExtHdrsLen == 0) {
    return TRUE;
  }

  if ((ExtHdrs == NULL && ExtHdrsLen != 0) || (ExtHdrs != NULL && ExtHdrsLen == 0)) {
    return FALSE;
  }

  Pointer = 0;
  CountD  = 0;
  CountF  = 0;
  CountA  = 0;

  while (Pointer <= ExtHdrsLen) {

    switch (*NextHeader) {
    case IP6_HOP_BY_HOP:
      if (Pointer != 0) {
        return FALSE;
      }

    //
    // Fall through
    //
    case IP6_DESTINATION:
      if (*NextHeader == IP6_DESTINATION) {
        CountD++;
      }

      if (CountD > 2) {
        return FALSE;
      }

      NextHeader = ExtHdrs + Pointer;

      Pointer++;
      Option     = ExtHdrs + Pointer;
      OptionLen  = (UINT8) ((*Option + 1) * 8 - 2);
      Option++;
      Pointer++;

      Pointer = Pointer + OptionLen;
      break;

    case IP6_FRAGMENT:
      if (++CountF > 1) {
        return FALSE;
      }
      //
      // RFC2402, AH header should after fragment header.
      //
      if (CountA > 1) {
        return FALSE;
      }

      NextHeader = ExtHdrs + Pointer;
      Pointer    = Pointer + 8;
      break;

    case IP6_AH:
      if (++CountA > 1) {
        return FALSE;
      }

      Option     = ExtHdrs + Pointer;
      NextHeader = Option;
      Option++;
      //
      // RFC2402, Payload length is specified in 32-bit words, minus "2".
      //
      OptionLen  = (UINT8) ((*Option + 2) * 4);
      Pointer    = Pointer + OptionLen;
      break;

    default:
      *LastHeader = NextHeader;
       if (RealExtsLen != NULL) {
         *RealExtsLen = Pointer;
       }

       return TRUE;
    }
  }

  *LastHeader = NextHeader;

  if (RealExtsLen != NULL) {
    *RealExtsLen = Pointer;
  }

  return TRUE;
}

/**
  The actual entry to process the tunnel header and inner header for tunnel mode
  outbound traffic.

  This function is the subfunction of IpSecEspInboundPacket(). It change the destination
  Ip address to the station address and recalculate the uplayyer's checksum.


  @param[in, out] IpHead             Points to the IP header containing the ESP header
                                     to be trimed on input, and without ESP header
                                     on return.
  @param[in]      IpPayload          The decrypted Ip payload. It start from the inner
                                     header.
  @param[in]      IpVersion          The version of IP.
  @param[in]      SadData            Pointer of the relevant SAD.
  @param[in, out] LastHead           The Last Header in IP header on return.

**/
VOID
IpSecTunnelInboundPacket (
  IN OUT UINT8           *IpHead,
  IN     UINT8           *IpPayload,
  IN     UINT8           IpVersion,
  IN     IPSEC_SAD_DATA  *SadData,
  IN OUT UINT8           *LastHead
  )
{
  EFI_UDP_HEADER   *UdpHeader;
  TCP_HEAD         *TcpHeader;
  UINT16            *Checksum;
  UINT16           PseudoChecksum;
  UINT16           PacketChecksum;
  UINT32           OptionLen;
  IP6_ICMP_HEAD    *Icmp6Head;

  Checksum = NULL;

  if (IpVersion == IP_VERSION_4) {
    //
    // Zero OutIP header use this to indicate the input packet is under
    // IPsec Tunnel protected.
    //
    ZeroMem (
      (IP4_HEAD *)IpHead,
      sizeof (IP4_HEAD)
      );
    CopyMem (
      &((IP4_HEAD *)IpPayload)->Dst,
      &SadData->TunnelDestAddress.v4,
      sizeof (EFI_IPv4_ADDRESS)
      );

    //
    // Recalculate IpHeader Checksum
    //
    if (((IP4_HEAD *)(IpPayload))->Checksum != 0 ) {
      ((IP4_HEAD *)(IpPayload))->Checksum = 0;
      ((IP4_HEAD *)(IpPayload))->Checksum = (UINT16) (~NetblockChecksum (
                                                        (UINT8 *)IpPayload,
                                                        ((IP4_HEAD *)IpPayload)->HeadLen << 2
                                                        ));


    }

    //
    // Recalcualte PseudoChecksum
    //
    switch (((IP4_HEAD *)IpPayload)->Protocol) {
    case EFI_IP_PROTO_UDP :
      UdpHeader = (EFI_UDP_HEADER *)((UINT8 *)IpPayload + (((IP4_HEAD *)IpPayload)->HeadLen << 2));
      Checksum  = & UdpHeader->Checksum;
      *Checksum = 0;
      break;

    case EFI_IP_PROTO_TCP:
      TcpHeader = (TCP_HEAD *) ((UINT8 *)IpPayload + (((IP4_HEAD *)IpPayload)->HeadLen << 2));
      Checksum  = &TcpHeader->Checksum;
      *Checksum = 0;
      break;

    default:
      break;
      }
    PacketChecksum = NetblockChecksum (
                       (UINT8 *)IpPayload + (((IP4_HEAD *)IpPayload)->HeadLen << 2),
                       NTOHS (((IP4_HEAD *)IpPayload)->TotalLen) - (((IP4_HEAD *)IpPayload)->HeadLen << 2)
                       );
    PseudoChecksum = NetPseudoHeadChecksum (
                       ((IP4_HEAD *)IpPayload)->Src,
                       ((IP4_HEAD *)IpPayload)->Dst,
                       ((IP4_HEAD *)IpPayload)->Protocol,
                       0
                       );

      if (Checksum != NULL) {
        *Checksum = NetAddChecksum (PacketChecksum, PseudoChecksum);
        *Checksum = (UINT16) ~(NetAddChecksum (*Checksum, HTONS((UINT16)(NTOHS (((IP4_HEAD *)IpPayload)->TotalLen) - (((IP4_HEAD *)IpPayload)->HeadLen << 2)))));
      }
    }else {
      //
      //  Zero OutIP header use this to indicate the input packet is under
      //  IPsec Tunnel protected.
      //
      ZeroMem (
        IpHead,
        sizeof (EFI_IP6_HEADER)
        );
      CopyMem (
        &((EFI_IP6_HEADER*)IpPayload)->DestinationAddress,
        &SadData->TunnelDestAddress.v6,
        sizeof (EFI_IPv6_ADDRESS)
        );

      //
      // Get the Extension Header and Header length.
      //
      IpSecIsIp6ExtsValid (
        &((EFI_IP6_HEADER *)IpPayload)->NextHeader,
        IpPayload + sizeof (EFI_IP6_HEADER),
        ((EFI_IP6_HEADER *)IpPayload)->PayloadLength,
        &LastHead,
        &OptionLen
        );

      //
      // Recalcualte PseudoChecksum
      //
      switch (*LastHead) {
      case EFI_IP_PROTO_UDP:
        UdpHeader = (EFI_UDP_HEADER *)((UINT8 *)IpPayload + sizeof (EFI_IP6_HEADER) + OptionLen);
        Checksum  = &UdpHeader->Checksum;
        *Checksum = 0;
        break;

      case EFI_IP_PROTO_TCP:
        TcpHeader = (TCP_HEAD *)(IpPayload + sizeof (EFI_IP6_HEADER) + OptionLen);
        Checksum  = &TcpHeader->Checksum;
        *Checksum = 0;
        break;

      case IP6_ICMP:
        Icmp6Head  = (IP6_ICMP_HEAD *) (IpPayload + sizeof (EFI_IP6_HEADER) + OptionLen);
        Checksum   = &Icmp6Head->Checksum;
        *Checksum  = 0;
        break;
      }
      PacketChecksum = NetblockChecksum (
                         IpPayload + sizeof (EFI_IP6_HEADER) + OptionLen,
                         NTOHS(((EFI_IP6_HEADER *)IpPayload)->PayloadLength) - OptionLen
                         );
      PseudoChecksum = NetIp6PseudoHeadChecksum (
                         &((EFI_IP6_HEADER *)IpPayload)->SourceAddress,
                         &((EFI_IP6_HEADER *)IpPayload)->DestinationAddress,
                         *LastHead,
                         0
                         );

    if (Checksum != NULL) {
      *Checksum = NetAddChecksum (PacketChecksum, PseudoChecksum);
      *Checksum = (UINT16) ~(NetAddChecksum (
                               *Checksum,
                               HTONS ((UINT16)((NTOHS (((EFI_IP6_HEADER *)(IpPayload))->PayloadLength)) - OptionLen))
                               ));
    }
  }
}

/**
  The actual entry to create inner header for tunnel mode inbound traffic.

  This function is the subfunction of IpSecEspOutboundPacket(). It create
  the sending packet by encrypting its payload and inserting ESP header in the orginal
  IP header, then return the IpHeader and IPsec protected Fragmentable.

  @param[in, out] IpHead             Points to IP header containing the orginal IP header
                                     to be processed on input, and inserted ESP header
                                     on return.
  @param[in]      IpVersion          The version of IP.
  @param[in]      SadData            The related SAD data.
  @param[in, out] LastHead           The Last Header in IP header.
  @param[in]      OptionsBuffer      Pointer to the options buffer.
  @param[in]      OptionsLength      Length of the options buffer.
  @param[in, out] FragmentTable      Pointer to a list of fragments to be protected by
                                     IPsec on input, and with IPsec protected
                                     on return.
  @param[in]      FragmentCount      The number of fragments.

  @retval EFI_SUCCESS              The operation was successful.
  @retval EFI_OUT_OF_RESOURCES     The required system resources can't be allocated.

**/
UINT8 *
IpSecTunnelOutboundPacket (
  IN OUT UINT8                   *IpHead,
  IN     UINT8                   IpVersion,
  IN     IPSEC_SAD_DATA          *SadData,
  IN OUT UINT8                   *LastHead,
  IN     VOID                    **OptionsBuffer,
  IN     UINT32                  *OptionsLength,
  IN OUT EFI_IPSEC_FRAGMENT_DATA **FragmentTable,
  IN     UINT32                  *FragmentCount
  )
{
  UINT8         *InnerHead;
  NET_BUF       *Packet;
  UINT16        PacketChecksum;
  UINT16        *Checksum;
  UINT16        PseudoChecksum;
  IP6_ICMP_HEAD *IcmpHead;

  Checksum = NULL;
  if (OptionsLength == NULL) {
    return NULL;
  }

  if (IpVersion == IP_VERSION_4) {
    InnerHead = AllocateZeroPool (sizeof (IP4_HEAD) + *OptionsLength);
    ASSERT (InnerHead != NULL);
    CopyMem (
      InnerHead,
      IpHead,
      sizeof (IP4_HEAD)
      );
    CopyMem (
      InnerHead + sizeof (IP4_HEAD),
      *OptionsBuffer,
      *OptionsLength
      );
  } else {
    InnerHead = AllocateZeroPool (sizeof (EFI_IP6_HEADER) + *OptionsLength);
    ASSERT (InnerHead != NULL);
    CopyMem (
      InnerHead,
      IpHead,
      sizeof (EFI_IP6_HEADER)
      );
    CopyMem (
      InnerHead + sizeof (EFI_IP6_HEADER),
      *OptionsBuffer,
      *OptionsLength
      );
  }
  if (OptionsBuffer != NULL) {
    if (*OptionsLength != 0) {

      *OptionsBuffer = NULL;
      *OptionsLength = 0;
    }
  }

  //
  // 2. Reassamlbe Fragment into Packet
  //
  Packet = NetbufFromExt (
             (NET_FRAGMENT *)(*FragmentTable),
             *FragmentCount,
             0,
             0,
             IpSecOnRecyclePacket,
             NULL
             );
  ASSERT (Packet != NULL);
  //
  // 3. Check the Last Header, if it is TCP, UDP or ICMP recalcualate its pesudo
  //    CheckSum.
  //
  switch (*LastHead) {
  case EFI_IP_PROTO_UDP:
    Packet->Udp = (EFI_UDP_HEADER *) NetbufGetByte (Packet, 0, 0);
    ASSERT (Packet->Udp != NULL);
    Checksum = &Packet->Udp->Checksum;
    *Checksum = 0;
    break;

  case EFI_IP_PROTO_TCP:
    Packet->Tcp = (TCP_HEAD *) NetbufGetByte (Packet, 0, 0);
    ASSERT (Packet->Tcp != NULL);
    Checksum = &Packet->Tcp->Checksum;
    *Checksum = 0;
    break;

  case IP6_ICMP:
    IcmpHead = (IP6_ICMP_HEAD *) NetbufGetByte (Packet, 0, NULL);
    ASSERT (IcmpHead != NULL);
    Checksum = &IcmpHead->Checksum;
    *Checksum = 0;
    break;

  default:
    break;
  }

  PacketChecksum = NetbufChecksum (Packet);

  if (IpVersion == IP_VERSION_4) {
    //
    // Replace the source address of Inner Header.
    //
    CopyMem (
      &((IP4_HEAD *)InnerHead)->Src,
      &SadData->SpdSelector->LocalAddress[0].Address.v4,
      sizeof (EFI_IPv4_ADDRESS)
      );

    PacketChecksum = NetbufChecksum (Packet);
    PseudoChecksum = NetPseudoHeadChecksum (
                       ((IP4_HEAD *)InnerHead)->Src,
                       ((IP4_HEAD *)InnerHead)->Dst,
                       *LastHead,
                       0
                       );

   } else {
     //
     // Replace the source address of Inner Header.
     //
     CopyMem (
       &((EFI_IP6_HEADER *)InnerHead)->SourceAddress,
       &(SadData->SpdSelector->LocalAddress[0].Address.v6),
       sizeof (EFI_IPv6_ADDRESS)
       );
     PacketChecksum = NetbufChecksum (Packet);
     PseudoChecksum = NetIp6PseudoHeadChecksum (
                      &((EFI_IP6_HEADER *)InnerHead)->SourceAddress,
                      &((EFI_IP6_HEADER *)InnerHead)->DestinationAddress,
                      *LastHead,
                      0
                      );

   }
   if (Checksum != NULL) {
     *Checksum = NetAddChecksum (PacketChecksum, PseudoChecksum);
     *Checksum = (UINT16) ~(NetAddChecksum ((UINT16)*Checksum, HTONS ((UINT16) Packet->TotalSize)));
   }

  if (Packet != NULL) {
    NetbufFree (Packet);
  }
  return InnerHead;
}

/**
  The actual entry to relative function processes the inbound traffic of ESP header.

  This function is the subfunction of IpSecProtectInboundPacket(). It checks the
  received packet security property and trim the ESP header and then returns without
  an IPsec protected IP Header and FramgmentTable.

  @param[in]      IpVersion          The version of IP.
  @param[in, out] IpHead             Points to the IP header containing the ESP header
                                     to be trimed on input, and without ESP header
                                     on return.
  @param[out]     LastHead           The Last Header in IP header on return.
  @param[in, out] OptionsBuffer      Pointer to the options buffer.
  @param[in, out] OptionsLength      Length of the options buffer.
  @param[in, out] FragmentTable      Pointer to a list of fragments in the form of IPsec
                                     protected on input, and without IPsec protected
                                     on return.
  @param[in, out] FragmentCount      The number of fragments.
  @param[out]     SpdSelector        Pointer to contain the address of SPD selector on return.
  @param[out]     RecycleEvent       The event for recycling of resources.

  @retval EFI_SUCCESS              The operation was successful.
  @retval EFI_ACCESS_DENIED        One or more following conditions is TRUE:
                                   - ESP header was not found or mal-format.
                                   - The related SAD entry was not found.
                                   - The related SAD entry does not support the ESP protocol.
  @retval EFI_OUT_OF_RESOURCES     The required system resource can't be allocated.

**/
EFI_STATUS
IpSecEspInboundPacket (
  IN     UINT8                       IpVersion,
  IN OUT VOID                        *IpHead,
     OUT UINT8                       *LastHead,
  IN OUT VOID                        **OptionsBuffer,
  IN OUT UINT32                      *OptionsLength,
  IN OUT EFI_IPSEC_FRAGMENT_DATA     **FragmentTable,
  IN OUT UINT32                      *FragmentCount,
     OUT EFI_IPSEC_SPD_SELECTOR      **SpdSelector,
     OUT EFI_EVENT                   *RecycleEvent
  )
{
  EFI_STATUS            Status;
  NET_BUF               *Payload;
  UINTN                 EspSize;
  UINTN                 IvSize;
  UINTN                 BlockSize;
  UINTN                 MiscSize;
  UINTN                 PlainPayloadSize;
  UINTN                 PaddingSize;
  UINTN                 IcvSize;
  UINT8                 *ProcessBuffer;
  EFI_ESP_HEADER        *EspHeader;
  EFI_ESP_TAIL          *EspTail;
  EFI_IPSEC_SA_ID       *SaId;
  IPSEC_SAD_DATA        *SadData;
  IPSEC_SAD_ENTRY       *SadEntry;
  IPSEC_RECYCLE_CONTEXT *RecycleContext;
  UINT8                 NextHeader;
  UINT16                IpSecHeadSize;
  UINT8                 *InnerHead;

  Status            = EFI_SUCCESS;
  Payload           = NULL;
  ProcessBuffer     = NULL;
  RecycleContext    = NULL;
  *RecycleEvent     = NULL;
  PlainPayloadSize  = 0;
  NextHeader        = 0;

  //
  // Build netbuf from fragment table first.
  //
  Payload = NetbufFromExt (
              (NET_FRAGMENT *) *FragmentTable,
              *FragmentCount,
              0,
              sizeof (EFI_ESP_HEADER),
              IpSecOnRecyclePacket,
              NULL
              );
  if (Payload == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Get the esp size and esp header from netbuf.
  //
  EspSize   = Payload->TotalSize;
  EspHeader = (EFI_ESP_HEADER *) NetbufGetByte (Payload, 0, NULL);

  if (EspHeader == NULL) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Parse destination address from ip header and found the related SAD Entry.
  //
  SadEntry = IpSecFoundSadFromInboundPacket (
               IpHead,
               IpVersion,
               NTOHL (EspHeader->Spi)
               );

  if (SadEntry == NULL) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  SaId    = SadEntry->Id;
  SadData = SadEntry->Data;

  //
  // Only support esp protocol currently.
  //
  if (SaId->Proto != EfiIPsecESP) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  if (!SadData->ManualSet) {
    //
    // TODO: Check SA lifetime and sequence number
    //
  }

  //
  // Allocate buffer for decryption and authentication.
  //
  ProcessBuffer = AllocateZeroPool (EspSize);
  if (ProcessBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  NetbufCopy (Payload, 0, (UINT32) EspSize, ProcessBuffer);

  //
  // Get the IcvSize for authentication and BlockSize/IvSize for Decryption.
  //
  IcvSize   = IpSecGetIcvLength (SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId);
  IvSize    = IpSecGetEncryptIvLength (SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId);
  BlockSize = IpSecGetEncryptBlockSize (SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId);

  //
  // Make sure the ESP packet is not mal-formt.
  // 1. Check whether the Espsize is larger than ESP header + IvSize + EspTail + IcvSize.
  // 2. Check whether the left payload size is multiple of IvSize.
  //
  MiscSize = sizeof (EFI_ESP_HEADER) + IvSize + IcvSize;
  if (EspSize <= (MiscSize + sizeof (EFI_ESP_TAIL))) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }
  if ((EspSize - MiscSize) % BlockSize != 0) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Authenticate the ESP packet.
  //
  if (SadData->AlgoInfo.EspAlgoInfo.AuthKey != NULL) {
    Status = IpSecEspAuthVerifyPayload (
               ProcessBuffer,
               EspSize,
               SadEntry,
               IcvSize
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }
  //
  // Decrypt the payload by the SAD entry if it has decrypt key.
  //
  if (SadData->AlgoInfo.EspAlgoInfo.EncKey != NULL) {
    Status = IpSecCryptoIoDecrypt (
               SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId,
               SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKey,
               SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKeyLength << 3,
               ProcessBuffer + sizeof (EFI_ESP_HEADER),
               ProcessBuffer + sizeof (EFI_ESP_HEADER) + IvSize,
               EspSize - sizeof (EFI_ESP_HEADER) - IvSize - IcvSize,
               ProcessBuffer + sizeof (EFI_ESP_HEADER) + IvSize
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Parse EspTail and compute the plain payload size.
  //
  EspTail           = (EFI_ESP_TAIL *) (ProcessBuffer + EspSize - IcvSize - sizeof (EFI_ESP_TAIL));
  PaddingSize       = EspTail->PaddingLength;
  NextHeader        = EspTail->NextHeader;

  if (EspSize <= (MiscSize + sizeof (EFI_ESP_TAIL) + PaddingSize)) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }
  PlainPayloadSize  = EspSize - MiscSize - sizeof (EFI_ESP_TAIL) - PaddingSize;

  //
  // TODO: handle anti-replay window
  //
  //
  // Decryption and authentication with esp has been done, so it's time to
  // reload the new packet, create recycle event and fixup ip header.
  //
  RecycleContext = AllocateZeroPool (sizeof (IPSEC_RECYCLE_CONTEXT));
  if (RecycleContext == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpSecRecycleCallback,
                  RecycleContext,
                  RecycleEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // The caller will take responsible to handle the original fragment table
  //
  *FragmentTable = AllocateZeroPool (sizeof (EFI_IPSEC_FRAGMENT_DATA));
  if (*FragmentTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  RecycleContext->PayloadBuffer       = ProcessBuffer;
  RecycleContext->FragmentTable       = *FragmentTable;

  //
  // If Tunnel, recalculate upper-layyer PesudoCheckSum and trim the out
  //
  if (SadData->Mode == EfiIPsecTunnel) {
    InnerHead = ProcessBuffer + sizeof (EFI_ESP_HEADER) + IvSize;
    IpSecTunnelInboundPacket (
      IpHead,
      InnerHead,
      IpVersion,
      SadData,
      LastHead
      );

    if (IpVersion == IP_VERSION_4) {
      (*FragmentTable)[0].FragmentBuffer  = InnerHead ;
      (*FragmentTable)[0].FragmentLength  = (UINT32) PlainPayloadSize;

    }else {
      (*FragmentTable)[0].FragmentBuffer  = InnerHead;
      (*FragmentTable)[0].FragmentLength  = (UINT32) PlainPayloadSize;
    }
  } else {
    (*FragmentTable)[0].FragmentBuffer  = ProcessBuffer + sizeof (EFI_ESP_HEADER) + IvSize;
    (*FragmentTable)[0].FragmentLength  = (UINT32) PlainPayloadSize;
  }

  *FragmentCount                      = 1;

  //
  // Update the total length field in ip header since processed by esp.
  //
  if (!SadData->Mode == EfiIPsecTunnel) {
    if (IpVersion == IP_VERSION_4) {
      ((IP4_HEAD *) IpHead)->TotalLen = HTONS ((UINT16) ((((IP4_HEAD *) IpHead)->HeadLen << 2) + PlainPayloadSize));
    } else {
      IpSecHeadSize                              = IpSecGetPlainExtHeadSize (IpHead, LastHead);
      ((EFI_IP6_HEADER *) IpHead)->PayloadLength = HTONS ((UINT16)(IpSecHeadSize + PlainPayloadSize));
    }
    //
    // Update the next layer field in ip header since esp header inserted.
    //
    *LastHead = NextHeader;
  }


  //
  // Update the SPD association of the SAD entry.
  //
  *SpdSelector = SadData->SpdSelector;

ON_EXIT:
  if (Payload != NULL) {
    NetbufFree (Payload);
  }

  if (EFI_ERROR (Status)) {
    if (ProcessBuffer != NULL) {
      FreePool (ProcessBuffer);
    }

    if (RecycleContext != NULL) {
      FreePool (RecycleContext);
    }

    if (*RecycleEvent != NULL) {
      gBS->CloseEvent (*RecycleEvent);
    }
  }

  return Status;
}

/**
  The actual entry to the relative function processes the output traffic using the ESP protocol.

  This function is the subfunction of IpSecProtectOutboundPacket(). It protected
  the sending packet by encrypting its payload and inserting ESP header in the orginal
  IP header, then return the IpHeader and IPsec protected Fragmentable.

  @param[in]      IpVersion          The version of IP.
  @param[in, out] IpHead             Points to IP header containing the orginal IP header
                                     to be processed on input, and inserted ESP header
                                     on return.
  @param[in, out] LastHead           The Last Header in IP header.
  @param[in, out] OptionsBuffer      Pointer to the options buffer.
  @param[in, out] OptionsLength      Length of the options buffer.
  @param[in, out] FragmentTable      Pointer to a list of fragments to be protected by
                                     IPsec on input, and with IPsec protected
                                     on return.
  @param[in, out] FragmentCount      The number of fragments.
  @param[in]      SadEntry           The related SAD entry.
  @param[out]     RecycleEvent       The event for recycling of resources.

  @retval EFI_SUCCESS              The operation was successful.
  @retval EFI_OUT_OF_RESOURCES     The required system resources can't be allocated.

**/
EFI_STATUS
IpSecEspOutboundPacket (
  IN UINT8                           IpVersion,
  IN OUT VOID                        *IpHead,
  IN OUT UINT8                       *LastHead,
  IN OUT VOID                        **OptionsBuffer,
  IN OUT UINT32                      *OptionsLength,
  IN OUT EFI_IPSEC_FRAGMENT_DATA     **FragmentTable,
  IN OUT UINT32                      *FragmentCount,
  IN     IPSEC_SAD_ENTRY             *SadEntry,
     OUT EFI_EVENT                   *RecycleEvent
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  EFI_IPSEC_SA_ID       *SaId;
  IPSEC_SAD_DATA        *SadData;
  IPSEC_RECYCLE_CONTEXT *RecycleContext;
  UINT8                 *ProcessBuffer;
  UINTN                 BytesCopied;
  INTN                  EncryptBlockSize;// Size of encryption block, 4 bytes aligned and >= 4
  UINTN                 EspSize;         // Total size of esp wrapped ip payload
  UINTN                 IvSize;          // Size of IV, optional, might be 0
  UINTN                 PlainPayloadSize;// Original IP payload size
  UINTN                 PaddingSize;     // Size of padding
  UINTN                 EncryptSize;     // Size of data to be encrypted, start after IV and
                                         // stop before ICV
  UINTN                 IcvSize;         // Size of ICV, optional, might be 0
  UINT8                 *RestOfPayload;  // Start of Payload after IV
  UINT8                 *Padding;        // Start address of padding
  EFI_ESP_HEADER        *EspHeader;      // Start address of ESP frame
  EFI_ESP_TAIL          *EspTail;        // Address behind padding
  UINT8                 *InnerHead;
  HASH_DATA_FRAGMENT    HashFragment[1];

  Status          = EFI_ACCESS_DENIED;
  SaId            = SadEntry->Id;
  SadData         = SadEntry->Data;
  ProcessBuffer   = NULL;
  RecycleContext  = NULL;
  *RecycleEvent   = NULL;
  InnerHead       = NULL;

  if (!SadData->ManualSet &&
      SadData->AlgoInfo.EspAlgoInfo.EncKey == NULL &&
      SadData->AlgoInfo.EspAlgoInfo.AuthKey == NULL
      ) {
    //
    // Invalid manual SAD entry configuration.
    //
    goto ON_EXIT;
  }

  //
  // Create OutHeader according to Inner Header
  //
  if (SadData->Mode == EfiIPsecTunnel) {
    InnerHead = IpSecTunnelOutboundPacket (
                  IpHead,
                  IpVersion,
                  SadData,
                  LastHead,
                  OptionsBuffer,
                  OptionsLength,
                  FragmentTable,
                  FragmentCount
                  );

    if (InnerHead == NULL) {
      return EFI_INVALID_PARAMETER;
    }

  }

  //
  // Calculate enctrypt block size, need iv by default and 4 bytes alignment.
  //
  EncryptBlockSize  = 4;

  if (SadData->AlgoInfo.EspAlgoInfo.EncKey != NULL) {
    EncryptBlockSize  = IpSecGetEncryptBlockSize (SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId);

    if (EncryptBlockSize < 0 || (EncryptBlockSize != 1 && EncryptBlockSize % 4 != 0)) {
      goto ON_EXIT;
    }
  }

  //
  // Calculate the plain payload size accroding to the fragment table.
  //
  PlainPayloadSize = 0;
  for (Index = 0; Index < *FragmentCount; Index++) {
    PlainPayloadSize += (*FragmentTable)[Index].FragmentLength;
  }

  //
  // Add IPHeader size for Tunnel Mode
  //
  if (SadData->Mode == EfiIPsecTunnel) {
    if (IpVersion == IP_VERSION_4) {
      PlainPayloadSize += sizeof (IP4_HEAD);
    } else {
      PlainPayloadSize += sizeof (EFI_IP6_HEADER);
    }
    //
    // OPtions should be encryption into it
    //
    PlainPayloadSize += *OptionsLength;
  }


  //
  // Calculate icv size, optional by default and 4 bytes alignment.
  //
  IcvSize = 0;
  if (SadData->AlgoInfo.EspAlgoInfo.AuthKey != NULL) {
    IcvSize = IpSecGetIcvLength (SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId);
    if (IcvSize % 4 != 0) {
      goto ON_EXIT;
    }
  }

  //
  // Calcuate the total size of esp wrapped ip payload.
  //
  IvSize        = IpSecGetEncryptIvLength (SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId);
  EncryptSize   = (PlainPayloadSize + sizeof (EFI_ESP_TAIL) + EncryptBlockSize - 1) / EncryptBlockSize * EncryptBlockSize;
  PaddingSize   = EncryptSize - PlainPayloadSize - sizeof (EFI_ESP_TAIL);
  EspSize       = sizeof (EFI_ESP_HEADER) + IvSize + EncryptSize + IcvSize;

  ProcessBuffer = AllocateZeroPool (EspSize);
  if (ProcessBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Calculate esp header and esp tail including header, payload and padding.
  //
  EspHeader     = (EFI_ESP_HEADER *) ProcessBuffer;
  RestOfPayload = (UINT8 *) (EspHeader + 1) + IvSize;
  Padding       = RestOfPayload + PlainPayloadSize;
  EspTail       = (EFI_ESP_TAIL *) (Padding + PaddingSize);

  //
  // Fill the sn and spi fields in esp header.
  //
  EspHeader->SequenceNumber = HTONL ((UINT32) SadData->SequenceNumber + 1);
  //EspHeader->SequenceNumber = HTONL ((UINT32) SadData->SequenceNumber);
  EspHeader->Spi            = HTONL (SaId->Spi);

  //
  // Copy the rest of payload (after iv) from the original fragment buffer.
  //
  BytesCopied = 0;

  //
  // For Tunnel Mode
  //
  if (SadData->Mode == EfiIPsecTunnel) {
    if (IpVersion == IP_VERSION_4) {
      //
      // HeadLen, Total Length
      //
      ((IP4_HEAD *)InnerHead)->HeadLen  = (UINT8) ((sizeof (IP4_HEAD) + *OptionsLength) >> 2);
      ((IP4_HEAD *)InnerHead)->TotalLen = HTONS ((UINT16) PlainPayloadSize);
      ((IP4_HEAD *)InnerHead)->Checksum = 0;
      ((IP4_HEAD *)InnerHead)->Checksum = (UINT16) (~NetblockChecksum (
                                                  (UINT8 *)InnerHead,
                                                  sizeof(IP4_HEAD)
                                                  ));
      CopyMem (
        RestOfPayload + BytesCopied,
        InnerHead,
        sizeof (IP4_HEAD) + *OptionsLength
        );
      BytesCopied += sizeof (IP4_HEAD) + *OptionsLength;

    } else {
    ((EFI_IP6_HEADER *)InnerHead)->PayloadLength = HTONS ((UINT16) (PlainPayloadSize - sizeof (EFI_IP6_HEADER)));
      CopyMem (
        RestOfPayload + BytesCopied,
        InnerHead,
        sizeof (EFI_IP6_HEADER) + *OptionsLength
        );
      BytesCopied += sizeof (EFI_IP6_HEADER) + *OptionsLength;
    }
  }

  for (Index = 0; Index < *FragmentCount; Index++) {
    CopyMem (
      (RestOfPayload + BytesCopied),
      (*FragmentTable)[Index].FragmentBuffer,
      (*FragmentTable)[Index].FragmentLength
      );
    BytesCopied += (*FragmentTable)[Index].FragmentLength;
  }
  //
  // Fill the padding buffer by natural number sequence.
  //
  for (Index = 0; Index < PaddingSize; Index++) {
    Padding[Index] = (UINT8) (Index + 1);
  }
  //
  // Fill the padding length and next header fields in esp tail.
  //
  EspTail->PaddingLength  = (UINT8) PaddingSize;
  EspTail->NextHeader     = *LastHead;

  //
  // Fill the next header for Tunnel mode.
  //
  if (SadData->Mode == EfiIPsecTunnel) {
    if (IpVersion == IP_VERSION_4) {
      EspTail->NextHeader = 4;
    } else {
      EspTail->NextHeader = 41;
    }
  }

  //
  // Generate iv at random by crypt library.
  //
  Status = IpSecGenerateIv (
             (UINT8 *) (EspHeader + 1),
             IvSize
             );


  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Encryption the payload (after iv) by the SAD entry if has encrypt key.
  //
  if (SadData->AlgoInfo.EspAlgoInfo.EncKey != NULL) {
    Status = IpSecCryptoIoEncrypt (
               SadEntry->Data->AlgoInfo.EspAlgoInfo.EncAlgoId,
               SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKey,
               SadEntry->Data->AlgoInfo.EspAlgoInfo.EncKeyLength << 3,
               (UINT8 *)(EspHeader + 1),
               RestOfPayload,
               EncryptSize,
               RestOfPayload
               );

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Authenticate the esp wrapped buffer by the SAD entry if it has auth key.
  //
  if (SadData->AlgoInfo.EspAlgoInfo.AuthKey != NULL) {

    HashFragment[0].Data     = ProcessBuffer;
    HashFragment[0].DataSize = EspSize - IcvSize;
    Status = IpSecCryptoIoHmac (
               SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthAlgoId,
               SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKey,
               SadEntry->Data->AlgoInfo.EspAlgoInfo.AuthKeyLength,
               HashFragment,
               1,
               ProcessBuffer + EspSize - IcvSize,
               IcvSize
               );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Encryption and authentication with esp has been done, so it's time to
  // reload the new packet, create recycle event and fixup ip header.
  //
  RecycleContext = AllocateZeroPool (sizeof (IPSEC_RECYCLE_CONTEXT));
  if (RecycleContext == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpSecRecycleCallback,
                  RecycleContext,
                  RecycleEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // Caller take responsible to handle the original fragment table.
  //
  *FragmentTable = AllocateZeroPool (sizeof (EFI_IPSEC_FRAGMENT_DATA));
  if (*FragmentTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  RecycleContext->FragmentTable       = *FragmentTable;
  RecycleContext->PayloadBuffer       = ProcessBuffer;
  (*FragmentTable)[0].FragmentBuffer  = ProcessBuffer;
  (*FragmentTable)[0].FragmentLength  = (UINT32) EspSize;
  *FragmentCount                      = 1;

  //
  // Update the total length field in ip header since processed by esp.
  //
  if (IpVersion == IP_VERSION_4) {
    ((IP4_HEAD *) IpHead)->TotalLen = HTONS ((UINT16) ((((IP4_HEAD *) IpHead)->HeadLen << 2) + EspSize));
  } else {
    ((EFI_IP6_HEADER *) IpHead)->PayloadLength = (UINT16) (IpSecGetPlainExtHeadSize (IpHead, LastHead) + EspSize);
  }

  //
  // If tunnel mode, it should change the outer Ip header with tunnel source address
  // and destination tunnel address.
  //
  if (SadData->Mode == EfiIPsecTunnel) {
    if (IpVersion == IP_VERSION_4) {
      CopyMem (
        &((IP4_HEAD *) IpHead)->Src,
        &SadData->TunnelSourceAddress.v4,
        sizeof (EFI_IPv4_ADDRESS)
        );
      CopyMem (
        &((IP4_HEAD *) IpHead)->Dst,
        &SadData->TunnelDestAddress.v4,
        sizeof (EFI_IPv4_ADDRESS)
        );
    } else {
      CopyMem (
        &((EFI_IP6_HEADER *) IpHead)->SourceAddress,
        &SadData->TunnelSourceAddress.v6,
        sizeof (EFI_IPv6_ADDRESS)
        );
      CopyMem (
        &((EFI_IP6_HEADER *) IpHead)->DestinationAddress,
        &SadData->TunnelDestAddress.v6,
        sizeof (EFI_IPv6_ADDRESS)
        );
    }
  }

  //
  // Update the next layer field in ip header since esp header inserted.
  //
  *LastHead = IPSEC_ESP_PROTOCOL;

  //
  // Increase the sn number in SAD entry according to rfc4303.
  //
  SadData->SequenceNumber++;

ON_EXIT:
  if (EFI_ERROR (Status)) {
    if (ProcessBuffer != NULL) {
      FreePool (ProcessBuffer);
    }

    if (RecycleContext != NULL) {
      FreePool (RecycleContext);
    }

    if (*RecycleEvent != NULL) {
      gBS->CloseEvent (*RecycleEvent);
    }
  }

  return Status;
}

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
  )
{
  if (*LastHead == IPSEC_ESP_PROTOCOL) {
    //
    // Process the esp ipsec header of the inbound traffic.
    //
    return IpSecEspInboundPacket (
             IpVersion,
             IpHead,
             LastHead,
             OptionsBuffer,
             OptionsLength,
             FragmentTable,
             FragmentCount,
             SpdEntry,
             RecycleEvent
             );
  }
  //
  // The other protocols are not supported.
  //
  return EFI_UNSUPPORTED;
}

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
  )
{
  if (SadEntry->Id->Proto == EfiIPsecESP) {
    //
    // Process the esp ipsec header of the outbound traffic.
    //
    return IpSecEspOutboundPacket (
             IpVersion,
             IpHead,
             LastHead,
             OptionsBuffer,
             OptionsLength,
             FragmentTable,
             FragmentCount,
             SadEntry,
             RecycleEvent
             );
  }
  //
  // The other protocols are not supported.
  //
  return EFI_UNSUPPORTED;
}
