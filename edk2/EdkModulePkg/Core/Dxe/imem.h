/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  imem.h

Abstract:

  Head file to imem.h


Revision History

--*/

#ifndef _IMEM_H_
#define _IMEM_H_

#if defined (MDE_CPU_IPF)
//
// For Itanium machines make the default allocations 8K aligned
//
#define EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT  (EFI_PAGE_SIZE * 2)
#define DEFAULT_PAGE_ALLOCATION                     (EFI_PAGE_SIZE * 2)

#else
//
// For genric EFI machines make the default allocations 4K aligned
//
#define EFI_ACPI_RUNTIME_PAGE_ALLOCATION_ALIGNMENT  (EFI_PAGE_SIZE)
#define DEFAULT_PAGE_ALLOCATION                     (EFI_PAGE_SIZE)

#endif


//
// MEMORY_MAP_ENTRY
//

#define MEMORY_MAP_SIGNATURE   EFI_SIGNATURE_32('m','m','a','p')
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

VOID *
CoreAllocatePoolPages (
  IN EFI_MEMORY_TYPE   PoolType,
  IN UINTN             NumberOfPages,
  IN UINTN             Alignment
  )
/*++

Routine Description:

  Internal function.  Used by the pool functions to allocate pages
  to back pool allocation requests.

Arguments:

  PoolType      - The type of memory for the new pool pages

  NumberOfPages - No of pages to allocate
  
  Alignment     - Bits to align.

Returns:

  The allocated memory, or NULL

--*/
;


VOID
CoreFreePoolPages (
  IN EFI_PHYSICAL_ADDRESS   Memory,
  IN UINTN                  NumberOfPages
  )
/*++

Routine Description:

  Internal function.  Frees pool pages allocated via AllocatePoolPages ()

Arguments:

  Memory        - The base address to free

  NumberOfPages - The number of pages to free

Returns:

  None

--*/
;


VOID *
CoreAllocatePoolI (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            Size
  )
/*++

Routine Description:

  Internal function to allocate pool of a particular type.

  Caller must have the memory lock held


Arguments:

  PoolType    - Type of pool to allocate

  Size        - The amount of pool to allocate

Returns:

  The allocate pool, or NULL

--*/
;


EFI_STATUS
CoreFreePoolI (
  IN VOID           *Buffer
  )
/*++

Routine Description:

  Internal function to free a pool entry.

  Caller must have the memory lock held


Arguments:

  Buffer      - The allocated pool entry to free

Returns:

  EFI_INVALID_PARAMETER     - Buffer not valid
  
  EFI_SUCCESS               - Buffer successfully freed.

--*/
;


VOID
CoreAcquireMemoryLock (
  VOID
  )
/*++

Routine Description:

  Enter critical section by gaining lock on gMemoryLock

Arguments:

  None

Returns:

  None

--*/
;

VOID
CoreReleaseMemoryLock (
  VOID
  )
/*++

Routine Description:

  Exit critical section by releasing lock on gMemoryLock

Arguments:

  None

Returns:

  None

--*/
;


//
// Internal Global data
//

extern EFI_LOCK           gMemoryLock; 
extern LIST_ENTRY         gMemoryMap;
extern MEMORY_MAP         *gMemoryLastConvert;
extern LIST_ENTRY         mGcdMemorySpaceMap;
#endif
