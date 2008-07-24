/** @file
  UEFI Memory pool management functions.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <DxeMain.h>

#define POOL_FREE_SIGNATURE   EFI_SIGNATURE_32('p','f','r','0')
typedef struct {
  UINT32          Signature;
  UINT32          Index;
  LIST_ENTRY      Link;
} POOL_FREE;


#define POOL_HEAD_SIGNATURE   EFI_SIGNATURE_32('p','h','d','0')
typedef struct {
  UINT32          Signature;
  UINT32          Size;
  EFI_MEMORY_TYPE Type;
  UINTN           Reserved;
  CHAR8           Data[1];
} POOL_HEAD;

#define SIZE_OF_POOL_HEAD EFI_FIELD_OFFSET(POOL_HEAD,Data)

#define POOL_TAIL_SIGNATURE   EFI_SIGNATURE_32('p','t','a','l')
typedef struct {
  UINT32      Signature;
  UINT32      Size;
} POOL_TAIL;


#define POOL_SHIFT  7

#define POOL_OVERHEAD (SIZE_OF_POOL_HEAD + sizeof(POOL_TAIL))

#define HEAD_TO_TAIL(a)   \
  ((POOL_TAIL *) (((CHAR8 *) (a)) + (a)->Size - sizeof(POOL_TAIL)));


#define SIZE_TO_LIST(a)   ((a) >> POOL_SHIFT)
#define LIST_TO_SIZE(a)   ((a+1) << POOL_SHIFT)

#define MAX_POOL_LIST       SIZE_TO_LIST(DEFAULT_PAGE_ALLOCATION)

#define MAX_POOL_SIZE     (MAX_ADDRESS - POOL_OVERHEAD)

//
// Globals
//

#define POOL_SIGNATURE  EFI_SIGNATURE_32('p','l','s','t')
typedef struct {
    INTN             Signature;
    UINTN            Used;
    EFI_MEMORY_TYPE  MemoryType;
    LIST_ENTRY       FreeList[MAX_POOL_LIST];
    LIST_ENTRY       Link;
} POOL;

//
// Pool header for each memory type.
//
POOL            mPoolHead[EfiMaxMemoryType];

//
// List of pool header to search for the appropriate memory type.
//
LIST_ENTRY      mPoolHeadList;


/**
  Called to initialize the pool.

**/
VOID
CoreInitializePool (
  VOID
  )
{
  UINTN  Type;
  UINTN  Index;

  for (Type=0; Type < EfiMaxMemoryType; Type++) {
    mPoolHead[Type].Signature  = 0;
    mPoolHead[Type].Used       = 0;
    mPoolHead[Type].MemoryType = (EFI_MEMORY_TYPE) Type;
    for (Index=0; Index < MAX_POOL_LIST; Index++) {
        InitializeListHead (&mPoolHead[Type].FreeList[Index]);
    }
  }
  InitializeListHead (&mPoolHeadList);
}


/**
  Look up pool head for specified memory type.

  @param  MemoryType             Memory type of which pool head is looked for

  @return Pointer of Corresponding pool head.

**/
POOL *
LookupPoolHead (
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  LIST_ENTRY      *Link;
  POOL            *Pool;
  UINTN           Index;

  if (MemoryType >= 0 && MemoryType < EfiMaxMemoryType) {
    return &mPoolHead[MemoryType];
  }

  if (MemoryType < 0) {

    for (Link = mPoolHeadList.ForwardLink; Link != &mPoolHeadList; Link = Link->ForwardLink) {
      Pool = CR(Link, POOL, Link, POOL_SIGNATURE);
      if (Pool->MemoryType == MemoryType) {
        return Pool;
      }
    }

    Pool = CoreAllocatePoolI (EfiBootServicesData, sizeof (POOL));
    if (Pool == NULL) {
      return NULL;
    }

    Pool->Signature = POOL_SIGNATURE;
    Pool->Used      = 0;
    Pool->MemoryType = MemoryType;
    for (Index=0; Index < MAX_POOL_LIST; Index++) {
      InitializeListHead (&Pool->FreeList[Index]);
    }

    InsertHeadList (&mPoolHeadList, &Pool->Link);

    return Pool;
  }

  return NULL;
}



/**
  Allocate pool of a particular type.

  @param  PoolType               Type of pool to allocate
  @param  Size                   The amount of pool to allocate
  @param  Buffer                 The address to return a pointer to the allocated
                                 pool

  @retval EFI_INVALID_PARAMETER  PoolType not valid
  @retval EFI_OUT_OF_RESOURCES   Size exceeds max pool size or allocation failed.
  @retval EFI_SUCCESS            Pool successfully allocated.

**/
EFI_STATUS
EFIAPI
CoreAllocatePool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            Size,
  OUT VOID            **Buffer
  )
{
  EFI_STATUS    Status;

  //
  // If it's not a valid type, fail it
  //
  if ((PoolType >= EfiMaxMemoryType && PoolType <= 0x7fffffff) ||
       PoolType == EfiConventionalMemory) {
    return EFI_INVALID_PARAMETER;
  }

  *Buffer = NULL;

  //
  // If size is too large, fail it
  // Base on the EFI spec, return status of EFI_OUT_OF_RESOURCES
  //
  if (Size > MAX_POOL_SIZE) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Acquire the memory lock and make the allocation
  //
  Status = CoreAcquireLockOrFail (&gMemoryLock);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Buffer = CoreAllocatePoolI (PoolType, Size);
  CoreReleaseMemoryLock ();
  return (*Buffer != NULL) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
}



/**
  Internal function to allocate pool of a particular type.
  Caller must have the memory lock held

  @param  PoolType               Type of pool to allocate
  @param  Size                   The amount of pool to allocate

  @return The allocate pool, or NULL

**/
VOID *
CoreAllocatePoolI (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            Size
  )
{
  POOL        *Pool;
  POOL_FREE   *Free;
  POOL_HEAD   *Head;
  POOL_TAIL   *Tail;
  CHAR8       *NewPage;
  VOID        *Buffer;
  UINTN       Index;
  UINTN       FSize;
  UINTN       Offset;
  UINTN       Adjustment;
  UINTN       NoPages;

  ASSERT_LOCKED (&gMemoryLock);

  //
  // Adjust the size by the pool header & tail overhead
  //

  //
  // Adjusting the Size to be of proper alignment so that
  // we don't get an unaligned access fault later when
  // pool_Tail is being initialized
  //
  ALIGN_VARIABLE (Size, Adjustment);

  Size += POOL_OVERHEAD;
  Index = SIZE_TO_LIST(Size);
  Pool = LookupPoolHead (PoolType);
  if (Pool== NULL) {
    return NULL;
  }
  Head = NULL;

  //
  // If allocation is over max size, just allocate pages for the request
  // (slow)
  //
  if (Index >= MAX_POOL_LIST) {
    NoPages = EFI_SIZE_TO_PAGES(Size) + EFI_SIZE_TO_PAGES (DEFAULT_PAGE_ALLOCATION) - 1;
    NoPages &= ~(EFI_SIZE_TO_PAGES (DEFAULT_PAGE_ALLOCATION) - 1);
    Head = CoreAllocatePoolPages (PoolType, NoPages, DEFAULT_PAGE_ALLOCATION);
    goto Done;
  }

  //
  // If there's no free pool in the proper list size, go get some more pages
  //
  if (IsListEmpty (&Pool->FreeList[Index])) {

    //
    // Get another page
    //
    NewPage = CoreAllocatePoolPages(PoolType, EFI_SIZE_TO_PAGES (DEFAULT_PAGE_ALLOCATION), DEFAULT_PAGE_ALLOCATION);
    if (NewPage == NULL) {
      goto Done;
    }

    //
    // Carve up new page into free pool blocks
    //
    Offset = 0;
    while (Offset < DEFAULT_PAGE_ALLOCATION) {
      ASSERT (Index < MAX_POOL_LIST);
      FSize = LIST_TO_SIZE(Index);

      while (Offset + FSize <= DEFAULT_PAGE_ALLOCATION) {
        Free = (POOL_FREE *) &NewPage[Offset];
        Free->Signature = POOL_FREE_SIGNATURE;
        Free->Index     = (UINT32)Index;
        InsertHeadList (&Pool->FreeList[Index], &Free->Link);
        Offset += FSize;
      }

      Index -= 1;
    }

    ASSERT (Offset == DEFAULT_PAGE_ALLOCATION);
    Index = SIZE_TO_LIST(Size);
  }

  //
  // Remove entry from free pool list
  //
  Free = CR (Pool->FreeList[Index].ForwardLink, POOL_FREE, Link, POOL_FREE_SIGNATURE);
  RemoveEntryList (&Free->Link);

  Head = (POOL_HEAD *) Free;

Done:
  Buffer = NULL;

  if (Head != NULL) {

    //
    // If we have a pool buffer, fill in the header & tail info
    //
    Head->Signature = POOL_HEAD_SIGNATURE;
    Head->Size      = (UINT32) Size;
    Head->Type      = (EFI_MEMORY_TYPE) PoolType;
    Tail            = HEAD_TO_TAIL (Head);
    Tail->Signature = POOL_TAIL_SIGNATURE;
    Tail->Size      = (UINT32) Size;
    Buffer          = Head->Data;
    DEBUG_CLEAR_MEMORY (Buffer, Size - POOL_OVERHEAD);

    DEBUG ((
      DEBUG_POOL,
      "AllocatePoolI: Type %x, Addr %x (len %x) %,d\n", PoolType,
      Buffer,
      Size - POOL_OVERHEAD,
      Pool->Used
      ));

    //
    // Account the allocation
    //
    Pool->Used += Size;

  } else {
    DEBUG ((DEBUG_ERROR | DEBUG_POOL, "AllocatePool: failed to allocate %d bytes\n", Size));
  }

  return Buffer;
}



/**
  Frees pool.

  @param  Buffer                 The allocated pool entry to free

  @retval EFI_INVALID_PARAMETER  Buffer is not a valid value.
  @retval EFI_SUCCESS            Pool successfully freed.

**/
EFI_STATUS
EFIAPI
CoreFreePool (
  IN VOID        *Buffer
  )
{
  EFI_STATUS Status;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireMemoryLock ();
  Status = CoreFreePoolI (Buffer);
  CoreReleaseMemoryLock ();
  return Status;
}



/**
  Internal function to free a pool entry.
  Caller must have the memory lock held

  @param  Buffer                 The allocated pool entry to free

  @retval EFI_INVALID_PARAMETER  Buffer not valid
  @retval EFI_SUCCESS            Buffer successfully freed.

**/
EFI_STATUS
CoreFreePoolI (
  IN VOID       *Buffer
  )
{
  POOL        *Pool;
  POOL_HEAD   *Head;
  POOL_TAIL   *Tail;
  POOL_FREE   *Free;
  UINTN       Index;
  UINTN       NoPages;
  UINTN       Size;
  CHAR8       *NewPage;
  UINTN       FSize;
  UINTN       Offset;
  BOOLEAN     AllFree;

  ASSERT(NULL != Buffer);
  //
  // Get the head & tail of the pool entry
  //
  Head = CR (Buffer, POOL_HEAD, Data, POOL_HEAD_SIGNATURE);
  ASSERT(NULL != Head);

  if (Head->Signature != POOL_HEAD_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  Tail = HEAD_TO_TAIL (Head);
  ASSERT(NULL != Tail);

  //
  // Debug
  //
  ASSERT (Tail->Signature == POOL_TAIL_SIGNATURE);
  ASSERT (Head->Size == Tail->Size);
  ASSERT_LOCKED (&gMemoryLock);

  if (Tail->Signature != POOL_TAIL_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  if (Head->Size != Tail->Size) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine the pool type and account for it
  //
  Size = Head->Size;
  Pool = LookupPoolHead (Head->Type);
  if (Pool == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Pool->Used -= Size;
  DEBUG ((DEBUG_POOL, "FreePool: %x (len %x) %,d\n", Head->Data, Head->Size - POOL_OVERHEAD, Pool->Used));

  //
  // Determine the pool list
  //
  Index = SIZE_TO_LIST(Size);
  DEBUG_CLEAR_MEMORY (Head, Size);

  //
  // If it's not on the list, it must be pool pages
  //
  if (Index >= MAX_POOL_LIST) {

    //
    // Return the memory pages back to free memory
    //
    NoPages = EFI_SIZE_TO_PAGES(Size) + EFI_SIZE_TO_PAGES (DEFAULT_PAGE_ALLOCATION) - 1;
    NoPages &= ~(EFI_SIZE_TO_PAGES (DEFAULT_PAGE_ALLOCATION) - 1);
    CoreFreePoolPages ((EFI_PHYSICAL_ADDRESS) (UINTN) Head, NoPages);

  } else {

    //
    // Put the pool entry onto the free pool list
    //
    Free = (POOL_FREE *) Head;
    ASSERT(NULL != Free);
    Free->Signature = POOL_FREE_SIGNATURE;
    Free->Index     = (UINT32)Index;
    InsertHeadList (&Pool->FreeList[Index], &Free->Link);

    //
    // See if all the pool entries in the same page as Free are freed pool
    // entries
    //
    NewPage = (CHAR8 *)((UINTN)Free & ~((DEFAULT_PAGE_ALLOCATION) -1));
    Free = (POOL_FREE *) &NewPage[0];
    ASSERT(Free != NULL);

    if (Free->Signature == POOL_FREE_SIGNATURE) {

      Index = Free->Index;

      AllFree = TRUE;
      Offset = 0;

      while ((Offset < DEFAULT_PAGE_ALLOCATION) && (AllFree)) {
        FSize = LIST_TO_SIZE(Index);
        while (Offset + FSize <= DEFAULT_PAGE_ALLOCATION) {
          Free = (POOL_FREE *) &NewPage[Offset];
          ASSERT(NULL != Free);
          if (Free->Signature != POOL_FREE_SIGNATURE) {
            AllFree = FALSE;
          }
          Offset += FSize;
        }
        Index -= 1;
      }

      if (AllFree) {

        //
        // All of the pool entries in the same page as Free are free pool
        // entries
        // Remove all of these pool entries from the free loop lists.
        //
        Free = (POOL_FREE *) &NewPage[0];
        ASSERT(NULL != Free);
        Index = Free->Index;
        Offset = 0;

        while (Offset < DEFAULT_PAGE_ALLOCATION) {
          FSize = LIST_TO_SIZE(Index);
          while (Offset + FSize <= DEFAULT_PAGE_ALLOCATION) {
            Free = (POOL_FREE *) &NewPage[Offset];
            ASSERT(NULL != Free);
            RemoveEntryList (&Free->Link);
            Offset += FSize;
          }
          Index -= 1;
        }

        //
        // Free the page
        //
        CoreFreePoolPages ((EFI_PHYSICAL_ADDRESS) (UINTN)NewPage, EFI_SIZE_TO_PAGES (DEFAULT_PAGE_ALLOCATION));
      }
    }
  }

  //
  // If this is an OS specific memory type, then check to see if the last
  // portion of that memory type has been freed.  If it has, then free the
  // list entry for that memory type
  //
  if (Pool->MemoryType < 0 && Pool->Used == 0) {
    RemoveEntryList (&Pool->Link);
    CoreFreePoolI (Pool);
  }

  return EFI_SUCCESS;
}
