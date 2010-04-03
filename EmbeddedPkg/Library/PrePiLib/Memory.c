/** @file
  Implementation of the 6 PEI Ffs (FV) APIs in library form.

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PrePi.h>



/**
  Allocates one or more 4KB pages of type EfiBootServicesData.

  Allocates the number of 4KB pages of MemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID * 
EFIAPI
AllocatePages (
  IN UINTN            Pages
  )
{
  EFI_PEI_HOB_POINTERS                    Hob;
  EFI_PHYSICAL_ADDRESS                    Offset;

  Hob.Raw = GetHobList ();

  // Check to see if on 4k boundary
  Offset = Hob.HandoffInformationTable->EfiFreeMemoryTop & 0xFFF;
  if (Offset != 0) {
    // If not aligned, make the allocation aligned.
    Hob.HandoffInformationTable->EfiFreeMemoryTop -= Offset;
  }

  //
  // Verify that there is sufficient memory to satisfy the allocation
  //
  if (Hob.HandoffInformationTable->EfiFreeMemoryTop - ((Pages * EFI_PAGE_SIZE) + sizeof (EFI_HOB_MEMORY_ALLOCATION)) < Hob.HandoffInformationTable->EfiFreeMemoryBottom) {
    return 0;
  } else {
    //
    // Update the PHIT to reflect the memory usage
    //
    Hob.HandoffInformationTable->EfiFreeMemoryTop -= Pages * EFI_PAGE_SIZE;

    // This routine used to create a memory allocation HOB a la PEI, but that's not
    // necessary for us.
    
    return (VOID *)(UINTN)Hob.HandoffInformationTable->EfiFreeMemoryTop;
  }
}


/**
  Allocates one or more 4KB pages of type EfiBootServicesData at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiBootServicesData with an
  alignment specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is
  returned.  If there is not enough memory at the specified alignment remaining to satisfy the
  request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  Pages                 The number of 4 KB pages to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  VOID    *Memory;
  UINTN   AlignmentMask;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);

  if (Pages == 0) {
    return NULL;
  }
  //
  // Make sure that Pages plus EFI_SIZE_TO_PAGES (Alignment) does not overflow.
  //
  ASSERT (Pages <= (MAX_ADDRESS - EFI_SIZE_TO_PAGES (Alignment)));
  //
  // We would rather waste some memory to save PEI code size.
  //
  Memory = (VOID *)(UINTN)AllocatePages (Pages + EFI_SIZE_TO_PAGES (Alignment));
  if (Alignment == 0) {
    AlignmentMask = Alignment;
  } else {
    AlignmentMask = Alignment - 1;  
  }
  return (VOID *) (UINTN) (((UINTN) Memory + AlignmentMask) & ~AlignmentMask);
}




/**
  Allocates a buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData and returns a
  pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocatePool (
  IN UINTN  AllocationSize
  )
{
  EFI_HOB_MEMORY_POOL      *Hob;

  Hob = GetHobList ();

  
  //
  // Verify that there is sufficient memory to satisfy the allocation
  //
  if (AllocationSize > 0x10000) {
    // Please call AllcoatePages for big allocations
    return 0;
  } else {

    Hob = (EFI_HOB_MEMORY_POOL *)CreateHob (EFI_HOB_TYPE_MEMORY_POOL, (UINT16)(sizeof (EFI_HOB_TYPE_MEMORY_POOL) + AllocationSize));
    return (VOID *)(Hob + 1);
  }
}



