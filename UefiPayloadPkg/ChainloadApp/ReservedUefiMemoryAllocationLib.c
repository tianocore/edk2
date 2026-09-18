/** @file
  MemoryAllocationLib instance that keeps ArmMmuLib's translation-table
  pages as EfiReservedMemoryType.

  ChainloadApp installs its own translation tables via
  ArmConfigureMmu() while the outer firmware is still running, so
  that the payload can be entered with the MMU and caches enabled.
  ArmMmuLib allocates every level of that hierarchy through
  MemoryAllocationLib::AllocatePages(), which the stock
  UefiMemoryAllocationLib backs with EfiBootServicesData.  After
  ExitBootServices() the payload's memory-map HOB reports
  EfiBootServicesData as free RAM, so DXE could allocate over the
  live tables.  Overriding AllocatePages() to EfiReservedMemoryType
  keeps every table page in an isolated Reserved descriptor in the
  outer memory-map snapshot, so it never coalesces with adjacent
  conventional memory and the payload's HOB-memory search cannot
  select it.  Every Reserved page allocation is also recorded so
  ChainloadApp can hand the payload an explicit list of boot-time
  reservations for it to publish as SYSTEM_MEMORY pinned by an
  EfiBootServicesData allocation HOB, letting the OS reclaim the
  table pages after it has installed its own translation.

  Only AllocatePages() is redirected.  Pool allocations remain
  EfiBootServicesData: they back short-lived buffers (Print(), the
  memory-map snapshot) that are freed before the branch and never
  carried across the handoff.  The library implements only the
  MemoryAllocationLib functions the module's objects reference;
  the runtime, aligned and remaining variants are omitted.
  AllocateCopyPool() and ReallocatePool() are implemented because
  UefiLib and UefiDevicePathLib reference them, and a strict PE
  linker resolves the reference even when the calling function is
  never used.

  Copyright (c) 2026, Amazon.com, Inc. or its affiliates. All Rights Reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

//
// Every Reserved page allocation this library returns is recorded so
// that ChainloadApp can hand the payload an explicit list of the
// launcher's own boot-time-only reservations (the translation-table
// pages ArmMmuLib allocates through here, in addition to the FV, HOB
// list and stack ChainloadApp allocates itself).  The library is a
// per-module override for ChainloadApp only, so exporting these as
// plain globals is sufficient; there is no MemoryAllocationLib
// interface for it.
//
#define RESERVED_PAGE_ALLOC_MAX  64

EFI_PHYSICAL_ADDRESS  gReservedPageAllocBase[RESERVED_PAGE_ALLOC_MAX];
UINTN                 gReservedPageAllocPages[RESERVED_PAGE_ALLOC_MAX];
UINTN                 gReservedPageAllocCount;

/**
  Record a Reserved page allocation for later reclamation.

  Appends the range to the exported gReservedPageAlloc* table that
  ChainloadApp turns into the boot-time reservation HOB.  If the
  table is full the range is dropped with a warning; the pages stay
  Reserved and are not reclaimed to the OS.

  @param  Base                  The base address of the allocation.
  @param  Pages                 The number of 4 KB pages allocated.

**/
STATIC
VOID
RecordReservedPageAlloc (
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN UINTN                 Pages
  )
{
  if (gReservedPageAllocCount < RESERVED_PAGE_ALLOC_MAX) {
    gReservedPageAllocBase[gReservedPageAllocCount]  = Base;
    gReservedPageAllocPages[gReservedPageAllocCount] = Pages;
    gReservedPageAllocCount++;
  } else {
    DEBUG ((
      DEBUG_WARN,
      "%a: table full at 0x%Lx (%u pages); page stays Reserved and is "
      "not reclaimed to the OS\n",
      __func__,
      (UINT64)Base,
      (UINT32)Pages
      ));
  }
}

/**
  Allocates one or more 4KB pages of a certain memory type.

  Allocates the number of 4KB pages of a certain memory type and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.  Reserved-type allocations are recorded for later reclamation.

  @param  MemoryType            The type of memory to allocate.
  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
STATIC
VOID *
InternalAllocatePages (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            Pages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;

  if (Pages == 0) {
    return NULL;
  }

  Status = gBS->AllocatePages (AllocateAnyPages, MemoryType, Pages, &Memory);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  if (MemoryType == EfiReservedMemoryType) {
    RecordReservedPageAlloc (Memory, Pages);
  }

  return (VOID *)(UINTN)Memory;
}

/**
  Allocates one or more 4KB pages of type EfiReservedMemoryType.

  Allocates the number of 4KB pages of type EfiReservedMemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
{
  return InternalAllocatePages (EfiReservedMemoryType, Pages);
}

/**
  Frees one or more 4KB pages that were previously allocated with one of the page allocation
  functions in the Memory Allocation Library.

  Frees the number of 4KB pages specified by Pages from the buffer specified by Buffer.  Buffer
  must have been allocated on a previous call to the page allocation services of the Memory
  Allocation Library.  If it is not possible to free allocated pages, then this function will
  perform no actions.

  If Pages is zero, then ASSERT().

  @param  Buffer                The pointer to the buffer of pages to free.
  @param  Pages                 The number of 4 KB pages to free.

**/
VOID
EFIAPI
FreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  EFI_STATUS  Status;

  ASSERT (Pages != 0);
  Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)Buffer, Pages);
  ASSERT_EFI_ERROR (Status);
}

/**
  Allocates a buffer of a certain pool type.

  Allocates the number bytes specified by AllocationSize of a certain pool type and returns a
  pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  MemoryType            The type of memory to allocate.
  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
STATIC
VOID *
InternalAllocatePool (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            AllocationSize
  )
{
  EFI_STATUS  Status;
  VOID        *Memory;

  Status = gBS->AllocatePool (MemoryType, AllocationSize, &Memory);
  if (EFI_ERROR (Status)) {
    Memory = NULL;
  }

  return Memory;
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
  return InternalAllocatePool (EfiBootServicesData, AllocationSize);
}

/**
  Allocates and zeros a buffer of a certain pool type.

  Allocates the number bytes specified by AllocationSize of a certain pool type, clears the buffer
  with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a valid
  buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the request,
  then NULL is returned.

  @param  PoolType              The type of memory to allocate.
  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
STATIC
VOID *
InternalAllocateZeroPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize
  )
{
  VOID  *Memory;

  Memory = InternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
    ZeroMem (Memory, AllocationSize);
  }

  return Memory;
}

/**
  Allocates and zeros a buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData, clears the
  buffer with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a
  valid buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateZeroPool (
  IN UINTN  AllocationSize
  )
{
  return InternalAllocateZeroPool (EfiBootServicesData, AllocationSize);
}

/**
  Frees a buffer that was previously allocated with one of the pool allocation functions in the
  Memory Allocation Library.

  Frees the buffer specified by Buffer.  Buffer must have been allocated on a previous call to the
  pool allocation services of the Memory Allocation Library.  If it is not possible to free pool
  resources, then this function will perform no actions.

  @param  Buffer                The pointer to the buffer to free.

**/
VOID
EFIAPI
FreePool (
  IN VOID  *Buffer
  )
{
  EFI_STATUS  Status;

  Status = gBS->FreePool (Buffer);
  ASSERT_EFI_ERROR (Status);
}

/**
  Copies a buffer to an allocated buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type
  EfiBootServicesData, copies AllocationSize bytes from SourceBuffer to
  the newly allocated buffer, and returns a pointer to the allocated
  buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  If SourceBuffer is NULL, then ASSERT().
  If AllocationSize is greater than (MAX_ADDRESS - SourceBuffer + 1),
  then ASSERT().

  @param  AllocationSize  The number of bytes to allocate.
  @param  SourceBuffer    The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *SourceBuffer
  )
{
  VOID  *Memory;

  ASSERT (SourceBuffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN)SourceBuffer + 1));

  Memory = InternalAllocatePool (EfiBootServicesData, AllocationSize);
  if (Memory != NULL) {
    Memory = CopyMem (Memory, SourceBuffer, AllocationSize);
  }

  return Memory;
}

/**
  Reallocates a buffer of type EfiBootServicesData.

  Allocates NewSize bytes of type EfiBootServicesData, copies
  MIN (OldSize, NewSize) bytes from OldBuffer to the newly allocated
  buffer, frees OldBuffer, and returns a pointer to the allocated
  buffer.  If NewSize is 0, then a valid buffer of 0 size is returned.
  If there is not enough memory remaining to satisfy the request, then
  NULL is returned and OldBuffer is not freed.

  If the allocation of the new buffer fails, then OldBuffer is not
  freed.
  If OldSize is greater than NewSize, then ASSERT().

  @param  OldSize        The size, in bytes, of OldBuffer.
  @param  NewSize        The size, in bytes, of the buffer to reallocate.
  @param  OldBuffer      The buffer to copy to the allocated buffer.
                         This is an optional parameter that may be NULL.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
ReallocatePool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer  OPTIONAL
  )
{
  VOID  *NewBuffer;

  ASSERT (OldSize <= NewSize);

  NewBuffer = InternalAllocatePool (EfiBootServicesData, NewSize);
  if ((NewBuffer != NULL) && (OldBuffer != NULL)) {
    NewBuffer = CopyMem (NewBuffer, OldBuffer, OldSize);
    FreePool (OldBuffer);
  }

  return NewBuffer;
}
