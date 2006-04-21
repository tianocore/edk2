/** @file
	Memory Allocation Library Services

	Copyright (c) 2006, Intel Corporation                                                         
	All rights reserved. This program and the accompanying materials                          
	are licensed and made available under the terms and conditions of the BSD License         
	which accompanies this distribution.  The full text of the license may be found at        
	http://opensource.org/licenses/bsd-license.php                                            

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

	Module Name:	MemoryAllocationLib.h

**/

#ifndef __MEMORY_ALLOCATION_LIB_H__
#define __MEMORY_ALLOCATION_LIB_H__

/**
	Allocates the number of 4KB pages specified by Pages of type EfiBootServicesData.

	@param	Pages The number of 4 KB pages to allocate.

	@return
	A pointer to the allocated buffer.  The buffer returned is aligned on a 4KB boundary.
	If Pages is 0, then NULL is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
;

/**
	Allocates the number of 4KB pages specified by Pages of type EfiRuntimeServicesData. 

	@param	Pages The number of 4 KB pages to allocate.

	@return
	A pointer to the allocated buffer.  The buffer returned is aligned on a 4KB boundary.
	If Pages is 0, then NULL is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateRuntimePages (
  IN UINTN  Pages
  )
;

/**
	Allocates the number of 4KB pages specified by Pages of type EfiReservedMemoryType. 

	@param	Pages The number of 4 KB pages to allocate.

	@return
	A pointer to the allocated buffer.  The buffer returned is aligned on a 4KB boundary.
	If Pages is 0, then NULL is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateReservedPages (
  IN UINTN  Pages
  )
;

/**
	Frees one or more 4KB pages that were previously allocated with 
	one of the page allocation functions in the Memory Allocation Library.

	@param	Buffer Pointer to the buffer of pages to free.
	@param	Pages The number of 4 KB pages to free.

	None.

**/
VOID
EFIAPI
FreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
;

/**
	Allocates the number of 4KB pages specified by Pages of type EfiBootServicesData with an alignment specified by Alignment.   

	@param	Pages The number of 4 KB pages to allocate.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	The allocated buffer is returned.  If Pages is 0, then NULL is returned.
	If there is not enough memory at the specified alignment remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
;

/**
	Allocates the number of 4KB pages specified by Pages of type EfiRuntimeServicesData with an alignment specified by Alignment.   

	@param	Pages The number of 4 KB pages to allocate.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	The allocated buffer is returned.  If Pages is 0, then NULL is returned.
	If there is not enough memory at the specified alignment remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedRuntimePages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
;

/**
	Allocates one or more 4KB pages of type EfiReservedMemoryType at a specified alignment.

	@param	Pages The number of 4 KB pages to allocate.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	The allocated buffer is returned.  If Pages is 0, then NULL is returned.
	If there is not enough memory at the specified alignment remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedReservedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
;

/**
	Frees one or more 4KB pages that were previously allocated with 
	one of the aligned page allocation functions in the Memory Allocation Library.

	@param	Buffer Pointer to the buffer of pages to free.
	@param	Pages The number of 4 KB pages to free.

	None.

**/
VOID
EFIAPI
FreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
;

/**
	Allocates a buffer of type EfiBootServicesData.

	@param	AllocationSize The number of bytes to allocate.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocatePool (
  IN UINTN  AllocationSize
  )
;

/**
	Allocates a buffer of type EfiRuntimeServicesData.

	@param	AllocationSize The number of bytes to allocate.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateRuntimePool (
  IN UINTN  AllocationSize
  )
;

/**
	Allocates a buffer of type EfiReservedMemoryType.

	@param	AllocationSize The number of bytes to allocate.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateReservedPool (
  IN UINTN  AllocationSize
  )
;

/**
	Allocates and zeros a buffer of type EfiBootServicesData.

	@param	AllocationSize The number of bytes to allocate and zero.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateZeroPool (
  IN UINTN  AllocationSize
  )
;

/**
	Allocates and zeros a buffer of type EfiRuntimeServicesData.

	@param	AllocationSize The number of bytes to allocate and zero.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateRuntimeZeroPool (
  IN UINTN  AllocationSize
  )
;

/**
	Allocates and zeros a buffer of type EfiReservedMemoryType.

	@param	AllocationSize The number of bytes to allocate and zero.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateReservedZeroPool (
  IN UINTN  AllocationSize
  )
;

/**
	Copies a buffer to an allocated buffer of type EfiBootServicesData. 

	@param	AllocationSize The number of bytes to allocate.
	@param	Buffer The buffer to copy to the allocated buffer.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
;

/**
	Copies a buffer to an allocated buffer of type EfiRuntimeServicesData. 

	@param	AllocationSize The number of bytes to allocate.
	@param	Buffer The buffer to copy to the allocated buffer.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
;

/**
	Copies a buffer to an allocated buffer of type EfiReservedMemoryType. 

	@param	AllocationSize The number of bytes to allocate.
	@param	Buffer The buffer to copy to the allocated buffer.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
;

/**
	Frees a buffer that was previously allocated with one of the pool allocation functions 
	in the Memory Allocation Library.

	@param	Buffer Pointer to the buffer to free.

	None.

**/
VOID
EFIAPI
FreePool (
  IN VOID   *Buffer
  )
;

/**
	Allocates a buffer of type EfiBootServicesData at a specified alignment.

	@param	AllocationSize The number of bytes to allocate.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
		If Alignment is zero, then byte alignment is used.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
;

/**
	Allocates a buffer of type EfiRuntimeServicesData at a specified alignment.

	@param	AllocationSize The number of bytes to allocate.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedRuntimePool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
;

/**
	Allocates a buffer of type EfiReservedMemoryType at a specified alignment.

	@param	AllocationSize The number of bytes to allocate.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedReservedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
;

/**
	Allocates and zeros a buffer of type EfiBootServicesData at a specified alignment.

	@param	AllocationSize The number of bytes to allocate.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
;

/**
	Allocates and zeros a buffer of type EfiRuntimeServicesData at a specified alignment.

	@param	AllocationSize The number of bytes to allocate.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedRuntimeZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
;

/**
	Allocates and zeros a buffer of type EfiReservedMemoryType at a specified alignment.

	@param	AllocationSize The number of bytes to allocate.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedReservedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
;

/**
	Copies a buffer to an allocated buffer of type EfiBootServicesData at a specified alignment.

	@param	AllocationSize The number of bytes to allocate.
	@param	Buffer The buffer to copy to the allocated buffer.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
;

/**
	Copies a buffer to an allocated buffer of type EfiRuntimeServicesData at a specified alignment.

	@param	AllocationSize The number of bytes to allocate.
	@param	Buffer The buffer to copy to the allocated buffer.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
;

/**
	Copies a buffer to an allocated buffer of type EfiReservedMemoryType at a specified alignment.

	@param	AllocationSize The number of bytes to allocate.
	@param	Buffer The buffer to copy to the allocated buffer.
	@param	Alignment The requested alignment of the allocation.  Must be a power of two.
	If Alignment is zero, then byte alignment is used.

	@return
	A pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.
	If there is not enough memory remaining to satisfy the request, then NULL is returned.

**/
VOID *
EFIAPI
AllocateAlignedReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
;

/**
	Frees a buffer that was previously allocated with one of the aligned pool allocation functions 
	in the Memory Allocation Library.

	@param	Buffer Pointer to the buffer to free.

	None.

**/
VOID
EFIAPI
FreeAlignedPool (
  IN VOID   *Buffer
  )
;

#endif
