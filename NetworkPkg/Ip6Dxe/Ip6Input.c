/** @file
  IP6 internal functions to process the incoming packets.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

/**
  Create an empty assemble entry for the packet identified by
  (Dst, Src, Id). The default life for the packet is 60 seconds.

  @param[in]  Dst                    The destination address.
  @param[in]  Src                    The source address.
  @param[in]  Id                     The ID field in the IP header.

  @return NULL if failed to allocate memory for the entry. Otherwise,
          the pointer to the just created reassemble entry.

**/
IP6_ASSEMBLE_ENTRY *
Ip6CreateAssembleEntry (
  IN EFI_IPv6_ADDRESS  *Dst,
  IN EFI_IPv6_ADDRESS  *Src,
  IN UINT32            Id
  )
{
  IP6_ASSEMBLE_ENTRY  *Assemble;

  Assemble = AllocatePool (sizeof (IP6_ASSEMBLE_ENTRY));
  if (Assemble == NULL) {
    return NULL;
  }

  IP6_COPY_ADDRESS (&Assemble->Dst, Dst);
  IP6_COPY_ADDRESS (&Assemble->Src, Src);
  InitializeListHead (&Assemble->Fragments);

  Assemble->Id   = Id;
  Assemble->Life = IP6_FRAGMENT_LIFE + 1;

  Assemble->TotalLen = 0;
  Assemble->CurLen   = 0;
  Assemble->Head     = NULL;
  Assemble->Info     = NULL;
  Assemble->Packet   = NULL;

  return Assemble;
}

/**
  Release all the fragments of a packet, then free the assemble entry.

  @param[in]  Assemble               The assemble entry to free.

**/
VOID
Ip6FreeAssembleEntry (
  IN IP6_ASSEMBLE_ENTRY  *Assemble
  )
{
  LIST_ENTRY  *Entry;
  LIST_ENTRY  *Next;
  NET_BUF     *Fragment;

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Assemble->Fragments) {
    Fragment = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);

    RemoveEntryList (Entry);
    NetbufFree (Fragment);
  }

  if (Assemble->Packet != NULL) {
    NetbufFree (Assemble->Packet);
  }

  FreePool (Assemble);
}

/**
  Release all the fragments of the packet. This is the callback for
  the assembled packet's OnFree. It will free the assemble entry,
  which in turn frees all the fragments of the packet.

  @param[in]  Arg                    The assemble entry to free.

**/
VOID
EFIAPI
Ip6OnFreeFragments (
  IN VOID  *Arg
  )
{
  Ip6FreeAssembleEntry ((IP6_ASSEMBLE_ENTRY *)Arg);
}

/**
  Trim the packet to fit in [Start, End), and update per the
  packet information.

  @param[in, out]  Packet   Packet to trim.
  @param[in]       Start    The sequence of the first byte to fit in.
  @param[in]       End      One beyond the sequence of last byte to fit in.

**/
VOID
Ip6TrimPacket (
  IN OUT NET_BUF  *Packet,
  IN INTN         Start,
  IN INTN         End
  )
{
  IP6_CLIP_INFO  *Info;
  INTN           Len;

  Info = IP6_GET_CLIP_INFO (Packet);

  ASSERT (Info->Start + Info->Length == Info->End);
  ASSERT ((Info->Start < End) && (Start < Info->End));

  if (Info->Start < Start) {
    Len = Start - Info->Start;

    NetbufTrim (Packet, (UINT32)Len, NET_BUF_HEAD);
    Info->Start   = (UINT32)Start;
    Info->Length -= (UINT32)Len;
  }

  if (End < Info->End) {
    Len = End - Info->End;

    NetbufTrim (Packet, (UINT32)Len, NET_BUF_TAIL);
    Info->End     = (UINT32)End;
    Info->Length -= (UINT32)Len;
  }
}

/**
  Reassemble the IP fragments. If all the fragments of the packet
  have been received, it will wrap the packet in a net buffer then
  return it to caller. If the packet can't be assembled, NULL is
  returned.

  @param[in, out] Table  The assemble table used. A new assemble entry will be created
                         if the Packet is from a new chain of fragments.
  @param[in]      Packet The fragment to assemble. It might be freed if the fragment
                         can't be re-assembled.

  @return NULL if the packet can't be reassembled. The pointer to the just assembled
          packet if all the fragments of the packet have arrived.

**/
NET_BUF *
Ip6Reassemble (
  IN OUT IP6_ASSEMBLE_TABLE  *Table,
  IN NET_BUF                 *Packet
  )
{
  EFI_IP6_HEADER      *Head;
  IP6_CLIP_INFO       *This;
  IP6_CLIP_INFO       *Node;
  IP6_ASSEMBLE_ENTRY  *Assemble;
  IP6_ASSEMBLE_ENTRY  *Entry;
  LIST_ENTRY          *ListHead;
  LIST_ENTRY          *Prev;
  LIST_ENTRY          *Cur;
  NET_BUF             *Fragment;
  NET_BUF             *TmpPacket;
  NET_BUF             *NewPacket;
  NET_BUF             *Duplicate;
  UINT8               *DupHead;
  INTN                Index;
  UINT16              UnFragmentLen;
  UINT8               *NextHeader;

  Head = Packet->Ip.Ip6;
  This = IP6_GET_CLIP_INFO (Packet);

  ASSERT (Head != NULL);

  //
  // Find the corresponding assemble entry by (Dst, Src, Id)
  //
  Assemble = NULL;
  Index    = IP6_ASSEMBLE_HASH (&Head->DestinationAddress, &Head->SourceAddress, This->Id);

  NET_LIST_FOR_EACH (Cur, &Table->Bucket[Index]) {
    Entry = NET_LIST_USER_STRUCT (Cur, IP6_ASSEMBLE_ENTRY, Link);

    if ((Entry->Id == This->Id) &&
        EFI_IP6_EQUAL (&Entry->Src, &Head->SourceAddress) &&
        EFI_IP6_EQUAL (&Entry->Dst, &Head->DestinationAddress)
        )
    {
      Assemble = Entry;
      break;
    }
  }

  //
  // Create a new entry if can not find an existing one, insert it to assemble table
  //
  if (Assemble == NULL) {
    Assemble = Ip6CreateAssembleEntry (
                 &Head->DestinationAddress,
                 &Head->SourceAddress,
                 This->Id
                 );

    if (Assemble == NULL) {
      goto Error;
    }

    InsertHeadList (&Table->Bucket[Index], &Assemble->Link);
  }

  //
  // Find the point to insert the packet: before the first
  // fragment with THIS.Start < CUR.Start. the previous one
  // has PREV.Start <= THIS.Start < CUR.Start.
  //
  ListHead = &Assemble->Fragments;

  NET_LIST_FOR_EACH (Cur, ListHead) {
    Fragment = NET_LIST_USER_STRUCT (Cur, NET_BUF, List);

    if (This->Start < IP6_GET_CLIP_INFO (Fragment)->Start) {
      break;
    }
  }

  //
  // Check whether the current fragment overlaps with the previous one.
  // It holds that: PREV.Start <= THIS.Start < THIS.End. Only need to
  // check whether THIS.Start < PREV.End for overlap. If two fragments
  // overlaps, trim the overlapped part off THIS fragment.
  //
  if ((Prev = Cur->BackLink) != ListHead) {
    Fragment = NET_LIST_USER_STRUCT (Prev, NET_BUF, List);
    Node     = IP6_GET_CLIP_INFO (Fragment);

    if (This->Start < Node->End) {
      if (This->End <= Node->End) {
        goto Error;
      }

      //
      // Trim the previous fragment from tail.
      //
      Ip6TrimPacket (Fragment, Node->Start, This->Start);
    }
  }

  //
  // Insert the fragment into the packet. The fragment may be removed
  // from the list by the following checks.
  //
  NetListInsertBefore (Cur, &Packet->List);

  //
  // Check the packets after the insert point. It holds that:
  // THIS.Start <= NODE.Start < NODE.End. The equality holds
  // if PREV and NEXT are continuous. THIS fragment may fill
  // several holes. Remove the completely overlapped fragments
  //
  while (Cur != ListHead) {
    Fragment = NET_LIST_USER_STRUCT (Cur, NET_BUF, List);
    Node     = IP6_GET_CLIP_INFO (Fragment);

    //
    // Remove fragments completely overlapped by this fragment
    //
    if (Node->End <= This->End) {
      Cur = Cur->ForwardLink;

      RemoveEntryList (&Fragment->List);
      Assemble->CurLen -= Node->Length;

      NetbufFree (Fragment);
      continue;
    }

    //
    // The conditions are: THIS.Start <= NODE.Start, and THIS.End <
    // NODE.End. Two fragments overlaps if NODE.Start < THIS.End.
    // If two fragments start at the same offset, remove THIS fragment
    // because ((THIS.Start == NODE.Start) && (THIS.End < NODE.End)).
    //
    if (Node->Start < This->End) {
      if (This->Start == Node->Start) {
        RemoveEntryList (&Packet->List);
        goto Error;
      }

      Ip6TrimPacket (Packet, This->Start, Node->Start);
    }

    break;
  }

  //
  // Update the assemble info: increase the current length. If it is
  // the frist fragment, update the packet's IP head and per packet
  // info. If it is the last fragment, update the total length.
  //
  Assemble->CurLen += This->Length;

  if (This->Start == 0) {
    //
    // Once the first fragment is enqueued, it can't be removed
    // from the fragment list. So, Assemble->Head always point
    // to valid memory area.
    //
    if ((Assemble->Head != NULL) || (Assemble->Packet != NULL)) {
      goto Error;
    }

    //
    // Backup the first fragment in case the reassembly of that packet fail.
    //
    Duplicate = NetbufDuplicate (Packet, NULL, sizeof (EFI_IP6_HEADER));
    if (Duplicate == NULL) {
      goto Error;
    }

    //
    // Revert IP head to network order.
    //
    DupHead = NetbufGetByte (Duplicate, 0, NULL);
    ASSERT (DupHead != NULL);
    Duplicate->Ip.Ip6 = Ip6NtohHead ((EFI_IP6_HEADER *)DupHead);
    Assemble->Packet  = Duplicate;

    //
    // Adjust the unfragmentable part in first fragment
    //
    UnFragmentLen = (UINT16)(This->HeadLen - sizeof (EFI_IP6_HEADER));
    if (UnFragmentLen == 0) {
      //
      // There is not any unfragmentable extension header.
      //
      ASSERT (Head->NextHeader == IP6_FRAGMENT);
      Head->NextHeader = This->NextHeader;
    } else {
      NextHeader = NetbufGetByte (
                     Packet,
                     This->FormerNextHeader + sizeof (EFI_IP6_HEADER),
                     0
                     );
      if (NextHeader == NULL) {
        goto Error;
      }

      *NextHeader = This->NextHeader;
    }

    Assemble->Head = Head;
    Assemble->Info = IP6_GET_CLIP_INFO (Packet);
  }

  //
  // Don't update the length more than once.
  //
  if ((This->LastFrag != 0) && (Assemble->TotalLen == 0)) {
    Assemble->TotalLen = This->End;
  }

  //
  // Deliver the whole packet if all the fragments received.
  // All fragments received if:
  //  1. received the last one, so, the total length is known
  //  2. received all the data. If the last fragment on the
  //     queue ends at the total length, all data is received.
  //
  if ((Assemble->TotalLen != 0) && (Assemble->CurLen >= Assemble->TotalLen)) {
    RemoveEntryList (&Assemble->Link);

    //
    // If the packet is properly formatted, the last fragment's End
    // equals to the packet's total length. Otherwise, the packet
    // is a fake, drop it now.
    //
    Fragment = NET_LIST_USER_STRUCT (ListHead->BackLink, NET_BUF, List);
    if (IP6_GET_CLIP_INFO (Fragment)->End != (INTN)Assemble->TotalLen) {
      Ip6FreeAssembleEntry (Assemble);
      goto Error;
    }

    Fragment = NET_LIST_HEAD (ListHead, NET_BUF, List);
    This     = Assemble->Info;

    //
    // This TmpPacket is used to hold the unfragmentable part, i.e.,
    // the IPv6 header and the unfragmentable extension headers. Be noted that
    // the Fragment Header is excluded.
    //
    TmpPacket = NetbufGetFragment (Fragment, 0, This->HeadLen, 0);
    ASSERT (TmpPacket != NULL);

    NET_LIST_FOR_EACH (Cur, ListHead) {
      //
      // Trim off the unfragment part plus the fragment header from all fragments.
      //
      Fragment = NET_LIST_USER_STRUCT (Cur, NET_BUF, List);
      NetbufTrim (Fragment, This->HeadLen + sizeof (IP6_FRAGMENT_HEADER), TRUE);
    }

    InsertHeadList (ListHead, &TmpPacket->List);

    //
    // Wrap the packet in a net buffer then deliver it up
    //
    NewPacket = NetbufFromBufList (
                  &Assemble->Fragments,
                  0,
                  0,
                  Ip6OnFreeFragments,
                  Assemble
                  );

    if (NewPacket == NULL) {
      Ip6FreeAssembleEntry (Assemble);
      goto Error;
    }

    NewPacket->Ip.Ip6 = Assemble->Head;

    CopyMem (IP6_GET_CLIP_INFO (NewPacket), Assemble->Info, sizeof (IP6_CLIP_INFO));

    return NewPacket;
  }

  return NULL;

Error:
  NetbufFree (Packet);
  return NULL;
}

/**
  The callback function for the net buffer that wraps the packet processed by
  IPsec. It releases the wrap packet and also signals IPsec to free the resources.

  @param[in]  Arg       The wrap context.

**/
VOID
EFIAPI
Ip6IpSecFree (
  IN VOID  *Arg
  )
{
  IP6_IPSEC_WRAP  *Wrap;

  Wrap = (IP6_IPSEC_WRAP *)Arg;

  if (Wrap->IpSecRecycleSignal != NULL) {
    gBS->SignalEvent (Wrap->IpSecRecycleSignal);
  }

  NetbufFree (Wrap->Packet);

  FreePool (Wrap);

  return;
}

/**
  The work function to locate the IPsec protocol to process the inbound or
  outbound IP packets. The process routine handles the packet with the following
  actions: bypass the packet, discard the packet, or protect the packet.

  @param[in]       IpSb          The IP6 service instance.
  @param[in, out]  Head          The caller-supplied IP6 header.
  @param[in, out]  LastHead      The next header field of last IP header.
  @param[in, out]  Netbuf        The IP6 packet to be processed by IPsec.
  @param[in, out]  ExtHdrs       The caller-supplied options.
  @param[in, out]  ExtHdrsLen    The length of the option.
  @param[in]       Direction     The directionality in an SPD entry,
                                 EfiIPsecInBound, or EfiIPsecOutBound.
  @param[in]       Context       The token's wrap.

  @retval EFI_SUCCESS            The IPsec protocol is not available or disabled.
  @retval EFI_SUCCESS            The packet was bypassed, and all buffers remain the same.
  @retval EFI_SUCCESS            The packet was protected.
  @retval EFI_ACCESS_DENIED      The packet was discarded.
  @retval EFI_OUT_OF_RESOURCES   There are not sufficient resources to complete the operation.
  @retval EFI_BUFFER_TOO_SMALL   The number of non-empty blocks is bigger than the
                                 number of input data blocks when building a fragment table.

**/
EFI_STATUS
Ip6IpSecProcessPacket (
  IN     IP6_SERVICE            *IpSb,
  IN OUT EFI_IP6_HEADER         **Head,
  IN OUT UINT8                  *LastHead,
  IN OUT NET_BUF                **Netbuf,
  IN OUT UINT8                  **ExtHdrs,
  IN OUT UINT32                 *ExtHdrsLen,
  IN     EFI_IPSEC_TRAFFIC_DIR  Direction,
  IN     VOID                   *Context
  )
{
  NET_FRAGMENT      *FragmentTable;
  NET_FRAGMENT      *OriginalFragmentTable;
  UINT32            FragmentCount;
  UINT32            OriginalFragmentCount;
  EFI_EVENT         RecycleEvent;
  NET_BUF           *Packet;
  IP6_TXTOKEN_WRAP  *TxWrap;
  IP6_IPSEC_WRAP    *IpSecWrap;
  EFI_STATUS        Status;
  EFI_IP6_HEADER    *PacketHead;
  UINT8             *Buf;
  EFI_IP6_HEADER    ZeroHead;

  Status = EFI_SUCCESS;

  if (!mIpSec2Installed) {
    goto ON_EXIT;
  }

  ASSERT (mIpSec != NULL);

  Packet        = *Netbuf;
  RecycleEvent  = NULL;
  IpSecWrap     = NULL;
  FragmentTable = NULL;
  PacketHead    = NULL;
  Buf           = NULL;
  TxWrap        = (IP6_TXTOKEN_WRAP *)Context;
  FragmentCount = Packet->BlockOpNum;
  ZeroMem (&ZeroHead, sizeof (EFI_IP6_HEADER));

  //
  // Check whether the ipsec enable variable is set.
  //
  if (mIpSec->DisabledFlag) {
    //
    // If IPsec is disabled, restore the original MTU
    //
    IpSb->MaxPacketSize = IpSb->OldMaxPacketSize;
    goto ON_EXIT;
  } else {
    //
    // If IPsec is enabled, use the MTU which reduce the IPsec header length.
    //
    IpSb->MaxPacketSize = IpSb->OldMaxPacketSize - IP6_MAX_IPSEC_HEADLEN;
  }

  //
  // Bypass all multicast inbound or outbound traffic.
  //
  if (IP6_IS_MULTICAST (&(*Head)->DestinationAddress) || IP6_IS_MULTICAST (&(*Head)->SourceAddress)) {
    goto ON_EXIT;
  }

  //
  // Rebuild fragment table from netbuf to ease ipsec process.
  //
  FragmentTable = AllocateZeroPool (FragmentCount * sizeof (NET_FRAGMENT));

  if (FragmentTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status                = NetbufBuildExt (Packet, FragmentTable, &FragmentCount);
  OriginalFragmentTable = FragmentTable;
  OriginalFragmentCount = FragmentCount;

  if (EFI_ERROR (Status)) {
    FreePool (FragmentTable);
    goto ON_EXIT;
  }

  //
  // Convert host byte order to network byte order
  //
  Ip6NtohHead (*Head);

  Status = mIpSec->ProcessExt (
                     mIpSec,
                     IpSb->Controller,
                     IP_VERSION_6,
                     (VOID *)(*Head),
                     LastHead,
                     (VOID **)ExtHdrs,
                     ExtHdrsLen,
                     (EFI_IPSEC_FRAGMENT_DATA  **)(&FragmentTable),
                     &FragmentCount,
                     Direction,
                     &RecycleEvent
                     );
  //
  // Convert back to host byte order
  //
  Ip6NtohHead (*Head);

  if (EFI_ERROR (Status)) {
    FreePool (OriginalFragmentTable);
    goto ON_EXIT;
  }

  if ((OriginalFragmentCount == FragmentCount) && (OriginalFragmentTable == FragmentTable)) {
    //
    // For ByPass Packet
    //
    FreePool (FragmentTable);
    goto ON_EXIT;
  } else {
    //
    // Free the FragmentTable which allocated before calling the IPsec.
    //
    FreePool (OriginalFragmentTable);
  }

  if ((Direction == EfiIPsecOutBound) && (TxWrap != NULL)) {
    TxWrap->IpSecRecycleSignal = RecycleEvent;
    TxWrap->Packet             = NetbufFromExt (
                                   FragmentTable,
                                   FragmentCount,
                                   IP6_MAX_HEADLEN,
                                   0,
                                   Ip6FreeTxToken,
                                   TxWrap
                                   );
    if (TxWrap->Packet == NULL) {
      TxWrap->Packet = *Netbuf;
      Status         = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    CopyMem (
      IP6_GET_CLIP_INFO (TxWrap->Packet),
      IP6_GET_CLIP_INFO (Packet),
      sizeof (IP6_CLIP_INFO)
      );

    NetIpSecNetbufFree (Packet);
    *Netbuf = TxWrap->Packet;
  } else {
    IpSecWrap = AllocateZeroPool (sizeof (IP6_IPSEC_WRAP));

    if (IpSecWrap == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      gBS->SignalEvent (RecycleEvent);
      goto ON_EXIT;
    }

    IpSecWrap->IpSecRecycleSignal = RecycleEvent;
    IpSecWrap->Packet             = Packet;
    Packet                        = NetbufFromExt (
                                      FragmentTable,
                                      FragmentCount,
                                      IP6_MAX_HEADLEN,
                                      0,
                                      Ip6IpSecFree,
                                      IpSecWrap
                                      );

    if (Packet == NULL) {
      Packet = IpSecWrap->Packet;
      gBS->SignalEvent (RecycleEvent);
      FreePool (IpSecWrap);
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    if ((Direction == EfiIPsecInBound) && (0 != CompareMem (&ZeroHead, *Head, sizeof (EFI_IP6_HEADER)))) {
      PacketHead = (EFI_IP6_HEADER *)NetbufAllocSpace (
                                       Packet,
                                       sizeof (EFI_IP6_HEADER) + *ExtHdrsLen,
                                       NET_BUF_HEAD
                                       );
      if (PacketHead == NULL) {
        *Netbuf = Packet;
        Status  = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }

      CopyMem (PacketHead, *Head, sizeof (EFI_IP6_HEADER));
      *Head          = PacketHead;
      Packet->Ip.Ip6 = PacketHead;

      if (*ExtHdrs != NULL) {
        Buf = (UINT8 *)(PacketHead + 1);
        CopyMem (Buf, *ExtHdrs, *ExtHdrsLen);
      }

      NetbufTrim (Packet, sizeof (EFI_IP6_HEADER) + *ExtHdrsLen, TRUE);
      CopyMem (
        IP6_GET_CLIP_INFO (Packet),
        IP6_GET_CLIP_INFO (IpSecWrap->Packet),
        sizeof (IP6_CLIP_INFO)
        );
    }

    *Netbuf = Packet;
  }

ON_EXIT:
  return Status;
}

/**
  Pre-process the IPv6 packet. First validates the IPv6 packet, and
  then reassembles packet if it is necessary.

  @param[in]      IpSb          The IP6 service instance.
  @param[in, out] Packet        The received IP6 packet to be processed.
  @param[in]      Flag          The link layer flag for the packet received, such
                                as multicast.
  @param[out]     Payload       The pointer to the payload of the received packet.
                                it starts from the first byte of the extension header.
  @param[out]     LastHead      The pointer of NextHeader of the last extension
                                header processed by IP6.
  @param[out]     ExtHdrsLen    The length of the whole option.
  @param[out]     UnFragmentLen The length of unfragmented length of extension headers.
  @param[out]     Fragmented    Indicate whether the packet is fragmented.
  @param[out]     Head          The pointer to the EFI_IP6_Header.

  @retval     EFI_SUCCESS              The received packet is well format.
  @retval     EFI_INVALID_PARAMETER    The received packet is malformed.

**/
EFI_STATUS
Ip6PreProcessPacket (
  IN     IP6_SERVICE  *IpSb,
  IN OUT NET_BUF      **Packet,
  IN     UINT32       Flag,
  OUT UINT8           **Payload,
  OUT UINT8           **LastHead,
  OUT UINT32          *ExtHdrsLen,
  OUT UINT32          *UnFragmentLen,
  OUT BOOLEAN         *Fragmented,
  OUT EFI_IP6_HEADER  **Head
  )
{
  UINT16               PayloadLen;
  UINT16               TotalLen;
  UINT32               FormerHeadOffset;
  UINT32               HeadLen;
  IP6_FRAGMENT_HEADER  *FragmentHead;
  UINT16               FragmentOffset;
  IP6_CLIP_INFO        *Info;
  EFI_IPv6_ADDRESS     Loopback;

  HeadLen    = 0;
  PayloadLen = 0;
  //
  // Check whether the input packet is a valid packet
  //
  if ((*Packet)->TotalSize < IP6_MIN_HEADLEN) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get header information of the packet.
  //
  *Head = (EFI_IP6_HEADER *)NetbufGetByte (*Packet, 0, NULL);
  if (*Head == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Multicast addresses must not be used as source addresses in IPv6 packets.
  //
  if (((*Head)->Version != 6) || (IP6_IS_MULTICAST (&(*Head)->SourceAddress))) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // A packet with a destination address of loopback ::1/128 or unspecified must be dropped.
  //
  ZeroMem (&Loopback, sizeof (EFI_IPv6_ADDRESS));
  Loopback.Addr[15] = 0x1;
  if ((CompareMem (&Loopback, &(*Head)->DestinationAddress, sizeof (EFI_IPv6_ADDRESS)) == 0) ||
      (NetIp6IsUnspecifiedAddr (&(*Head)->DestinationAddress)))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Convert the IP header to host byte order.
  //
  (*Packet)->Ip.Ip6 = Ip6NtohHead (*Head);

  //
  // Get the per packet info.
  //
  Info           = IP6_GET_CLIP_INFO (*Packet);
  Info->LinkFlag = Flag;
  Info->CastType = 0;

  if (IpSb->MnpConfigData.EnablePromiscuousReceive) {
    Info->CastType = Ip6Promiscuous;
  }

  if (Ip6IsOneOfSetAddress (IpSb, &(*Head)->DestinationAddress, NULL, NULL)) {
    Info->CastType = Ip6Unicast;
  } else if (IP6_IS_MULTICAST (&(*Head)->DestinationAddress)) {
    if (Ip6FindMldEntry (IpSb, &(*Head)->DestinationAddress) != NULL) {
      Info->CastType = Ip6Multicast;
    }
  }

  //
  // Drop the packet that is not delivered to us.
  //
  if (Info->CastType == 0) {
    return EFI_INVALID_PARAMETER;
  }

  PayloadLen = (*Head)->PayloadLength;

  Info->Start    = 0;
  Info->Length   = PayloadLen;
  Info->End      = Info->Start + Info->Length;
  Info->HeadLen  = (UINT16)sizeof (EFI_IP6_HEADER);
  Info->Status   = EFI_SUCCESS;
  Info->LastFrag = FALSE;

  TotalLen = (UINT16)(PayloadLen + sizeof (EFI_IP6_HEADER));

  //
  // Mnp may deliver frame trailer sequence up, trim it off.
  //
  if (TotalLen < (*Packet)->TotalSize) {
    NetbufTrim (*Packet, (*Packet)->TotalSize - TotalLen, FALSE);
  }

  if (TotalLen != (*Packet)->TotalSize) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check the extension headers, if exist validate them
  //
  if (PayloadLen != 0) {
    *Payload = AllocatePool ((UINTN)PayloadLen);
    if (*Payload == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    NetbufCopy (*Packet, sizeof (EFI_IP6_HEADER), PayloadLen, *Payload);
  }

  if (!Ip6IsExtsValid (
         IpSb,
         *Packet,
         &(*Head)->NextHeader,
         *Payload,
         (UINT32)PayloadLen,
         TRUE,
         &FormerHeadOffset,
         LastHead,
         ExtHdrsLen,
         UnFragmentLen,
         Fragmented
         ))
  {
    return EFI_INVALID_PARAMETER;
  }

  HeadLen = sizeof (EFI_IP6_HEADER) + *UnFragmentLen;

  if (*Fragmented) {
    //
    // Get the fragment offset from the Fragment header
    //
    FragmentHead = (IP6_FRAGMENT_HEADER *)NetbufGetByte (*Packet, HeadLen, NULL);
    if (FragmentHead == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    FragmentOffset = NTOHS (FragmentHead->FragmentOffset);

    if ((FragmentOffset & 0x1) == 0) {
      Info->LastFrag = TRUE;
    }

    FragmentOffset &= (~0x1);

    //
    // This is the first fragment of the packet
    //
    if (FragmentOffset == 0) {
      Info->NextHeader = FragmentHead->NextHeader;
    }

    Info->HeadLen          = (UINT16)HeadLen;
    HeadLen               += sizeof (IP6_FRAGMENT_HEADER);
    Info->Start            = FragmentOffset;
    Info->Length           = TotalLen - (UINT16)HeadLen;
    Info->End              = Info->Start + Info->Length;
    Info->Id               = FragmentHead->Identification;
    Info->FormerNextHeader = FormerHeadOffset;

    //
    // Fragments should in the unit of 8 octets long except the last one.
    //
    if ((Info->LastFrag == 0) && (Info->Length % 8 != 0)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Reassemble the packet.
    //
    *Packet = Ip6Reassemble (&IpSb->Assemble, *Packet);
    if (*Packet == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Re-check the assembled packet to get the right values.
    //
    *Head      = (*Packet)->Ip.Ip6;
    PayloadLen = (*Head)->PayloadLength;
    if (PayloadLen != 0) {
      if (*Payload != NULL) {
        FreePool (*Payload);
      }

      *Payload = AllocatePool ((UINTN)PayloadLen);
      if (*Payload == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      NetbufCopy (*Packet, sizeof (EFI_IP6_HEADER), PayloadLen, *Payload);
    }

    if (!Ip6IsExtsValid (
           IpSb,
           *Packet,
           &(*Head)->NextHeader,
           *Payload,
           (UINT32)PayloadLen,
           TRUE,
           NULL,
           LastHead,
           ExtHdrsLen,
           UnFragmentLen,
           Fragmented
           ))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Trim the head off, after this point, the packet is headless.
  // and Packet->TotalLen == Info->Length.
  //
  NetbufTrim (*Packet, sizeof (EFI_IP6_HEADER) + *ExtHdrsLen, TRUE);

  return EFI_SUCCESS;
}

/**
  The IP6 input routine. It is called by the IP6_INTERFACE when an
  IP6 fragment is received from MNP.

  @param[in]  Packet             The IP6 packet received.
  @param[in]  IoStatus           The return status of receive request.
  @param[in]  Flag               The link layer flag for the packet received, such
                                 as multicast.
  @param[in]  Context            The IP6 service instance that owns the MNP.

**/
VOID
Ip6AcceptFrame (
  IN NET_BUF     *Packet,
  IN EFI_STATUS  IoStatus,
  IN UINT32      Flag,
  IN VOID        *Context
  )
{
  IP6_SERVICE     *IpSb;
  EFI_IP6_HEADER  *Head;
  UINT8           *Payload;
  UINT8           *LastHead;
  UINT32          UnFragmentLen;
  UINT32          ExtHdrsLen;
  BOOLEAN         Fragmented;
  EFI_STATUS      Status;
  EFI_IP6_HEADER  ZeroHead;

  IpSb = (IP6_SERVICE *)Context;
  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  Payload  = NULL;
  LastHead = NULL;

  //
  // Check input parameters
  //
  if (EFI_ERROR (IoStatus) || (IpSb->State == IP6_SERVICE_DESTROY)) {
    goto Drop;
  }

  //
  // Pre-Process the Ipv6 Packet and then reassemble if it is necessary.
  //
  Status = Ip6PreProcessPacket (
             IpSb,
             &Packet,
             Flag,
             &Payload,
             &LastHead,
             &ExtHdrsLen,
             &UnFragmentLen,
             &Fragmented,
             &Head
             );
  if (EFI_ERROR (Status)) {
    goto Restart;
  }

  //
  // After trim off, the packet is a esp/ah/udp/tcp/icmp6 net buffer,
  // and no need consider any other ahead ext headers.
  //
  Status = Ip6IpSecProcessPacket (
             IpSb,
             &Head,
             LastHead, // need get the lasthead value for input
             &Packet,
             &Payload,
             &ExtHdrsLen,
             EfiIPsecInBound,
             NULL
             );

  if (EFI_ERROR (Status)) {
    goto Restart;
  }

  //
  // If the packet is protected by IPsec Tunnel Mode, Check the Inner Ip Packet.
  //
  ZeroMem (&ZeroHead, sizeof (EFI_IP6_HEADER));
  if (0 == CompareMem (Head, &ZeroHead, sizeof (EFI_IP6_HEADER))) {
    Status = Ip6PreProcessPacket (
               IpSb,
               &Packet,
               Flag,
               &Payload,
               &LastHead,
               &ExtHdrsLen,
               &UnFragmentLen,
               &Fragmented,
               &Head
               );
    if (EFI_ERROR (Status)) {
      goto Restart;
    }
  }

  //
  // Check the Packet again.
  //
  if (Packet == NULL) {
    goto Restart;
  }

  //
  // Packet may have been changed. The ownership of the packet
  // is transferred to the packet process logic.
  //
  Head                               = Packet->Ip.Ip6;
  IP6_GET_CLIP_INFO (Packet)->Status = EFI_SUCCESS;

  switch (*LastHead) {
    case IP6_ICMP:
      Ip6IcmpHandle (IpSb, Head, Packet);
      break;
    default:
      Ip6Demultiplex (IpSb, Head, Packet);
  }

  Packet = NULL;

  //
  // Dispatch the DPCs queued by the NotifyFunction of the rx token's events
  // which are signaled with received data.
  //
  DispatchDpc ();

Restart:
  if (Payload != NULL) {
    FreePool (Payload);
  }

  Ip6ReceiveFrame (Ip6AcceptFrame, IpSb);

Drop:
  if (Packet != NULL) {
    NetbufFree (Packet);
  }

  return;
}

/**
  Initialize an already allocated assemble table. This is generally
  the assemble table embedded in the IP6 service instance.

  @param[in, out]  Table    The assemble table to initialize.

**/
VOID
Ip6CreateAssembleTable (
  IN OUT IP6_ASSEMBLE_TABLE  *Table
  )
{
  UINT32  Index;

  for (Index = 0; Index < IP6_ASSEMLE_HASH_SIZE; Index++) {
    InitializeListHead (&Table->Bucket[Index]);
  }
}

/**
  Clean up the assemble table by removing all of the fragments
  and assemble entries.

  @param[in, out]  Table    The assemble table to clean up.

**/
VOID
Ip6CleanAssembleTable (
  IN OUT IP6_ASSEMBLE_TABLE  *Table
  )
{
  LIST_ENTRY          *Entry;
  LIST_ENTRY          *Next;
  IP6_ASSEMBLE_ENTRY  *Assemble;
  UINT32              Index;

  for (Index = 0; Index < IP6_ASSEMLE_HASH_SIZE; Index++) {
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &Table->Bucket[Index]) {
      Assemble = NET_LIST_USER_STRUCT (Entry, IP6_ASSEMBLE_ENTRY, Link);

      RemoveEntryList (Entry);
      Ip6FreeAssembleEntry (Assemble);
    }
  }
}

/**
  The signal handle of IP6's recycle event. It is called back
  when the upper layer releases the packet.

  @param[in]  Event         The IP6's recycle event.
  @param[in]  Context       The context of the handle, which is a IP6_RXDATA_WRAP.

**/
VOID
EFIAPI
Ip6OnRecyclePacket (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  IP6_RXDATA_WRAP  *Wrap;

  Wrap = (IP6_RXDATA_WRAP *)Context;

  EfiAcquireLockOrFail (&Wrap->IpInstance->RecycleLock);
  RemoveEntryList (&Wrap->Link);
  EfiReleaseLock (&Wrap->IpInstance->RecycleLock);

  ASSERT (!NET_BUF_SHARED (Wrap->Packet));
  NetbufFree (Wrap->Packet);

  gBS->CloseEvent (Wrap->RxData.RecycleSignal);
  FreePool (Wrap);
}

/**
  Wrap the received packet to a IP6_RXDATA_WRAP, which will be
  delivered to the upper layer. Each IP6 child that accepts the
  packet will get a not-shared copy of the packet which is wrapped
  in the IP6_RXDATA_WRAP. The IP6_RXDATA_WRAP->RxData is passed
  to the upper layer. The upper layer will signal the recycle event in
  it when it is done with the packet.

  @param[in]  IpInstance    The IP6 child to receive the packet.
  @param[in]  Packet        The packet to deliver up.

  @return NULL if it failed to wrap the packet; otherwise, the wrapper.

**/
IP6_RXDATA_WRAP *
Ip6WrapRxData (
  IN IP6_PROTOCOL  *IpInstance,
  IN NET_BUF       *Packet
  )
{
  IP6_RXDATA_WRAP       *Wrap;
  EFI_IP6_RECEIVE_DATA  *RxData;
  EFI_STATUS            Status;

  Wrap = AllocatePool (IP6_RXDATA_WRAP_SIZE (Packet->BlockOpNum));

  if (Wrap == NULL) {
    return NULL;
  }

  InitializeListHead (&Wrap->Link);

  Wrap->IpInstance = IpInstance;
  Wrap->Packet     = Packet;
  RxData           = &Wrap->RxData;

  ZeroMem (&RxData->TimeStamp, sizeof (EFI_TIME));

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Ip6OnRecyclePacket,
                  Wrap,
                  &RxData->RecycleSignal
                  );

  if (EFI_ERROR (Status)) {
    FreePool (Wrap);
    return NULL;
  }

  ASSERT (Packet->Ip.Ip6 != NULL);

  //
  // The application expects a network byte order header.
  //
  RxData->HeaderLength = sizeof (EFI_IP6_HEADER);
  RxData->Header       = (EFI_IP6_HEADER *)Ip6NtohHead (Packet->Ip.Ip6);
  RxData->DataLength   = Packet->TotalSize;

  //
  // Build the fragment table to be delivered up.
  //
  RxData->FragmentCount = Packet->BlockOpNum;
  NetbufBuildExt (Packet, (NET_FRAGMENT *)RxData->FragmentTable, &RxData->FragmentCount);

  return Wrap;
}

/**
  Check whether this IP child accepts the packet.

  @param[in]  IpInstance    The IP child to check.
  @param[in]  Head          The IP header of the packet.
  @param[in]  Packet        The data of the packet.

  @retval     TRUE          The child wants to receive the packet.
  @retval     FALSE         The child does not want to receive the packet.

**/
BOOLEAN
Ip6InstanceFrameAcceptable (
  IN IP6_PROTOCOL    *IpInstance,
  IN EFI_IP6_HEADER  *Head,
  IN NET_BUF         *Packet
  )
{
  IP6_ICMP_ERROR_HEAD  Icmp;
  EFI_IP6_CONFIG_DATA  *Config;
  IP6_CLIP_INFO        *Info;
  UINT8                *Proto;
  UINT32               Index;
  UINT8                *ExtHdrs;
  UINT16               ErrMsgPayloadLen;
  UINT8                *ErrMsgPayload;

  Config = &IpInstance->ConfigData;
  Proto  = NULL;

  //
  // Dirty trick for the Tiano UEFI network stack implementation. If
  // ReceiveTimeout == -1, the receive of the packet for this instance
  // is disabled. The UEFI spec don't have such captibility. We add
  // this to improve the performance because IP will make a copy of
  // the received packet for each accepting instance. Some IP instances
  // used by UDP/TCP only send packets, they don't wants to receive.
  //
  if (Config->ReceiveTimeout == (UINT32)(-1)) {
    return FALSE;
  }

  if (Config->AcceptPromiscuous) {
    return TRUE;
  }

  //
  // Check whether the protocol is acceptable.
  //
  ExtHdrs = NetbufGetByte (Packet, 0, NULL);

  if (!Ip6IsExtsValid (
         IpInstance->Service,
         Packet,
         &Head->NextHeader,
         ExtHdrs,
         (UINT32)Head->PayloadLength,
         TRUE,
         NULL,
         &Proto,
         NULL,
         NULL,
         NULL
         ))
  {
    return FALSE;
  }

  //
  // The upper layer driver may want to receive the ICMPv6 error packet
  // invoked by its packet, like UDP.
  //
  if ((*Proto == IP6_ICMP) && (!Config->AcceptAnyProtocol) && (*Proto != Config->DefaultProtocol)) {
    NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *)&Icmp);

    if (Icmp.Head.Type <= ICMP_V6_ERROR_MAX) {
      if (!Config->AcceptIcmpErrors) {
        return FALSE;
      }

      //
      // Get the protocol of the invoking packet of ICMPv6 error packet.
      //
      ErrMsgPayloadLen = NTOHS (Icmp.IpHead.PayloadLength);
      ErrMsgPayload    = NetbufGetByte (Packet, sizeof (Icmp), NULL);

      if (!Ip6IsExtsValid (
             NULL,
             NULL,
             &Icmp.IpHead.NextHeader,
             ErrMsgPayload,
             ErrMsgPayloadLen,
             TRUE,
             NULL,
             &Proto,
             NULL,
             NULL,
             NULL
             ))
      {
        return FALSE;
      }
    }
  }

  //
  // Match the protocol
  //
  if (!Config->AcceptAnyProtocol && (*Proto != Config->DefaultProtocol)) {
    return FALSE;
  }

  //
  // Check for broadcast, the caller has computed the packet's
  // cast type for this child's interface.
  //
  Info = IP6_GET_CLIP_INFO (Packet);

  //
  // If it is a multicast packet, check whether we are in the group.
  //
  if (Info->CastType == Ip6Multicast) {
    //
    // Receive the multicast if the instance wants to receive all packets.
    //
    if (NetIp6IsUnspecifiedAddr (&IpInstance->ConfigData.StationAddress)) {
      return TRUE;
    }

    for (Index = 0; Index < IpInstance->GroupCount; Index++) {
      if (EFI_IP6_EQUAL (IpInstance->GroupList + Index, &Head->DestinationAddress)) {
        break;
      }
    }

    return (BOOLEAN)(Index < IpInstance->GroupCount);
  }

  return TRUE;
}

/**
  Enqueue a shared copy of the packet to the IP6 child if the
  packet is acceptable to it. Here the data of the packet is
  shared, but the net buffer isn't.

  @param  IpInstance             The IP6 child to enqueue the packet to.
  @param  Head                   The IP header of the received packet.
  @param  Packet                 The data of the received packet.

  @retval EFI_NOT_STARTED        The IP child hasn't been configured.
  @retval EFI_INVALID_PARAMETER  The child doesn't want to receive the packet.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate some resources
  @retval EFI_SUCCESS            A shared copy the packet is enqueued to the child.

**/
EFI_STATUS
Ip6InstanceEnquePacket (
  IN IP6_PROTOCOL    *IpInstance,
  IN EFI_IP6_HEADER  *Head,
  IN NET_BUF         *Packet
  )
{
  IP6_CLIP_INFO  *Info;
  NET_BUF        *Clone;

  //
  // Check whether the packet is acceptable to this instance.
  //
  if (IpInstance->State != IP6_STATE_CONFIGED) {
    return EFI_NOT_STARTED;
  }

  if (!Ip6InstanceFrameAcceptable (IpInstance, Head, Packet)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Enqueue a shared copy of the packet.
  //
  Clone = NetbufClone (Packet);

  if (Clone == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set the receive time out for the assembled packet. If it expires,
  // packet will be removed from the queue.
  //
  Info       = IP6_GET_CLIP_INFO (Clone);
  Info->Life = IP6_US_TO_SEC (IpInstance->ConfigData.ReceiveTimeout);

  InsertTailList (&IpInstance->Received, &Clone->List);
  return EFI_SUCCESS;
}

/**
  Deliver the received packets to the upper layer if there are both received
  requests and enqueued packets. If the enqueued packet is shared, it will
  duplicate it to a non-shared packet, release the shared packet, then
  deliver the non-shared packet up.

  @param[in]  IpInstance         The IP child to deliver the packet up.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources to deliver the
                                 packets.
  @retval EFI_SUCCESS            All the enqueued packets that can be delivered
                                 are delivered up.

**/
EFI_STATUS
Ip6InstanceDeliverPacket (
  IN IP6_PROTOCOL  *IpInstance
  )
{
  EFI_IP6_COMPLETION_TOKEN  *Token;
  IP6_RXDATA_WRAP           *Wrap;
  NET_BUF                   *Packet;
  NET_BUF                   *Dup;
  UINT8                     *Head;

  //
  // Deliver a packet if there are both a packet and a receive token.
  //
  while (!IsListEmpty (&IpInstance->Received) && !NetMapIsEmpty (&IpInstance->RxTokens)) {
    Packet = NET_LIST_HEAD (&IpInstance->Received, NET_BUF, List);

    if (!NET_BUF_SHARED (Packet)) {
      //
      // If this is the only instance that wants the packet, wrap it up.
      //
      Wrap = Ip6WrapRxData (IpInstance, Packet);

      if (Wrap == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      RemoveEntryList (&Packet->List);
    } else {
      //
      // Create a duplicated packet if this packet is shared
      //
      Dup = NetbufDuplicate (Packet, NULL, sizeof (EFI_IP6_HEADER));

      if (Dup == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Copy the IP head over. The packet to deliver up is
      // headless. Trim the head off after copy. The IP head
      // may be not continuous before the data.
      //
      Head = NetbufAllocSpace (Dup, sizeof (EFI_IP6_HEADER), NET_BUF_HEAD);
      ASSERT (Head != NULL);
      Dup->Ip.Ip6 = (EFI_IP6_HEADER *)Head;

      CopyMem (Head, Packet->Ip.Ip6, sizeof (EFI_IP6_HEADER));
      NetbufTrim (Dup, sizeof (EFI_IP6_HEADER), TRUE);

      Wrap = Ip6WrapRxData (IpInstance, Dup);

      if (Wrap == NULL) {
        NetbufFree (Dup);
        return EFI_OUT_OF_RESOURCES;
      }

      RemoveEntryList (&Packet->List);
      NetbufFree (Packet);

      Packet = Dup;
    }

    //
    // Insert it into the delivered packet, then get a user's
    // receive token, pass the wrapped packet up.
    //
    EfiAcquireLockOrFail (&IpInstance->RecycleLock);
    InsertHeadList (&IpInstance->Delivered, &Wrap->Link);
    EfiReleaseLock (&IpInstance->RecycleLock);

    Token                = NetMapRemoveHead (&IpInstance->RxTokens, NULL);
    Token->Status        = IP6_GET_CLIP_INFO (Packet)->Status;
    Token->Packet.RxData = &Wrap->RxData;

    gBS->SignalEvent (Token->Event);
  }

  return EFI_SUCCESS;
}

/**
  Enqueue a received packet to all the IP children that share
  the same interface.

  @param[in]  IpSb          The IP6 service instance that receive the packet.
  @param[in]  Head          The header of the received packet.
  @param[in]  Packet        The data of the received packet.
  @param[in]  IpIf          The interface to enqueue the packet to.

  @return The number of the IP6 children that accepts the packet.

**/
INTN
Ip6InterfaceEnquePacket (
  IN IP6_SERVICE     *IpSb,
  IN EFI_IP6_HEADER  *Head,
  IN NET_BUF         *Packet,
  IN IP6_INTERFACE   *IpIf
  )
{
  IP6_PROTOCOL   *IpInstance;
  IP6_CLIP_INFO  *Info;
  LIST_ENTRY     *Entry;
  INTN           Enqueued;
  INTN           LocalType;
  INTN           SavedType;

  //
  // First, check that the packet is acceptable to this interface
  // and find the local cast type for the interface.
  //
  LocalType = 0;
  Info      = IP6_GET_CLIP_INFO (Packet);

  if (IpIf->PromiscRecv) {
    LocalType = Ip6Promiscuous;
  } else {
    LocalType = Info->CastType;
  }

  //
  // Iterate through the ip instances on the interface, enqueue
  // the packet if filter passed. Save the original cast type,
  // and pass the local cast type to the IP children on the
  // interface. The global cast type will be restored later.
  //
  SavedType      = Info->CastType;
  Info->CastType = (UINT32)LocalType;

  Enqueued = 0;

  NET_LIST_FOR_EACH (Entry, &IpIf->IpInstances) {
    IpInstance = NET_LIST_USER_STRUCT (Entry, IP6_PROTOCOL, AddrLink);
    NET_CHECK_SIGNATURE (IpInstance, IP6_PROTOCOL_SIGNATURE);

    if (Ip6InstanceEnquePacket (IpInstance, Head, Packet) == EFI_SUCCESS) {
      Enqueued++;
    }
  }

  Info->CastType = (UINT32)SavedType;
  return Enqueued;
}

/**
  Deliver the packet for each IP6 child on the interface.

  @param[in]  IpSb          The IP6 service instance that received the packet.
  @param[in]  IpIf          The IP6 interface to deliver the packet.

**/
VOID
Ip6InterfaceDeliverPacket (
  IN IP6_SERVICE    *IpSb,
  IN IP6_INTERFACE  *IpIf
  )
{
  IP6_PROTOCOL  *IpInstance;
  LIST_ENTRY    *Entry;

  NET_LIST_FOR_EACH (Entry, &IpIf->IpInstances) {
    IpInstance = NET_LIST_USER_STRUCT (Entry, IP6_PROTOCOL, AddrLink);
    Ip6InstanceDeliverPacket (IpInstance);
  }
}

/**
  De-multiplex the packet. the packet delivery is processed in two
  passes. The first pass will enqueue a shared copy of the packet
  to each IP6 child that accepts the packet. The second pass will
  deliver a non-shared copy of the packet to each IP6 child that
  has pending receive requests. Data is copied if more than one
  child wants to consume the packet, because each IP child needs
  its own copy of the packet to make changes.

  @param[in]  IpSb          The IP6 service instance that received the packet.
  @param[in]  Head          The header of the received packet.
  @param[in]  Packet        The data of the received packet.

  @retval EFI_NOT_FOUND     No IP child accepts the packet.
  @retval EFI_SUCCESS       The packet is enqueued or delivered to some IP
                            children.

**/
EFI_STATUS
Ip6Demultiplex (
  IN IP6_SERVICE     *IpSb,
  IN EFI_IP6_HEADER  *Head,
  IN NET_BUF         *Packet
  )
{
  LIST_ENTRY     *Entry;
  IP6_INTERFACE  *IpIf;
  INTN           Enqueued;

  //
  // Two pass delivery: first, enqueue a shared copy of the packet
  // to each instance that accept the packet.
  //
  Enqueued = 0;

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP6_INTERFACE, Link);

    if (IpIf->Configured) {
      Enqueued += Ip6InterfaceEnquePacket (IpSb, Head, Packet, IpIf);
    }
  }

  //
  // Second: deliver a duplicate of the packet to each instance.
  // Release the local reference first, so that the last instance
  // getting the packet will not copy the data.
  //
  NetbufFree (Packet);
  Packet = NULL;

  if (Enqueued == 0) {
    return EFI_NOT_FOUND;
  }

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP6_INTERFACE, Link);

    if (IpIf->Configured) {
      Ip6InterfaceDeliverPacket (IpSb, IpIf);
    }
  }

  return EFI_SUCCESS;
}

/**
  Decrease the life of the transmitted packets. If it is
  decreased to zero, cancel the packet. This function is
  called by Ip6packetTimerTicking that provides timeout for both the
  received-but-not-delivered and transmitted-but-not-recycle
  packets.

  @param[in]  Map           The IP6 child's transmit map.
  @param[in]  Item          Current transmitted packet.
  @param[in]  Context       Not used.

  @retval EFI_SUCCESS       Always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
Ip6SentPacketTicking (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  IP6_TXTOKEN_WRAP  *Wrap;

  Wrap = (IP6_TXTOKEN_WRAP *)Item->Value;
  ASSERT (Wrap != NULL);

  if ((Wrap->Life > 0) && (--Wrap->Life == 0)) {
    Ip6CancelPacket (Wrap->IpInstance->Interface, Wrap->Packet, EFI_ABORTED);
  }

  return EFI_SUCCESS;
}

/**
  Timeout the fragments, and the enqueued, and transmitted packets.

  @param[in]  IpSb          The IP6 service instance to timeout.

**/
VOID
Ip6PacketTimerTicking (
  IN IP6_SERVICE  *IpSb
  )
{
  LIST_ENTRY          *InstanceEntry;
  LIST_ENTRY          *Entry;
  LIST_ENTRY          *Next;
  IP6_PROTOCOL        *IpInstance;
  IP6_ASSEMBLE_ENTRY  *Assemble;
  NET_BUF             *Packet;
  IP6_CLIP_INFO       *Info;
  UINT32              Index;

  //
  // First, time out the fragments. The packet's life is counting down
  // once the first-arriving fragment of that packet was received.
  //
  for (Index = 0; Index < IP6_ASSEMLE_HASH_SIZE; Index++) {
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &(IpSb->Assemble.Bucket[Index])) {
      Assemble = NET_LIST_USER_STRUCT (Entry, IP6_ASSEMBLE_ENTRY, Link);

      if ((Assemble->Life > 0) && (--Assemble->Life == 0)) {
        //
        // If the first fragment (the one with a Fragment Offset of zero)
        // has been received, an ICMP Time Exceeded - Fragment Reassembly
        // Time Exceeded message should be sent to the source of that fragment.
        //
        if ((Assemble->Packet != NULL) &&
            !IP6_IS_MULTICAST (&Assemble->Head->DestinationAddress))
        {
          Ip6SendIcmpError (
            IpSb,
            Assemble->Packet,
            NULL,
            &Assemble->Head->SourceAddress,
            ICMP_V6_TIME_EXCEEDED,
            ICMP_V6_TIMEOUT_REASSEMBLE,
            NULL
            );
        }

        //
        // If reassembly of a packet is not completed within 60 seconds of
        // the reception of the first-arriving fragment of that packet, the
        // reassembly must be abandoned and all the fragments that have been
        // received for that packet must be discarded.
        //
        RemoveEntryList (Entry);
        Ip6FreeAssembleEntry (Assemble);
      }
    }
  }

  NET_LIST_FOR_EACH (InstanceEntry, &IpSb->Children) {
    IpInstance = NET_LIST_USER_STRUCT (InstanceEntry, IP6_PROTOCOL, Link);

    //
    // Second, time out the assembled packets enqueued on each IP child.
    //
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &IpInstance->Received) {
      Packet = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);
      Info   = IP6_GET_CLIP_INFO (Packet);

      if ((Info->Life > 0) && (--Info->Life == 0)) {
        RemoveEntryList (Entry);
        NetbufFree (Packet);
      }
    }

    //
    // Third: time out the transmitted packets.
    //
    NetMapIterate (&IpInstance->TxTokens, Ip6SentPacketTicking, NULL);
  }
}
