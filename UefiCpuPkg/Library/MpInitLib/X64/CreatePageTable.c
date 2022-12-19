/** @file
  Function to create page talbe.
  Only create page table for x64, and leave the CreatePageTable empty for Ia32.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/CpuPageTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

/**
  Create 1:1 mapping page table in reserved memory to map the specified address range.

  @param[in]      LinearAddress  The start of the linear address range.
  @param[in]      Length         The length of the linear address range.

  @return The page table to be created.
**/
UINTN
CreatePageTable (
  IN UINTN  Address,
  IN UINTN  Length
  )
{
  EFI_STATUS  Status;
  VOID        *PageTableBuffer;
  UINTN       PageTableBufferSize;
  UINTN       PageTable;

  IA32_MAP_ATTRIBUTE  MapAttribute;
  IA32_MAP_ATTRIBUTE  MapMask;

  MapAttribute.Uint64         = Address;
  MapAttribute.Bits.Present   = 1;
  MapAttribute.Bits.ReadWrite = 1;

  MapMask.Bits.PageTableBaseAddress = 1;
  MapMask.Bits.Present              = 1;
  MapMask.Bits.ReadWrite            = 1;

  PageTable           = 0;
  PageTableBufferSize = 0;

  Status = PageTableMap (
             &PageTable,
             Paging4Level,
             NULL,
             &PageTableBufferSize,
             Address,
             Length,
             &MapAttribute,
             &MapMask
             );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  DEBUG ((DEBUG_INFO, "AP Page Table Buffer Size = %x\n", PageTableBufferSize));

  PageTableBuffer = AllocateReservedPages (EFI_SIZE_TO_PAGES (PageTableBufferSize));
  ASSERT (PageTableBuffer != NULL);
  Status = PageTableMap (
             &PageTable,
             Paging4Level,
             PageTableBuffer,
             &PageTableBufferSize,
             Address,
             Length,
             &MapAttribute,
             &MapMask
             );
  ASSERT_EFI_ERROR (Status);
  return PageTable;
}
