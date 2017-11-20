/** @file
  Data structure and functions to allocate and free memory space.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IMEM_H_
#define _IMEM_H_

//
// +---------------------------------------------------+
// | 0..(EfiMaxMemoryType - 1)    - Normal memory type |
// +---------------------------------------------------+
// | EfiMaxMemoryType..0x6FFFFFFF - Invalid            |
// +---------------------------------------------------+
// | 0x70000000..0x7FFFFFFF       - OEM reserved       |
// +---------------------------------------------------+
// | 0x80000000..0xFFFFFFFF       - OS reserved        |
// +---------------------------------------------------+
//
#define MEMORY_TYPE_OS_RESERVED_MIN                 0x80000000
#define MEMORY_TYPE_OS_RESERVED_MAX                 0xFFFFFFFF
#define MEMORY_TYPE_OEM_RESERVED_MIN                0x70000000
#define MEMORY_TYPE_OEM_RESERVED_MAX                0x7FFFFFFF

//
// MEMORY_MAP_ENTRY
//

#define MEMORY_MAP_SIGNATURE   SIGNATURE_32('m','m','a','p')
typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Link;
  BOOLEAN         FromPages;

  EFI_MEMORY_TYPE Type;
  UINT64          Start;
  UINT64          End;

  UINT64          VirtualStart;
  UINT64          Attribute;
} MEMORY_MAP;

//
// Internal prototypes
//


/**
  Internal function.  Used by the pool functions to allocate pages
  to back pool allocation requests.

  @param  PoolType               The type of memory for the new pool pages
  @param  NumberOfPages          No of pages to allocate
  @param  Alignment              Bits to align.
  @param  NeedGuard              Flag to indicate Guard page is needed or not

  @return The allocated memory, or NULL

**/
VOID *
CoreAllocatePoolPages (
  IN EFI_MEMORY_TYPE    PoolType,
  IN UINTN              NumberOfPages,
  IN UINTN              Alignment,
  IN BOOLEAN            NeedGuard
  );



/**
  Internal function.  Frees pool pages allocated via AllocatePoolPages ()

  @param  Memory                 The base address to free
  @param  NumberOfPages          The number of pages to free

**/
VOID
CoreFreePoolPages (
  IN EFI_PHYSICAL_ADDRESS   Memory,
  IN UINTN                  NumberOfPages
  );



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
  );



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
  );



/**
  Enter critical section by gaining lock on gMemoryLock.

**/
VOID
CoreAcquireMemoryLock (
  VOID
  );


/**
  Exit critical section by releasing lock on gMemoryLock.

**/
VOID
CoreReleaseMemoryLock (
  VOID
  );

/**
  Allocates pages from the memory map.

  @param  Type                   The type of allocation to perform
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into
  @param  NumberOfPages          The number of pages to allocate
  @param  Memory                 A pointer to receive the base allocated memory
                                 address
  @param  NeedGuard              Flag to indicate Guard page is needed or not

  @return Status. On success, Memory is filled in with the base address allocated
  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in
                                 spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
CoreInternalAllocatePages (
  IN EFI_ALLOCATE_TYPE      Type,
  IN EFI_MEMORY_TYPE        MemoryType,
  IN UINTN                  NumberOfPages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory,
  IN BOOLEAN                NeedGuard
  );

//
// Internal Global data
//

extern EFI_LOCK           gMemoryLock;
extern LIST_ENTRY         gMemoryMap;
extern LIST_ENTRY         mGcdMemorySpaceMap;
#endif
