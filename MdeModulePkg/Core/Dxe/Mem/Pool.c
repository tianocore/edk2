/** @file
  UEFI Memory pool management functions.

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"
#include "Imem.h"
#include "HeapGuard.h"

STATIC EFI_LOCK mPoolMemoryLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);

#define POOL_FREE_SIGNATURE   SIGNATURE_32('p','f','r','0')
typedef struct {
  UINT32          Signature;
  UINT32          Index;
  LIST_ENTRY      Link;
} POOL_FREE;


#define POOL_HEAD_SIGNATURE   SIGNATURE_32('p','h','d','0')
typedef struct {
  UINT32          Signature;
  UINT32          Reserved;
  EFI_MEMORY_TYPE Type;
  UINTN           Size;
  CHAR8           Data[1];
} POOL_HEAD;

#define SIZE_OF_POOL_HEAD OFFSET_OF(POOL_HEAD,Data)

#define POOL_TAIL_SIGNATURE   SIGNATURE_32('p','t','a','l')
typedef struct {
  UINT32      Signature;
  UINT32      Reserved;
  UINTN       Size;
} POOL_TAIL;

#define POOL_OVERHEAD (SIZE_OF_POOL_HEAD + sizeof(POOL_TAIL))

#define HEAD_TO_TAIL(a)   \
  ((POOL_TAIL *) (((CHAR8 *) (a)) + (a)->Size - sizeof(POOL_TAIL)));

//
// Each element is the sum of the 2 previous ones: this allows us to migrate
// blocks between bins by splitting them up, while not wasting too much memory
// as we would in a strict power-of-2 sequence
//
STATIC CONST UINT16 mPoolSizeTable[] = {
  128, 256, 384, 640, 1024, 1664, 2688, 4352, 7040, 11392, 18432, 29824
};

#define SIZE_TO_LIST(a)   (GetPoolIndexFromSize (a))
#define LIST_TO_SIZE(a)   (mPoolSizeTable [a])

#define MAX_POOL_LIST     (ARRAY_SIZE (mPoolSizeTable))

#define MAX_POOL_SIZE     (MAX_ADDRESS - POOL_OVERHEAD)

//
// Globals
//

#define POOL_SIGNATURE  SIGNATURE_32('p','l','s','t')
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
LIST_ENTRY      mPoolHeadList = INITIALIZE_LIST_HEAD_VARIABLE (mPoolHeadList);

/**
  Get pool size table index from the specified size.

  @param  Size          The specified size to get index from pool table.

  @return               The index of pool size table.

**/
STATIC
UINTN
GetPoolIndexFromSize (
  UINTN   Size
  )
{
  UINTN   Index;

  for (Index = 0; Index < MAX_POOL_LIST; Index++) {
    if (mPoolSizeTable [Index] >= Size) {
      return Index;
    }
  }
  return MAX_POOL_LIST;
}

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

  if ((UINT32)MemoryType < EfiMaxMemoryType) {
    return &mPoolHead[MemoryType];
  }

  //
  // MemoryType values in the range 0x80000000..0xFFFFFFFF are reserved for use by UEFI
  // OS loaders that are provided by operating system vendors.
  // MemoryType values in the range 0x70000000..0x7FFFFFFF are reserved for OEM use.
  //
  if ((UINT32) MemoryType >= MEMORY_TYPE_OEM_RESERVED_MIN) {

    for (Link = mPoolHeadList.ForwardLink; Link != &mPoolHeadList; Link = Link->ForwardLink) {
      Pool = CR(Link, POOL, Link, POOL_SIGNATURE);
      if (Pool->MemoryType == MemoryType) {
        return Pool;
      }
    }

    Pool = CoreAllocatePoolI (EfiBootServicesData, sizeof (POOL), FALSE);
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

  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
                                 PoolType is in the range EfiMaxMemoryType..0x6FFFFFFF.
                                 PoolType is EfiPersistentMemory.
  @retval EFI_OUT_OF_RESOURCES   Size exceeds max pool size or allocation failed.
  @retval EFI_SUCCESS            Pool successfully allocated.

**/
EFI_STATUS
EFIAPI
CoreInternalAllocatePool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            Size,
  OUT VOID            **Buffer
  )
{
  EFI_STATUS            Status;
  BOOLEAN               NeedGuard;

  //
  // If it's not a valid type, fail it
  //
  if ((PoolType >= EfiMaxMemoryType && PoolType < MEMORY_TYPE_OEM_RESERVED_MIN) ||
       (PoolType == EfiConventionalMemory) || (PoolType == EfiPersistentMemory)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
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

  NeedGuard = IsPoolTypeToGuard (PoolType) && !mOnGuarding;

  //
  // Acquire the memory lock and make the allocation
  //
  Status = CoreAcquireLockOrFail (&mPoolMemoryLock);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Buffer = CoreAllocatePoolI (PoolType, Size, NeedGuard);
  CoreReleaseLock (&mPoolMemoryLock);
  return (*Buffer != NULL) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
}

/**
  Allocate pool of a particular type.

  @param  PoolType               Type of pool to allocate
  @param  Size                   The amount of pool to allocate
  @param  Buffer                 The address to return a pointer to the allocated
                                 pool

  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
                                 PoolType is in the range EfiMaxMemoryType..0x6FFFFFFF.
                                 PoolType is EfiPersistentMemory.
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
  EFI_STATUS  Status;

  Status = CoreInternalAllocatePool (PoolType, Size, Buffer);
  if (!EFI_ERROR (Status)) {
    CoreUpdateProfile (
      (EFI_PHYSICAL_ADDRESS) (UINTN) RETURN_ADDRESS (0),
      MemoryProfileActionAllocatePool,
      PoolType,
      Size,
      *Buffer,
      NULL
      );
    InstallMemoryAttributesTableOnMemoryAllocation (PoolType);
  }
  return Status;
}

/**
  Internal function.  Used by the pool functions to allocate pages
  to back pool allocation requests.

  @param  PoolType               The type of memory for the new pool pages
  @param  NoPages                No of pages to allocate
  @param  Granularity            Bits to align.
  @param  NeedGuard              Flag to indicate Guard page is needed or not

  @return The allocated memory, or NULL

**/
STATIC
VOID *
CoreAllocatePoolPagesI (
  IN EFI_MEMORY_TYPE    PoolType,
  IN UINTN              NoPages,
  IN UINTN              Granularity,
  IN BOOLEAN            NeedGuard
  )
{
  VOID        *Buffer;
  EFI_STATUS  Status;

  Status = CoreAcquireLockOrFail (&gMemoryLock);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Buffer = CoreAllocatePoolPages (PoolType, NoPages, Granularity, NeedGuard);
  CoreReleaseMemoryLock ();

  if (Buffer != NULL) {
    if (NeedGuard) {
      SetGuardForMemory ((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer, NoPages);
    }
    ApplyMemoryProtectionPolicy(EfiConventionalMemory, PoolType,
      (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer, EFI_PAGES_TO_SIZE (NoPages));
  }
  return Buffer;
}

/**
  Internal function to allocate pool of a particular type.
  Caller must have the memory lock held

  @param  PoolType               Type of pool to allocate
  @param  Size                   The amount of pool to allocate
  @param  NeedGuard              Flag to indicate Guard page is needed or not

  @return The allocate pool, or NULL

**/
VOID *
CoreAllocatePoolI (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            Size,
  IN BOOLEAN          NeedGuard
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
  UINTN       Offset, MaxOffset;
  UINTN       NoPages;
  UINTN       Granularity;
  BOOLEAN     HasPoolTail;

  ASSERT_LOCKED (&mPoolMemoryLock);

  if  (PoolType == EfiACPIReclaimMemory   ||
       PoolType == EfiACPIMemoryNVS       ||
       PoolType == EfiRuntimeServicesCode ||
       PoolType == EfiRuntimeServicesData) {

    Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
  } else {
    Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  }

  //
  // Adjust the size by the pool header & tail overhead
  //

  HasPoolTail  = !(NeedGuard &&
                   ((PcdGet8 (PcdHeapGuardPropertyMask) & BIT7) == 0));

  //
  // Adjusting the Size to be of proper alignment so that
  // we don't get an unaligned access fault later when
  // pool_Tail is being initialized
  //
  Size = ALIGN_VARIABLE (Size);

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
  if (Index >= SIZE_TO_LIST (Granularity) || NeedGuard) {
    if (!HasPoolTail) {
      Size -= sizeof (POOL_TAIL);
    }
    NoPages = EFI_SIZE_TO_PAGES (Size) + EFI_SIZE_TO_PAGES (Granularity) - 1;
    NoPages &= ~(UINTN)(EFI_SIZE_TO_PAGES (Granularity) - 1);
    Head = CoreAllocatePoolPagesI (PoolType, NoPages, Granularity, NeedGuard);
    if (NeedGuard) {
      Head = AdjustPoolHeadA ((EFI_PHYSICAL_ADDRESS)(UINTN)Head, NoPages, Size);
    }
    goto Done;
  }

  //
  // If there's no free pool in the proper list size, go get some more pages
  //
  if (IsListEmpty (&Pool->FreeList[Index])) {

    Offset = LIST_TO_SIZE (Index);
    MaxOffset = Granularity;

    //
    // Check the bins holding larger blocks, and carve one up if needed
    //
    while (++Index < SIZE_TO_LIST (Granularity)) {
      if (!IsListEmpty (&Pool->FreeList[Index])) {
        Free = CR (Pool->FreeList[Index].ForwardLink, POOL_FREE, Link, POOL_FREE_SIGNATURE);
        RemoveEntryList (&Free->Link);
        NewPage = (VOID *) Free;
        MaxOffset = LIST_TO_SIZE (Index);
        goto Carve;
      }
    }

    //
    // Get another page
    //
    NewPage = CoreAllocatePoolPagesI (PoolType, EFI_SIZE_TO_PAGES (Granularity),
                                      Granularity, NeedGuard);
    if (NewPage == NULL) {
      goto Done;
    }

    //
    // Serve the allocation request from the head of the allocated block
    //
Carve:
    Head = (POOL_HEAD *) NewPage;

    //
    // Carve up remaining space into free pool blocks
    //
    Index--;
    while (Offset < MaxOffset) {
      ASSERT (Index < MAX_POOL_LIST);
      FSize = LIST_TO_SIZE(Index);

      while (Offset + FSize <= MaxOffset) {
        Free = (POOL_FREE *) &NewPage[Offset];
        Free->Signature = POOL_FREE_SIGNATURE;
        Free->Index     = (UINT32)Index;
        InsertHeadList (&Pool->FreeList[Index], &Free->Link);
        Offset += FSize;
      }
      Index -= 1;
    }

    ASSERT (Offset == MaxOffset);
    goto Done;
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
    // Account the allocation
    //
    Pool->Used += Size;

    //
    // If we have a pool buffer, fill in the header & tail info
    //
    Head->Signature = POOL_HEAD_SIGNATURE;
    Head->Size      = Size;
    Head->Type      = (EFI_MEMORY_TYPE) PoolType;
    Buffer          = Head->Data;

    if (HasPoolTail) {
      Tail            = HEAD_TO_TAIL (Head);
      Tail->Signature = POOL_TAIL_SIGNATURE;
      Tail->Size      = Size;

      Size -= POOL_OVERHEAD;
    } else {
      Size -= SIZE_OF_POOL_HEAD;
    }

    DEBUG_CLEAR_MEMORY (Buffer, Size);

    DEBUG ((
      DEBUG_POOL,
      "AllocatePoolI: Type %x, Addr %p (len %lx) %,ld\n", PoolType,
      Buffer,
      (UINT64)Size,
      (UINT64) Pool->Used
      ));


  } else {
    DEBUG ((DEBUG_ERROR | DEBUG_POOL, "AllocatePool: failed to allocate %ld bytes\n", (UINT64) Size));
  }

  return Buffer;
}



/**
  Frees pool.

  @param  Buffer                 The allocated pool entry to free
  @param  PoolType               Pointer to pool type

  @retval EFI_INVALID_PARAMETER  Buffer is not a valid value.
  @retval EFI_SUCCESS            Pool successfully freed.

**/
EFI_STATUS
EFIAPI
CoreInternalFreePool (
  IN VOID               *Buffer,
  OUT EFI_MEMORY_TYPE   *PoolType OPTIONAL
  )
{
  EFI_STATUS Status;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireLock (&mPoolMemoryLock);
  Status = CoreFreePoolI (Buffer, PoolType);
  CoreReleaseLock (&mPoolMemoryLock);
  return Status;
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
  IN VOID  *Buffer
  )
{
  EFI_STATUS        Status;
  EFI_MEMORY_TYPE   PoolType;

  Status = CoreInternalFreePool (Buffer, &PoolType);
  if (!EFI_ERROR (Status)) {
    CoreUpdateProfile (
      (EFI_PHYSICAL_ADDRESS) (UINTN) RETURN_ADDRESS (0),
      MemoryProfileActionFreePool,
      PoolType,
      0,
      Buffer,
      NULL
      );
    InstallMemoryAttributesTableOnMemoryAllocation (PoolType);
  }
  return Status;
}

/**
  Internal function.  Frees pool pages allocated via CoreAllocatePoolPagesI().

  @param  PoolType               The type of memory for the pool pages
  @param  Memory                 The base address to free
  @param  NoPages                The number of pages to free

**/
STATIC
VOID
CoreFreePoolPagesI (
  IN EFI_MEMORY_TYPE        PoolType,
  IN EFI_PHYSICAL_ADDRESS   Memory,
  IN UINTN                  NoPages
  )
{
  CoreAcquireMemoryLock ();
  CoreFreePoolPages (Memory, NoPages);
  CoreReleaseMemoryLock ();

  ApplyMemoryProtectionPolicy (PoolType, EfiConventionalMemory,
    (EFI_PHYSICAL_ADDRESS)(UINTN)Memory, EFI_PAGES_TO_SIZE (NoPages));
}

/**
  Internal function.  Frees guarded pool pages.

  @param  PoolType               The type of memory for the pool pages
  @param  Memory                 The base address to free
  @param  NoPages                The number of pages to free

**/
STATIC
VOID
CoreFreePoolPagesWithGuard (
  IN EFI_MEMORY_TYPE        PoolType,
  IN EFI_PHYSICAL_ADDRESS   Memory,
  IN UINTN                  NoPages
  )
{
  EFI_PHYSICAL_ADDRESS    MemoryGuarded;
  UINTN                   NoPagesGuarded;

  MemoryGuarded  = Memory;
  NoPagesGuarded = NoPages;

  AdjustMemoryF (&Memory, &NoPages);
  //
  // It's safe to unset Guard page inside memory lock because there should
  // be no memory allocation occurred in updating memory page attribute at
  // this point. And unsetting Guard page before free will prevent Guard
  // page just freed back to pool from being allocated right away before
  // marking it usable (from non-present to present).
  //
  UnsetGuardForMemory (MemoryGuarded, NoPagesGuarded);
  if (NoPages > 0) {
    CoreFreePoolPagesI (PoolType, Memory, NoPages);
  }
}

/**
  Internal function to free a pool entry.
  Caller must have the memory lock held

  @param  Buffer                 The allocated pool entry to free
  @param  PoolType               Pointer to pool type

  @retval EFI_INVALID_PARAMETER  Buffer not valid
  @retval EFI_SUCCESS            Buffer successfully freed.

**/
EFI_STATUS
CoreFreePoolI (
  IN VOID               *Buffer,
  OUT EFI_MEMORY_TYPE   *PoolType OPTIONAL
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
  UINTN       Offset;
  BOOLEAN     AllFree;
  UINTN       Granularity;
  BOOLEAN     IsGuarded;
  BOOLEAN     HasPoolTail;

  ASSERT(Buffer != NULL);
  //
  // Get the head & tail of the pool entry
  //
  Head = CR (Buffer, POOL_HEAD, Data, POOL_HEAD_SIGNATURE);
  ASSERT(Head != NULL);

  if (Head->Signature != POOL_HEAD_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  IsGuarded   = IsPoolTypeToGuard (Head->Type) &&
                IsMemoryGuarded ((EFI_PHYSICAL_ADDRESS)(UINTN)Head);
  HasPoolTail = !(IsGuarded &&
                  ((PcdGet8 (PcdHeapGuardPropertyMask) & BIT7) == 0));

  if (HasPoolTail) {
    Tail = HEAD_TO_TAIL (Head);
    ASSERT (Tail != NULL);

    //
    // Debug
    //
    ASSERT (Tail->Signature == POOL_TAIL_SIGNATURE);
    ASSERT (Head->Size == Tail->Size);

    if (Tail->Signature != POOL_TAIL_SIGNATURE) {
      return EFI_INVALID_PARAMETER;
    }

    if (Head->Size != Tail->Size) {
      return EFI_INVALID_PARAMETER;
    }
  }

  ASSERT_LOCKED (&mPoolMemoryLock);

  //
  // Determine the pool type and account for it
  //
  Size = Head->Size;
  Pool = LookupPoolHead (Head->Type);
  if (Pool == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Pool->Used -= Size;
  DEBUG ((DEBUG_POOL, "FreePool: %p (len %lx) %,ld\n", Head->Data, (UINT64)(Head->Size - POOL_OVERHEAD), (UINT64) Pool->Used));

  if  (Head->Type == EfiACPIReclaimMemory   ||
       Head->Type == EfiACPIMemoryNVS       ||
       Head->Type == EfiRuntimeServicesCode ||
       Head->Type == EfiRuntimeServicesData) {

    Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
  } else {
    Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  }

  if (PoolType != NULL) {
    *PoolType = Head->Type;
  }

  //
  // Determine the pool list
  //
  Index = SIZE_TO_LIST(Size);
  DEBUG_CLEAR_MEMORY (Head, Size);

  //
  // If it's not on the list, it must be pool pages
  //
  if (Index >= SIZE_TO_LIST (Granularity) || IsGuarded) {

    //
    // Return the memory pages back to free memory
    //
    NoPages = EFI_SIZE_TO_PAGES (Size) + EFI_SIZE_TO_PAGES (Granularity) - 1;
    NoPages &= ~(UINTN)(EFI_SIZE_TO_PAGES (Granularity) - 1);
    if (IsGuarded) {
      Head = AdjustPoolHeadF ((EFI_PHYSICAL_ADDRESS)(UINTN)Head);
      CoreFreePoolPagesWithGuard (
        Pool->MemoryType,
        (EFI_PHYSICAL_ADDRESS)(UINTN)Head,
        NoPages
        );
    } else {
      CoreFreePoolPagesI (
        Pool->MemoryType,
        (EFI_PHYSICAL_ADDRESS)(UINTN)Head,
        NoPages
        );
    }

  } else {

    //
    // Put the pool entry onto the free pool list
    //
    Free = (POOL_FREE *) Head;
    ASSERT(Free != NULL);
    Free->Signature = POOL_FREE_SIGNATURE;
    Free->Index     = (UINT32)Index;
    InsertHeadList (&Pool->FreeList[Index], &Free->Link);

    //
    // See if all the pool entries in the same page as Free are freed pool
    // entries
    //
    NewPage = (CHAR8 *)((UINTN)Free & ~(Granularity - 1));
    Free = (POOL_FREE *) &NewPage[0];
    ASSERT(Free != NULL);

    if (Free->Signature == POOL_FREE_SIGNATURE) {

      AllFree = TRUE;
      Offset = 0;

      while ((Offset < Granularity) && (AllFree)) {
        Free = (POOL_FREE *) &NewPage[Offset];
        ASSERT(Free != NULL);
        if (Free->Signature != POOL_FREE_SIGNATURE) {
          AllFree = FALSE;
        }
        Offset += LIST_TO_SIZE(Free->Index);
      }

      if (AllFree) {

        //
        // All of the pool entries in the same page as Free are free pool
        // entries
        // Remove all of these pool entries from the free loop lists.
        //
        Free = (POOL_FREE *) &NewPage[0];
        ASSERT(Free != NULL);
        Offset = 0;

        while (Offset < Granularity) {
          Free = (POOL_FREE *) &NewPage[Offset];
          ASSERT(Free != NULL);
          RemoveEntryList (&Free->Link);
          Offset += LIST_TO_SIZE(Free->Index);
        }

        //
        // Free the page
        //
        CoreFreePoolPagesI (Pool->MemoryType, (EFI_PHYSICAL_ADDRESS) (UINTN)NewPage,
          EFI_SIZE_TO_PAGES (Granularity));
      }
    }
  }

  //
  // If this is an OS/OEM specific memory type, then check to see if the last
  // portion of that memory type has been freed.  If it has, then free the
  // list entry for that memory type
  //
  if (((UINT32) Pool->MemoryType >= MEMORY_TYPE_OEM_RESERVED_MIN) && Pool->Used == 0) {
    RemoveEntryList (&Pool->Link);
    CoreFreePoolI (Pool, NULL);
  }

  return EFI_SUCCESS;
}

