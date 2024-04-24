/** @file ArmPageTableMemoryAllocation.h

Logic facilitating the allocation of page table memory.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ARM_PAGE_TABLE_MEM_ALLOC_H_
#define ARM_PAGE_TABLE_MEM_ALLOC_H_

#define ARM_PAGE_TABLE_MEM_ALLOC_PROTOCOL  { \
    0x623be44e, 0x506d, 0x487f, {0xa3, 0xa6, 0x92, 0xda, 0xef, 0x30, 0x2c, 0x70 } \
    }

extern EFI_GUID  gArmPageTableMemoryAllocationProtocolGuid;

typedef struct _PAGE_TABLE_MEM_ALLOC_PROTOCOL PAGE_TABLE_MEM_ALLOC_PROTOCOL;

// Protocol Interface functions.

/**
  Allocate memory for the page table.

  @param[in]  NumPages          Number of pages to allocate.

  @retval A pointer to the allocated page table memory or NULL if the
          allocation failed.
**/
typedef
VOID *
(EFIAPI *ALLOCATE_PAGE_TABLE_MEMORY)(
  IN UINTN NumPages
  );

typedef struct _PAGE_TABLE_MEM_ALLOC_PROTOCOL {
  ALLOCATE_PAGE_TABLE_MEMORY    AllocatePageTableMemory;
} PAGE_TABLE_MEM_ALLOC_PROTOCOL;

#endif
