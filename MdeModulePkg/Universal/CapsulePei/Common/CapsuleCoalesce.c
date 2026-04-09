/** @file
  The logic to process capsule.

  Caution: This module requires additional review when modified.
  This driver will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  CapsuleDataCoalesce() will do basic validation before coalesce capsule data
  into memory.

(C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>

#include <Guid/CapsuleVendor.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>

#include "CommonHeader.h"

#define MIN_COALESCE_ADDR  (1024 * 1024)

/**
  Given a pointer to the capsule block list, info on the available system
  memory, and the size of a buffer, find a free block of memory where a
  buffer of the given size can be copied to safely.

  @param BlockList   Pointer to head of capsule block descriptors
  @param MemBase     Pointer to the base of memory in which we want to find free space
  @param MemSize     The size of the block of memory pointed to by MemBase
  @param DataSize    How big a free block we want to find

  @return A pointer to a memory block of at least DataSize that lies somewhere
          between MemBase and (MemBase + MemSize). The memory pointed to does not
          contain any of the capsule block descriptors or capsule blocks pointed to
          by the BlockList.

**/
UINT8 *
FindFreeMem (
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockList,
  UINT8                         *MemBase,
  UINTN                         MemSize,
  UINTN                         DataSize
  );

/**
  The capsule block descriptors may be fragmented and spread all over memory.
  To simplify the coalescing of capsule blocks, first coalesce all the
  capsule block descriptors low in memory.

  The descriptors passed in can be fragmented throughout memory. Here
  they are relocated into memory to turn them into a contiguous (null
  terminated) array.

  @param PeiServices    pointer to PEI services table
  @param BlockList      pointer to the capsule block descriptors
  @param NumDescriptors number of capsule data block descriptors, whose Length is non-zero.
  @param MemBase        base of system memory in which we can work
  @param MemSize        size of the system memory pointed to by MemBase

  @retval NULL    could not relocate the descriptors
  @retval Pointer to the base of the successfully-relocated block descriptors.

**/
EFI_CAPSULE_BLOCK_DESCRIPTOR *
RelocateBlockDescriptors (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockList,
  IN UINTN                         NumDescriptors,
  IN UINT8                         *MemBase,
  IN UINTN                         MemSize
  );

/**
  Check every capsule header.

  @param CapsuleHeader   The pointer to EFI_CAPSULE_HEADER

  @retval FALSE  Capsule is OK
  @retval TRUE   Capsule is corrupted

**/
BOOLEAN
IsCapsuleCorrupted (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  );

/**
  Determine if two buffers overlap in memory.

  @param Buff1   pointer to first buffer
  @param Size1   size of Buff1
  @param Buff2   pointer to second buffer
  @param Size2   size of Buff2

  @retval TRUE    Buffers overlap in memory.
  @retval FALSE   Buffer doesn't overlap.

**/
BOOLEAN
IsOverlapped (
  UINT8  *Buff1,
  UINTN  Size1,
  UINT8  *Buff2,
  UINTN  Size2
  );

/**
  Given a pointer to a capsule block descriptor, traverse the list to figure
  out how many legitimate descriptors there are, and how big the capsule it
  refers to is.

  @param Desc            Pointer to the capsule block descriptors
  @param NumDescriptors  Optional pointer to where to return the number of capsule data descriptors, whose Length is non-zero.
  @param CapsuleSize     Optional pointer to where to return the capsule image size
  @param CapsuleNumber   Optional pointer to where to return the number of capsule

  @retval EFI_NOT_FOUND   No descriptors containing data in the list
  @retval EFI_SUCCESS     Return data is valid

**/
EFI_STATUS
GetCapsuleInfo (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR  *Desc,
  IN OUT UINTN                     *NumDescriptors OPTIONAL,
  IN OUT UINTN                     *CapsuleSize OPTIONAL,
  IN OUT UINTN                     *CapsuleNumber OPTIONAL
  );

/**
  Given a pointer to the capsule block list, info on the available system
  memory, and the size of a buffer, find a free block of memory where a
  buffer of the given size can be copied to safely.

  @param BlockList   Pointer to head of capsule block descriptors
  @param MemBase     Pointer to the base of memory in which we want to find free space
  @param MemSize     The size of the block of memory pointed to by MemBase
  @param DataSize    How big a free block we want to find

  @return A pointer to a memory block of at least DataSize that lies somewhere
          between MemBase and (MemBase + MemSize). The memory pointed to does not
          contain any of the capsule block descriptors or capsule blocks pointed to
          by the BlockList.

**/
UINT8 *
FindFreeMem (
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockList,
  UINT8                         *MemBase,
  UINTN                         MemSize,
  UINTN                         DataSize
  )
{
  UINTN                         Size;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *CurrDesc;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *TempDesc;
  UINT8                         *MemEnd;
  BOOLEAN                       Failed;

  //
  // Need at least enough to copy the data to at the end of the buffer, so
  // say the end is less the data size for easy comparisons here.
  //
  MemEnd   = MemBase + MemSize - DataSize;
  CurrDesc = BlockList;
  //
  // Go through all the descriptor blocks and see if any obstruct the range
  //
  while (CurrDesc != NULL) {
    //
    // Get the size of this block list and see if it's in the way
    //
    Failed   = FALSE;
    TempDesc = CurrDesc;
    Size     = sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
    while (TempDesc->Length != 0) {
      Size += sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
      TempDesc++;
    }

    if (IsOverlapped (MemBase, DataSize, (UINT8 *)CurrDesc, Size)) {
      //
      // Set our new base to the end of this block list and start all over
      //
      MemBase  = (UINT8 *)CurrDesc + Size;
      CurrDesc = BlockList;
      if (MemBase > MemEnd) {
        return NULL;
      }

      Failed = TRUE;
    }

    //
    // Now go through all the blocks and make sure none are in the way
    //
    while ((CurrDesc->Length != 0) && (!Failed)) {
      if (IsOverlapped (MemBase, DataSize, (UINT8 *)(UINTN)CurrDesc->Union.DataBlock, (UINTN)CurrDesc->Length)) {
        //
        // Set our new base to the end of this block and start all over
        //
        Failed   = TRUE;
        MemBase  = (UINT8 *)((UINTN)CurrDesc->Union.DataBlock) + CurrDesc->Length;
        CurrDesc = BlockList;
        if (MemBase > MemEnd) {
          return NULL;
        }
      }

      CurrDesc++;
    }

    //
    // Normal continuation -- jump to next block descriptor list
    //
    if (!Failed) {
      CurrDesc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *)(UINTN)CurrDesc->Union.ContinuationPointer;
    }
  }

  return MemBase;
}

/**
  Validate capsule by MemoryResource.

  @param MemoryResource  Pointer to the buffer of memory resource descriptor.
  @param Address         Address to be validated.
  @param Size            Size to be validated.

  @retval TRUE  No memory resource descriptor reported in HOB list before capsule Coalesce,
                or it is valid in one MemoryResource.
          FALSE It is not in any MemoryResource.

**/
BOOLEAN
ValidateCapsuleByMemoryResource (
  IN MEMORY_RESOURCE_DESCRIPTOR  *MemoryResource,
  IN EFI_PHYSICAL_ADDRESS        Address,
  IN UINT64                      Size
  )
{
  UINTN  Index;

  //
  // Sanity Check
  //
  if (Size > MAX_ADDRESS) {
    DEBUG ((DEBUG_ERROR, "ERROR: Size(0x%lx) > MAX_ADDRESS\n", Size));
    return FALSE;
  }

  //
  // Sanity Check
  //
  if (Address > (MAX_ADDRESS - Size)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Address(0x%lx) > (MAX_ADDRESS - Size(0x%lx))\n", Address, Size));
    return FALSE;
  }

  if (MemoryResource == NULL) {
    //
    // No memory resource descriptor reported in HOB list before capsule Coalesce.
    //
    return TRUE;
  }

  for (Index = 0; MemoryResource[Index].ResourceLength != 0; Index++) {
    if ((Address >= MemoryResource[Index].PhysicalStart) &&
        ((Address + Size) <= (MemoryResource[Index].PhysicalStart + MemoryResource[Index].ResourceLength)))
    {
      DEBUG ((
        DEBUG_INFO,
        "Address(0x%lx) Size(0x%lx) in MemoryResource[0x%x] - Start(0x%lx) Length(0x%lx)\n",
        Address,
        Size,
        Index,
        MemoryResource[Index].PhysicalStart,
        MemoryResource[Index].ResourceLength
        ));
      return TRUE;
    }
  }

  DEBUG ((DEBUG_ERROR, "ERROR: Address(0x%lx) Size(0x%lx) not in any MemoryResource\n", Address, Size));
  return FALSE;
}

/**
  Check the integrity of the capsule descriptors.

  @param BlockList       Pointer to the capsule descriptors
  @param MemoryResource  Pointer to the buffer of memory resource descriptor.

  @retval NULL           BlockList is not valid.
  @retval LastBlockDesc  Last one Block in BlockList

**/
EFI_CAPSULE_BLOCK_DESCRIPTOR *
ValidateCapsuleIntegrity (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockList,
  IN MEMORY_RESOURCE_DESCRIPTOR    *MemoryResource
  )
{
  EFI_CAPSULE_HEADER            *CapsuleHeader;
  UINT64                        CapsuleSize;
  UINTN                         CapsuleCount;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *Ptr;

  DEBUG ((DEBUG_INFO, "ValidateCapsuleIntegrity\n"));

  //
  // Go through the list to look for inconsistencies. Check for:
  //   * misaligned block descriptors.
  //   * The first capsule header guid
  //   * The first capsule header flag
  //   * The first capsule header HeaderSize
  //   * Below check will be done in ValidateCapsuleByMemoryResource()
  //     Length > MAX_ADDRESS
  //     Ptr + sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR) > MAX_ADDRESS
  //     DataBlock + Length > MAX_ADDRESS
  //
  CapsuleSize  = 0;
  CapsuleCount = 0;
  Ptr          = BlockList;

  if (!ValidateCapsuleByMemoryResource (MemoryResource, (EFI_PHYSICAL_ADDRESS)(UINTN)Ptr, sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR))) {
    return NULL;
  }

  DEBUG ((DEBUG_INFO, "Ptr - 0x%p\n", Ptr));
  DEBUG ((DEBUG_INFO, "Ptr->Length - 0x%lx\n", Ptr->Length));
  DEBUG ((DEBUG_INFO, "Ptr->Union - 0x%lx\n", Ptr->Union.ContinuationPointer));
  while ((Ptr->Length != 0) || (Ptr->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS)(UINTN)NULL)) {
    //
    // Make sure the descriptor is aligned at UINT64 in memory
    //
    if ((UINTN)Ptr & (sizeof (UINT64) - 1)) {
      DEBUG ((DEBUG_ERROR, "ERROR: BlockList address failed alignment check\n"));
      return NULL;
    }

    if (Ptr->Length == 0) {
      //
      // Descriptor points to another list of block descriptors somewhere
      // else.
      //
      Ptr = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *)(UINTN)Ptr->Union.ContinuationPointer;
      if (!ValidateCapsuleByMemoryResource (MemoryResource, (EFI_PHYSICAL_ADDRESS)(UINTN)Ptr, sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR))) {
        return NULL;
      }

      DEBUG ((DEBUG_INFO, "Ptr(C) - 0x%p\n", Ptr));
      DEBUG ((DEBUG_INFO, "Ptr->Length - 0x%lx\n", Ptr->Length));
      DEBUG ((DEBUG_INFO, "Ptr->Union - 0x%lx\n", Ptr->Union.ContinuationPointer));
    } else {
      if (!ValidateCapsuleByMemoryResource (MemoryResource, Ptr->Union.DataBlock, Ptr->Length)) {
        return NULL;
      }

      //
      // To enhance the reliability of check-up, the first capsule's header is checked here.
      // More reliabilities check-up will do later.
      //
      if (CapsuleSize == 0) {
        //
        // Move to the first capsule to check its header.
        //
        CapsuleHeader = (EFI_CAPSULE_HEADER *)((UINTN)Ptr->Union.DataBlock);
        //
        // Sanity check
        //
        if (Ptr->Length < sizeof (EFI_CAPSULE_HEADER)) {
          DEBUG ((DEBUG_ERROR, "ERROR: Ptr->Length(0x%lx) < sizeof(EFI_CAPSULE_HEADER)\n", Ptr->Length));
          return NULL;
        }

        //
        // Make sure HeaderSize field is valid
        //
        if (CapsuleHeader->HeaderSize > CapsuleHeader->CapsuleImageSize) {
          DEBUG ((DEBUG_ERROR, "ERROR: CapsuleHeader->HeaderSize(0x%x) > CapsuleHeader->CapsuleImageSize(0x%x)\n", CapsuleHeader->HeaderSize, CapsuleHeader->CapsuleImageSize));
          return NULL;
        }

        if (IsCapsuleCorrupted (CapsuleHeader)) {
          return NULL;
        }

        CapsuleCount++;
        CapsuleSize = CapsuleHeader->CapsuleImageSize;
      }

      if (CapsuleSize >= Ptr->Length) {
        CapsuleSize = CapsuleSize - Ptr->Length;
      } else {
        DEBUG ((DEBUG_ERROR, "ERROR: CapsuleSize(0x%lx) < Ptr->Length(0x%lx)\n", CapsuleSize, Ptr->Length));
        //
        // Sanity check
        //
        return NULL;
      }

      //
      // Move to next BLOCK descriptor
      //
      Ptr++;
      if (!ValidateCapsuleByMemoryResource (MemoryResource, (EFI_PHYSICAL_ADDRESS)(UINTN)Ptr, sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR))) {
        return NULL;
      }

      DEBUG ((DEBUG_INFO, "Ptr(B) - 0x%p\n", Ptr));
      DEBUG ((DEBUG_INFO, "Ptr->Length - 0x%lx\n", Ptr->Length));
      DEBUG ((DEBUG_INFO, "Ptr->Union - 0x%lx\n", Ptr->Union.ContinuationPointer));
    }
  }

  if (CapsuleCount == 0) {
    //
    // No any capsule is found in BlockList
    //
    DEBUG ((DEBUG_ERROR, "ERROR: CapsuleCount(0x%x) == 0\n", CapsuleCount));
    return NULL;
  }

  if (CapsuleSize != 0) {
    //
    // Capsule data is incomplete.
    //
    DEBUG ((DEBUG_ERROR, "ERROR: CapsuleSize(0x%lx) != 0\n", CapsuleSize));
    return NULL;
  }

  return Ptr;
}

/**
  The capsule block descriptors may be fragmented and spread all over memory.
  To simplify the coalescing of capsule blocks, first coalesce all the
  capsule block descriptors low in memory.

  The descriptors passed in can be fragmented throughout memory. Here
  they are relocated into memory to turn them into a contiguous (null
  terminated) array.

  @param PeiServices    pointer to PEI services table
  @param BlockList      pointer to the capsule block descriptors
  @param NumDescriptors number of capsule data block descriptors, whose Length is non-zero.
  @param MemBase        base of system memory in which we can work
  @param MemSize        size of the system memory pointed to by MemBase

  @retval NULL    could not relocate the descriptors
  @retval Pointer to the base of the successfully-relocated block descriptors.

**/
EFI_CAPSULE_BLOCK_DESCRIPTOR  *
RelocateBlockDescriptors (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockList,
  IN UINTN                         NumDescriptors,
  IN UINT8                         *MemBase,
  IN UINTN                         MemSize
  )
{
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *NewBlockList;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *CurrBlockDescHead;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *TempBlockDesc;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *PrevBlockDescTail;
  UINTN                         BufferSize;
  UINT8                         *RelocBuffer;
  UINTN                         BlockListSize;

  //
  // Get the info on the blocks and descriptors. Since we're going to move
  // the descriptors low in memory, adjust the base/size values accordingly here.
  // NumDescriptors is the number of legit data descriptors, so add one for
  // a terminator. (Already done by caller, no check is needed.)
  //

  BufferSize   = NumDescriptors * sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
  NewBlockList = (EFI_CAPSULE_BLOCK_DESCRIPTOR *)MemBase;
  if (MemSize < BufferSize) {
    return NULL;
  }

  MemSize -= BufferSize;
  MemBase += BufferSize;
  //
  // Go through all the blocks and make sure none are in the way
  //
  TempBlockDesc = BlockList;
  while (TempBlockDesc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS)(UINTN)NULL) {
    if (TempBlockDesc->Length == 0) {
      //
      // Next block of descriptors
      //
      TempBlockDesc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *)(UINTN)TempBlockDesc->Union.ContinuationPointer;
    } else {
      //
      // If the capsule data pointed to by this descriptor is in the way,
      // move it.
      //
      if (IsOverlapped (
            (UINT8 *)NewBlockList,
            BufferSize,
            (UINT8 *)(UINTN)TempBlockDesc->Union.DataBlock,
            (UINTN)TempBlockDesc->Length
            ))
      {
        //
        // Relocate the block
        //
        RelocBuffer = FindFreeMem (BlockList, MemBase, MemSize, (UINTN)TempBlockDesc->Length);
        if (RelocBuffer == NULL) {
          return NULL;
        }

        CopyMem ((VOID *)RelocBuffer, (VOID *)(UINTN)TempBlockDesc->Union.DataBlock, (UINTN)TempBlockDesc->Length);
        DEBUG ((DEBUG_INFO, "Capsule relocate descriptors from/to/size  0x%lX 0x%lX 0x%lX\n", TempBlockDesc->Union.DataBlock, (UINT64)(UINTN)RelocBuffer, TempBlockDesc->Length));
        TempBlockDesc->Union.DataBlock = (EFI_PHYSICAL_ADDRESS)(UINTN)RelocBuffer;
      }

      TempBlockDesc++;
    }
  }

  //
  // Now go through all the block descriptors to make sure that they're not
  // in the memory region we want to copy them to.
  //
  CurrBlockDescHead = BlockList;
  PrevBlockDescTail = NULL;
  while ((CurrBlockDescHead != NULL) && (CurrBlockDescHead->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS)(UINTN)NULL)) {
    //
    // Get the size of this list then see if it overlaps our low region
    //
    TempBlockDesc = CurrBlockDescHead;
    BlockListSize = sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
    while (TempBlockDesc->Length != 0) {
      BlockListSize += sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
      TempBlockDesc++;
    }

    if (IsOverlapped (
          (UINT8 *)NewBlockList,
          BufferSize,
          (UINT8 *)CurrBlockDescHead,
          BlockListSize
          ))
    {
      //
      // Overlaps, so move it out of the way
      //
      RelocBuffer = FindFreeMem (BlockList, MemBase, MemSize, BlockListSize);
      if (RelocBuffer == NULL) {
        return NULL;
      }

      CopyMem ((VOID *)RelocBuffer, (VOID *)CurrBlockDescHead, BlockListSize);
      DEBUG ((DEBUG_INFO, "Capsule reloc descriptor block #2\n"));
      //
      // Point the previous block's next point to this copied version. If
      // the tail pointer is null, then this is the first descriptor block.
      //
      if (PrevBlockDescTail == NULL) {
        BlockList = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *)RelocBuffer;
      } else {
        PrevBlockDescTail->Union.DataBlock = (EFI_PHYSICAL_ADDRESS)(UINTN)RelocBuffer;
      }
    }

    //
    // Save our new tail and jump to the next block list
    //
    PrevBlockDescTail = TempBlockDesc;
    CurrBlockDescHead = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *)(UINTN)TempBlockDesc->Union.ContinuationPointer;
  }

  //
  // Cleared out low memory. Now copy the descriptors down there.
  //
  TempBlockDesc     = BlockList;
  CurrBlockDescHead = NewBlockList;
  while ((TempBlockDesc != NULL) && (TempBlockDesc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS)(UINTN)NULL)) {
    if (TempBlockDesc->Length != 0) {
      CurrBlockDescHead->Union.DataBlock = TempBlockDesc->Union.DataBlock;
      CurrBlockDescHead->Length          = TempBlockDesc->Length;
      CurrBlockDescHead++;
      TempBlockDesc++;
    } else {
      TempBlockDesc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *)(UINTN)TempBlockDesc->Union.ContinuationPointer;
    }
  }

  //
  // Null terminate
  //
  CurrBlockDescHead->Union.ContinuationPointer = (EFI_PHYSICAL_ADDRESS)(UINTN)NULL;
  CurrBlockDescHead->Length                    = 0;
  return NewBlockList;
}

/**
  Determine if two buffers overlap in memory.

  @param Buff1   pointer to first buffer
  @param Size1   size of Buff1
  @param Buff2   pointer to second buffer
  @param Size2   size of Buff2

  @retval TRUE    Buffers overlap in memory.
  @retval FALSE   Buffer doesn't overlap.

**/
BOOLEAN
IsOverlapped (
  UINT8  *Buff1,
  UINTN  Size1,
  UINT8  *Buff2,
  UINTN  Size2
  )
{
  //
  // If buff1's end is less than the start of buff2, then it's ok.
  // Also, if buff1's start is beyond buff2's end, then it's ok.
  //
  if (((Buff1 + Size1) <= Buff2) || (Buff1 >= (Buff2 + Size2))) {
    return FALSE;
  }

  return TRUE;
}

/**
  Given a pointer to a capsule block descriptor, traverse the list to figure
  out how many legitimate descriptors there are, and how big the capsule it
  refers to is.

  @param Desc            Pointer to the capsule block descriptors
  @param NumDescriptors  Optional pointer to where to return the number of capsule data descriptors, whose Length is non-zero.
  @param CapsuleSize     Optional pointer to where to return the capsule image size
  @param CapsuleNumber   Optional pointer to where to return the number of capsule

  @retval EFI_NOT_FOUND   No descriptors containing data in the list
  @retval EFI_SUCCESS     Return data is valid

**/
EFI_STATUS
GetCapsuleInfo (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR  *Desc,
  IN OUT UINTN                     *NumDescriptors OPTIONAL,
  IN OUT UINTN                     *CapsuleSize OPTIONAL,
  IN OUT UINTN                     *CapsuleNumber OPTIONAL
  )
{
  UINTN               Count;
  UINTN               Size;
  UINTN               Number;
  UINTN               ThisCapsuleImageSize;
  EFI_CAPSULE_HEADER  *CapsuleHeader;

  DEBUG ((DEBUG_INFO, "GetCapsuleInfo enter\n"));

  ASSERT (Desc != NULL);

  Count                = 0;
  Size                 = 0;
  Number               = 0;
  ThisCapsuleImageSize = 0;

  while (Desc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS)(UINTN)NULL) {
    if (Desc->Length == 0) {
      //
      // Descriptor points to another list of block descriptors somewhere
      //
      Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *)(UINTN)Desc->Union.ContinuationPointer;
    } else {
      //
      // Sanity Check
      // It is needed, because ValidateCapsuleIntegrity() only validate one individual capsule Size.
      // While here we need check all capsules size.
      //
      if (Desc->Length >= (MAX_ADDRESS - Size)) {
        DEBUG ((DEBUG_ERROR, "ERROR: Desc->Length(0x%lx) >= (MAX_ADDRESS - Size(0x%x))\n", Desc->Length, Size));
        return EFI_OUT_OF_RESOURCES;
      }

      Size += (UINTN)Desc->Length;
      Count++;

      //
      // See if this is first capsule's header
      //
      if (ThisCapsuleImageSize == 0) {
        CapsuleHeader = (EFI_CAPSULE_HEADER *)((UINTN)Desc->Union.DataBlock);
        //
        // This has been checked in ValidateCapsuleIntegrity()
        //
        Number++;
        ThisCapsuleImageSize = CapsuleHeader->CapsuleImageSize;
      }

      //
      // This has been checked in ValidateCapsuleIntegrity()
      //
      ASSERT (ThisCapsuleImageSize >= Desc->Length);
      ThisCapsuleImageSize = (UINTN)(ThisCapsuleImageSize - Desc->Length);

      //
      // Move to next
      //
      Desc++;
    }
  }

  //
  // If no descriptors, then fail
  //
  if (Count == 0) {
    DEBUG ((DEBUG_ERROR, "ERROR: Count == 0\n"));
    return EFI_NOT_FOUND;
  }

  //
  // checked in ValidateCapsuleIntegrity()
  //
  ASSERT (ThisCapsuleImageSize == 0);

  if (NumDescriptors != NULL) {
    *NumDescriptors = Count;
  }

  if (CapsuleSize != NULL) {
    *CapsuleSize = Size;
  }

  if (CapsuleNumber != NULL) {
    *CapsuleNumber = Number;
  }

  return EFI_SUCCESS;
}

/**
  Check every capsule header.

  @param CapsuleHeader   The pointer to EFI_CAPSULE_HEADER

  @retval FALSE  Capsule is OK
  @retval TRUE   Capsule is corrupted

**/
BOOLEAN
IsCapsuleCorrupted (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  //
  // A capsule to be updated across a system reset should contain CAPSULE_FLAGS_PERSIST_ACROSS_RESET.
  //
  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) == 0) {
    return TRUE;
  }

  //
  // Make sure the flags combination is supported by the platform.
  //
  if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)) == CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) {
    return TRUE;
  }

  if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_INITIATE_RESET)) == CAPSULE_FLAGS_INITIATE_RESET) {
    return TRUE;
  }

  return FALSE;
}

/**
  Try to verify the integrity of a capsule test pattern before the
  capsule gets coalesced. This can be useful in narrowing down
  where capsule data corruption occurs.

  The test pattern mode fills in memory with a counting UINT32 value.
  If the capsule is not divided up in a multiple of 4-byte blocks, then
  things get messy doing the check. Therefore there are some cases
  here where we just give up and skip the pre-coalesce check.

  @param PeiServices  PEI services table
  @param Desc         Pointer to capsule descriptors
**/
VOID
CapsuleTestPatternPreCoalesce (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR  *Desc
  )
{
  UINT32  *TestPtr;
  UINT32  TestCounter;
  UINT32  TestSize;

  DEBUG ((DEBUG_INFO, "CapsuleTestPatternPreCoalesce\n"));

  //
  // Find first data descriptor
  //
  while ((Desc->Length == 0) && (Desc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS)(UINTN)NULL)) {
    Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *)(UINTN)Desc->Union.ContinuationPointer;
  }

  if (Desc->Union.ContinuationPointer == 0) {
    return;
  }

  //
  // First one better be long enough to at least hold the test signature
  //
  if (Desc->Length < sizeof (UINT32)) {
    DEBUG ((DEBUG_INFO, "Capsule test pattern pre-coalesce punted #1\n"));
    return;
  }

  TestPtr = (UINT32 *)(UINTN)Desc->Union.DataBlock;
  //
  // 0x54534554 "TEST"
  //
  if (*TestPtr != 0x54534554) {
    return;
  }

  TestCounter = 0;
  TestSize    = (UINT32)Desc->Length - 2 * sizeof (UINT32);
  //
  // Skip over the signature and the size fields in the pattern data header
  //
  TestPtr += 2;
  while (1) {
    if ((TestSize & 0x03) != 0) {
      DEBUG ((DEBUG_INFO, "Capsule test pattern pre-coalesce punted #2\n"));
      return;
    }

    while (TestSize > 0) {
      if (*TestPtr != TestCounter) {
        DEBUG ((DEBUG_INFO, "Capsule test pattern pre-coalesce failed data corruption check\n"));
        return;
      }

      TestSize -= sizeof (UINT32);
      TestCounter++;
      TestPtr++;
    }

    Desc++;
    while ((Desc->Length == 0) && (Desc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS)(UINTN)NULL)) {
      Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *)(UINTN)Desc->Union.ContinuationPointer;
    }

    if (Desc->Union.ContinuationPointer == (EFI_PHYSICAL_ADDRESS)(UINTN)NULL) {
      return;
    }

    TestSize = (UINT32)Desc->Length;
    TestPtr  = (UINT32 *)(UINTN)Desc->Union.DataBlock;
  }
}

/**
  Checks for the presence of capsule descriptors.
  Get capsule descriptors from variable CapsuleUpdateData, CapsuleUpdateData1, CapsuleUpdateData2...

  @param BlockListBuffer            Pointer to the buffer of capsule descriptors variables
  @param MemoryResource             Pointer to the buffer of memory resource descriptor.
  @param BlockDescriptorList        Pointer to the capsule descriptors list

  @retval EFI_SUCCESS               a valid capsule is present
  @retval EFI_NOT_FOUND             if a valid capsule is not present
**/
EFI_STATUS
BuildCapsuleDescriptors (
  IN  EFI_PHYSICAL_ADDRESS          *BlockListBuffer,
  IN  MEMORY_RESOURCE_DESCRIPTOR    *MemoryResource,
  OUT EFI_CAPSULE_BLOCK_DESCRIPTOR  **BlockDescriptorList
  )
{
  UINTN                         Index;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *LastBlock;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *TempBlock;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *HeadBlock;

  DEBUG ((DEBUG_INFO, "BuildCapsuleDescriptors enter\n"));

  LastBlock = NULL;
  HeadBlock = NULL;
  TempBlock = NULL;
  Index     = 0;

  while (BlockListBuffer[Index] != 0) {
    //
    // Test integrity of descriptors.
    //
    if (BlockListBuffer[Index] < MAX_ADDRESS) {
      TempBlock = ValidateCapsuleIntegrity ((EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)BlockListBuffer[Index], MemoryResource);
      if (TempBlock != NULL) {
        if (LastBlock == NULL) {
          LastBlock = TempBlock;

          //
          // Return the base of the block descriptors
          //
          HeadBlock = (EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)BlockListBuffer[Index];
        } else {
          //
          // Combine the different BlockList into single BlockList.
          //
          LastBlock->Union.DataBlock = (EFI_PHYSICAL_ADDRESS)(UINTN)BlockListBuffer[Index];
          LastBlock->Length          = 0;
          LastBlock                  = TempBlock;
        }
      }
    } else {
      DEBUG ((DEBUG_ERROR, "ERROR: BlockListBuffer[Index](0x%lx) < MAX_ADDRESS\n", BlockListBuffer[Index]));
    }

    Index++;
  }

  if (HeadBlock != NULL) {
    *BlockDescriptorList = HeadBlock;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  The function to coalesce a fragmented capsule in memory.

  Memory Map for coalesced capsule:
  MemBase +   ---->+---------------------------+<-----------+
  MemSize          | ------------------------- |            |
                   | |  Capsule [Num-1]      | |            |
                   | ------------------------- |            |
                   | |  ................     | |            |
                   | ------------------------- |            |
                   | |  Capsule [1]          | |            |
                   | ------------------------- |            |
                   | |  Capsule [0]          | |            |
                   | ------------------------- |            |
                   |    Capsule Image          |            |
CapsuleImageBase-->+---------------------------+
                   | ------------------------- |            |
                   | |  CapsuleOffset[Num-1] | |            |
                   | ------------------------- |            |
                   | |  ................     | |        CapsuleSize
                   | ------------------------- |            |
                   | |  CapsuleOffset[1]     | |            |
                   | ------------------------- |            |
                   | |  CapsuleOffset[0]     | |            |
                   |---------------------------|            |
                   | |  CapsuleNumber        | |            |
                   | ------------------------- |            |
                   | |  CapsuleAllImageSize  | |            |
                   | ------------------------- |            |
                   |    PrivateData            |            |
     DestPtr  ---->+---------------------------+<-----------+
                   |                           |            |
                   |     FreeMem               |        FreeMemSize
                   |                           |            |
   FreeMemBase --->+---------------------------+<-----------+
                   |    Terminator             |
                   +---------------------------+
                   |    BlockDescriptor n      |
                   +---------------------------+
                   |    .................      |
                   +---------------------------+
                   |    BlockDescriptor 1      |
                   +---------------------------+
                   |    BlockDescriptor 0      |
                   +---------------------------+
                   |    PrivateDataDesc 0      |
      MemBase ---->+---------------------------+<----- BlockList

  Caution: This function may receive untrusted input.
  The capsule data is external input, so this routine will do basic validation before
  coalesce capsule data into memory.

  @param PeiServices        General purpose services available to every PEIM.
  @param BlockListBuffer    Pointer to the buffer of Capsule Descriptor Variables.
  @param MemoryResource     Pointer to the buffer of memory resource descriptor.
  @param MemoryBase         Pointer to the base of a block of memory that we can walk
                            all over while trying to coalesce our buffers.
                            On output, this variable will hold the base address of
                            a coalesced capsule.
  @param MemorySize         Size of the memory region pointed to by MemoryBase.
                            On output, this variable will contain the size of the
                            coalesced capsule.

  @retval EFI_NOT_FOUND     If we could not find the capsule descriptors.

  @retval EFI_BUFFER_TOO_SMALL
                            If we could not coalesce the capsule in the memory
                            region provided to us.

  @retval EFI_SUCCESS       Processed the capsule successfully.
**/
EFI_STATUS
EFIAPI
CapsuleDataCoalesce (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_PHYSICAL_ADDRESS        *BlockListBuffer,
  IN MEMORY_RESOURCE_DESCRIPTOR  *MemoryResource,
  IN OUT VOID                    **MemoryBase,
  IN OUT UINTN                   *MemorySize
  )
{
  VOID                           *NewCapsuleBase;
  VOID                           *CapsuleImageBase;
  UINTN                          CapsuleIndex;
  UINT8                          *FreeMemBase;
  UINT8                          *DestPtr;
  UINTN                          DestLength;
  UINT8                          *RelocPtr;
  UINTN                          CapsuleTimes;
  UINT64                         SizeLeft;
  UINT64                         CapsuleImageSize;
  UINTN                          CapsuleSize;
  UINTN                          CapsuleNumber;
  UINTN                          DescriptorsSize;
  UINTN                          FreeMemSize;
  UINTN                          NumDescriptors;
  BOOLEAN                        CapsuleBeginFlag;
  EFI_STATUS                     Status;
  EFI_CAPSULE_HEADER             *CapsuleHeader;
  EFI_CAPSULE_PEIM_PRIVATE_DATA  PrivateData;
  EFI_CAPSULE_PEIM_PRIVATE_DATA  *PrivateDataPtr;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *BlockList;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *CurrentBlockDesc;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *TempBlockDesc;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   PrivateDataDesc[2];

  DEBUG ((DEBUG_INFO, "CapsuleDataCoalesce enter\n"));

  CapsuleIndex     = 0;
  SizeLeft         = 0;
  CapsuleTimes     = 0;
  CapsuleImageSize = 0;
  PrivateDataPtr   = NULL;
  CapsuleHeader    = NULL;
  CapsuleBeginFlag = TRUE;
  CapsuleSize      = 0;
  NumDescriptors   = 0;

  //
  // Build capsule descriptors list
  //
  Status = BuildCapsuleDescriptors (BlockListBuffer, MemoryResource, &BlockList);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG_CODE (
    CapsuleTestPatternPreCoalesce (PeiServices, BlockList);
    );

  //
  // Get the size of our descriptors and the capsule size. GetCapsuleInfo()
  // returns the number of descriptors that actually point to data, so add
  // one for a terminator. Do that below.
  //
  Status = GetCapsuleInfo (BlockList, &NumDescriptors, &CapsuleSize, &CapsuleNumber);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "CapsuleSize - 0x%x\n", CapsuleSize));
  DEBUG ((DEBUG_INFO, "CapsuleNumber - 0x%x\n", CapsuleNumber));
  DEBUG ((DEBUG_INFO, "NumDescriptors - 0x%x\n", NumDescriptors));
  if ((CapsuleSize == 0) || (NumDescriptors == 0) || (CapsuleNumber == 0)) {
    return EFI_NOT_FOUND;
  }

  if (CapsuleNumber - 1 >= (MAX_ADDRESS - (sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA)  + sizeof (UINT64))) / sizeof (UINT64)) {
    DEBUG ((DEBUG_ERROR, "ERROR: CapsuleNumber - 0x%x\n", CapsuleNumber));
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Initialize our local copy of private data. When we're done, we'll create a
  // descriptor for it as well so that it can be put into free memory without
  // trashing anything.
  //
  PrivateData.Signature           = EFI_CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE;
  PrivateData.CapsuleAllImageSize = (UINT64)CapsuleSize;
  PrivateData.CapsuleNumber       = (UINT64)CapsuleNumber;
  PrivateData.CapsuleOffset[0]    = 0;
  //
  // NOTE: Only data in sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) is valid, CapsuleOffset field is uninitialized at this moment.
  // The code sets partial length here for Descriptor.Length check, but later it will use full length to reserve those PrivateData region.
  //
  PrivateDataDesc[0].Union.DataBlock = (EFI_PHYSICAL_ADDRESS)(UINTN)&PrivateData;
  PrivateDataDesc[0].Length          = sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA);
  PrivateDataDesc[1].Union.DataBlock = (EFI_PHYSICAL_ADDRESS)(UINTN)BlockList;
  PrivateDataDesc[1].Length          = 0;
  //
  // Add PrivateDataDesc[0] in beginning, as it is new descriptor. PrivateDataDesc[1] is NOT needed.
  // In addition, one NULL terminator is added in the end. See RelocateBlockDescriptors().
  //
  NumDescriptors += 2;
  //
  // Sanity check
  //
  if (CapsuleSize >= (MAX_ADDRESS - (sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) + (CapsuleNumber - 1) * sizeof (UINT64) + sizeof (UINT64)))) {
    DEBUG ((DEBUG_ERROR, "ERROR: CapsuleSize - 0x%x\n", CapsuleSize));
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Need add sizeof(UINT64) for PrivateData alignment
  //
  CapsuleSize += sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) + (CapsuleNumber - 1) * sizeof (UINT64) + sizeof (UINT64);
  BlockList    = PrivateDataDesc;
  //
  // Sanity check
  //
  if (NumDescriptors >= (MAX_ADDRESS / sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR))) {
    DEBUG ((DEBUG_ERROR, "ERROR: NumDescriptors - 0x%x\n", NumDescriptors));
    return EFI_BUFFER_TOO_SMALL;
  }

  DescriptorsSize = NumDescriptors * sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
  //
  // Sanity check
  //
  if (DescriptorsSize >= (MAX_ADDRESS - CapsuleSize)) {
    DEBUG ((DEBUG_ERROR, "ERROR: DescriptorsSize - 0x%lx, CapsuleSize - 0x%lx\n", (UINT64)DescriptorsSize, (UINT64)CapsuleSize));
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Don't go below some min address. If the base is below it,
  // then move it up and adjust the size accordingly.
  //
  DEBUG ((DEBUG_INFO, "Capsule Memory range from 0x%8X to 0x%8X\n", (UINTN)*MemoryBase, (UINTN)*MemoryBase + *MemorySize));
  if ((UINTN)*MemoryBase < (UINTN)MIN_COALESCE_ADDR) {
    if (((UINTN)*MemoryBase + *MemorySize) < (UINTN)MIN_COALESCE_ADDR) {
      DEBUG ((DEBUG_ERROR, "ERROR: *MemoryBase + *MemorySize - 0x%x\n", (UINTN)*MemoryBase + *MemorySize));
      return EFI_BUFFER_TOO_SMALL;
    } else {
      *MemorySize = *MemorySize - ((UINTN)MIN_COALESCE_ADDR - (UINTN)*MemoryBase);
      *MemoryBase = (VOID *)(UINTN)MIN_COALESCE_ADDR;
    }
  }

  if (*MemorySize <= (CapsuleSize + DescriptorsSize)) {
    DEBUG ((DEBUG_ERROR, "ERROR: CapsuleSize + DescriptorsSize - 0x%x\n", CapsuleSize + DescriptorsSize));
    return EFI_BUFFER_TOO_SMALL;
  }

  FreeMemBase = *MemoryBase;
  FreeMemSize = *MemorySize;
  DEBUG ((DEBUG_INFO, "Capsule Free Memory from 0x%8X to 0x%8X\n", (UINTN)FreeMemBase, (UINTN)FreeMemBase + FreeMemSize));

  //
  // Relocate all the block descriptors to low memory to make further
  // processing easier.
  //
  BlockList = RelocateBlockDescriptors (PeiServices, BlockList, NumDescriptors, FreeMemBase, FreeMemSize);
  if (BlockList == NULL) {
    //
    // Not enough room to relocate the descriptors
    //
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Take the top of memory for the capsule. UINT64 align up.
  //
  DestPtr          = FreeMemBase + FreeMemSize - CapsuleSize;
  DestPtr          = (UINT8 *)(((UINTN)DestPtr + sizeof (UINT64) - 1) & ~(sizeof (UINT64) - 1));
  FreeMemBase      = (UINT8 *)BlockList + DescriptorsSize;
  FreeMemSize      = (UINTN)DestPtr - (UINTN)FreeMemBase;
  NewCapsuleBase   = (VOID *)DestPtr;
  CapsuleImageBase = (UINT8 *)NewCapsuleBase + sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) + (CapsuleNumber - 1) * sizeof (UINT64);

  PrivateDataPtr = (EFI_CAPSULE_PEIM_PRIVATE_DATA *)NewCapsuleBase;

  //
  // Move all the blocks to the top (high) of memory.
  // Relocate all the obstructing blocks. Note that the block descriptors
  // were coalesced when they were relocated, so we can just ++ the pointer.
  //
  CurrentBlockDesc = BlockList;
  while ((CurrentBlockDesc->Length != 0) || (CurrentBlockDesc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS)(UINTN)NULL)) {
    if (CapsuleTimes == 0) {
      //
      // The first entry is the block descriptor for EFI_CAPSULE_PEIM_PRIVATE_DATA.
      // CapsuleOffset field is uninitialized at this time. No need copy it, but need to reserve for future use.
      //
      ASSERT (CurrentBlockDesc->Union.DataBlock == (UINT64)(UINTN)&PrivateData);
      DestLength = sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) + (CapsuleNumber - 1) * sizeof (UINT64);
    } else {
      DestLength = (UINTN)CurrentBlockDesc->Length;
    }

    //
    // See if any of the remaining capsule blocks are in the way
    //
    TempBlockDesc = CurrentBlockDesc;
    while (TempBlockDesc->Length != 0) {
      //
      // Is this block in the way of where we want to copy the current descriptor to?
      //
      if (IsOverlapped (
            (UINT8 *)DestPtr,
            (UINTN)DestLength,
            (UINT8 *)(UINTN)TempBlockDesc->Union.DataBlock,
            (UINTN)TempBlockDesc->Length
            ))
      {
        //
        // Relocate the block
        //
        RelocPtr = FindFreeMem (BlockList, FreeMemBase, FreeMemSize, (UINTN)TempBlockDesc->Length);
        if (RelocPtr == NULL) {
          return EFI_BUFFER_TOO_SMALL;
        }

        CopyMem ((VOID *)RelocPtr, (VOID *)(UINTN)TempBlockDesc->Union.DataBlock, (UINTN)TempBlockDesc->Length);
        DEBUG ((
          DEBUG_INFO,
          "Capsule reloc data block from 0x%8X to 0x%8X with size 0x%8X\n",
          (UINTN)TempBlockDesc->Union.DataBlock,
          (UINTN)RelocPtr,
          (UINTN)TempBlockDesc->Length
          ));

        TempBlockDesc->Union.DataBlock = (EFI_PHYSICAL_ADDRESS)(UINTN)RelocPtr;
      }

      //
      // Next descriptor
      //
      TempBlockDesc++;
    }

    //
    // Ok, we made it through. Copy the block.
    // we just support greping one capsule from the lists of block descs list.
    //
    CapsuleTimes++;
    //
    // Skip the first block descriptor that filled with EFI_CAPSULE_PEIM_PRIVATE_DATA
    //
    if (CapsuleTimes > 1) {
      //
      // For every capsule entry point, check its header to determine whether to relocate it.
      // If it is invalid, skip it and move on to the next capsule. If it is valid, relocate it.
      //
      if (CapsuleBeginFlag) {
        CapsuleBeginFlag = FALSE;
        CapsuleHeader    = (EFI_CAPSULE_HEADER *)(UINTN)CurrentBlockDesc->Union.DataBlock;
        SizeLeft         = CapsuleHeader->CapsuleImageSize;

        //
        // No more check here is needed, because IsCapsuleCorrupted() already in ValidateCapsuleIntegrity()
        //
        ASSERT (CapsuleIndex < CapsuleNumber);

        //
        // Relocate this capsule
        //
        CapsuleImageSize += SizeLeft;
        //
        // Cache the begin offset of this capsule
        //
        ASSERT (PrivateDataPtr->Signature == EFI_CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE);
        ASSERT ((UINTN)DestPtr >= (UINTN)CapsuleImageBase);
        PrivateDataPtr->CapsuleOffset[CapsuleIndex++] = (UINTN)DestPtr - (UINTN)CapsuleImageBase;
      }

      //
      // Below ASSERT is checked in ValidateCapsuleIntegrity()
      //
      ASSERT (CurrentBlockDesc->Length <= SizeLeft);

      CopyMem ((VOID *)DestPtr, (VOID *)(UINTN)(CurrentBlockDesc->Union.DataBlock), (UINTN)CurrentBlockDesc->Length);
      DEBUG ((
        DEBUG_INFO,
        "Capsule coalesce block no.0x%lX from 0x%lX to 0x%lX with size 0x%lX\n",
        (UINT64)CapsuleTimes,
        CurrentBlockDesc->Union.DataBlock,
        (UINT64)(UINTN)DestPtr,
        CurrentBlockDesc->Length
        ));
      DestPtr  += CurrentBlockDesc->Length;
      SizeLeft -= CurrentBlockDesc->Length;

      if (SizeLeft == 0) {
        //
        // Here is the end of the current capsule image.
        //
        CapsuleBeginFlag = TRUE;
      }
    } else {
      //
      // The first entry is the block descriptor for EFI_CAPSULE_PEIM_PRIVATE_DATA.
      // CapsuleOffset field is uninitialized at this time. No need copy it, but need to reserve for future use.
      //
      ASSERT (CurrentBlockDesc->Length == sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA));
      ASSERT ((UINTN)DestPtr == (UINTN)NewCapsuleBase);
      CopyMem ((VOID *)DestPtr, (VOID *)(UINTN)CurrentBlockDesc->Union.DataBlock, (UINTN)CurrentBlockDesc->Length);
      DestPtr += sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) + (CapsuleNumber - 1) * sizeof (UINT64);
    }

    //
    // Walk through the block descriptor list.
    //
    CurrentBlockDesc++;
  }

  //
  // We return the base of memory we want reserved, and the size.
  // The memory peim should handle it appropriately from there.
  //
  *MemorySize = (UINTN)CapsuleSize;
  *MemoryBase = (VOID *)NewCapsuleBase;

  ASSERT (PrivateDataPtr->Signature == EFI_CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE);
  ASSERT (PrivateDataPtr->CapsuleAllImageSize == CapsuleImageSize);
  ASSERT (PrivateDataPtr->CapsuleNumber == CapsuleIndex);

  return EFI_SUCCESS;
}
