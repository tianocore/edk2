/** @file
  Support routines for memory allocation routines for use with drivers.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  MemoryAllocationLib.c

**/



/**
  Allocates the number of 4KB pages specified by Pages of a certain memory type.

  @param  MemoryType The type of memory to allocate.
  @param  Pages The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer.  The buffer returned is aligned on a 4KB boundary.
  If Pages is 0, then NULL is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
InternalAllocatePages (
  IN EFI_MEMORY_TYPE  MemoryType,  
  IN UINTN            Pages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory; 
  EFI_PEI_SERVICES      **PeiServices;

  if (Pages == 0) {
    return NULL;
  }

  PeiServices = GetPeiServicesTablePointer ();
  Status = (*PeiServices)->AllocatePages (PeiServices, MemoryType, Pages, &Memory);
  if (EFI_ERROR (Status)) {
    Memory = 0;
  }
  return (VOID *) (UINTN) Memory;
}

/**
  Allocates the number of 4KB pages specified by Pages of type EfiBootServicesData.

  @param  Pages The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer.  The buffer returned is aligned on a 4KB boundary.
  If Pages is 0, then NULL is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
{
  return InternalAllocatePages (EfiBootServicesData, Pages);
}

/**
  Allocates the number of 4KB pages specified by Pages of type EfiRuntimeServicesData. 

  @param  Pages The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer.  The buffer returned is aligned on a 4KB boundary.
  If Pages is 0, then NULL is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateRuntimePages (
  IN UINTN  Pages
  )
{
  return InternalAllocatePages (EfiRuntimeServicesData, Pages);
}

/**
  Allocates the number of 4KB pages specified by Pages of type EfiReservedMemoryType. 

  @param  Pages The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer.  The buffer returned is aligned on a 4KB boundary.
  If Pages is 0, then NULL is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateReservedPages (
  IN UINTN  Pages
  )
{
  return InternalAllocatePages (EfiReservedMemoryType, Pages);
}

/**
  Frees one or more 4KB pages that were previously allocated with 
  one of the page allocation functions in the Memory Allocation Library.

  @param  Buffer Pointer to the buffer of pages to free.
  @param  Pages The number of 4 KB pages to free.

**/
VOID
EFIAPI
FreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  //
  // PEI phase does not support to free pages, so leave it as NOP.
  //
}

/**
  Allocates the number of 4KB pages specified by Pages of a certian memory type 
  with an alignment specified by Alignment. 

  @param  MemoryType The type of memory to allocate.
  @param  Pages The number of 4 KB pages to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return The allocated buffer is returned.  If Pages is 0, then NULL is returned.
  If there is not enough memory at the specified alignment remaining to satisfy the request, then NULL is returned.

**/
VOID *
InternalAllocateAlignedPages (
  IN EFI_MEMORY_TYPE  MemoryType,  
  IN UINTN            Pages,
  IN UINTN            Alignment
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
  Memory = InternalAllocatePages (MemoryType, Pages + EFI_SIZE_TO_PAGES (Alignment));
  if (Alignment == 0) {
    AlignmentMask = Alignment;
  } else {
    AlignmentMask = Alignment - 1;  
  }
  return (VOID *) (UINTN) (((UINTN) Memory + AlignmentMask) & ~AlignmentMask);
}

/**
  Allocates the number of 4KB pages specified by Pages of type EfiBootServicesData
  with an alignment specified by Alignment.   

  @param  Pages The number of 4 KB pages to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return The allocated buffer is returned.  If Pages is 0, then NULL is returned.
  If there is not enough memory at the specified alignment remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPages (EfiBootServicesData, Pages, Alignment);
}

/**
  Allocates the number of 4KB pages specified by Pages of type EfiRuntimeServicesData
  with an alignment specified by Alignment.   

  @param  Pages The number of 4 KB pages to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return The allocated buffer is returned.  If Pages is 0, then NULL is returned.
  If there is not enough memory at the specified alignment remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedRuntimePages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPages (EfiRuntimeServicesData, Pages, Alignment);
}

/**
  Allocates one or more 4KB pages of type EfiReservedMemoryType at a specified alignment.

  @param  Pages The number of 4 KB pages to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return The allocated buffer is returned.  If Pages is 0, then NULL is returned.
  If there is not enough memory at the specified alignment remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedReservedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPages (EfiReservedMemoryType, Pages, Alignment);
}

/**
  Frees one or more 4KB pages that were previously allocated with 
  one of the aligned page allocation functions in the Memory Allocation Library.

  @param  Buffer Pointer to the buffer of pages to free.
  @param  Pages The number of 4 KB pages to free.

**/
VOID
EFIAPI
FreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  //
  // PEI phase does not support to free pages, so leave it as NOP.
  //
}

/**
  Allocates a buffer of a certain memory type.

  @param  MemoryType The type of memory to allocate.
  @param  AllocationSize The number of bytes to allocate.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
InternalAllocatePool (
  IN EFI_MEMORY_TYPE  MemoryType,  
  IN UINTN            AllocationSize
  )
{
  //
  // If we need lots of small runtime/reserved memory type from PEI in the future, 
  // we can consider providing a more complex algorithm that allocates runtime pages and 
  // provide pool allocations from those pages. 
  //
  return InternalAllocatePages (MemoryType, EFI_SIZE_TO_PAGES (AllocationSize));
}

/**
  Allocates a buffer of type EfiBootServicesData.

  @param  AllocationSize The number of bytes to allocate.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocatePool (
  IN UINTN  AllocationSize
  )
{
  EFI_STATUS        Status;
  EFI_PEI_SERVICES  **PeiServices;
  VOID              *Buffer;
  
  PeiServices = GetPeiServicesTablePointer ();

  Status = (*PeiServices)->AllocatePool (PeiServices, AllocationSize, &Buffer);
  if (EFI_ERROR (Status)) {
    Buffer = NULL;
  }
  return Buffer;
}

/**
  Allocates a buffer of type EfiRuntimeServicesData.

  @param  AllocationSize The number of bytes to allocate.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateRuntimePool (
  IN UINTN  AllocationSize
  )
{
  return InternalAllocatePool (EfiRuntimeServicesData, AllocationSize);
}

/**
  Allocates a buffer of type EfiReservedMemoryType.

  @param  AllocationSize The number of bytes to allocate.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateReservedPool (
  IN UINTN  AllocationSize
  )
{
  return InternalAllocatePool (EfiReservedMemoryType, AllocationSize);
}

/**
  Allocates and zeros a buffer of a certian pool type.

  @param  PoolType The type of memory to allocate.
  @param  AllocationSize The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
InternalAllocateZeroPool (
  IN EFI_MEMORY_TYPE  PoolType,  
  IN UINTN            AllocationSize
  ) 
{
  VOID  *Memory;

  Memory = InternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

/**
  Allocates and zeros a buffer of type EfiBootServicesData.

  @param  AllocationSize The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateZeroPool (
  IN UINTN  AllocationSize
  )
{
  VOID  *Memory;

  Memory = AllocatePool (AllocationSize);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

/**
  Allocates and zeros a buffer of type EfiRuntimeServicesData.

  @param  AllocationSize The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateRuntimeZeroPool (
  IN UINTN  AllocationSize
  )
{
  return InternalAllocateZeroPool (EfiRuntimeServicesData, AllocationSize);
}

/**
  Allocates and zeros a buffer of type EfiReservedMemoryType.

  @param  AllocationSize The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateReservedZeroPool (
  IN UINTN  AllocationSize
  )
{
  return InternalAllocateZeroPool (EfiReservedMemoryType, AllocationSize);
}

/**
  Copies a buffer to an allocated buffer of a certian memory type. 

  @param  MemoryType The type of pool to allocate.
  @param  AllocationSize The number of bytes to allocate and zero.
  @param  Buffer The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
InternalAllocateCopyPool (
  IN EFI_MEMORY_TYPE  PoolType,  
  IN UINTN            AllocationSize,
  IN CONST VOID       *Buffer
  ) 
{
  VOID  *Memory;

  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = InternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
     Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
} 

/**
  Copies a buffer to an allocated buffer of type EfiBootServicesData. 

  @param  AllocationSize The number of bytes to allocate.
  @param  Buffer The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  VOID  *Memory;

  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = AllocatePool (AllocationSize);
  if (Memory != NULL) {
     Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

/**
  Copies a buffer to an allocated buffer of type EfiRuntimeServicesData. 

  @param  AllocationSize The number of bytes to allocate.
  @param  Buffer The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return InternalAllocateCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer);
}

/**
  Copies a buffer to an allocated buffer of type EfiReservedMemoryType. 

  @param  AllocationSize The number of bytes to allocate.
  @param  Buffer The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return InternalAllocateCopyPool (EfiReservedMemoryType, AllocationSize, Buffer);
}

/**
  Copies a buffer to an allocated buffer of type EfiReservedMemoryType at a specified alignment.

  @param  Buffer Pointer to the buffer to free.

**/
VOID
EFIAPI
FreePool (
  IN VOID   *Buffer
  )
{
  //
  // PEI phase does not support to free pool, so leave it as NOP.
  //
}

/**
  Allocates a buffer of a certain pool type at a specified alignment.

  @param  PoolType The type of pool to allocate.
  @param  AllocationSize The number of bytes to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.                            If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
InternalAllocateAlignedPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN UINTN            Alignment
  )
{
  VOID    *RawAddress;
  UINTN   AlignedAddress;
  UINTN   AlignmentMask;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);
  
  if (Alignment == 0) {
    AlignmentMask = Alignment;
  } else {
    AlignmentMask = Alignment - 1;
  }
  //
  // Make sure that AllocationSize plus AlignmentMask does not overflow.
  //
  ASSERT (AllocationSize <= (MAX_ADDRESS - AlignmentMask));

  RawAddress      = InternalAllocatePool (PoolType, AllocationSize + AlignmentMask);
  
  AlignedAddress  = ((UINTN) RawAddress + AlignmentMask) & ~AlignmentMask;
    
  return (VOID *) AlignedAddress;
}

/**
  Allocates a buffer of type EfiBootServicesData at a specified alignment.

  @param  AllocationSize The number of bytes to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.                            If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  VOID    *RawAddress;
  UINTN   AlignedAddress;
  UINTN   AlignmentMask;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);

  if (Alignment == 0) {
    AlignmentMask = Alignment;
  } else {
    AlignmentMask = Alignment - 1;
  }

  //
  // Make sure that AllocationSize plus AlignmentMask does not overflow.
  //
  ASSERT (AllocationSize <= (MAX_ADDRESS - AlignmentMask));

  RawAddress      = AllocatePool (AllocationSize + AlignmentMask);
  
  AlignedAddress  = ((UINTN) RawAddress + AlignmentMask) & ~AlignmentMask;
    
  return (VOID *) AlignedAddress;
}

/**
  Allocates a buffer of type EfiRuntimeServicesData at a specified alignment.

  @param  AllocationSize The number of bytes to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedRuntimePool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPool (EfiRuntimeServicesData, AllocationSize, Alignment);
}

/**
  Allocates a buffer of type EfiReservedMemoryType at a specified alignment.

  @param  AllocationSize The number of bytes to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedReservedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedPool (EfiReservedMemoryType, AllocationSize, Alignment);
}

/**
  Allocates and zeros a buffer of type EfiBootServicesData at a specified alignment.

  @param  PoolType The type of pool to allocate.
  @param  AllocationSize The number of bytes to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
InternalAllocateAlignedZeroPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN UINTN            Alignment
  )
{
  VOID    *Memory;

  Memory = InternalAllocateAlignedPool (PoolType, AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

/**
  Allocates and zeros a buffer of type EfiBootServicesData at a specified alignment.

  @param  AllocationSize The number of bytes to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  VOID    *Memory;

  Memory = AllocateAlignedPool (AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

/**
  Allocates and zeros a buffer of type EfiRuntimeServicesData at a specified alignment.

  @param  AllocationSize The number of bytes to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedRuntimeZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedZeroPool (EfiRuntimeServicesData, AllocationSize, Alignment);
}

/**
  Allocates and zeros a buffer of type EfiReservedMemoryType at a specified alignment.

  @param  AllocationSize The number of bytes to allocate.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedReservedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return InternalAllocateAlignedZeroPool (EfiReservedMemoryType, AllocationSize, Alignment);
}

/**
  Copies a buffer to an allocated buffer of type EfiBootServicesData at a specified alignment.

  @param  PoolType The type of pool to allocate.
  @param  AllocationSize The number of bytes to allocate.
  @param  Buffer The buffer to copy to the allocated buffer.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
InternalAllocateAlignedCopyPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN CONST VOID       *Buffer,
  IN UINTN            Alignment
  )
{
  VOID  *Memory;
  
  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = InternalAllocateAlignedPool (PoolType, AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

/**
  Copies a buffer to an allocated buffer of type EfiBootServicesData at a specified alignment.

  @param  AllocationSize The number of bytes to allocate.
  @param  Buffer The buffer to copy to the allocated buffer.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  VOID  *Memory;
  
  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = AllocateAlignedPool (AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

/**
  Copies a buffer to an allocated buffer of type EfiRuntimeServicesData at a specified alignment.

  @param  AllocationSize The number of bytes to allocate.
  @param  Buffer The buffer to copy to the allocated buffer.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return InternalAllocateAlignedCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer, Alignment);
}

/**
  Copies a buffer to an allocated buffer of type EfiReservedMemoryType at a specified alignment.

  @param  AllocationSize The number of bytes to allocate.
  @param  Buffer The buffer to copy to the allocated buffer.
  @param  Alignment The requested alignment of the allocation.  Must be a power of two.
  If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return InternalAllocateAlignedCopyPool (EfiReservedMemoryType, AllocationSize, Buffer, Alignment);
}

/**
  Frees a buffer that was previously allocated with one of the aligned pool allocation functions 
  in the Memory Allocation Library.

  @param  Buffer Pointer to the buffer to free.

**/
VOID
EFIAPI
FreeAlignedPool (
  IN VOID   *Buffer
  )
{
  //
  // PEI phase does not support to free pool, so leave it as NOP.
  //
}
