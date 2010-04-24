/** @file
  Data structure and functions to allocate and free memory space.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IMEM_H_
#define _IMEM_H_

#if defined (MDE_CPU_IPF)
///
/// For Itanium machines make the default allocations 8K aligned
///
#define EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT  (EFI_PAGE_SIZE * 2)
#define DEFAULT_PAGE_ALLOCATION                     (EFI_PAGE_SIZE * 2)

#else
///
/// For genric EFI machines make the default allocations 4K aligned
///
#define EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT  (EFI_PAGE_SIZE)
#define DEFAULT_PAGE_ALLOCATION                     (EFI_PAGE_SIZE)

#endif


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

  @return The allocated memory, or NULL

**/
VOID *
CoreAllocatePoolPages (
  IN EFI_MEMORY_TYPE    PoolType,
  IN UINTN              NumberOfPages,
  IN UINTN              Alignment
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

  @return The allocate pool, or NULL

**/
VOID *
CoreAllocatePoolI (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            Size
  );



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


//
// Internal Global data
//

extern EFI_LOCK           gMemoryLock;
extern LIST_ENTRY         gMemoryMap;
extern LIST_ENTRY         mGcdMemorySpaceMap;
#endif
