/** @file
  Data type, macros and function prototypes of heap guard feature.

Copyright (c) 2017-2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HEAPGUARD_H_
#define _HEAPGUARD_H_

//
// Following macros are used to define and access the guarded memory bitmap
// table.
//
// To simplify the access and reduce the memory used for this table, the
// table is constructed in the similar way as page table structure but in
// reverse direction, i.e. from bottom growing up to top.
//
//    - 1-bit tracks 1 page (4KB)
//    - 1-UINT64 map entry tracks 256KB memory
//    - 1K-UINT64 map table tracks 256MB memory
//    - Five levels of tables can track any address of memory of 64-bit
//      system, like below.
//
//       512   *   512   *   512   *   512    *    1K   *  64b *     4K
//    111111111 111111111 111111111 111111111 1111111111 111111 111111111111
//    63        54        45        36        27         17     11         0
//       9b        9b        9b        9b         10b      6b       12b
//       L0   ->   L1   ->   L2   ->   L3   ->    L4   -> bits  ->  page
//      1FF       1FF       1FF       1FF         3FF      3F       FFF
//
// L4 table has 1K * sizeof(UINT64) = 8K (2-page), which can track 256MB
// memory. Each table of L0-L3 will be allocated when its memory address
// range is to be tracked. Only 1-page will be allocated each time. This
// can save memories used to establish this map table.
//
// For a normal configuration of system with 4G memory, two levels of tables
// can track the whole memory, because two levels (L3+L4) of map tables have
// already coverred 37-bit of memory address. And for a normal UEFI BIOS,
// less than 128M memory would be consumed during boot. That means we just
// need
//
//          1-page (L3) + 2-page (L4)
//
// memory (3 pages) to track the memory allocation works. In this case,
// there's no need to setup L0-L2 tables.
//

//
// Each entry occupies 8B/64b. 1-page can hold 512 entries, which spans 9
// bits in address. (512 = 1 << 9)
//
#define BYTE_LENGTH_SHIFT  3                              // (8 = 1 << 3)

#define GUARDED_HEAP_MAP_TABLE_ENTRY_SHIFT  \
        (EFI_PAGE_SHIFT - BYTE_LENGTH_SHIFT)

#define GUARDED_HEAP_MAP_TABLE_DEPTH  5

// Use UINT64_index + bit_index_of_UINT64 to locate the bit in may
#define GUARDED_HEAP_MAP_ENTRY_BIT_SHIFT  6               // (64 = 1 << 6)

#define GUARDED_HEAP_MAP_ENTRY_BITS         \
        (1 << GUARDED_HEAP_MAP_ENTRY_BIT_SHIFT)

#define GUARDED_HEAP_MAP_ENTRY_BYTES        \
        (GUARDED_HEAP_MAP_ENTRY_BITS / 8)

// L4 table address width: 64 - 9 * 4 - 6 - 12 = 10b
#define GUARDED_HEAP_MAP_ENTRY_SHIFT              \
        (GUARDED_HEAP_MAP_ENTRY_BITS              \
         - GUARDED_HEAP_MAP_TABLE_ENTRY_SHIFT * 4 \
         - GUARDED_HEAP_MAP_ENTRY_BIT_SHIFT       \
         - EFI_PAGE_SHIFT)

// L4 table address mask: (1 << 10 - 1) = 0x3FF
#define GUARDED_HEAP_MAP_ENTRY_MASK               \
        ((1 << GUARDED_HEAP_MAP_ENTRY_SHIFT) - 1)

// Size of each L4 table: (1 << 10) * 8 = 8KB = 2-page
#define GUARDED_HEAP_MAP_SIZE                     \
        ((1 << GUARDED_HEAP_MAP_ENTRY_SHIFT) * GUARDED_HEAP_MAP_ENTRY_BYTES)

// Memory size tracked by one L4 table: 8KB * 8 * 4KB = 256MB
#define GUARDED_HEAP_MAP_UNIT_SIZE                \
        (GUARDED_HEAP_MAP_SIZE * 8 * EFI_PAGE_SIZE)

// L4 table entry number: 8KB / 8 = 1024
#define GUARDED_HEAP_MAP_ENTRIES_PER_UNIT         \
        (GUARDED_HEAP_MAP_SIZE / GUARDED_HEAP_MAP_ENTRY_BYTES)

// L4 table entry indexing
#define GUARDED_HEAP_MAP_ENTRY_INDEX(Address)                       \
        (RShiftU64 (Address, EFI_PAGE_SHIFT                         \
                             + GUARDED_HEAP_MAP_ENTRY_BIT_SHIFT)    \
         & GUARDED_HEAP_MAP_ENTRY_MASK)

// L4 table entry bit indexing
#define GUARDED_HEAP_MAP_ENTRY_BIT_INDEX(Address)       \
        (RShiftU64 (Address, EFI_PAGE_SHIFT)            \
         & ((1 << GUARDED_HEAP_MAP_ENTRY_BIT_SHIFT) - 1))

//
// Total bits (pages) tracked by one L4 table (65536-bit)
//
#define GUARDED_HEAP_MAP_BITS                               \
        (1 << (GUARDED_HEAP_MAP_ENTRY_SHIFT                 \
               + GUARDED_HEAP_MAP_ENTRY_BIT_SHIFT))

//
// Bit indexing inside the whole L4 table (0 - 65535)
//
#define GUARDED_HEAP_MAP_BIT_INDEX(Address)                     \
        (RShiftU64 (Address, EFI_PAGE_SHIFT)                    \
         & ((1 << (GUARDED_HEAP_MAP_ENTRY_SHIFT                 \
                   + GUARDED_HEAP_MAP_ENTRY_BIT_SHIFT)) - 1))

//
// Memory address bit width tracked by L4 table: 10 + 6 + 12 = 28
//
#define GUARDED_HEAP_MAP_TABLE_SHIFT                                      \
        (GUARDED_HEAP_MAP_ENTRY_SHIFT + GUARDED_HEAP_MAP_ENTRY_BIT_SHIFT  \
         + EFI_PAGE_SHIFT)

//
// Macro used to initialize the local array variable for map table traversing
// {55, 46, 37, 28, 18}
//
#define GUARDED_HEAP_MAP_TABLE_DEPTH_SHIFTS                                 \
  {                                                                         \
    GUARDED_HEAP_MAP_TABLE_SHIFT + GUARDED_HEAP_MAP_TABLE_ENTRY_SHIFT * 3,  \
    GUARDED_HEAP_MAP_TABLE_SHIFT + GUARDED_HEAP_MAP_TABLE_ENTRY_SHIFT * 2,  \
    GUARDED_HEAP_MAP_TABLE_SHIFT + GUARDED_HEAP_MAP_TABLE_ENTRY_SHIFT,      \
    GUARDED_HEAP_MAP_TABLE_SHIFT,                                           \
    EFI_PAGE_SHIFT + GUARDED_HEAP_MAP_ENTRY_BIT_SHIFT                       \
  }

//
// Masks used to extract address range of each level of table
// {0x1FF, 0x1FF, 0x1FF, 0x1FF, 0x3FF}
//
#define GUARDED_HEAP_MAP_TABLE_DEPTH_MASKS                                  \
  {                                                                         \
    (1 << GUARDED_HEAP_MAP_TABLE_ENTRY_SHIFT) - 1,                          \
    (1 << GUARDED_HEAP_MAP_TABLE_ENTRY_SHIFT) - 1,                          \
    (1 << GUARDED_HEAP_MAP_TABLE_ENTRY_SHIFT) - 1,                          \
    (1 << GUARDED_HEAP_MAP_TABLE_ENTRY_SHIFT) - 1,                          \
    (1 << GUARDED_HEAP_MAP_ENTRY_SHIFT) - 1                                 \
  }

//
// Memory type to guard (matching the related PCD definition)
//
#define GUARD_HEAP_TYPE_PAGE   BIT0
#define GUARD_HEAP_TYPE_POOL   BIT1
#define GUARD_HEAP_TYPE_FREED  BIT4
#define GUARD_HEAP_TYPE_ALL         \
        (GUARD_HEAP_TYPE_PAGE|GUARD_HEAP_TYPE_POOL|GUARD_HEAP_TYPE_FREED)

//
// Debug message level
//
#define HEAP_GUARD_DEBUG_LEVEL  (DEBUG_POOL|DEBUG_PAGE)

typedef struct {
  UINT32                  TailMark;
  UINT32                  HeadMark;
  EFI_PHYSICAL_ADDRESS    Address;
  LIST_ENTRY              Link;
} HEAP_GUARD_NODE;

/**
  Internal function.  Converts a memory range to the specified type.
  The range must exist in the memory map.

  @param  Start                  The first address of the range Must be page
                                 aligned.
  @param  NumberOfPages          The number of pages to convert.
  @param  NewType                The new type for the memory range.

  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_NOT_FOUND          Could not find a descriptor cover the specified
                                 range or convertion not allowed.
  @retval EFI_SUCCESS            Successfully converts the memory range to the
                                 specified type.

**/
EFI_STATUS
CoreConvertPages (
  IN UINT64           Start,
  IN UINT64           NumberOfPages,
  IN EFI_MEMORY_TYPE  NewType
  );

/**
  Allocate or free guarded memory.

  @param[in]  Start           Start address of memory to allocate or free.
  @param[in]  NumberOfPages   Memory size in pages.
  @param[in]  NewType         Memory type to convert to.

  @return VOID.
**/
EFI_STATUS
CoreConvertPagesWithGuard (
  IN UINT64           Start,
  IN UINTN            NumberOfPages,
  IN EFI_MEMORY_TYPE  NewType
  );

/**
  Set head Guard and tail Guard for the given memory range.

  @param[in]  Memory          Base address of memory to set guard for.
  @param[in]  NumberOfPages   Memory size in pages.

  @return VOID.
**/
VOID
SetGuardForMemory (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  );

/**
  Unset head Guard and tail Guard for the given memory range.

  @param[in]  Memory          Base address of memory to unset guard for.
  @param[in]  NumberOfPages   Memory size in pages.

  @return VOID.
**/
VOID
UnsetGuardForMemory (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  );

/**
  Adjust the base and number of pages to really allocate according to Guard.

  @param[in,out]  Memory          Base address of free memory.
  @param[in,out]  NumberOfPages   Size of memory to allocate.

  @return VOID.
**/
VOID
AdjustMemoryA (
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory,
  IN OUT UINTN                 *NumberOfPages
  );

/**
  Adjust the start address and number of pages to free according to Guard.

  The purpose of this function is to keep the shared Guard page with adjacent
  memory block if it's still in guard, or free it if no more sharing. Another
  is to reserve pages as Guard pages in partial page free situation.

  @param[in,out]  Memory          Base address of memory to free.
  @param[in,out]  NumberOfPages   Size of memory to free.

  @return VOID.
**/
VOID
AdjustMemoryF (
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory,
  IN OUT UINTN                 *NumberOfPages
  );

/**
  Adjust address of free memory according to existing and/or required Guard.

  This function will check if there're existing Guard pages of adjacent
  memory blocks, and try to use it as the Guard page of the memory to be
  allocated.

  @param[in]  Start           Start address of free memory block.
  @param[in]  Size            Size of free memory block.
  @param[in]  SizeRequested   Size of memory to allocate.

  @return The end address of memory block found.
  @return 0 if no enough space for the required size of memory and its Guard.
**/
UINT64
AdjustMemoryS (
  IN UINT64  Start,
  IN UINT64  Size,
  IN UINT64  SizeRequested
  );

/**
  Check to see if the pool at the given address should be guarded or not.

  @param[in]  MemoryType      Pool type to check.


  @return TRUE  The given type of pool should be guarded.
  @return FALSE The given type of pool should not be guarded.
**/
BOOLEAN
IsPoolTypeToGuard (
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Check to see if the page at the given address should be guarded or not.

  @param[in]  MemoryType      Page type to check.
  @param[in]  AllocateType    Allocation type to check.

  @return TRUE  The given type of page should be guarded.
  @return FALSE The given type of page should not be guarded.
**/
BOOLEAN
IsPageTypeToGuard (
  IN EFI_MEMORY_TYPE    MemoryType,
  IN EFI_ALLOCATE_TYPE  AllocateType
  );

/**
  Check to see if the page at the given address is guarded or not.

  @param[in]  Address     The address to check for.

  @return TRUE  The page at Address is guarded.
  @return FALSE The page at Address is not guarded.
**/
BOOLEAN
EFIAPI
IsMemoryGuarded (
  IN EFI_PHYSICAL_ADDRESS  Address
  );

/**
  Check to see if the page at the given address is a Guard page or not.

  @param[in]  Address     The address to check for.

  @return TRUE  The page at Address is a Guard page.
  @return FALSE The page at Address is not a Guard page.
**/
BOOLEAN
EFIAPI
IsGuardPage (
  IN EFI_PHYSICAL_ADDRESS  Address
  );

/**
  Dump the guarded memory bit map.
**/
VOID
EFIAPI
DumpGuardedMemoryBitmap (
  VOID
  );

/**
  Adjust the pool head position to make sure the Guard page is adjavent to
  pool tail or pool head.

  @param[in]  Memory    Base address of memory allocated.
  @param[in]  NoPages   Number of pages actually allocated.
  @param[in]  Size      Size of memory requested.
                        (plus pool head/tail overhead)

  @return Address of pool head.
**/
VOID *
AdjustPoolHeadA (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NoPages,
  IN UINTN                 Size
  );

/**
  Get the page base address according to pool head address.

  @param[in]  Memory    Head address of pool to free.

  @return Address of pool head.
**/
VOID *
AdjustPoolHeadF (
  IN EFI_PHYSICAL_ADDRESS  Memory
  );

/**
  Check to see if the heap guard is enabled for page and/or pool allocation.

  @param[in]  GuardType   Specify the sub-type(s) of Heap Guard.

  @return TRUE/FALSE.
**/
BOOLEAN
IsHeapGuardEnabled (
  UINT8  GuardType
  );

/**
  Notify function used to set all Guard pages after CPU Arch Protocol installed.
**/
VOID
HeapGuardCpuArchProtocolNotify (
  VOID
  );

/**
  This function checks to see if the given memory map descriptor in a memory map
  can be merged with any guarded free pages.

  @param  MemoryMapEntry    A pointer to a descriptor in MemoryMap.
  @param  MaxAddress        Maximum address to stop the merge.

  @return VOID

**/
VOID
MergeGuardPages (
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry,
  IN EFI_PHYSICAL_ADDRESS   MaxAddress
  );

/**
  Record freed pages as well as mark them as not-present, if enabled.

  @param[in]  BaseAddress   Base address of just freed pages.
  @param[in]  Pages         Number of freed pages.

  @return VOID.
**/
VOID
EFIAPI
GuardFreedPagesChecked (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINTN                 Pages
  );

/**
  Put part (at most 64 pages a time) guarded free pages back to free page pool.

  Freed memory guard is used to detect Use-After-Free (UAF) memory issue, which
  makes use of 'Used then throw away' way to detect any illegal access to freed
  memory. The thrown-away memory will be marked as not-present so that any access
  to those memory (after free) will be caught by page-fault exception.

  The problem is that this will consume lots of memory space. Once no memory
  left in pool to allocate, we have to restore part of the freed pages to their
  normal function. Otherwise the whole system will stop functioning.

  @param  StartAddress    Start address of promoted memory.
  @param  EndAddress      End address of promoted memory.

  @return TRUE    Succeeded to promote memory.
  @return FALSE   No free memory found.

**/
BOOLEAN
PromoteGuardedFreePages (
  OUT EFI_PHYSICAL_ADDRESS  *StartAddress,
  OUT EFI_PHYSICAL_ADDRESS  *EndAddress
  );

extern BOOLEAN  mOnGuarding;

#endif
