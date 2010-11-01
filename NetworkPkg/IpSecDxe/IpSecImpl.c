/** @file
  The implementation of IPsec Protocol

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecConfigImpl.h"

EFI_IPSEC2_PROTOCOL  mIpSecInstance = { IpSecProcess, NULL, TRUE };

extern LIST_ENTRY   mConfigData[IPsecConfigDataTypeMaximum];

/**
  Check if the specified Address is the Valid Address Range.

  This function checks if the bytes after prefixed length are all Zero in this
  Address. This Address is supposed to point to a range address,  meaning it only
  gives the correct prefixed address.

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

  @return  The pointer to a certain SAD entry.

**/
IPSEC_SAD_ENTRY *
IpSecLookupSadBySpd (
  IN LIST_ENTRY                 *SadList,
  IN EFI_IP_ADDRESS             *DestAddress
  )
{
  LIST_ENTRY      *Entry;
  IPSEC_SAD_ENTRY *SadEntry;

  for (Entry = SadList->ForwardLink; Entry != SadList; Entry = Entry->ForwardLink) {

    SadEntry = IPSEC_SAD_ENTRY_FROM_SPD (Entry);
    //
    // Find the right sad entry which contains the appointed dest address.
    //
    if (CompareMem (
          &SadEntry->Id->DestAddress,
          DestAddress,
          sizeof (EFI_IP_ADDRESS)
          ) == 0) {
      return SadEntry;
    }
  }

  return NULL;
}

/**
  Find the SAD through whole SAD list.

  @param[in]  Spi               The SPI used to search the SAD entry.
  @param[in]  DestAddress       The destination used to search the SAD entry.

  @return  the pointer to a certain SAD entry.

**/
IPSEC_SAD_ENTRY *
IpSecLookupSadBySpi (
  IN UINT32                   Spi,
  IN EFI_IP_ADDRESS           *DestAddress
  )
{
  LIST_ENTRY      *Entry;
  LIST_ENTRY      *SadList;
  IPSEC_SAD_ENTRY *SadEntry;

  SadList = &mConfigData[IPsecConfigDataTypeSad];

  for (Entry = SadList->ForwardLink; Entry != SadList; Entry = Entry->ForwardLink) {

    SadEntry = IPSEC_SAD_ENTRY_FROM_LIST (Entry);
    //
    // Find the right sad entry which contain the appointed spi and dest addr.
    //
    if (SadEntry->Id->Spi == Spi && CompareMem (
                                      &SadEntry->Id->DestAddress,
                                      DestAddress,
                                      sizeof (EFI_IP_ADDRESS)
                                      ) == 0) {

      return SadEntry;
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
  @retval EFI_DEVICE_ERROR  GC_TODO: Add description for return value.
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
  IPSEC_SAD_ENTRY *Entry;
  IPSEC_SAD_DATA  *Data;
  EFI_IP_ADDRESS  DestIp;
  UINT32          SeqNum32;

  *SadEntry   = NULL;
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
  // Find the sad entry in the spd.sas list according to the dest address.
  //
  Entry = IpSecLookupSadBySpd (&SpdEntry->Data->Sas, &DestIp);

  if (Entry == NULL) {

    if (OldLastHead != IP6_ICMP ||
        (OldLastHead == IP6_ICMP && *IpPayload == ICMP_V6_ECHO_REQUEST)
        ) {
      //
      // TODO: Start ike negotiation process except the request packet of ping.
      //
      //IkeNegotiate (UdpService, SpdEntry, &DestIp);
    }

    return EFI_NOT_READY;
  }

  Data = Entry->Data;

  if (!Data->ManualSet) {
    if (Data->ESNEnabled) {
      //
      // Validate the 64bit sn number if 64bit sn enabled.
      //
      if (Data->SequenceNumber + 1 < Data->SequenceNumber) {
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
      if (SeqNum32 + 1 < SeqNum32) {
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

  @retval EFI_IPSEC_ACTION  The support action of SPD entry.
  @retval -1                If the input packet header doesn't match the SpdEntry.

**/
EFI_IPSEC_ACTION
IpSecLookupSpdEntry (
  IN IPSEC_SPD_ENTRY         *SpdEntry,
  IN UINT8                   IpVersion,
  IN VOID                    *IpHead,
  IN UINT8                   *IpPayload,
  IN UINT8                   Protocol,
  IN BOOLEAN                 IsOutbound
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
    // Find the right spd entry if match the 5 key elements.
    //
    return SpdEntry->Data->Action;
  }

  return (EFI_IPSEC_ACTION) - 1;
}

/**
  Handles IPsec packet processing for inbound and outbound IP packets.

  The EFI_IPSEC_PROCESS process routine handles each inbound or outbound packet.
  The behavior is that it can perform one of the following actions:
  bypass the packet, discard the packet, or protect the packet.

  @param[in]      This             Pointer to the EFI_IPSEC_PROTOCOL instance.
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
  )
{
  IPSEC_PRIVATE_DATA  *Private;
  IPSEC_SPD_ENTRY     *SpdEntry;
  IPSEC_SAD_ENTRY     *SadEntry;
  LIST_ENTRY          *SpdList;
  LIST_ENTRY          *Entry;
  EFI_IPSEC_ACTION    Action;
  EFI_STATUS          Status;
  UINT8               *IpPayload;
  UINT8               OldLastHead;
  BOOLEAN             IsOutbound;

  Private         = IPSEC_PRIVATE_DATA_FROM_IPSEC (This);
  IpPayload       = (*FragmentTable)[0].FragmentBuffer;
  IsOutbound      = (BOOLEAN) ((TrafficDirection == EfiIPsecOutBound) ? TRUE : FALSE);
  OldLastHead     = *LastHead;
  *RecycleSignal  = NULL;

  if (!IsOutbound) {
    //
    // For inbound traffic, process the ipsec header of the packet.
    //
    Status = IpSecProtectInboundPacket (
              IpVersion,
              IpHead,
              LastHead,
              OptionsBuffer,
              OptionsLength,
              FragmentTable,
              FragmentCount,
              &SpdEntry,
              RecycleSignal
              );

    if (Status == EFI_ACCESS_DENIED) {
      //
      // The packet is denied to access.
      //
      goto ON_EXIT;
    }

    if (Status == EFI_SUCCESS) {
      //
      // Check the spd entry if the packet is accessible.
      //
      if (SpdEntry == NULL) {
        Status = EFI_ACCESS_DENIED;
        goto ON_EXIT;
      }
      Action = IpSecLookupSpdEntry (
                SpdEntry,
                IpVersion,
                IpHead,
                IpPayload,
                *LastHead,
                IsOutbound
                );

      if (Action != EfiIPsecActionProtect) {
        //
        // Discard the packet if the spd entry is not protect.
        //
        gBS->SignalEvent (*RecycleSignal);
        *RecycleSignal  = NULL;
        Status          = EFI_ACCESS_DENIED;
      }

      goto ON_EXIT;
    }
  }

  Status  = EFI_ACCESS_DENIED;
  SpdList = &mConfigData[IPsecConfigDataTypeSpd];

  for (Entry = SpdList->ForwardLink; Entry != SpdList; Entry = Entry->ForwardLink) {
    //
    // For outbound and non-ipsec Inbound traffic: check the spd entry.
    //
    SpdEntry = IPSEC_SPD_ENTRY_FROM_LIST (Entry);
    Action = IpSecLookupSpdEntry (
              SpdEntry,
              IpVersion,
              IpHead,
              IpPayload,
              OldLastHead,
              IsOutbound
              );

    switch (Action) {

    case EfiIPsecActionProtect:

      if (IsOutbound) {
        //
        // For outbound traffic, lookup the sad entry.
        //
        Status = IpSecLookupSadEntry (
                  Private,
                  NicHandle,
                  IpVersion,
                  IpHead,
                  IpPayload,
                  OldLastHead,
                  SpdEntry,
                  &SadEntry
                  );

        if (SadEntry != NULL) {
          //
          // Process the packet by the found sad entry.
          //
          Status = IpSecProtectOutboundPacket (
                    IpVersion,
                    IpHead,
                    LastHead,
                    OptionsBuffer,
                    OptionsLength,
                    FragmentTable,
                    FragmentCount,
                    SadEntry,
                    RecycleSignal
                    );

        } else if (OldLastHead == IP6_ICMP && *IpPayload != ICMP_V6_ECHO_REQUEST) {
          //
          // TODO: if no need return not ready to upper layer, change here.
          //
          Status = EFI_SUCCESS;
        }
      } else if (OldLastHead == IP6_ICMP && *IpPayload != ICMP_V6_ECHO_REQUEST) {
        //
        // For inbound icmpv6 traffic except ping request, accept the packet
        // although no sad entry associated with protect spd entry.
        //
        IpSecLookupSadEntry (
          Private,
          NicHandle,
          IpVersion,
          IpHead,
          IpPayload,
          OldLastHead,
          SpdEntry,
          &SadEntry
          );
        if (SadEntry == NULL) {
          Status = EFI_SUCCESS;
        }
      }

      goto ON_EXIT;

    case EfiIPsecActionBypass:
      Status = EFI_SUCCESS;
      goto ON_EXIT;

    case EfiIPsecActionDiscard:
      goto ON_EXIT;

    default:
      //
      // Discard the packet if no spd entry match.
      //
      break;
    }
  }

ON_EXIT:
  return Status;
}

