/** @file
  The logic to process capsule.

  Caution: This module requires additional review when modified.
  This driver will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  CapsuleDataCoalesce() will do basic validation before coalesce capsule data
  into memory.

Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <PiPei.h>

#include <Guid/CapsuleVendor.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>

#define MIN_COALESCE_ADDR                     (1024 * 1024)
#define MAX_SUPPORT_CAPSULE_NUM               50

#define EFI_CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('C', 'a', 'p', 'D')

typedef struct {
  UINT32  Signature;
  UINT32  CapsuleSize;
} EFI_CAPSULE_PEIM_PRIVATE_DATA;

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
  EFI_CAPSULE_BLOCK_DESCRIPTOR     *BlockList,
  UINT8                            *MemBase,
  UINTN                             MemSize,
  UINTN                             DataSize
  );

/**
  Check the integrity of the capsule descriptors.

  @param BlockList    Pointer to the capsule descriptors

  @retval NULL           BlockList is not valid.
  @retval LastBlockDesc  Last one Block in BlockList

**/
EFI_CAPSULE_BLOCK_DESCRIPTOR *
ValidateCapsuleIntegrity (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR    *BlockList
  );

/**
  The capsule block descriptors may be fragmented and spread all over memory.
  To simplify the coalescing of capsule blocks, first coalesce all the
  capsule block descriptors low in memory.

  The descriptors passed in can be fragmented throughout memory. Here
  they are relocated into memory to turn them into a contiguous (null
  terminated) array.

  @param PeiServices pointer to PEI services table
  @param BlockList   pointer to the capsule block descriptors
  @param MemBase     base of system memory in which we can work
  @param MemSize     size of the system memory pointed to by MemBase

  @retval NULL    could not relocate the descriptors
  @retval Pointer to the base of the successfully-relocated block descriptors. 

**/
EFI_CAPSULE_BLOCK_DESCRIPTOR *
RelocateBlockDescriptors (
  IN EFI_PEI_SERVICES                  **PeiServices,
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR      *BlockList,
  IN UINT8                             *MemBase,
  IN UINTN                             MemSize
  );

/**
  Check every capsule header.

  @param CapsuleHeader   The pointer to EFI_CAPSULE_HEADER

  @retval FALSE  Capsule is OK
  @retval TRUE   Capsule is corrupted 

**/
BOOLEAN
IsCapsuleCorrupted (
  IN EFI_CAPSULE_HEADER       *CapsuleHeader
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
  UINT8     *Buff1,
  UINTN     Size1,
  UINT8     *Buff2,
  UINTN     Size2
  );

/**
  Given a pointer to a capsule block descriptor, traverse the list to figure
  out how many legitimate descriptors there are, and how big the capsule it
  refers to is.

  @param Desc            Pointer to the capsule block descriptors
                         NumDescriptors  - optional pointer to where to return the number of descriptors
                         CapsuleSize     - optional pointer to where to return the capsule size
  @param NumDescriptors  Optional pointer to where to return the number of descriptors
  @param CapsuleSize     Optional pointer to where to return the capsule size

  @retval EFI_NOT_FOUND   No descriptors containing data in the list
  @retval EFI_SUCCESS     Return data is valid

**/
EFI_STATUS
GetCapsuleInfo (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR   *Desc,
  IN OUT UINTN                      *NumDescriptors OPTIONAL,
  IN OUT UINTN                      *CapsuleSize OPTIONAL
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
  EFI_CAPSULE_BLOCK_DESCRIPTOR      *BlockList,
  UINT8                             *MemBase,
  UINTN                             MemSize,
  UINTN                             DataSize
  )
{
  UINTN                           Size;
  EFI_CAPSULE_BLOCK_DESCRIPTOR    *CurrDesc;
  EFI_CAPSULE_BLOCK_DESCRIPTOR    *TempDesc;
  UINT8                           *MemEnd;
  BOOLEAN                         Failed;

  //
  // Need at least enough to copy the data to at the end of the buffer, so
  // say the end is less the data size for easy comparisons here.
  //
  MemEnd    = MemBase + MemSize - DataSize;
  CurrDesc  = BlockList;
  //
  // Go through all the descriptor blocks and see if any obstruct the range
  //
  while (CurrDesc != NULL) {
    //
    // Get the size of this block list and see if it's in the way
    //
    Failed    = FALSE;
    TempDesc  = CurrDesc;
    Size      = sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
    while (TempDesc->Length != 0) {
      Size += sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
      TempDesc++;
    }

    if (IsOverlapped (MemBase, DataSize, (UINT8 *) CurrDesc, Size)) {
      //
      // Set our new base to the end of this block list and start all over
      //
      MemBase   = (UINT8 *) CurrDesc + Size;
      CurrDesc  = BlockList;
      if (MemBase > MemEnd) {
        return NULL;
      }

      Failed = TRUE;
    }
    //
    // Now go through all the blocks and make sure none are in the way
    //
    while ((CurrDesc->Length != 0) && (!Failed)) {
      if (IsOverlapped (MemBase, DataSize, (UINT8 *) (UINTN) CurrDesc->Union.DataBlock, (UINTN) CurrDesc->Length)) {
        //
        // Set our new base to the end of this block and start all over
        //
        Failed    = TRUE;
        MemBase   = (UINT8 *) ((UINTN) CurrDesc->Union.DataBlock) + CurrDesc->Length;
        CurrDesc  = BlockList;
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
      CurrDesc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) (UINTN) CurrDesc->Union.ContinuationPointer;
    }
  }
  return MemBase;
}

/**
  Check the integrity of the capsule descriptors.

  @param BlockList    Pointer to the capsule descriptors

  @retval NULL           BlockList is not valid.
  @retval LastBlockDesc  Last one Block in BlockList

**/
EFI_CAPSULE_BLOCK_DESCRIPTOR *
ValidateCapsuleIntegrity (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR    *BlockList
  )
{
  EFI_CAPSULE_HEADER             *CapsuleHeader;
  UINT64                         CapsuleSize;
  UINT32                         CapsuleCount;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *Ptr;

  //
  // Go through the list to look for inconsistencies. Check for:
  //   * misaligned block descriptors.
  //   * The first capsule header guid
  //   * The first capsule header flag
  //   * Data + Length < Data (wrap)
  CapsuleSize  = 0;
  CapsuleCount = 0;
  Ptr = BlockList;
  while ((Ptr->Length != 0) || (Ptr->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS) (UINTN) NULL)) {
    //
    // Make sure the descriptor is aligned at UINT64 in memory
    //
    if ((UINTN) Ptr & 0x07) {
      DEBUG ((EFI_D_ERROR, "BlockList address failed alignment check\n"));
      return NULL;
    }

    if (Ptr->Length == 0) {
      //
      // Descriptor points to another list of block descriptors somewhere
      // else.
      //
      Ptr = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) (UINTN) Ptr->Union.ContinuationPointer;
    } else {
      //
      //To enhance the reliability of check-up, the first capsule's header is checked here.
      //More reliabilities check-up will do later.
      //
      if (CapsuleSize == 0) {
        //
        //Move to the first capsule to check its header.
        //
        CapsuleHeader = (EFI_CAPSULE_HEADER*)((UINTN)Ptr->Union.DataBlock);
        if (IsCapsuleCorrupted (CapsuleHeader)) {
          return NULL;
        }
        CapsuleCount ++;
        CapsuleSize = CapsuleHeader->CapsuleImageSize;
      }

      if (CapsuleSize >= Ptr->Length) {
        CapsuleSize = CapsuleSize - Ptr->Length;
      } else {
        CapsuleSize = 0;
      }

      //
      // Move to next BLOCK descriptor
      //
      Ptr++;
    }
  }

  if ((CapsuleCount == 0) || (CapsuleSize != 0)) {
    //
    // No any capsule is found in BlockList or capsule data is corrupted.
    //
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

  @param PeiServices pointer to PEI services table
  @param BlockList   pointer to the capsule block descriptors
  @param MemBase     base of system memory in which we can work
  @param MemSize     size of the system memory pointed to by MemBase

  @retval NULL    could not relocate the descriptors
  @retval Pointer to the base of the successfully-relocated block descriptors. 

**/
EFI_CAPSULE_BLOCK_DESCRIPTOR  *
RelocateBlockDescriptors (
  IN EFI_PEI_SERVICES                   **PeiServices,
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR       *BlockList,
  IN UINT8                              *MemBase,
  IN UINTN                              MemSize
  )
{
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *NewBlockList;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *CurrBlockDescHead;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *TempBlockDesc;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *PrevBlockDescTail;
  UINTN                          NumDescriptors;
  UINTN                          BufferSize;
  UINT8                          *RelocBuffer;
  UINTN                          BlockListSize;
  //
  // Get the info on the blocks and descriptors. Since we're going to move
  // the descriptors low in memory, adjust the base/size values accordingly here.
  // GetCapsuleInfo() returns the number of legit descriptors, so add one for
  // a terminator.
  //
  if (GetCapsuleInfo (BlockList, &NumDescriptors, NULL) != EFI_SUCCESS) {
    return NULL;
  }

  NumDescriptors++;
  BufferSize    = NumDescriptors * sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
  NewBlockList  = (EFI_CAPSULE_BLOCK_DESCRIPTOR *) MemBase;
  if (MemSize < BufferSize) {
    return NULL;
  }

  MemSize -= BufferSize;
  MemBase += BufferSize;
  //
  // Go through all the blocks and make sure none are in the way
  //
  TempBlockDesc = BlockList;
  while (TempBlockDesc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS) (UINTN) NULL) {
    if (TempBlockDesc->Length == 0) {
      //
      // Next block of descriptors
      //
      TempBlockDesc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) (UINTN) TempBlockDesc->Union.ContinuationPointer;
    } else {
      //
      // If the capsule data pointed to by this descriptor is in the way,
      // move it.
      //
      if (IsOverlapped (
            (UINT8 *) NewBlockList,
            BufferSize,
            (UINT8 *) (UINTN) TempBlockDesc->Union.DataBlock,
            (UINTN) TempBlockDesc->Length
            )) {
        //
        // Relocate the block
        //
        RelocBuffer = FindFreeMem (BlockList, MemBase, MemSize, (UINTN) TempBlockDesc->Length);
        if (RelocBuffer == NULL) {
          return NULL;
        }

        CopyMem ((VOID *) RelocBuffer, (VOID *) (UINTN) TempBlockDesc->Union.DataBlock, (UINTN) TempBlockDesc->Length);
        DEBUG ((EFI_D_INFO, "Capsule relocate descriptors from/to/size  0x%X 0x%X 0x%X\n", (UINT32)(UINTN)TempBlockDesc->Union.DataBlock, (UINT32)(UINTN)RelocBuffer, (UINT32)(UINTN)TempBlockDesc->Length));
        TempBlockDesc->Union.DataBlock = (EFI_PHYSICAL_ADDRESS) (UINTN) RelocBuffer;
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
  while ((CurrBlockDescHead != NULL) && (CurrBlockDescHead->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS) (UINTN) NULL)) {
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
          (UINT8 *) NewBlockList,
          BufferSize,
          (UINT8 *) CurrBlockDescHead,
          BlockListSize
          )) {
      //
      // Overlaps, so move it out of the way
      //
      RelocBuffer = FindFreeMem (BlockList, MemBase, MemSize, BlockListSize);
      if (RelocBuffer == NULL) {
        return NULL;
      }
      CopyMem ((VOID *) RelocBuffer, (VOID *) CurrBlockDescHead, BlockListSize);
      DEBUG ((EFI_D_INFO, "Capsule reloc descriptor block #2\n"));
      //
      // Point the previous block's next point to this copied version. If
      // the tail pointer is null, then this is the first descriptor block.
      //
      if (PrevBlockDescTail == NULL) {
        BlockList = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) RelocBuffer;
      } else {
        PrevBlockDescTail->Union.DataBlock = (EFI_PHYSICAL_ADDRESS) (UINTN) RelocBuffer;
      }
    }
    //
    // Save our new tail and jump to the next block list
    //
    PrevBlockDescTail = TempBlockDesc;
    CurrBlockDescHead = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) (UINTN) TempBlockDesc->Union.ContinuationPointer;
  }
  //
  // Cleared out low memory. Now copy the descriptors down there.
  //
  TempBlockDesc     = BlockList;
  CurrBlockDescHead = NewBlockList;
  while ((TempBlockDesc != NULL) && (TempBlockDesc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS) (UINTN) NULL)) {
    if (TempBlockDesc->Length != 0) {
      CurrBlockDescHead->Union.DataBlock = TempBlockDesc->Union.DataBlock;
      CurrBlockDescHead->Length = TempBlockDesc->Length;
      CurrBlockDescHead++;
      TempBlockDesc++;
    } else {
      TempBlockDesc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) (UINTN) TempBlockDesc->Union.ContinuationPointer;
    }
  }
  //
  // Null terminate
  //
  CurrBlockDescHead->Union.ContinuationPointer   = (EFI_PHYSICAL_ADDRESS) (UINTN) NULL;
  CurrBlockDescHead->Length = 0;
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
  UINT8     *Buff1,
  UINTN     Size1,
  UINT8     *Buff2,
  UINTN     Size2
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
                         NumDescriptors  - optional pointer to where to return the number of descriptors
                         CapsuleSize     - optional pointer to where to return the capsule size
  @param NumDescriptors  Optional pointer to where to return the number of descriptors
  @param CapsuleSize     Optional pointer to where to return the capsule size

  @retval EFI_NOT_FOUND   No descriptors containing data in the list
  @retval EFI_SUCCESS     Return data is valid

**/
EFI_STATUS
GetCapsuleInfo (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR   *Desc,
  IN OUT UINTN                      *NumDescriptors OPTIONAL,
  IN OUT UINTN                      *CapsuleSize OPTIONAL
  )
{
  UINTN Count;
  UINTN Size;

  ASSERT (Desc != NULL);

  Count = 0;
  Size  = 0;

  while (Desc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS) (UINTN) NULL) {
    if (Desc->Length == 0) {
      //
      // Descriptor points to another list of block descriptors somewhere
      //
      Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) (UINTN) Desc->Union.ContinuationPointer;
    } else {
      Size += (UINTN) Desc->Length;
      Count++;
      Desc++;
    }
  }
  //
  // If no descriptors, then fail
  //
  if (Count == 0) {
    return EFI_NOT_FOUND;
  }

  if (NumDescriptors != NULL) {
    *NumDescriptors = Count;
  }

  if (CapsuleSize != NULL) {
    *CapsuleSize = Size;
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
  IN EFI_CAPSULE_HEADER       *CapsuleHeader
  )
{
  //
  //A capsule to be updated across a system reset should contain CAPSULE_FLAGS_PERSIST_ACROSS_RESET.
  //
  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) == 0) {
    return TRUE;
  }
  //
  //Make sure the flags combination is supported by the platform.
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
  //
  // Find first data descriptor
  //
  while ((Desc->Length == 0) && (Desc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS) (UINTN) NULL)) {
    Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) (UINTN) Desc->Union.ContinuationPointer;
  }

  if (Desc->Union.ContinuationPointer == 0) {
    return ;
  }
  //
  // First one better be long enough to at least hold the test signature
  //
  if (Desc->Length < sizeof (UINT32)) {
    DEBUG ((EFI_D_INFO, "Capsule test pattern pre-coalesce punted #1\n"));
    return ;
  }

  TestPtr = (UINT32 *) (UINTN) Desc->Union.DataBlock;
  //
  // 0x54534554 "TEST"
  //
  if (*TestPtr != 0x54534554) {
    return ;
  }

  TestCounter = 0;
  TestSize    = (UINT32) Desc->Length - 2 * sizeof (UINT32);
  //
  // Skip over the signature and the size fields in the pattern data header
  //
  TestPtr += 2;
  while (1) {
    if ((TestSize & 0x03) != 0) {
      DEBUG ((EFI_D_INFO, "Capsule test pattern pre-coalesce punted #2\n"));
      return ;
    }

    while (TestSize > 0) {
      if (*TestPtr != TestCounter) {
        DEBUG ((EFI_D_INFO, "Capsule test pattern pre-coalesce failed data corruption check\n"));
        return ;
      }

      TestSize -= sizeof (UINT32);
      TestCounter++;
      TestPtr++;
    }
    Desc++;
    while ((Desc->Length == 0) && (Desc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS) (UINTN) NULL)) {
      Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) (UINTN) Desc->Union.ContinuationPointer;
    }

    if (Desc->Union.ContinuationPointer == (EFI_PHYSICAL_ADDRESS) (UINTN) NULL) {
      return ;
    }
    TestSize = (UINT32) Desc->Length;
    TestPtr  = (UINT32 *) (UINTN) Desc->Union.DataBlock;
  }
}

/**
  Checks for the presence of capsule descriptors.
  Get capsule descriptors from variable CapsuleUpdateData, CapsuleUpdateData1, CapsuleUpdateData2...

  @param BlockListBuffer            Pointer to the buffer of capsule descriptors variables
  @param BlockDescriptorList        Pointer to the capsule descriptors list

  @retval EFI_SUCCESS               a valid capsule is present
  @retval EFI_NOT_FOUND             if a valid capsule is not present
**/
EFI_STATUS
BuildCapsuleDescriptors (
  IN  EFI_PHYSICAL_ADDRESS            *BlockListBuffer,
  OUT EFI_CAPSULE_BLOCK_DESCRIPTOR    **BlockDescriptorList 
  )
{
  UINTN                            Index;
  EFI_CAPSULE_BLOCK_DESCRIPTOR     *LastBlock;
  EFI_CAPSULE_BLOCK_DESCRIPTOR     *TempBlock;
  EFI_CAPSULE_BLOCK_DESCRIPTOR     *HeadBlock;

  LastBlock         = NULL;
  HeadBlock         = NULL;
  TempBlock         = NULL;
  Index             = 0;

  while (BlockListBuffer[Index] != 0) {
    //
    // Test integrity of descriptors.
    //
    TempBlock = ValidateCapsuleIntegrity ((EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)BlockListBuffer[Index]);
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
    Index ++;
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
  MemSize          |    CapsuleOffset[49]      |            |
                   +---------------------------+            |
                   |    ................       |            |
                   +---------------------------+            |
                   |    CapsuleOffset[2]       |            |
                   +---------------------------+            |
                   |    CapsuleOffset[1]       |            |
                   +---------------------------+            |
                   |    CapsuleOffset[0]       |       CapsuleSize     
                   +---------------------------+            |
                   |    CapsuleNumber          |            |
                   +---------------------------+            |
                   |                           |            |       
                   |                           |            |       
                   |    Capsule Image          |            |   
                   |                           |            |       
                   |                           |            |       
                   +---------------------------+            |
                   |    PrivateData            |            |
   DestPtr  ---->  +---------------------------+<-----------+
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
  @param BlockListBuffer    Point to the buffer of Capsule Descriptor Variables.
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
  IN EFI_PEI_SERVICES                **PeiServices,
  IN EFI_PHYSICAL_ADDRESS            *BlockListBuffer,
  IN OUT VOID                        **MemoryBase,
  IN OUT UINTN                       *MemorySize
  )
{
  VOID                           *NewCapsuleBase;
  VOID                           *DataPtr;
  UINT8                          CapsuleIndex;
  UINT8                          *FreeMemBase;
  UINT8                          *DestPtr;
  UINT8                          *RelocPtr;
  UINT32                         CapsuleOffset[MAX_SUPPORT_CAPSULE_NUM]; 
  UINT32                         *AddDataPtr;
  UINT32                         CapsuleTimes; 
  UINT64                         SizeLeft; 
  UINT64                         CapsuleImageSize; 
  UINTN                          CapsuleSize;
  UINTN                          DescriptorsSize;
  UINTN                          FreeMemSize;
  UINTN                          NumDescriptors;
  BOOLEAN                        IsCorrupted;
  BOOLEAN                        CapsuleBeginFlag;
  EFI_STATUS                     Status;
  EFI_CAPSULE_HEADER             *CapsuleHeader;
  EFI_CAPSULE_PEIM_PRIVATE_DATA  PrivateData;
  EFI_CAPSULE_PEIM_PRIVATE_DATA  *PrivateDataPtr;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *BlockList;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *CurrentBlockDesc;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *TempBlockDesc;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   PrivateDataDesc[2];

  CapsuleIndex     = 0;
  SizeLeft         = 0;
  CapsuleTimes     = 0;
  CapsuleImageSize = 0;
  PrivateDataPtr   = NULL;
  AddDataPtr       = NULL;
  CapsuleHeader    = NULL;
  CapsuleBeginFlag = TRUE;
  IsCorrupted      = TRUE;
  CapsuleSize      = 0;
  NumDescriptors   = 0;
  
  //
  // Build capsule descriptors list
  //
  Status = BuildCapsuleDescriptors (BlockListBuffer, &BlockList);
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
  GetCapsuleInfo (BlockList, &NumDescriptors, &CapsuleSize);
  if ((CapsuleSize == 0) || (NumDescriptors == 0)) {
    return EFI_NOT_FOUND;
  }

  //
  // Initialize our local copy of private data. When we're done, we'll create a
  // descriptor for it as well so that it can be put into free memory without
  // trashing anything.
  //
  PrivateData.Signature     = EFI_CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE;
  PrivateData.CapsuleSize   = (UINT32) CapsuleSize;
  PrivateDataDesc[0].Union.DataBlock  = (EFI_PHYSICAL_ADDRESS) (UINTN) &PrivateData;
  PrivateDataDesc[0].Length           = sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA);
  PrivateDataDesc[1].Union.DataBlock  = (EFI_PHYSICAL_ADDRESS) (UINTN) BlockList;
  PrivateDataDesc[1].Length           = 0;
  //
  // In addition to PrivateDataDesc[1:0], one terminator is added
  // See below RelocateBlockDescriptors()
  //
  NumDescriptors  += 3;
  CapsuleSize     += sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) + sizeof(CapsuleOffset) + sizeof(UINT32);
  BlockList        = PrivateDataDesc;
  DescriptorsSize  = NumDescriptors * sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);

  //
  // Don't go below some min address. If the base is below it,
  // then move it up and adjust the size accordingly.
  //
  DEBUG ((EFI_D_INFO, "Capsule Memory range from 0x%8X to 0x%8X\n", (UINTN) *MemoryBase, (UINTN)*MemoryBase + *MemorySize));
  if ((UINTN)*MemoryBase < (UINTN) MIN_COALESCE_ADDR) {
    if (((UINTN)*MemoryBase + *MemorySize) < (UINTN) MIN_COALESCE_ADDR) {
      return EFI_BUFFER_TOO_SMALL;
    } else {
      *MemorySize = *MemorySize - ((UINTN) MIN_COALESCE_ADDR - (UINTN) *MemoryBase);
      *MemoryBase = (VOID *) (UINTN) MIN_COALESCE_ADDR;
    }
  }

  if (*MemorySize <= (CapsuleSize + DescriptorsSize)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  FreeMemBase = *MemoryBase;
  FreeMemSize = *MemorySize;
  DEBUG ((EFI_D_INFO, "Capsule Free Memory from 0x%8X to 0x%8X\n", (UINTN) FreeMemBase, (UINTN) FreeMemBase + FreeMemSize));

  //
  // Relocate all the block descriptors to low memory to make further
  // processing easier.
  //
  BlockList = RelocateBlockDescriptors (PeiServices, BlockList, FreeMemBase, FreeMemSize);
  if (BlockList == NULL) {
    //
    // Not enough room to relocate the descriptors
    //
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Take the top of memory for the capsule. Naturally align.
  //
  DestPtr         = FreeMemBase + FreeMemSize - CapsuleSize;
  DestPtr         = (UINT8 *) ((UINTN) DestPtr &~ (UINTN) (sizeof (UINTN) - 1));
  FreeMemBase     = (UINT8 *) BlockList + DescriptorsSize;
  FreeMemSize     = (UINTN) DestPtr - (UINTN) FreeMemBase;
  NewCapsuleBase  = (VOID *) DestPtr;

  //
  // Move all the blocks to the top (high) of memory.
  // Relocate all the obstructing blocks. Note that the block descriptors
  // were coalesced when they were relocated, so we can just ++ the pointer.
  //
  CurrentBlockDesc = BlockList;
  while ((CurrentBlockDesc->Length != 0) || (CurrentBlockDesc->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS) (UINTN) NULL)) {
    //
    // See if any of the remaining capsule blocks are in the way
    //
    TempBlockDesc = CurrentBlockDesc;
    while (TempBlockDesc->Length != 0) {
      //
      // Is this block in the way of where we want to copy the current descriptor to?
      //
      if (IsOverlapped (
            (UINT8 *) DestPtr,
            (UINTN) CurrentBlockDesc->Length,
            (UINT8 *) (UINTN) TempBlockDesc->Union.DataBlock,
            (UINTN) TempBlockDesc->Length
            )) {
        //
        // Relocate the block
        //
        RelocPtr = FindFreeMem (BlockList, FreeMemBase, FreeMemSize, (UINTN) TempBlockDesc->Length);
        if (RelocPtr == NULL) {
          return EFI_BUFFER_TOO_SMALL;
        }

        CopyMem ((VOID *) RelocPtr, (VOID *) (UINTN) TempBlockDesc->Union.DataBlock, (UINTN) TempBlockDesc->Length);
        DEBUG ((EFI_D_INFO, "Capsule reloc data block from 0x%8X to 0x%8X with size 0x%8X\n",
                (UINTN) TempBlockDesc->Union.DataBlock, (UINTN) RelocPtr, (UINTN) TempBlockDesc->Length));

        TempBlockDesc->Union.DataBlock = (EFI_PHYSICAL_ADDRESS) (UINTN) RelocPtr;
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
    CapsuleTimes ++;
    //
    //Skip the first block descriptor that filled with EFI_CAPSULE_PEIM_PRIVATE_DATA
    //
    if (CapsuleTimes > 1) {
      //
      //For every capsule entry point, check its header to determine whether to relocate it.
      //If it is invalid, skip it and move on to the next capsule. If it is valid, relocate it.
      //
      if (CapsuleBeginFlag) {
        CapsuleBeginFlag  = FALSE;
        CapsuleHeader     = (EFI_CAPSULE_HEADER*)(UINTN)CurrentBlockDesc->Union.DataBlock;
        SizeLeft          = CapsuleHeader->CapsuleImageSize;
        if (!IsCapsuleCorrupted (CapsuleHeader)) {

          if (CapsuleIndex > (MAX_SUPPORT_CAPSULE_NUM - 1)) {
            DEBUG ((EFI_D_ERROR, "Capsule number exceeds the max number of %d!\n", MAX_SUPPORT_CAPSULE_NUM));
            return  EFI_BUFFER_TOO_SMALL;
          }

          //
          // Relocate this valid capsule
          //
          IsCorrupted  = FALSE;
          CapsuleImageSize += SizeLeft;
          //
          // Cache the begin offset of this capsule
          //
          CapsuleOffset[CapsuleIndex++] = (UINT32) (UINTN) DestPtr - (UINT32)(UINTN)NewCapsuleBase - (UINT32)sizeof(EFI_CAPSULE_PEIM_PRIVATE_DATA);
        }
      }

      if (CurrentBlockDesc->Length < SizeLeft) {
        if (!IsCorrupted) {
          CopyMem ((VOID *) DestPtr, (VOID *) (UINTN) (CurrentBlockDesc->Union.DataBlock), (UINTN)CurrentBlockDesc->Length);
          DEBUG ((EFI_D_INFO, "Capsule coalesce block no.0x%8X from 0x%8lX to 0x%8lX with size 0x%8X\n",CapsuleTimes,
                 (UINTN)CurrentBlockDesc->Union.DataBlock, (UINTN)DestPtr, (UINTN)CurrentBlockDesc->Length));
          DestPtr += CurrentBlockDesc->Length;
        }
        SizeLeft -= CurrentBlockDesc->Length;
      } else {
        //
        //Here is the end of the current capsule image.
        //
        if (!IsCorrupted) {
          CopyMem ((VOID *) DestPtr, (VOID *)(UINTN)(CurrentBlockDesc->Union.DataBlock), (UINTN) SizeLeft);
          DEBUG ((EFI_D_INFO, "Capsule coalesce block no.0x%8X from 0x%8lX to 0x%8lX with size 0x%8X\n",CapsuleTimes,
                 (UINTN)CurrentBlockDesc->Union.DataBlock, (UINTN)DestPtr, (UINTN) SizeLeft));
          DestPtr += SizeLeft;
        }
        //
        // Start the next cycle
        //
        SizeLeft = 0;
        IsCorrupted = TRUE;
        CapsuleBeginFlag = TRUE; 
      }
    } else {
      //
      //The first entry is the block descriptor for EFI_CAPSULE_PEIM_PRIVATE_DATA.
      //
      CopyMem ((VOID *) DestPtr, (VOID *) (UINTN) CurrentBlockDesc->Union.DataBlock, (UINTN) CurrentBlockDesc->Length);
      DestPtr += CurrentBlockDesc->Length;
    }
    //
    //Walk through the block descriptor list.
    //
    CurrentBlockDesc++;
  }
  //
  // We return the base of memory we want reserved, and the size.
  // The memory peim should handle it appropriately from there.
  //
  *MemorySize = (UINTN) CapsuleSize;
  *MemoryBase = (VOID *) NewCapsuleBase;

  //
  //Append the offsets of mutiply capsules to the continous buffer
  //
  DataPtr    = (VOID*)((UINTN)NewCapsuleBase + sizeof(EFI_CAPSULE_PEIM_PRIVATE_DATA) + (UINTN)CapsuleImageSize);
  AddDataPtr = (UINT32*)(((UINTN) DataPtr + sizeof(UINT32) - 1) &~ (UINT32) (sizeof (UINT32) - 1));

  *AddDataPtr++ = CapsuleIndex;

  CopyMem (AddDataPtr, &CapsuleOffset[0], sizeof (UINT32) * CapsuleIndex);

  PrivateDataPtr = (EFI_CAPSULE_PEIM_PRIVATE_DATA *) NewCapsuleBase;
  PrivateDataPtr->CapsuleSize = (UINT32) CapsuleImageSize;

  return EFI_SUCCESS;
}
