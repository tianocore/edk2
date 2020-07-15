/** @file
  IP4 input process.

Copyright (c) 2005 - 2020, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip4Impl.h"


/**
  Create an empty assemble entry for the packet identified by
  (Dst, Src, Id, Protocol). The default life for the packet is
  120 seconds.

  @param[in]  Dst                    The destination address
  @param[in]  Src                    The source address
  @param[in]  Id                     The ID field in IP header
  @param[in]  Protocol               The protocol field in IP header

  @return NULL if failed to allocate memory for the entry, otherwise
          the point to just created reassemble entry.

**/
IP4_ASSEMBLE_ENTRY *
Ip4CreateAssembleEntry (
  IN IP4_ADDR               Dst,
  IN IP4_ADDR               Src,
  IN UINT16                 Id,
  IN UINT8                  Protocol
  )
{

  IP4_ASSEMBLE_ENTRY        *Assemble;

  Assemble = AllocatePool (sizeof (IP4_ASSEMBLE_ENTRY));

  if (Assemble == NULL) {
    return NULL;
  }

  InitializeListHead (&Assemble->Link);
  InitializeListHead (&Assemble->Fragments);

  Assemble->Dst      = Dst;
  Assemble->Src      = Src;
  Assemble->Id       = Id;
  Assemble->Protocol = Protocol;
  Assemble->TotalLen = 0;
  Assemble->CurLen   = 0;
  Assemble->Head     = NULL;
  Assemble->Info     = NULL;
  Assemble->Life     = IP4_FRAGMENT_LIFE;

  return Assemble;
}


/**
  Release all the fragments of a packet, then free the assemble entry.

  @param[in]  Assemble               The assemble entry to free

**/
VOID
Ip4FreeAssembleEntry (
  IN IP4_ASSEMBLE_ENTRY     *Assemble
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  NET_BUF                   *Fragment;

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Assemble->Fragments) {
    Fragment = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);

    RemoveEntryList (Entry);
    NetbufFree (Fragment);
  }

  FreePool (Assemble);
}


/**
  Initialize an already allocated assemble table. This is generally
  the assemble table embedded in the IP4 service instance.

  @param[in, out]  Table                  The assemble table to initialize.

**/
VOID
Ip4InitAssembleTable (
  IN OUT IP4_ASSEMBLE_TABLE     *Table
  )
{
  UINT32                    Index;

  for (Index = 0; Index < IP4_ASSEMLE_HASH_SIZE; Index++) {
    InitializeListHead (&Table->Bucket[Index]);
  }
}


/**
  Clean up the assemble table: remove all the fragments
  and assemble entries.

  @param[in]  Table                  The assemble table to clean up

**/
VOID
Ip4CleanAssembleTable (
  IN IP4_ASSEMBLE_TABLE     *Table
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP4_ASSEMBLE_ENTRY        *Assemble;
  UINT32                    Index;

  for (Index = 0; Index < IP4_ASSEMLE_HASH_SIZE; Index++) {
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &Table->Bucket[Index]) {
      Assemble = NET_LIST_USER_STRUCT (Entry, IP4_ASSEMBLE_ENTRY, Link);

      RemoveEntryList (Entry);
      Ip4FreeAssembleEntry (Assemble);
    }
  }
}


/**
  Trim the packet to fit in [Start, End), and update the per
  packet information.

  @param  Packet                 Packet to trim
  @param  Start                  The sequence of the first byte to fit in
  @param  End                    One beyond the sequence of last byte to fit in.

**/
VOID
Ip4TrimPacket (
  IN OUT NET_BUF                *Packet,
  IN     INTN                   Start,
  IN     INTN                   End
  )
{
  IP4_CLIP_INFO             *Info;
  INTN                      Len;

  Info = IP4_GET_CLIP_INFO (Packet);

  ASSERT (Info->Start + Info->Length == Info->End);
  ASSERT ((Info->Start < End) && (Start < Info->End));

   if (Info->Start < Start) {
    Len = Start - Info->Start;

    NetbufTrim (Packet, (UINT32) Len, NET_BUF_HEAD);
    Info->Start   = Start;
    Info->Length -= Len;
  }

  if (End < Info->End) {
    Len = End - Info->End;

    NetbufTrim (Packet, (UINT32) Len, NET_BUF_TAIL);
    Info->End     = End;
    Info->Length -= Len;
  }
}


/**
  Release all the fragments of the packet. This is the callback for
  the assembled packet's OnFree. It will free the assemble entry,
  which in turn will free all the fragments of the packet.

  @param[in]  Arg                    The assemble entry to free

**/
VOID
EFIAPI
Ip4OnFreeFragments (
  IN VOID                   *Arg
  )
{
  Ip4FreeAssembleEntry ((IP4_ASSEMBLE_ENTRY *) Arg);
}


/**
  Reassemble the IP fragments. If all the fragments of the packet
  have been received, it will wrap the packet in a net buffer then
  return it to caller. If the packet can't be assembled, NULL is
  return.

  @param  Table     The assemble table used. New assemble entry will be created
                    if the Packet is from a new chain of fragments.
  @param  Packet    The fragment to assemble. It might be freed if the fragment
                    can't be re-assembled.

  @return NULL if the packet can't be reassemble. The point to just assembled
          packet if all the fragments of the packet have arrived.

**/
NET_BUF *
Ip4Reassemble (
  IN OUT IP4_ASSEMBLE_TABLE     *Table,
  IN OUT NET_BUF                *Packet
  )
{
  IP4_HEAD                  *IpHead;
  IP4_CLIP_INFO             *This;
  IP4_CLIP_INFO             *Node;
  IP4_ASSEMBLE_ENTRY        *Assemble;
  LIST_ENTRY                *Head;
  LIST_ENTRY                *Prev;
  LIST_ENTRY                *Cur;
  NET_BUF                   *Fragment;
  NET_BUF                   *NewPacket;
  INTN                      Index;

  IpHead  = Packet->Ip.Ip4;
  This    = IP4_GET_CLIP_INFO (Packet);

  ASSERT (IpHead != NULL);

  //
  // First: find the related assemble entry
  //
  Assemble  = NULL;
  Index     = IP4_ASSEMBLE_HASH (IpHead->Dst, IpHead->Src, IpHead->Id, IpHead->Protocol);

  NET_LIST_FOR_EACH (Cur, &Table->Bucket[Index]) {
    Assemble = NET_LIST_USER_STRUCT (Cur, IP4_ASSEMBLE_ENTRY, Link);

    if ((Assemble->Dst == IpHead->Dst) && (Assemble->Src == IpHead->Src) &&
        (Assemble->Id == IpHead->Id)   && (Assemble->Protocol == IpHead->Protocol)) {
      break;
    }
  }

  //
  // Create a new assemble entry if no assemble entry is related to this packet
  //
  if (Cur == &Table->Bucket[Index]) {
    Assemble = Ip4CreateAssembleEntry (
                 IpHead->Dst,
                 IpHead->Src,
                 IpHead->Id,
                 IpHead->Protocol
                 );

    if (Assemble == NULL) {
      goto DROP;
    }

    InsertHeadList (&Table->Bucket[Index], &Assemble->Link);
  }
  //
  // Assemble shouldn't be NULL here
  //
  ASSERT (Assemble != NULL);

  //
  // Find the point to insert the packet: before the first
  // fragment with THIS.Start < CUR.Start. the previous one
  // has PREV.Start <= THIS.Start < CUR.Start.
  //
  Head = &Assemble->Fragments;

  NET_LIST_FOR_EACH (Cur, Head) {
    Fragment = NET_LIST_USER_STRUCT (Cur, NET_BUF, List);

    if (This->Start < IP4_GET_CLIP_INFO (Fragment)->Start) {
      break;
    }
  }

  //
  // Check whether the current fragment overlaps with the previous one.
  // It holds that: PREV.Start <= THIS.Start < THIS.End. Only need to
  // check whether THIS.Start < PREV.End for overlap. If two fragments
  // overlaps, trim the overlapped part off THIS fragment.
  //
  if ((Prev = Cur->BackLink) != Head) {
    Fragment  = NET_LIST_USER_STRUCT (Prev, NET_BUF, List);
    Node      = IP4_GET_CLIP_INFO (Fragment);

    if (This->Start < Node->End) {
      if (This->End <= Node->End) {
        NetbufFree (Packet);
        return NULL;
      }

      Ip4TrimPacket (Packet, Node->End, This->End);
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
  while (Cur != Head) {
    Fragment = NET_LIST_USER_STRUCT (Cur, NET_BUF, List);
    Node     = IP4_GET_CLIP_INFO (Fragment);

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
        goto DROP;
      }

      Ip4TrimPacket (Packet, This->Start, Node->Start);
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
    ASSERT (Assemble->Head == NULL);

    Assemble->Head  = IpHead;
    Assemble->Info  = IP4_GET_CLIP_INFO (Packet);
  }

  //
  // Don't update the length more than once.
  //
  if (IP4_LAST_FRAGMENT (IpHead->Fragment) && (Assemble->TotalLen == 0)) {
    Assemble->TotalLen = This->End;
  }

  //
  // Deliver the whole packet if all the fragments received.
  // All fragments received if:
  //  1. received the last one, so, the total length is know
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
    Fragment = NET_LIST_USER_STRUCT (Head->BackLink, NET_BUF, List);

    if (IP4_GET_CLIP_INFO (Fragment)->End != Assemble->TotalLen) {
      Ip4FreeAssembleEntry (Assemble);
      return NULL;
    }

    //
    // Wrap the packet in a net buffer then deliver it up
    //
    NewPacket = NetbufFromBufList (
                  &Assemble->Fragments,
                  0,
                  0,
                  Ip4OnFreeFragments,
                  Assemble
                  );

    if (NewPacket == NULL) {
      Ip4FreeAssembleEntry (Assemble);
      return NULL;
    }

    NewPacket->Ip.Ip4 = Assemble->Head;

    ASSERT (Assemble->Info != NULL);

    CopyMem (
      IP4_GET_CLIP_INFO (NewPacket),
      Assemble->Info,
      sizeof (*IP4_GET_CLIP_INFO (NewPacket))
      );

    return NewPacket;
  }

  return NULL;

DROP:
  NetbufFree (Packet);
  return NULL;
}

/**
  The callback function for the net buffer which wraps the packet processed by
  IPsec. It releases the wrap packet and also signals IPsec to free the resources.

  @param[in]  Arg       The wrap context

**/
VOID
EFIAPI
Ip4IpSecFree (
  IN VOID                   *Arg
  )
{
  IP4_IPSEC_WRAP            *Wrap;

  Wrap = (IP4_IPSEC_WRAP *) Arg;

  if (Wrap->IpSecRecycleSignal != NULL) {
    gBS->SignalEvent (Wrap->IpSecRecycleSignal);
  }

  NetbufFree (Wrap->Packet);

  FreePool (Wrap);

  return;
}

/**
  The work function to locate IPsec protocol to process the inbound or
  outbound IP packets. The process routine handls the packet with following
  actions: bypass the packet, discard the packet, or protect the packet.

  @param[in]       IpSb          The IP4 service instance.
  @param[in, out]  Head          The caller supplied IP4 header.
  @param[in, out]  Netbuf        The IP4 packet to be processed by IPsec.
  @param[in, out]  Options       The caller supplied options.
  @param[in, out]  OptionsLen    The length of the option.
  @param[in]       Direction     The directionality in an SPD entry,
                                 EfiIPsecInBound or EfiIPsecOutBound.
  @param[in]       Context       The token's wrap.

  @retval EFI_SUCCESS            The IPsec protocol is not available or disabled.
  @retval EFI_SUCCESS            The packet was bypassed and all buffers remain the same.
  @retval EFI_SUCCESS            The packet was protected.
  @retval EFI_ACCESS_DENIED      The packet was discarded.
  @retval EFI_OUT_OF_RESOURCES   There is no sufficient resource to complete the operation.
  @retval EFI_BUFFER_TOO_SMALL   The number of non-empty block is bigger than the
                                 number of input data blocks when build a fragment table.

**/
EFI_STATUS
Ip4IpSecProcessPacket (
  IN     IP4_SERVICE            *IpSb,
  IN OUT IP4_HEAD               **Head,
  IN OUT NET_BUF                **Netbuf,
  IN OUT UINT8                  **Options,
  IN OUT UINT32                 *OptionsLen,
  IN     EFI_IPSEC_TRAFFIC_DIR  Direction,
  IN     VOID                   *Context
  )
{
  NET_FRAGMENT              *FragmentTable;
  NET_FRAGMENT              *OriginalFragmentTable;
  UINT32                    FragmentCount;
  UINT32                    OriginalFragmentCount;
  EFI_EVENT                 RecycleEvent;
  NET_BUF                   *Packet;
  IP4_TXTOKEN_WRAP          *TxWrap;
  IP4_IPSEC_WRAP            *IpSecWrap;
  EFI_STATUS                Status;
  IP4_HEAD                  ZeroHead;

  Status        = EFI_SUCCESS;

  if (!mIpSec2Installed) {
    goto ON_EXIT;
  }
  ASSERT (mIpSec != NULL);

  Packet        = *Netbuf;
  RecycleEvent  = NULL;
  IpSecWrap     = NULL;
  FragmentTable = NULL;
  TxWrap        = (IP4_TXTOKEN_WRAP *) Context;
  FragmentCount = Packet->BlockOpNum;

  ZeroMem (&ZeroHead, sizeof (IP4_HEAD));

  //
  // Check whether the IPsec enable variable is set.
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
    IpSb->MaxPacketSize = IpSb->OldMaxPacketSize - IP4_MAX_IPSEC_HEADLEN;
  }

  //
  // Rebuild fragment table from netbuf to ease IPsec process.
  //
  FragmentTable = AllocateZeroPool (FragmentCount * sizeof (NET_FRAGMENT));

  if (FragmentTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = NetbufBuildExt (Packet, FragmentTable, &FragmentCount);

  //
  // Record the original FragmentTable and count.
  //
  OriginalFragmentTable = FragmentTable;
  OriginalFragmentCount = FragmentCount;

  if (EFI_ERROR (Status)) {
    FreePool (FragmentTable);
    goto ON_EXIT;
  }

  //
  // Convert host byte order to network byte order
  //
  Ip4NtohHead (*Head);

  Status = mIpSec->ProcessExt (
                     mIpSec,
                     IpSb->Controller,
                     IP_VERSION_4,
                     (VOID *) (*Head),
                     &(*Head)->Protocol,
                     (VOID **) Options,
                     OptionsLen,
                     (EFI_IPSEC_FRAGMENT_DATA **) (&FragmentTable),
                     &FragmentCount,
                     Direction,
                     &RecycleEvent
                     );
  //
  // Convert back to host byte order
  //
  Ip4NtohHead (*Head);

  if (EFI_ERROR (Status)) {
    FreePool (OriginalFragmentTable);
    goto ON_EXIT;
  }

  if (OriginalFragmentTable == FragmentTable && OriginalFragmentCount == FragmentCount) {
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

  if (Direction == EfiIPsecOutBound && TxWrap != NULL) {

    TxWrap->IpSecRecycleSignal = RecycleEvent;
    TxWrap->Packet             = NetbufFromExt (
                                   FragmentTable,
                                   FragmentCount,
                                   IP4_MAX_HEADLEN,
                                   0,
                                   Ip4FreeTxToken,
                                   TxWrap
                                   );
    if (TxWrap->Packet == NULL) {
      //
      // Recover the TxWrap->Packet, if meet a error, and the caller will free
      // the TxWrap.
      //
      TxWrap->Packet = *Netbuf;
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    //
    // Free original Netbuf.
    //
    NetIpSecNetbufFree (*Netbuf);
    *Netbuf = TxWrap->Packet;

  } else {

    IpSecWrap = AllocateZeroPool (sizeof (IP4_IPSEC_WRAP));

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
                                      IP4_MAX_HEADLEN,
                                      0,
                                      Ip4IpSecFree,
                                      IpSecWrap
                                      );

    if (Packet == NULL) {
      Packet = IpSecWrap->Packet;
      gBS->SignalEvent (RecycleEvent);
      FreePool (IpSecWrap);
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    if (Direction == EfiIPsecInBound && 0 != CompareMem (*Head, &ZeroHead, sizeof (IP4_HEAD))) {
      Ip4PrependHead (Packet, *Head, *Options, *OptionsLen);
      Ip4NtohHead (Packet->Ip.Ip4);
      NetbufTrim (Packet, ((*Head)->HeadLen << 2), TRUE);

      CopyMem (
        IP4_GET_CLIP_INFO (Packet),
        IP4_GET_CLIP_INFO (IpSecWrap->Packet),
        sizeof (IP4_CLIP_INFO)
        );
    }
    *Netbuf = Packet;
  }

ON_EXIT:
  return Status;
}

/**
  Pre-process the IPv4 packet. First validates the IPv4 packet, and
  then reassembles packet if it is necessary.

  @param[in]       IpSb            Pointer to IP4_SERVICE.
  @param[in, out]  Packet          Pointer to the Packet to be processed.
  @param[in]       Head            Pointer to the IP4_HEAD.
  @param[in]       Option          Pointer to a buffer which contains the IPv4 option.
  @param[in]       OptionLen       The length of Option in bytes.
  @param[in]       Flag            The link layer flag for the packet received, such
                                   as multicast.

  @retval     EFI_SUCCESS                The received packet is in well form.
  @retval     EFI_INVALID_PARAMETER      The received packet is malformed.

**/
EFI_STATUS
Ip4PreProcessPacket (
  IN     IP4_SERVICE    *IpSb,
  IN OUT NET_BUF        **Packet,
  IN     IP4_HEAD       *Head,
  IN     UINT8          *Option,
  IN     UINT32         OptionLen,
  IN     UINT32         Flag
  )
{
  IP4_CLIP_INFO             *Info;
  UINT32                    HeadLen;
  UINT32                    TotalLen;
  UINT16                    Checksum;

  //
  // Check if the IP4 header is correctly formatted.
  //
  HeadLen  = (Head->HeadLen << 2);
  TotalLen = NTOHS (Head->TotalLen);

  //
  // Mnp may deliver frame trailer sequence up, trim it off.
  //
  if (TotalLen < (*Packet)->TotalSize) {
    NetbufTrim (*Packet, (*Packet)->TotalSize - TotalLen, FALSE);
  }

  if ((Head->Ver != 4) || (HeadLen < IP4_MIN_HEADLEN) ||
      (TotalLen < HeadLen) || (TotalLen != (*Packet)->TotalSize)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Some OS may send IP packets without checksum.
  //
  Checksum = (UINT16) (~NetblockChecksum ((UINT8 *) Head, HeadLen));

  if ((Head->Checksum != 0) && (Checksum != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Convert the IP header to host byte order, then get the per packet info.
  //
  (*Packet)->Ip.Ip4  = Ip4NtohHead (Head);

  Info            = IP4_GET_CLIP_INFO (*Packet);
  Info->LinkFlag  = Flag;
  Info->CastType  = Ip4GetHostCast (IpSb, Head->Dst, Head->Src);
  Info->Start     = (Head->Fragment & IP4_HEAD_OFFSET_MASK) << 3;
  Info->Length    = Head->TotalLen - HeadLen;
  Info->End       = Info->Start + Info->Length;
  Info->Status    = EFI_SUCCESS;

  //
  // The packet is destinated to us if the CastType is non-zero.
  //
  if ((Info->CastType == 0) || (Info->End > IP4_MAX_PACKET_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Validate the options. Don't call the Ip4OptionIsValid if
  // there is no option to save some CPU process.
  //

  if ((OptionLen > 0) && !Ip4OptionIsValid (Option, OptionLen, TRUE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Trim the head off, after this point, the packet is headless,
  // and Packet->TotalLen == Info->Length.
  //
  NetbufTrim (*Packet, HeadLen, TRUE);

  //
  // Reassemble the packet if this is a fragment. The packet is a
  // fragment if its head has MF (more fragment) set, or it starts
  // at non-zero byte.
  //
  if (((Head->Fragment & IP4_HEAD_MF_MASK) != 0) || (Info->Start != 0)) {
    //
    // Drop the fragment if DF is set but it is fragmented. Gateway
    // need to send a type 4 destination unreache ICMP message here.
    //
    if ((Head->Fragment & IP4_HEAD_DF_MASK) != 0) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // The length of all but the last fragments is in the unit of 8 bytes.
    //
    if (((Head->Fragment & IP4_HEAD_MF_MASK) != 0) && (Info->Length % 8 != 0)) {
      return EFI_INVALID_PARAMETER;
    }

    *Packet = Ip4Reassemble (&IpSb->Assemble, *Packet);

    //
    // Packet assembly isn't complete, start receive more packet.
    //
    if (*Packet == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function checks the IPv4 packet length.

  @param[in]       Packet          Pointer to the IPv4 Packet to be checked.

  @retval TRUE                   The input IPv4 packet length is valid.
  @retval FALSE                  The input IPv4 packet length is invalid.

**/
BOOLEAN
Ip4IsValidPacketLength (
  IN NET_BUF        *Packet
  )
{
  //
  // Check the IP4 packet length.
  //
  if (Packet->TotalSize < IP4_MIN_HEADLEN) {
    return FALSE;
  }

  return TRUE;
}

/**
  The IP4 input routine. It is called by the IP4_INTERFACE when a
  IP4 fragment is received from MNP.

  @param[in]  Ip4Instance        The IP4 child that request the receive, most like
                                 it is NULL.
  @param[in]  Packet             The IP4 packet received.
  @param[in]  IoStatus           The return status of receive request.
  @param[in]  Flag               The link layer flag for the packet received, such
                                 as multicast.
  @param[in]  Context            The IP4 service instance that own the MNP.

**/
VOID
Ip4AccpetFrame (
  IN IP4_PROTOCOL           *Ip4Instance,
  IN NET_BUF                *Packet,
  IN EFI_STATUS             IoStatus,
  IN UINT32                 Flag,
  IN VOID                   *Context
  )
{
  IP4_SERVICE               *IpSb;
  IP4_HEAD                  *Head;
  EFI_STATUS                Status;
  IP4_HEAD                  ZeroHead;
  UINT8                     *Option;
  UINT32                    OptionLen;

  IpSb   = (IP4_SERVICE *) Context;
  Option = NULL;

  if (EFI_ERROR (IoStatus) || (IpSb->State == IP4_SERVICE_DESTROY)) {
    goto DROP;
  }

  if (!Ip4IsValidPacketLength (Packet)) {
    goto RESTART;
  }

  Head      = (IP4_HEAD *) NetbufGetByte (Packet, 0, NULL);
  ASSERT (Head != NULL);
  OptionLen = (Head->HeadLen << 2) - IP4_MIN_HEADLEN;
  if (OptionLen > 0) {
    Option = (UINT8 *) (Head + 1);
  }

  //
  // Validate packet format and reassemble packet if it is necessary.
  //
  Status = Ip4PreProcessPacket (
             IpSb,
             &Packet,
             Head,
             Option,
             OptionLen,
             Flag
             );

  if (EFI_ERROR (Status)) {
    goto RESTART;
  }

  //
  // After trim off, the packet is a esp/ah/udp/tcp/icmp6 net buffer,
  // and no need consider any other ahead ext headers.
  //
  Status = Ip4IpSecProcessPacket (
             IpSb,
             &Head,
             &Packet,
             &Option,
             &OptionLen,
             EfiIPsecInBound,
             NULL
             );

  if (EFI_ERROR (Status)) {
    goto RESTART;
  }

  //
  // If the packet is protected by tunnel mode, parse the inner Ip Packet.
  //
  ZeroMem (&ZeroHead, sizeof (IP4_HEAD));
  if (0 == CompareMem (Head, &ZeroHead, sizeof (IP4_HEAD))) {
    // Packet may have been changed. Head, HeadLen, TotalLen, and
    // info must be reloaded before use. The ownership of the packet
    // is transferred to the packet process logic.
    //
    if (!Ip4IsValidPacketLength (Packet)) {
      goto RESTART;
    }

    Head = (IP4_HEAD *) NetbufGetByte (Packet, 0, NULL);
    ASSERT (Head != NULL);
    Status = Ip4PreProcessPacket (
               IpSb,
               &Packet,
               Head,
               Option,
               OptionLen,
               Flag
               );
    if (EFI_ERROR (Status)) {
      goto RESTART;
    }
  }

  ASSERT (Packet != NULL);
  Head  = Packet->Ip.Ip4;
  IP4_GET_CLIP_INFO (Packet)->Status = EFI_SUCCESS;

  switch (Head->Protocol) {
  case EFI_IP_PROTO_ICMP:
    Ip4IcmpHandle (IpSb, Head, Packet);
    break;

  case IP4_PROTO_IGMP:
    Ip4IgmpHandle (IpSb, Head, Packet);
    break;

  default:
    Ip4Demultiplex (IpSb, Head, Packet, Option, OptionLen);
  }

  Packet = NULL;

  //
  // Dispatch the DPCs queued by the NotifyFunction of the rx token's events
  // which are signaled with received data.
  //
  DispatchDpc ();

RESTART:
  Ip4ReceiveFrame (IpSb->DefaultInterface, NULL, Ip4AccpetFrame, IpSb);

DROP:
  if (Packet != NULL) {
    NetbufFree (Packet);
  }

  return ;
}


/**
  Check whether this IP child accepts the packet.

  @param[in]  IpInstance             The IP child to check
  @param[in]  Head                   The IP header of the packet
  @param[in]  Packet                 The data of the packet

  @retval TRUE   If the child wants to receive the packet.
  @retval FALSE  Otherwise.

**/
BOOLEAN
Ip4InstanceFrameAcceptable (
  IN IP4_PROTOCOL           *IpInstance,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  )
{
  IP4_ICMP_ERROR_HEAD       Icmp;
  EFI_IP4_CONFIG_DATA       *Config;
  IP4_CLIP_INFO             *Info;
  UINT16                    Proto;
  UINT32                    Index;

  Config = &IpInstance->ConfigData;

  //
  // Dirty trick for the Tiano UEFI network stack implementation. If
  // ReceiveTimeout == -1, the receive of the packet for this instance
  // is disabled. The UEFI spec don't have such capability. We add
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
  // Use protocol from the IP header embedded in the ICMP error
  // message to filter, instead of ICMP itself. ICMP handle will
  // call Ip4Demultiplex to deliver ICMP errors.
  //
  Proto = Head->Protocol;

  if ((Proto == EFI_IP_PROTO_ICMP) && (!Config->AcceptAnyProtocol) && (Proto != Config->DefaultProtocol)) {
    NetbufCopy (Packet, 0, sizeof (Icmp.Head), (UINT8 *) &Icmp.Head);

    if (mIcmpClass[Icmp.Head.Type].IcmpClass == ICMP_ERROR_MESSAGE) {
      if (!Config->AcceptIcmpErrors) {
        return FALSE;
      }

      NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);
      Proto = Icmp.IpHead.Protocol;
    }
  }

  //
  // Match the protocol
  //
  if (!Config->AcceptAnyProtocol && (Proto != Config->DefaultProtocol)) {
    return FALSE;
  }

  //
  // Check for broadcast, the caller has computed the packet's
  // cast type for this child's interface.
  //
  Info = IP4_GET_CLIP_INFO (Packet);

  if (IP4_IS_BROADCAST (Info->CastType)) {
    return Config->AcceptBroadcast;
  }

  //
  // If it is a multicast packet, check whether we are in the group.
  //
  if (Info->CastType == IP4_MULTICAST) {
    //
    // Receive the multicast if the instance wants to receive all packets.
    //
    if (!IpInstance->ConfigData.UseDefaultAddress && (IpInstance->Interface->Ip == 0)) {
      return TRUE;
    }

    for (Index = 0; Index < IpInstance->GroupCount; Index++) {
      if (IpInstance->Groups[Index] == HTONL (Head->Dst)) {
        break;
      }
    }

    return (BOOLEAN)(Index < IpInstance->GroupCount);
  }

  return TRUE;
}


/**
  Enqueue a shared copy of the packet to the IP4 child if the
  packet is acceptable to it. Here the data of the packet is
  shared, but the net buffer isn't.

  @param[in]  IpInstance             The IP4 child to enqueue the packet to
  @param[in]  Head                   The IP header of the received packet
  @param[in]  Packet                 The data of the received packet

  @retval EFI_NOT_STARTED        The IP child hasn't been configured.
  @retval EFI_INVALID_PARAMETER  The child doesn't want to receive the packet
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate some resource
  @retval EFI_SUCCESS            A shared copy the packet is enqueued to the child.

**/
EFI_STATUS
Ip4InstanceEnquePacket (
  IN IP4_PROTOCOL           *IpInstance,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  )
{
  IP4_CLIP_INFO             *Info;
  NET_BUF                   *Clone;

  //
  // Check whether the packet is acceptable to this instance.
  //
  if (IpInstance->State != IP4_STATE_CONFIGED) {
    return EFI_NOT_STARTED;
  }

  if (!Ip4InstanceFrameAcceptable (IpInstance, Head, Packet)) {
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
  Info        = IP4_GET_CLIP_INFO (Clone);
  Info->Life  = IP4_US_TO_SEC (IpInstance->ConfigData.ReceiveTimeout);

  InsertTailList (&IpInstance->Received, &Clone->List);
  return EFI_SUCCESS;
}


/**
  The signal handle of IP4's recycle event. It is called back
  when the upper layer release the packet.

  @param  Event              The IP4's recycle event.
  @param  Context            The context of the handle, which is a
                             IP4_RXDATA_WRAP

**/
VOID
EFIAPI
Ip4OnRecyclePacket (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  IP4_RXDATA_WRAP           *Wrap;

  Wrap = (IP4_RXDATA_WRAP *) Context;

  EfiAcquireLockOrFail (&Wrap->IpInstance->RecycleLock);
  RemoveEntryList (&Wrap->Link);
  EfiReleaseLock (&Wrap->IpInstance->RecycleLock);

  ASSERT (!NET_BUF_SHARED (Wrap->Packet));
  NetbufFree (Wrap->Packet);

  gBS->CloseEvent (Wrap->RxData.RecycleSignal);
  FreePool (Wrap);
}


/**
  Wrap the received packet to a IP4_RXDATA_WRAP, which will be
  delivered to the upper layer. Each IP4 child that accepts the
  packet will get a not-shared copy of the packet which is wrapped
  in the IP4_RXDATA_WRAP. The IP4_RXDATA_WRAP->RxData is passed
  to the upper layer. Upper layer will signal the recycle event in
  it when it is done with the packet.

  @param[in]  IpInstance    The IP4 child to receive the packet.
  @param[in]  Packet        The packet to deliver up.

  @retval Wrap              if warp the packet succeed.
  @retval NULL              failed to wrap the packet .

**/
IP4_RXDATA_WRAP *
Ip4WrapRxData (
  IN IP4_PROTOCOL           *IpInstance,
  IN NET_BUF                *Packet
  )
{
  IP4_RXDATA_WRAP           *Wrap;
  EFI_IP4_RECEIVE_DATA      *RxData;
  EFI_STATUS                Status;
  BOOLEAN                   RawData;

  Wrap = AllocatePool (IP4_RXDATA_WRAP_SIZE (Packet->BlockOpNum));

  if (Wrap == NULL) {
    return NULL;
  }

  InitializeListHead (&Wrap->Link);

  Wrap->IpInstance  = IpInstance;
  Wrap->Packet      = Packet;
  RxData            = &Wrap->RxData;

  ZeroMem (RxData, sizeof (EFI_IP4_RECEIVE_DATA));

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Ip4OnRecyclePacket,
                  Wrap,
                  &RxData->RecycleSignal
                  );

  if (EFI_ERROR (Status)) {
    FreePool (Wrap);
    return NULL;
  }

  ASSERT (Packet->Ip.Ip4 != NULL);

  ASSERT (IpInstance != NULL);
  RawData = IpInstance->ConfigData.RawData;

  //
  // The application expects a network byte order header.
  //
  if (!RawData) {
    RxData->HeaderLength  = (Packet->Ip.Ip4->HeadLen << 2);
    RxData->Header        = (EFI_IP4_HEADER *) Ip4NtohHead (Packet->Ip.Ip4);
    RxData->OptionsLength = RxData->HeaderLength - IP4_MIN_HEADLEN;
    RxData->Options       = NULL;

    if (RxData->OptionsLength != 0) {
      RxData->Options = (VOID *) (RxData->Header + 1);
    }
  }

  RxData->DataLength  = Packet->TotalSize;

  //
  // Build the fragment table to be delivered up.
  //
  RxData->FragmentCount = Packet->BlockOpNum;
  NetbufBuildExt (Packet, (NET_FRAGMENT *) RxData->FragmentTable, &RxData->FragmentCount);

  return Wrap;
}


/**
  Deliver the received packets to upper layer if there are both received
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
Ip4InstanceDeliverPacket (
  IN IP4_PROTOCOL           *IpInstance
  )
{
  EFI_IP4_COMPLETION_TOKEN  *Token;
  IP4_RXDATA_WRAP           *Wrap;
  NET_BUF                   *Packet;
  NET_BUF                   *Dup;
  UINT8                     *Head;
  UINT32                    HeadLen;

  //
  // Deliver a packet if there are both a packet and a receive token.
  //
  while (!IsListEmpty (&IpInstance->Received) &&
         !NetMapIsEmpty (&IpInstance->RxTokens)) {

    Packet = NET_LIST_HEAD (&IpInstance->Received, NET_BUF, List);

    if (!NET_BUF_SHARED (Packet)) {
      //
      // If this is the only instance that wants the packet, wrap it up.
      //
      Wrap = Ip4WrapRxData (IpInstance, Packet);

      if (Wrap == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      RemoveEntryList (&Packet->List);

    } else {
      //
      // Create a duplicated packet if this packet is shared
      //
      if (IpInstance->ConfigData.RawData) {
        HeadLen = 0;
      } else {
        HeadLen = IP4_MAX_HEADLEN;
      }

      Dup = NetbufDuplicate (Packet, NULL, HeadLen);

      if (Dup == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      if (!IpInstance->ConfigData.RawData) {
        //
        // Copy the IP head over. The packet to deliver up is
        // headless. Trim the head off after copy. The IP head
        // may be not continuous before the data.
        //
        Head = NetbufAllocSpace (Dup, IP4_MAX_HEADLEN, NET_BUF_HEAD);
        ASSERT (Head != NULL);

        Dup->Ip.Ip4 = (IP4_HEAD *) Head;

        CopyMem (Head, Packet->Ip.Ip4, Packet->Ip.Ip4->HeadLen << 2);
        NetbufTrim (Dup, IP4_MAX_HEADLEN, TRUE);
      }

      Wrap = Ip4WrapRxData (IpInstance, Dup);

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
    Token->Status        = IP4_GET_CLIP_INFO (Packet)->Status;
    Token->Packet.RxData = &Wrap->RxData;

    gBS->SignalEvent (Token->Event);
  }

  return EFI_SUCCESS;
}


/**
  Enqueue a received packet to all the IP children that share
  the same interface.

  @param[in]  IpSb               The IP4 service instance that receive the packet.
  @param[in]  Head               The header of the received packet.
  @param[in]  Packet             The data of the received packet.
  @param[in]  Option             Point to the IP4 packet header options.
  @param[in]  OptionLen          Length of the IP4 packet header options.
  @param[in]  IpIf               The interface to enqueue the packet to.

  @return The number of the IP4 children that accepts the packet

**/
INTN
Ip4InterfaceEnquePacket (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet,
  IN UINT8                  *Option,
  IN UINT32                 OptionLen,
  IN IP4_INTERFACE          *IpIf
  )
{
  IP4_PROTOCOL              *IpInstance;
  IP4_CLIP_INFO             *Info;
  LIST_ENTRY                *Entry;
  INTN                      Enqueued;
  INTN                      LocalType;
  INTN                      SavedType;

  //
  // First, check that the packet is acceptable to this interface
  // and find the local cast type for the interface. A packet sent
  // to say 192.168.1.1 should NOT be deliver to 10.0.0.1 unless
  // promiscuous receiving.
  //
  LocalType = 0;
  Info      = IP4_GET_CLIP_INFO (Packet);

  if ((Info->CastType == IP4_MULTICAST) || (Info->CastType == IP4_LOCAL_BROADCAST)) {
    //
    // If the CastType is multicast, don't need to filter against
    // the group address here, Ip4InstanceFrameAcceptable will do
    // that later.
    //
    LocalType = Info->CastType;

  } else {
    //
    // Check the destination against local IP. If the station
    // address is 0.0.0.0, it means receiving all the IP destined
    // to local non-zero IP. Otherwise, it is necessary to compare
    // the destination to the interface's IP address.
    //
    if (IpIf->Ip == IP4_ALLZERO_ADDRESS) {
      LocalType = IP4_LOCAL_HOST;

    } else {
      LocalType = Ip4GetNetCast (Head->Dst, IpIf);

      if ((LocalType == 0) && IpIf->PromiscRecv) {
        LocalType = IP4_PROMISCUOUS;
      }
    }
  }

  if (LocalType == 0) {
    return 0;
  }

  //
  // Iterate through the ip instances on the interface, enqueue
  // the packet if filter passed. Save the original cast type,
  // and pass the local cast type to the IP children on the
  // interface. The global cast type will be restored later.
  //
  SavedType       = Info->CastType;
  Info->CastType  = LocalType;

  Enqueued        = 0;

  NET_LIST_FOR_EACH (Entry, &IpIf->IpInstances) {
    IpInstance = NET_LIST_USER_STRUCT (Entry, IP4_PROTOCOL, AddrLink);
    NET_CHECK_SIGNATURE (IpInstance, IP4_PROTOCOL_SIGNATURE);

    //
    // In RawData mode, add IPv4 headers and options back to packet.
    //
    if ((IpInstance->ConfigData.RawData) && (Option != NULL) && (OptionLen != 0)){
      Ip4PrependHead (Packet, Head, Option, OptionLen);
    }

    if (Ip4InstanceEnquePacket (IpInstance, Head, Packet) == EFI_SUCCESS) {
      Enqueued++;
    }
  }

  Info->CastType = SavedType;
  return Enqueued;
}


/**
  Deliver the packet for each IP4 child on the interface.

  @param[in]  IpSb               The IP4 service instance that received the packet
  @param[in]  IpIf               The IP4 interface to deliver the packet.

  @retval EFI_SUCCESS            It always returns EFI_SUCCESS now

**/
EFI_STATUS
Ip4InterfaceDeliverPacket (
  IN IP4_SERVICE            *IpSb,
  IN IP4_INTERFACE          *IpIf
  )
{
  IP4_PROTOCOL              *Ip4Instance;
  LIST_ENTRY                *Entry;

  NET_LIST_FOR_EACH (Entry, &IpIf->IpInstances) {
    Ip4Instance = NET_LIST_USER_STRUCT (Entry, IP4_PROTOCOL, AddrLink);
    Ip4InstanceDeliverPacket (Ip4Instance);
  }

  return EFI_SUCCESS;
}


/**
  Demultiple the packet. the packet delivery is processed in two
  passes. The first pass will enqueue a shared copy of the packet
  to each IP4 child that accepts the packet. The second pass will
  deliver a non-shared copy of the packet to each IP4 child that
  has pending receive requests. Data is copied if more than one
  child wants to consume the packet because each IP child needs
  its own copy of the packet to make changes.

  @param[in]  IpSb               The IP4 service instance that received the packet.
  @param[in]  Head               The header of the received packet.
  @param[in]  Packet             The data of the received packet.
  @param[in]  Option             Point to the IP4 packet header options.
  @param[in]  OptionLen          Length of the IP4 packet header options.

  @retval EFI_NOT_FOUND          No IP child accepts the packet.
  @retval EFI_SUCCESS            The packet is enqueued or delivered to some IP
                                 children.

**/
EFI_STATUS
Ip4Demultiplex (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet,
  IN UINT8                  *Option,
  IN UINT32                 OptionLen
  )
{
  LIST_ENTRY                *Entry;
  IP4_INTERFACE             *IpIf;
  INTN                      Enqueued;

  //
  // Two pass delivery: first, enqueue a shared copy of the packet
  // to each instance that accept the packet.
  //
  Enqueued = 0;

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP4_INTERFACE, Link);

    if (IpIf->Configured) {
      Enqueued += Ip4InterfaceEnquePacket (
                    IpSb,
                    Head,
                    Packet,
                    Option,
                    OptionLen,
                    IpIf
                    );
    }
  }

  //
  // Second: deliver a duplicate of the packet to each instance.
  // Release the local reference first, so that the last instance
  // getting the packet will not copy the data.
  //
  NetbufFree (Packet);

  if (Enqueued == 0) {
    return EFI_NOT_FOUND;
  }

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT (Entry, IP4_INTERFACE, Link);

    if (IpIf->Configured) {
      Ip4InterfaceDeliverPacket (IpSb, IpIf);
    }
  }

  return EFI_SUCCESS;
}


/**
  Timeout the fragment and enqueued packets.

  @param[in]  IpSb                   The IP4 service instance to timeout

**/
VOID
Ip4PacketTimerTicking (
  IN IP4_SERVICE            *IpSb
  )
{
  LIST_ENTRY                *InstanceEntry;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IP4_PROTOCOL              *IpInstance;
  IP4_ASSEMBLE_ENTRY        *Assemble;
  NET_BUF                   *Packet;
  IP4_CLIP_INFO             *Info;
  UINT32                    Index;

  //
  // First, time out the fragments. The packet's life is counting down
  // once the first-arrived fragment was received.
  //
  for (Index = 0; Index < IP4_ASSEMLE_HASH_SIZE; Index++) {
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &IpSb->Assemble.Bucket[Index]) {
      Assemble = NET_LIST_USER_STRUCT (Entry, IP4_ASSEMBLE_ENTRY, Link);

      if ((Assemble->Life > 0) && (--Assemble->Life == 0)) {
        RemoveEntryList (Entry);
        Ip4FreeAssembleEntry (Assemble);
      }
    }
  }

  NET_LIST_FOR_EACH (InstanceEntry, &IpSb->Children) {
    IpInstance = NET_LIST_USER_STRUCT (InstanceEntry, IP4_PROTOCOL, Link);

    //
    // Second, time out the assembled packets enqueued on each IP child.
    //
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &IpInstance->Received) {
      Packet = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);
      Info   = IP4_GET_CLIP_INFO (Packet);

      if ((Info->Life > 0) && (--Info->Life == 0)) {
        RemoveEntryList (Entry);
        NetbufFree (Packet);
      }
    }

    //
    // Third: time out the transmitted packets.
    //
    NetMapIterate (&IpInstance->TxTokens, Ip4SentPacketTicking, NULL);
  }
}
