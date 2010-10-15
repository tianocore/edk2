/** @file
  Network library functions providing net buffer operation support.

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/NetLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>


/**
  Allocate and build up the sketch for a NET_BUF.

  The net buffer allocated has the BlockOpNum's NET_BLOCK_OP, and its associated
  NET_VECTOR has the BlockNum's NET_BLOCK. But all the NET_BLOCK_OP and
  NET_BLOCK remain un-initialized.

  @param[in]  BlockNum       The number of NET_BLOCK in the vector of net buffer
  @param[in]  BlockOpNum     The number of NET_BLOCK_OP in the net buffer

  @return                    Pointer to the allocated NET_BUF, or NULL if the
                             allocation failed due to resource limit.

**/
NET_BUF *
NetbufAllocStruct (
  IN UINT32                 BlockNum,
  IN UINT32                 BlockOpNum
  )
{
  NET_BUF                   *Nbuf;
  NET_VECTOR                *Vector;

  ASSERT (BlockOpNum >= 1);

  //
  // Allocate three memory blocks.
  //
  Nbuf = AllocateZeroPool (NET_BUF_SIZE (BlockOpNum));

  if (Nbuf == NULL) {
    return NULL;
  }

  Nbuf->Signature           = NET_BUF_SIGNATURE;
  Nbuf->RefCnt              = 1;
  Nbuf->BlockOpNum          = BlockOpNum;
  InitializeListHead (&Nbuf->List);

  if (BlockNum != 0) {
    Vector = AllocateZeroPool (NET_VECTOR_SIZE (BlockNum));

    if (Vector == NULL) {
      goto FreeNbuf;
    }

    Vector->Signature = NET_VECTOR_SIGNATURE;
    Vector->RefCnt    = 1;
    Vector->BlockNum  = BlockNum;
    Nbuf->Vector      = Vector;
  }

  return Nbuf;

FreeNbuf:

  FreePool (Nbuf);
  return NULL;
}


/**
  Allocate a single block NET_BUF. Upon allocation, all the
  free space is in the tail room.

  @param[in]  Len              The length of the block.

  @return                      Pointer to the allocated NET_BUF, or NULL if the
                               allocation failed due to resource limit.

**/
NET_BUF  *
EFIAPI
NetbufAlloc (
  IN UINT32                 Len
  )
{
  NET_BUF                   *Nbuf;
  NET_VECTOR                *Vector;
  UINT8                     *Bulk;

  ASSERT (Len > 0);

  Nbuf = NetbufAllocStruct (1, 1);

  if (Nbuf == NULL) {
    return NULL;
  }

  Bulk = AllocatePool (Len);

  if (Bulk == NULL) {
    goto FreeNBuf;
  }

  Vector = Nbuf->Vector;
  Vector->Len                 = Len;

  Vector->Block[0].Bulk       = Bulk;
  Vector->Block[0].Len        = Len;

  Nbuf->BlockOp[0].BlockHead  = Bulk;
  Nbuf->BlockOp[0].BlockTail  = Bulk + Len;

  Nbuf->BlockOp[0].Head       = Bulk;
  Nbuf->BlockOp[0].Tail       = Bulk;
  Nbuf->BlockOp[0].Size       = 0;

  return Nbuf;

FreeNBuf:
  FreePool (Nbuf);
  return NULL;
}

/**
  Free the net vector.

  Decrease the reference count of the net vector by one. The real resource free
  operation isn't performed until the reference count of the net vector is
  decreased to 0.

  @param[in]  Vector                Pointer to the NET_VECTOR to be freed.

**/
VOID
NetbufFreeVector (
  IN NET_VECTOR             *Vector
  )
{
  UINT32                    Index;

  ASSERT (Vector != NULL);
  NET_CHECK_SIGNATURE (Vector, NET_VECTOR_SIGNATURE);
  ASSERT (Vector->RefCnt > 0);

  Vector->RefCnt--;

  if (Vector->RefCnt > 0) {
    return;
  }

  if (Vector->Free != NULL) {
    //
    // Call external free function to free the vector if it
    // isn't NULL. If NET_VECTOR_OWN_FIRST is set, release the
    // first block since it is allocated by us
    //
    if ((Vector->Flag & NET_VECTOR_OWN_FIRST) != 0) {
      gBS->FreePool (Vector->Block[0].Bulk);
    }

    Vector->Free (Vector->Arg);

  } else {
    //
    // Free each memory block associated with the Vector
    //
    for (Index = 0; Index < Vector->BlockNum; Index++) {
      gBS->FreePool (Vector->Block[Index].Bulk);
    }
  }

  FreePool (Vector);
}


/**
  Free the net buffer and its associated NET_VECTOR.

  Decrease the reference count of the net buffer by one. Free the associated net
  vector and itself if the reference count of the net buffer is decreased to 0.
  The net vector free operation just decrease the reference count of the net
  vector by one and do the real resource free operation when the reference count
  of the net vector is 0.

  @param[in]  Nbuf                  Pointer to the NET_BUF to be freed.

**/
VOID
EFIAPI
NetbufFree (
  IN NET_BUF                *Nbuf
  )
{
  ASSERT (Nbuf != NULL);
  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);
  ASSERT (Nbuf->RefCnt > 0);

  Nbuf->RefCnt--;

  if (Nbuf->RefCnt == 0) {
    //
    // Update Vector only when NBuf is to be released. That is,
    // all the sharing of Nbuf increse Vector's RefCnt by one
    //
    NetbufFreeVector (Nbuf->Vector);
    FreePool (Nbuf);
  }
}


/**
  Create a copy of the net buffer that shares the associated net vector.

  The reference count of the newly created net buffer is set to 1. The reference
  count of the associated net vector is increased by one.

  @param[in]  Nbuf              Pointer to the net buffer to be cloned.

  @return                       Pointer to the cloned net buffer, or NULL if the
                                allocation failed due to resource limit.

**/
NET_BUF *
EFIAPI
NetbufClone (
  IN NET_BUF                *Nbuf
  )
{
  NET_BUF                   *Clone;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

  Clone = AllocatePool (NET_BUF_SIZE (Nbuf->BlockOpNum));

  if (Clone == NULL) {
    return NULL;
  }

  Clone->Signature  = NET_BUF_SIGNATURE;
  Clone->RefCnt     = 1;
  InitializeListHead (&Clone->List);

  Clone->Ip   = Nbuf->Ip;
  Clone->Tcp  = Nbuf->Tcp;

  CopyMem (Clone->ProtoData, Nbuf->ProtoData, NET_PROTO_DATA);

  NET_GET_REF (Nbuf->Vector);

  Clone->Vector     = Nbuf->Vector;
  Clone->BlockOpNum = Nbuf->BlockOpNum;
  Clone->TotalSize  = Nbuf->TotalSize;
  CopyMem (Clone->BlockOp, Nbuf->BlockOp, sizeof (NET_BLOCK_OP) * Nbuf->BlockOpNum);

  return Clone;
}


/**
  Create a duplicated copy of the net buffer with data copied and HeadSpace
  bytes of head space reserved.

  The duplicated net buffer will allocate its own memory to hold the data of the
  source net buffer.

  @param[in]       Nbuf         Pointer to the net buffer to be duplicated from.
  @param[in, out]  Duplicate    Pointer to the net buffer to duplicate to, if
                                NULL a new net buffer is allocated.
  @param[in]      HeadSpace     Length of the head space to reserve.

  @return                       Pointer to the duplicated net buffer, or NULL if
                                the allocation failed due to resource limit.

**/
NET_BUF  *
EFIAPI
NetbufDuplicate (
  IN NET_BUF                *Nbuf,
  IN OUT NET_BUF            *Duplicate        OPTIONAL,
  IN UINT32                 HeadSpace
  )
{
  UINT8                     *Dst;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

  if (Duplicate == NULL) {
    Duplicate = NetbufAlloc (Nbuf->TotalSize + HeadSpace);
  }

  if (Duplicate == NULL) {
    return NULL;
  }

  //
  // Don't set the IP and TCP head point, since it is most
  // like that they are pointing to the memory of Nbuf.
  //
  CopyMem (Duplicate->ProtoData, Nbuf->ProtoData, NET_PROTO_DATA);
  NetbufReserve (Duplicate, HeadSpace);

  Dst = NetbufAllocSpace (Duplicate, Nbuf->TotalSize, NET_BUF_TAIL);
  NetbufCopy (Nbuf, 0, Nbuf->TotalSize, Dst);

  return Duplicate;
}


/**
  Free a list of net buffers.

  @param[in, out]  Head              Pointer to the head of linked net buffers.

**/
VOID
EFIAPI
NetbufFreeList (
  IN OUT LIST_ENTRY         *Head
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  NET_BUF                   *Nbuf;

  Entry = Head->ForwardLink;

  NET_LIST_FOR_EACH_SAFE (Entry, Next, Head) {
    Nbuf = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);
    NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

    RemoveEntryList (Entry);
    NetbufFree (Nbuf);
  }

  ASSERT (IsListEmpty (Head));
}


/**
  Get the index of NET_BLOCK_OP that contains the byte at Offset in the net
  buffer.

  This can be used to, for example, retrieve the IP header in the packet. It
  also can be used to get the fragment that contains the byte which is used
  mainly by the library implementation itself.

  @param[in]   Nbuf      Pointer to the net buffer.
  @param[in]   Offset    The offset of the byte.
  @param[out]  Index     Index of the NET_BLOCK_OP that contains the byte at
                         Offset.

  @return       Pointer to the Offset'th byte of data in the net buffer, or NULL
                if there is no such data in the net buffer.

**/
UINT8  *
EFIAPI
NetbufGetByte (
  IN  NET_BUF               *Nbuf,
  IN  UINT32                Offset,
  OUT UINT32                *Index  OPTIONAL
  )
{
  NET_BLOCK_OP              *BlockOp;
  UINT32                    Loop;
  UINT32                    Len;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

  if (Offset >= Nbuf->TotalSize) {
    return NULL;
  }

  BlockOp = Nbuf->BlockOp;
  Len     = 0;

  for (Loop = 0; Loop < Nbuf->BlockOpNum; Loop++) {

    if (Len + BlockOp[Loop].Size <= Offset) {
      Len += BlockOp[Loop].Size;
      continue;
    }

    if (Index != NULL) {
      *Index = Loop;
    }

    return BlockOp[Loop].Head + (Offset - Len);
  }

  return NULL;
}



/**
  Set the NET_BLOCK and corresponding NET_BLOCK_OP in the net buffer and
  corresponding net vector according to the bulk pointer and bulk length.

  All the pointers in the Index'th NET_BLOCK and NET_BLOCK_OP are set to the
  bulk's head and tail respectively. So, this function alone can't be used by
  NetbufAlloc.

  @param[in, out]  Nbuf       Pointer to the net buffer.
  @param[in]       Bulk       Pointer to the data.
  @param[in]       Len        Length of the bulk data.
  @param[in]       Index      The data block index in the net buffer the bulk
                              data should belong to.

**/
VOID
NetbufSetBlock (
  IN OUT NET_BUF            *Nbuf,
  IN UINT8                  *Bulk,
  IN UINT32                 Len,
  IN UINT32                 Index
  )
{
  NET_BLOCK_OP              *BlockOp;
  NET_BLOCK                 *Block;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);
  NET_CHECK_SIGNATURE (Nbuf->Vector, NET_VECTOR_SIGNATURE);
  ASSERT (Index < Nbuf->BlockOpNum);

  Block               = &(Nbuf->Vector->Block[Index]);
  BlockOp             = &(Nbuf->BlockOp[Index]);
  Block->Len          = Len;
  Block->Bulk         = Bulk;
  BlockOp->BlockHead  = Bulk;
  BlockOp->BlockTail  = Bulk + Len;
  BlockOp->Head       = Bulk;
  BlockOp->Tail       = Bulk + Len;
  BlockOp->Size       = Len;
}



/**
  Set the NET_BLOCK_OP in the net buffer. The corresponding NET_BLOCK
  structure is left untouched.

  Some times, there is no 1:1 relationship between NET_BLOCK and NET_BLOCK_OP.
  For example, that in NetbufGetFragment.

  @param[in, out]  Nbuf       Pointer to the net buffer.
  @param[in]       Bulk       Pointer to the data.
  @param[in]       Len        Length of the bulk data.
  @param[in]       Index      The data block index in the net buffer the bulk
                              data should belong to.

**/
VOID
NetbufSetBlockOp (
  IN OUT NET_BUF            *Nbuf,
  IN UINT8                  *Bulk,
  IN UINT32                 Len,
  IN UINT32                 Index
  )
{
  NET_BLOCK_OP              *BlockOp;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);
  ASSERT (Index < Nbuf->BlockOpNum);

  BlockOp             = &(Nbuf->BlockOp[Index]);
  BlockOp->BlockHead  = Bulk;
  BlockOp->BlockTail  = Bulk + Len;
  BlockOp->Head       = Bulk;
  BlockOp->Tail       = Bulk + Len;
  BlockOp->Size       = Len;
}


/**
  Helper function for NetbufGetFragment. NetbufGetFragment may allocate the
  first block to reserve HeadSpace bytes header space. So it needs to create a
  new net vector for the first block and can avoid copy for the remaining data
  by sharing the old net vector.

  @param[in]  Arg                   Point to the old NET_VECTOR.

**/
VOID
EFIAPI
NetbufGetFragmentFree (
  IN VOID                   *Arg
  )
{
  NET_VECTOR                *Vector;

  Vector = (NET_VECTOR *)Arg;
  NetbufFreeVector (Vector);
}


/**
  Create a NET_BUF structure which contains Len byte data of Nbuf starting from
  Offset.

  A new NET_BUF structure will be created but the associated data in NET_VECTOR
  is shared. This function exists to do IP packet fragmentation.

  @param[in]  Nbuf         Pointer to the net buffer to be extracted.
  @param[in]  Offset       Starting point of the data to be included in the new
                           net buffer.
  @param[in]  Len          Bytes of data to be included in the new net buffer.
  @param[in]  HeadSpace    Bytes of head space to reserve for protocol header.

  @return                  Pointer to the cloned net buffer, or NULL if the
                           allocation failed due to resource limit.

**/
NET_BUF  *
EFIAPI
NetbufGetFragment (
  IN NET_BUF                *Nbuf,
  IN UINT32                 Offset,
  IN UINT32                 Len,
  IN UINT32                 HeadSpace
  )
{
  NET_BUF                   *Child;
  NET_VECTOR                *Vector;
  NET_BLOCK_OP              *BlockOp;
  UINT32                    CurBlockOp;
  UINT32                    BlockOpNum;
  UINT8                     *FirstBulk;
  UINT32                    Index;
  UINT32                    First;
  UINT32                    Last;
  UINT32                    FirstSkip;
  UINT32                    FirstLen;
  UINT32                    LastLen;
  UINT32                    Cur;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

  if ((Len == 0) || (Offset + Len > Nbuf->TotalSize)) {
    return NULL;
  }

  //
  // First find the first and last BlockOp that contains
  // the valid data, and compute the offset of the first
  // BlockOp and length of the last BlockOp
  //
  BlockOp = Nbuf->BlockOp;
  Cur     = 0;

  for (Index = 0; Index < Nbuf->BlockOpNum; Index++) {
    if (Offset < Cur + BlockOp[Index].Size) {
      break;
    }

    Cur += BlockOp[Index].Size;
  }

  //
  // First is the index of the first BlockOp, FirstSkip is
  // the offset of the first byte in the first BlockOp.
  //
  First     = Index;
  FirstSkip = Offset - Cur;
  FirstLen  = BlockOp[Index].Size - FirstSkip;

  Last      = 0;
  LastLen   = 0;

  if (Len > FirstLen) {
    Cur += BlockOp[Index].Size;
    Index++;

    for (; Index < Nbuf->BlockOpNum; Index++) {
      if (Offset + Len <= Cur + BlockOp[Index].Size) {
        Last    = Index;
        LastLen = Offset + Len - Cur;
        break;
      }

      Cur += BlockOp[Index].Size;
    }

  } else {
    Last     = First;
    LastLen  = Len;
    FirstLen = Len;
  }

  ASSERT (Last >= First);
  BlockOpNum = Last - First + 1;
  CurBlockOp = 0;

  if (HeadSpace != 0) {
    //
    // Allocate an extra block to accomdate the head space.
    //
    BlockOpNum++;

    Child = NetbufAllocStruct (1, BlockOpNum);

    if (Child == NULL) {
      return NULL;
    }

    FirstBulk = AllocatePool (HeadSpace);

    if (FirstBulk == NULL) {
      goto FreeChild;
    }

    Vector        = Child->Vector;
    Vector->Free  = NetbufGetFragmentFree;
    Vector->Arg   = Nbuf->Vector;
    Vector->Flag  = NET_VECTOR_OWN_FIRST;
    Vector->Len   = HeadSpace;

    //
    // Reserve the head space in the first block
    //
    NetbufSetBlock (Child, FirstBulk, HeadSpace, 0);
    Child->BlockOp[0].Head += HeadSpace;
    Child->BlockOp[0].Size =  0;
    CurBlockOp++;

  } else {
    Child = NetbufAllocStruct (0, BlockOpNum);

    if (Child == NULL) {
      return NULL;
    }

    Child->Vector = Nbuf->Vector;
  }

  NET_GET_REF (Nbuf->Vector);
  Child->TotalSize = Len;

  //
  // Set all the BlockOp up, the first and last one are special
  // and need special process.
  //
  NetbufSetBlockOp (
    Child,
    Nbuf->BlockOp[First].Head + FirstSkip,
    FirstLen,
    CurBlockOp++
    );

  for (Index = First + 1; Index < Last; Index++) {
    NetbufSetBlockOp (
      Child,
      BlockOp[Index].Head,
      BlockOp[Index].Size,
      CurBlockOp++
      );
  }

  if (First != Last) {
    NetbufSetBlockOp (
      Child,
      BlockOp[Last].Head,
      LastLen,
      CurBlockOp
      );
  }

  CopyMem (Child->ProtoData, Nbuf->ProtoData, NET_PROTO_DATA);
  return Child;

FreeChild:

  FreePool (Child);
  return NULL;
}



/**
  Build a NET_BUF from external blocks.

  A new NET_BUF structure will be created from external blocks. Additional block
  of memory will be allocated to hold reserved HeadSpace bytes of header room
  and existing HeadLen bytes of header but the external blocks are shared by the
  net buffer to avoid data copying.

  @param[in]  ExtFragment           Pointer to the data block.
  @param[in]  ExtNum                The number of the data blocks.
  @param[in]  HeadSpace             The head space to be reserved.
  @param[in]  HeadLen               The length of the protocol header, This function
                                    will pull that number of data into a linear block.
  @param[in]  ExtFree               Pointer to the caller provided free function.
  @param[in]  Arg                   The argument passed to ExtFree when ExtFree is
                                    called.

  @return                  Pointer to the net buffer built from the data blocks,
                           or NULL if the allocation failed due to resource
                           limit.

**/
NET_BUF  *
EFIAPI
NetbufFromExt (
  IN NET_FRAGMENT           *ExtFragment,
  IN UINT32                 ExtNum,
  IN UINT32                 HeadSpace,
  IN UINT32                 HeadLen,
  IN NET_VECTOR_EXT_FREE    ExtFree,
  IN VOID                   *Arg          OPTIONAL
  )
{
  NET_BUF                   *Nbuf;
  NET_VECTOR                *Vector;
  NET_FRAGMENT              SavedFragment;
  UINT32                    SavedIndex;
  UINT32                    TotalLen;
  UINT32                    BlockNum;
  UINT8                     *FirstBlock;
  UINT32                    FirstBlockLen;
  UINT8                     *Header;
  UINT32                    CurBlock;
  UINT32                    Index;
  UINT32                    Len;
  UINT32                    Copied;

  ASSERT ((ExtFragment != NULL) && (ExtNum > 0) && (ExtFree != NULL));

  SavedFragment.Bulk = NULL;
  SavedFragment.Len  = 0;

  FirstBlockLen  = 0;
  FirstBlock     = NULL;
  BlockNum       = ExtNum;
  Index          = 0;
  TotalLen       = 0;
  SavedIndex     = 0;
  Len            = 0;
  Copied         = 0;

  //
  // No need to consolidate the header if the first block is
  // longer than the header length or there is only one block.
  //
  if ((ExtFragment[0].Len >= HeadLen) || (ExtNum == 1)) {
    HeadLen = 0;
  }

  //
  // Allocate an extra block if we need to:
  //  1. Allocate some header space
  //  2. aggreate the packet header
  //
  if ((HeadSpace != 0) || (HeadLen != 0)) {
    FirstBlockLen = HeadLen + HeadSpace;
    FirstBlock    = AllocatePool (FirstBlockLen);

    if (FirstBlock == NULL) {
      return NULL;
    }

    BlockNum++;
  }

  //
  // Copy the header to the first block, reduce the NET_BLOCK
  // to allocate by one for each block that is completely covered
  // by the first bulk.
  //
  if (HeadLen != 0) {
    Len    = HeadLen;
    Header = FirstBlock + HeadSpace;

    for (Index = 0; Index < ExtNum; Index++) {
      if (Len >= ExtFragment[Index].Len) {
        CopyMem (Header, ExtFragment[Index].Bulk, ExtFragment[Index].Len);

        Copied    += ExtFragment[Index].Len;
        Len       -= ExtFragment[Index].Len;
        Header    += ExtFragment[Index].Len;
        TotalLen  += ExtFragment[Index].Len;
        BlockNum--;

        if (Len == 0) {
          //
          // Increament the index number to point to the next
          // non-empty fragment.
          //
          Index++;
          break;
        }

      } else {
        CopyMem (Header, ExtFragment[Index].Bulk, Len);

        Copied    += Len;
        TotalLen  += Len;

        //
        // Adjust the block structure to exclude the data copied,
        // So, the left-over block can be processed as other blocks.
        // But it must be recovered later. (SavedIndex > 0) always
        // holds since we don't aggreate the header if the first block
        // is bigger enough that the header is continuous
        //
        SavedIndex    = Index;
        SavedFragment = ExtFragment[Index];
        ExtFragment[Index].Bulk += Len;
        ExtFragment[Index].Len  -= Len;
        break;
      }
    }
  }

  Nbuf = NetbufAllocStruct (BlockNum, BlockNum);

  if (Nbuf == NULL) {
    goto FreeFirstBlock;
  }

  Vector       = Nbuf->Vector;
  Vector->Free = ExtFree;
  Vector->Arg  = Arg;
  Vector->Flag = ((FirstBlockLen != 0) ? NET_VECTOR_OWN_FIRST : 0);

  //
  // Set the first block up which may contain
  // some head space and aggregated header
  //
  CurBlock = 0;

  if (FirstBlockLen != 0) {
    NetbufSetBlock (Nbuf, FirstBlock, HeadSpace + Copied, 0);
    Nbuf->BlockOp[0].Head += HeadSpace;
    Nbuf->BlockOp[0].Size =  Copied;

    CurBlock++;
  }

  for (; Index < ExtNum; Index++) {
    NetbufSetBlock (Nbuf, ExtFragment[Index].Bulk, ExtFragment[Index].Len, CurBlock);
    TotalLen += ExtFragment[Index].Len;
    CurBlock++;
  }

  Vector->Len     = TotalLen + HeadSpace;
  Nbuf->TotalSize = TotalLen;

  if (SavedIndex != 0) {
    ExtFragment[SavedIndex] = SavedFragment;
  }

  return Nbuf;

FreeFirstBlock:
  if (FirstBlock != NULL) {
    FreePool (FirstBlock);
  }
  return NULL;
}


/**
  Build a fragment table to contain the fragments in the net buffer. This is the
  opposite operation of the NetbufFromExt.

  @param[in]       Nbuf                  Point to the net buffer.
  @param[in, out]  ExtFragment           Pointer to the data block.
  @param[in, out]  ExtNum                The number of the data blocks.

  @retval EFI_BUFFER_TOO_SMALL  The number of non-empty block is bigger than
                                ExtNum.
  @retval EFI_SUCCESS           Fragment table is built successfully.

**/
EFI_STATUS
EFIAPI
NetbufBuildExt (
  IN NET_BUF                *Nbuf,
  IN OUT NET_FRAGMENT       *ExtFragment,
  IN OUT UINT32             *ExtNum
  )
{
  UINT32                    Index;
  UINT32                    Current;

  Current = 0;

  for (Index = 0; (Index < Nbuf->BlockOpNum); Index++) {
    if (Nbuf->BlockOp[Index].Size == 0) {
      continue;
    }

    if (Current < *ExtNum) {
      ExtFragment[Current].Bulk = Nbuf->BlockOp[Index].Head;
      ExtFragment[Current].Len  = Nbuf->BlockOp[Index].Size;
      Current++;
    } else {
      return EFI_BUFFER_TOO_SMALL;
    }
  }

  *ExtNum = Current;
  return EFI_SUCCESS;
}


/**
  Build a net buffer from a list of net buffers.

  All the fragments will be collected from the list of NEW_BUF and then a new
  net buffer will be created through NetbufFromExt.

  @param[in]   BufList    A List of the net buffer.
  @param[in]   HeadSpace  The head space to be reserved.
  @param[in]   HeaderLen  The length of the protocol header, This function
                          will pull that number of data into a linear block.
  @param[in]   ExtFree    Pointer to the caller provided free function.
  @param[in]   Arg        The argument passed to ExtFree when ExtFree is called.

  @return                 Pointer to the net buffer built from the list of net
                          buffers.

**/
NET_BUF  *
EFIAPI
NetbufFromBufList (
  IN LIST_ENTRY             *BufList,
  IN UINT32                 HeadSpace,
  IN UINT32                 HeaderLen,
  IN NET_VECTOR_EXT_FREE    ExtFree,
  IN VOID                   *Arg              OPTIONAL
  )
{
  NET_FRAGMENT              *Fragment;
  UINT32                    FragmentNum;
  LIST_ENTRY                *Entry;
  NET_BUF                   *Nbuf;
  UINT32                    Index;
  UINT32                    Current;

  //
  //Compute how many blocks are there
  //
  FragmentNum = 0;

  NET_LIST_FOR_EACH (Entry, BufList) {
    Nbuf = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);
    NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);
    FragmentNum += Nbuf->BlockOpNum;
  }

  //
  //Allocate and copy block points
  //
  Fragment = AllocatePool (sizeof (NET_FRAGMENT) * FragmentNum);

  if (Fragment == NULL) {
    return NULL;
  }

  Current = 0;

  NET_LIST_FOR_EACH (Entry, BufList) {
    Nbuf = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);
    NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

    for (Index = 0; Index < Nbuf->BlockOpNum; Index++) {
      if (Nbuf->BlockOp[Index].Size != 0) {
        Fragment[Current].Bulk = Nbuf->BlockOp[Index].Head;
        Fragment[Current].Len  = Nbuf->BlockOp[Index].Size;
        Current++;
      }
    }
  }

  Nbuf = NetbufFromExt (Fragment, Current, HeadSpace, HeaderLen, ExtFree, Arg);
  FreePool (Fragment);

  return Nbuf;
}


/**
  Reserve some space in the header room of the net buffer.

  Upon allocation, all the space are in the tail room of the buffer. Call this
  function to move some space to the header room. This function is quite limited
  in that it can only reserve space from the first block of an empty NET_BUF not
  built from the external. But it should be enough for the network stack.

  @param[in, out]  Nbuf     Pointer to the net buffer.
  @param[in]       Len      The length of buffer to be reserved from the header.

**/
VOID
EFIAPI
NetbufReserve (
  IN OUT NET_BUF            *Nbuf,
  IN UINT32                 Len
  )
{
  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);
  NET_CHECK_SIGNATURE (Nbuf->Vector, NET_VECTOR_SIGNATURE);

  ASSERT ((Nbuf->BlockOpNum == 1) && (Nbuf->TotalSize == 0));
  ASSERT ((Nbuf->Vector->Free == NULL) && (Nbuf->Vector->Len >= Len));

  Nbuf->BlockOp[0].Head += Len;
  Nbuf->BlockOp[0].Tail += Len;

  ASSERT (Nbuf->BlockOp[0].Tail <= Nbuf->BlockOp[0].BlockTail);
}


/**
  Allocate Len bytes of space from the header or tail of the buffer.

  @param[in, out]  Nbuf       Pointer to the net buffer.
  @param[in]       Len        The length of the buffer to be allocated.
  @param[in]       FromHead   The flag to indicate whether reserve the data
                              from head (TRUE) or tail (FALSE).

  @return                     Pointer to the first byte of the allocated buffer,
                              or NULL if there is no sufficient space.

**/
UINT8*
EFIAPI
NetbufAllocSpace (
  IN OUT NET_BUF            *Nbuf,
  IN UINT32                 Len,
  IN BOOLEAN                FromHead
  )
{
  NET_BLOCK_OP              *BlockOp;
  UINT32                    Index;
  UINT8                     *SavedTail;

  Index = 0;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);
  NET_CHECK_SIGNATURE (Nbuf->Vector, NET_VECTOR_SIGNATURE);

  ASSERT (Len > 0);

  if (FromHead) {
    //
    // Allocate some space from head. If the buffer is empty,
    // allocate from the first block. If it isn't, allocate
    // from the first non-empty block, or the block before that.
    //
    if (Nbuf->TotalSize == 0) {
      Index = 0;
    } else {
      NetbufGetByte (Nbuf, 0, &Index);

      if ((NET_HEADSPACE(&(Nbuf->BlockOp[Index])) < Len) && (Index > 0)) {
        Index--;
      }
    }

    BlockOp = &(Nbuf->BlockOp[Index]);

    if (NET_HEADSPACE (BlockOp) < Len) {
      return NULL;
    }

    BlockOp->Head   -= Len;
    BlockOp->Size   += Len;
    Nbuf->TotalSize += Len;

    return BlockOp->Head;

  } else {
    //
    // Allocate some space from the tail. If the buffer is empty,
    // allocate from the first block. If it isn't, allocate
    // from the last non-empty block, or the block after that.
    //
    if (Nbuf->TotalSize == 0) {
      Index = 0;
    } else {
      NetbufGetByte (Nbuf, Nbuf->TotalSize - 1, &Index);

      if ((NET_TAILSPACE(&(Nbuf->BlockOp[Index])) < Len) &&
          (Index < Nbuf->BlockOpNum - 1)) {

        Index++;
      }
    }

    BlockOp = &(Nbuf->BlockOp[Index]);

    if (NET_TAILSPACE (BlockOp) < Len) {
      return NULL;
    }

    SavedTail       = BlockOp->Tail;

    BlockOp->Tail   += Len;
    BlockOp->Size   += Len;
    Nbuf->TotalSize += Len;

    return SavedTail;
  }
}


/**
  Trim a single NET_BLOCK by Len bytes from the header or tail.

  @param[in, out]  BlockOp      Pointer to the NET_BLOCK.
  @param[in]       Len          The length of the data to be trimmed.
  @param[in]       FromHead     The flag to indicate whether trim data from head
                                (TRUE) or tail (FALSE).

**/
VOID
NetblockTrim (
  IN OUT NET_BLOCK_OP       *BlockOp,
  IN UINT32                 Len,
  IN BOOLEAN                FromHead
  )
{
  ASSERT ((BlockOp != NULL) && (BlockOp->Size >= Len));

  BlockOp->Size -= Len;

  if (FromHead) {
    BlockOp->Head += Len;
  } else {
    BlockOp->Tail -= Len;
  }
}


/**
  Trim Len bytes from the header or tail of the net buffer.

  @param[in, out]  Nbuf         Pointer to the net buffer.
  @param[in]       Len          The length of the data to be trimmed.
  @param[in]      FromHead      The flag to indicate whether trim data from head
                                (TRUE) or tail (FALSE).

  @return    Length of the actually trimmed data, which is possible to be less
             than Len because the TotalSize of Nbuf is less than Len.

**/
UINT32
EFIAPI
NetbufTrim (
  IN OUT NET_BUF            *Nbuf,
  IN UINT32                 Len,
  IN BOOLEAN                FromHead
  )
{
  NET_BLOCK_OP              *BlockOp;
  UINT32                    Index;
  UINT32                    Trimmed;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

  if (Len > Nbuf->TotalSize) {
    Len = Nbuf->TotalSize;
  }

  //
  // If FromTail is true, iterate backward. That
  // is, init Index to NBuf->BlockNum - 1, and
  // decrease it by 1 during each loop. Otherwise,
  // iterate forward. That is, init Index to 0, and
  // increase it by 1 during each loop.
  //
  Trimmed          = 0;
  Nbuf->TotalSize -= Len;

  Index   = (FromHead ? 0 : Nbuf->BlockOpNum - 1);
  BlockOp = Nbuf->BlockOp;

  for (;;) {
    if (BlockOp[Index].Size == 0) {
      Index += (FromHead ? 1 : -1);
      continue;
    }

    if (Len > BlockOp[Index].Size) {
      Len     -= BlockOp[Index].Size;
      Trimmed += BlockOp[Index].Size;
      NetblockTrim (&BlockOp[Index], BlockOp[Index].Size, FromHead);
    } else {
      Trimmed += Len;
      NetblockTrim (&BlockOp[Index], Len, FromHead);
      break;
    }

    Index += (FromHead ? 1 : -1);
  }

  return Trimmed;
}


/**
  Copy Len bytes of data from the specific offset of the net buffer to the
  destination memory.

  The Len bytes of data may cross the several fragments of the net buffer.

  @param[in]   Nbuf         Pointer to the net buffer.
  @param[in]   Offset       The sequence number of the first byte to copy.
  @param[in]   Len          Length of the data to copy.
  @param[in]   Dest         The destination of the data to copy to.

  @return           The length of the actual copied data, or 0 if the offset
                    specified exceeds the total size of net buffer.

**/
UINT32
EFIAPI
NetbufCopy (
  IN NET_BUF                *Nbuf,
  IN UINT32                 Offset,
  IN UINT32                 Len,
  IN UINT8                  *Dest
  )
{
  NET_BLOCK_OP              *BlockOp;
  UINT32                    Skip;
  UINT32                    Left;
  UINT32                    Copied;
  UINT32                    Index;
  UINT32                    Cur;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);
  ASSERT (Dest);

  if ((Len == 0) || (Nbuf->TotalSize <= Offset)) {
    return 0;
  }

  if (Nbuf->TotalSize - Offset < Len) {
    Len = Nbuf->TotalSize - Offset;
  }

  BlockOp = Nbuf->BlockOp;

  //
  // Skip to the offset. Don't make "Offset-By-One" error here.
  // Cur + BLOCK.SIZE is the first sequence number of next block.
  // So, (Offset < Cur + BLOCK.SIZE) means that the  first byte
  // is in the current block. if (Offset == Cur + BLOCK.SIZE), the
  // first byte is the next block's first byte.
  //
  Cur = 0;

  for (Index = 0; Index < Nbuf->BlockOpNum; Index++) {
    if (BlockOp[Index].Size == 0) {
      continue;
    }

    if (Offset < Cur + BlockOp[Index].Size) {
      break;
    }

    Cur += BlockOp[Index].Size;
  }

  //
  // Cur is the sequence number of the first byte in the block
  // Offset - Cur is the number of bytes before first byte to
  // to copy in the current block.
  //
  Skip  = Offset - Cur;
  Left  = BlockOp[Index].Size - Skip;

  if (Len <= Left) {
    CopyMem (Dest, BlockOp[Index].Head + Skip, Len);
    return Len;
  }

  CopyMem (Dest, BlockOp[Index].Head + Skip, Left);

  Dest  += Left;
  Len   -= Left;
  Copied = Left;

  Index++;

  for (; Index < Nbuf->BlockOpNum; Index++) {
    if (Len > BlockOp[Index].Size) {
      Len    -= BlockOp[Index].Size;
      Copied += BlockOp[Index].Size;

      CopyMem (Dest, BlockOp[Index].Head, BlockOp[Index].Size);
      Dest   += BlockOp[Index].Size;
    } else {
      Copied += Len;
      CopyMem (Dest, BlockOp[Index].Head, Len);
      break;
    }
  }

  return Copied;
}


/**
  Initiate the net buffer queue.

  @param[in, out]  NbufQue   Pointer to the net buffer queue to be initialized.

**/
VOID
EFIAPI
NetbufQueInit (
  IN OUT NET_BUF_QUEUE          *NbufQue
  )
{
  NbufQue->Signature  = NET_QUE_SIGNATURE;
  NbufQue->RefCnt     = 1;
  InitializeListHead (&NbufQue->List);

  InitializeListHead (&NbufQue->BufList);
  NbufQue->BufSize  = 0;
  NbufQue->BufNum   = 0;
}


/**
  Allocate and initialize a net buffer queue.

  @return         Pointer to the allocated net buffer queue, or NULL if the
                  allocation failed due to resource limit.

**/
NET_BUF_QUEUE  *
EFIAPI
NetbufQueAlloc (
  VOID
  )
{
  NET_BUF_QUEUE             *NbufQue;

  NbufQue = AllocatePool (sizeof (NET_BUF_QUEUE));
  if (NbufQue == NULL) {
    return NULL;
  }

  NetbufQueInit (NbufQue);

  return NbufQue;
}


/**
  Free a net buffer queue.

  Decrease the reference count of the net buffer queue by one. The real resource
  free operation isn't performed until the reference count of the net buffer
  queue is decreased to 0.

  @param[in]  NbufQue               Pointer to the net buffer queue to be freed.

**/
VOID
EFIAPI
NetbufQueFree (
  IN NET_BUF_QUEUE          *NbufQue
  )
{
  ASSERT (NbufQue != NULL);
  NET_CHECK_SIGNATURE (NbufQue, NET_QUE_SIGNATURE);

  NbufQue->RefCnt--;

  if (NbufQue->RefCnt == 0) {
    NetbufQueFlush (NbufQue);
    FreePool (NbufQue);
  }
}


/**
  Append a net buffer to the net buffer queue.

  @param[in, out]  NbufQue            Pointer to the net buffer queue.
  @param[in, out]  Nbuf               Pointer to the net buffer to be appended.

**/
VOID
EFIAPI
NetbufQueAppend (
  IN OUT NET_BUF_QUEUE          *NbufQue,
  IN OUT NET_BUF                *Nbuf
  )
{
  NET_CHECK_SIGNATURE (NbufQue, NET_QUE_SIGNATURE);
  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

  InsertTailList (&NbufQue->BufList, &Nbuf->List);

  NbufQue->BufSize += Nbuf->TotalSize;
  NbufQue->BufNum++;
}


/**
  Remove a net buffer from the head in the specific queue and return it.

  @param[in, out]  NbufQue               Pointer to the net buffer queue.

  @return           Pointer to the net buffer removed from the specific queue,
                    or NULL if there is no net buffer in the specific queue.

**/
NET_BUF  *
EFIAPI
NetbufQueRemove (
  IN OUT NET_BUF_QUEUE          *NbufQue
  )
{
  NET_BUF                   *First;

  NET_CHECK_SIGNATURE (NbufQue, NET_QUE_SIGNATURE);

  if (NbufQue->BufNum == 0) {
    return NULL;
  }

  First = NET_LIST_USER_STRUCT (NbufQue->BufList.ForwardLink, NET_BUF, List);

  NetListRemoveHead (&NbufQue->BufList);

  NbufQue->BufSize -= First->TotalSize;
  NbufQue->BufNum--;
  return First;
}


/**
  Copy Len bytes of data from the net buffer queue at the specific offset to the
  destination memory.

  The copying operation is the same as NetbufCopy but applies to the net buffer
  queue instead of the net buffer.

  @param[in]   NbufQue         Pointer to the net buffer queue.
  @param[in]   Offset          The sequence number of the first byte to copy.
  @param[in]   Len             Length of the data to copy.
  @param[out]  Dest            The destination of the data to copy to.

  @return       The length of the actual copied data, or 0 if the offset
                specified exceeds the total size of net buffer queue.

**/
UINT32
EFIAPI
NetbufQueCopy (
  IN NET_BUF_QUEUE          *NbufQue,
  IN UINT32                 Offset,
  IN UINT32                 Len,
  OUT UINT8                 *Dest
  )
{
  LIST_ENTRY                *Entry;
  NET_BUF                   *Nbuf;
  UINT32                    Skip;
  UINT32                    Left;
  UINT32                    Cur;
  UINT32                    Copied;

  NET_CHECK_SIGNATURE (NbufQue, NET_QUE_SIGNATURE);
  ASSERT (Dest != NULL);

  if ((Len == 0) || (NbufQue->BufSize <= Offset)) {
    return 0;
  }

  if (NbufQue->BufSize - Offset < Len) {
    Len = NbufQue->BufSize - Offset;
  }

  //
  // skip to the Offset
  //
  Cur   = 0;
  Nbuf  = NULL;

  NET_LIST_FOR_EACH (Entry, &NbufQue->BufList) {
    Nbuf = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);

    if (Offset < Cur + Nbuf->TotalSize) {
      break;
    }

    Cur += Nbuf->TotalSize;
  }

  ASSERT (Nbuf != NULL);

  //
  // Copy the data in the first buffer.
  //
  Skip  = Offset - Cur;
  Left  = Nbuf->TotalSize - Skip;

  if (Len < Left) {
    return NetbufCopy (Nbuf, Skip, Len, Dest);
  }

  NetbufCopy (Nbuf, Skip, Left, Dest);
  Dest  += Left;
  Len   -= Left;
  Copied = Left;

  //
  // Iterate over the others
  //
  Entry = Entry->ForwardLink;

  while ((Len > 0) && (Entry != &NbufQue->BufList)) {
    Nbuf = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);

    if (Len > Nbuf->TotalSize) {
      Len -= Nbuf->TotalSize;
      Copied += Nbuf->TotalSize;

      NetbufCopy (Nbuf, 0, Nbuf->TotalSize, Dest);
      Dest += Nbuf->TotalSize;

    } else {
      NetbufCopy (Nbuf, 0, Len, Dest);
      Copied += Len;
      break;
    }

    Entry = Entry->ForwardLink;
  }

  return Copied;
}


/**
  Trim Len bytes of data from the buffer queue and free any net buffer
  that is completely trimmed.

  The trimming operation is the same as NetbufTrim but applies to the net buffer
  queue instead of the net buffer.

  @param[in, out]  NbufQue               Pointer to the net buffer queue.
  @param[in]       Len                   Length of the data to trim.

  @return   The actual length of the data trimmed.

**/
UINT32
EFIAPI
NetbufQueTrim (
  IN OUT NET_BUF_QUEUE      *NbufQue,
  IN UINT32                 Len
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  NET_BUF                   *Nbuf;
  UINT32                    Trimmed;

  NET_CHECK_SIGNATURE (NbufQue, NET_QUE_SIGNATURE);

  if (Len == 0) {
    return 0;
  }

  if (Len > NbufQue->BufSize) {
    Len = NbufQue->BufSize;
  }

  NbufQue->BufSize -= Len;
  Trimmed = 0;

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &NbufQue->BufList) {
    Nbuf = NET_LIST_USER_STRUCT (Entry, NET_BUF, List);

    if (Len >= Nbuf->TotalSize) {
      Trimmed += Nbuf->TotalSize;
      Len -= Nbuf->TotalSize;

      RemoveEntryList (Entry);
      NetbufFree (Nbuf);

      NbufQue->BufNum--;

      if (Len == 0) {
        break;
      }

    } else {
      Trimmed += NetbufTrim (Nbuf, Len, NET_BUF_HEAD);
      break;
    }
  }

  return Trimmed;
}


/**
  Flush the net buffer queue.

  @param[in, out]  NbufQue               Pointer to the queue to be flushed.

**/
VOID
EFIAPI
NetbufQueFlush (
  IN OUT NET_BUF_QUEUE          *NbufQue
  )
{
  NET_CHECK_SIGNATURE (NbufQue, NET_QUE_SIGNATURE);

  NetbufFreeList (&NbufQue->BufList);

  NbufQue->BufNum   = 0;
  NbufQue->BufSize  = 0;
}


/**
  Compute the checksum for a bulk of data.

  @param[in]   Bulk                  Pointer to the data.
  @param[in]   Len                   Length of the data, in bytes.

  @return    The computed checksum.

**/
UINT16
EFIAPI
NetblockChecksum (
  IN UINT8                  *Bulk,
  IN UINT32                 Len
  )
{
  register UINT32           Sum;

  Sum = 0;

  while (Len > 1) {
    Sum += *(UINT16 *) Bulk;
    Bulk += 2;
    Len -= 2;
  }

  //
  // Add left-over byte, if any
  //
  if (Len > 0) {
    Sum += *(UINT8 *) Bulk;
  }

  //
  // Fold 32-bit sum to 16 bits
  //
  while ((Sum >> 16) != 0) {
    Sum = (Sum & 0xffff) + (Sum >> 16);

  }

  return (UINT16) Sum;
}


/**
  Add two checksums.

  @param[in]   Checksum1             The first checksum to be added.
  @param[in]   Checksum2             The second checksum to be added.

  @return         The new checksum.

**/
UINT16
EFIAPI
NetAddChecksum (
  IN UINT16                 Checksum1,
  IN UINT16                 Checksum2
  )
{
  UINT32                    Sum;

  Sum = Checksum1 + Checksum2;

  //
  // two UINT16 can only add up to a carry of 1.
  //
  if ((Sum >> 16) != 0) {
    Sum = (Sum & 0xffff) + 1;

  }

  return (UINT16) Sum;
}


/**
  Compute the checksum for a NET_BUF.

  @param[in]   Nbuf                  Pointer to the net buffer.

  @return    The computed checksum.

**/
UINT16
EFIAPI
NetbufChecksum (
  IN NET_BUF                *Nbuf
  )
{
  NET_BLOCK_OP              *BlockOp;
  UINT32                    Offset;
  UINT16                    TotalSum;
  UINT16                    BlockSum;
  UINT32                    Index;

  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);

  TotalSum  = 0;
  Offset    = 0;
  BlockOp   = Nbuf->BlockOp;

  for (Index = 0; Index < Nbuf->BlockOpNum; Index++) {
    if (BlockOp[Index].Size == 0) {
      continue;
    }

    BlockSum = NetblockChecksum (BlockOp[Index].Head, BlockOp[Index].Size);

    if ((Offset & 0x01) != 0) {
      //
      // The checksum starts with an odd byte, swap
      // the checksum before added to total checksum
      //
      BlockSum = SwapBytes16 (BlockSum);
    }

    TotalSum = NetAddChecksum (BlockSum, TotalSum);
    Offset  += BlockOp[Index].Size;
  }

  return TotalSum;
}


/**
  Compute the checksum for TCP/UDP pseudo header.

  Src and Dst are in network byte order, and Len is in host byte order.

  @param[in]   Src                   The source address of the packet.
  @param[in]   Dst                   The destination address of the packet.
  @param[in]   Proto                 The protocol type of the packet.
  @param[in]   Len                   The length of the packet.

  @return   The computed checksum.

**/
UINT16
EFIAPI
NetPseudoHeadChecksum (
  IN IP4_ADDR               Src,
  IN IP4_ADDR               Dst,
  IN UINT8                  Proto,
  IN UINT16                 Len
  )
{
  NET_PSEUDO_HDR            Hdr;

  //
  // Zero the memory to relieve align problems
  //
  ZeroMem (&Hdr, sizeof (Hdr));

  Hdr.SrcIp     = Src;
  Hdr.DstIp     = Dst;
  Hdr.Protocol  = Proto;
  Hdr.Len       = HTONS (Len);

  return NetblockChecksum ((UINT8 *) &Hdr, sizeof (Hdr));
}

/**
  Compute the checksum for TCP6/UDP6 pseudo header.

  Src and Dst are in network byte order, and Len is in host byte order.

  @param[in]   Src                   The source address of the packet.
  @param[in]   Dst                   The destination address of the packet.
  @param[in]   NextHeader            The protocol type of the packet.
  @param[in]   Len                   The length of the packet.

  @return   The computed checksum.

**/
UINT16
EFIAPI
NetIp6PseudoHeadChecksum (
  IN EFI_IPv6_ADDRESS       *Src,
  IN EFI_IPv6_ADDRESS       *Dst,
  IN UINT8                  NextHeader,
  IN UINT32                 Len
  )
{
  NET_IP6_PSEUDO_HDR        Hdr;

  //
  // Zero the memory to relieve align problems
  //
  ZeroMem (&Hdr, sizeof (Hdr));

  IP6_COPY_ADDRESS (&Hdr.SrcIp, Src);
  IP6_COPY_ADDRESS (&Hdr.DstIp, Dst);

  Hdr.NextHeader = NextHeader;
  Hdr.Len        = HTONL (Len);

  return NetblockChecksum ((UINT8 *) &Hdr, sizeof (Hdr));
}

/**
  The function frees the net buffer which allocated by the IP protocol. It releases 
  only the net buffer and doesn't call the external free function. 

  This function should be called after finishing the process of mIpSec->ProcessExt() 
  for outbound traffic. The (EFI_IPSEC2_PROTOCOL)->ProcessExt() allocates a new 
  buffer for the ESP, so there needs a function to free the old net buffer.

  @param[in]  Nbuf       The network buffer to be freed.

**/
VOID
NetIpSecNetbufFree (
  NET_BUF   *Nbuf
  )
{
  NET_CHECK_SIGNATURE (Nbuf, NET_BUF_SIGNATURE);
  ASSERT (Nbuf->RefCnt > 0);

  Nbuf->RefCnt--;

  if (Nbuf->RefCnt == 0) {
    
    //
    // Update Vector only when NBuf is to be released. That is,
    // all the sharing of Nbuf increse Vector's RefCnt by one
    //
    NET_CHECK_SIGNATURE (Nbuf->Vector, NET_VECTOR_SIGNATURE);
    ASSERT (Nbuf->Vector->RefCnt > 0);

    Nbuf->Vector->RefCnt--;

    if (Nbuf->Vector->RefCnt > 0) {
      return;
    }

    //
    // If NET_VECTOR_OWN_FIRST is set, release the first block since it is 
    // allocated by us
    //
    if ((Nbuf->Vector->Flag & NET_VECTOR_OWN_FIRST) != 0) {
      FreePool (Nbuf->Vector->Block[0].Bulk);
    }
    FreePool (Nbuf->Vector);
    FreePool (Nbuf); 
  } 
}

