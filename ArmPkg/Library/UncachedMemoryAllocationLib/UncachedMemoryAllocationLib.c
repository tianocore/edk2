/** @file

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UncachedMemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ArmLib.h>
#include <Protocol/Cpu.h>

EFI_PHYSICAL_ADDRESS
ConvertToPhysicalAddress (
  IN VOID *VirtualAddress
  )
{
  UINTN UncachedMemoryMask = (UINTN)PcdGet64(PcdArmUncachedMemoryMask);
  UINTN PhysicalAddress;
  
  PhysicalAddress = (UINTN)VirtualAddress & ~UncachedMemoryMask;
  
  return (EFI_PHYSICAL_ADDRESS)PhysicalAddress;
}

VOID *
ConvertToCachedAddress (
  IN VOID *Address
  )
{
  return (VOID *)(UINTN)ConvertToPhysicalAddress(Address);
}

VOID *
ConvertToUncachedAddress (
  IN VOID *Address
  )
{
  UINTN UncachedMemoryMask = (UINTN)PcdGet64(PcdArmUncachedMemoryMask);
  UINTN UncachedAddress;
  
  UncachedAddress = (UINTN)Address | UncachedMemoryMask;
  
  return (VOID *)UncachedAddress;
}

VOID
FlushCache (
  IN EFI_PHYSICAL_ADDRESS Address,
  IN UINTN                Size
  )
{
  EFI_CPU_ARCH_PROTOCOL *Cpu;
  EFI_STATUS            Status;
  
  Status = gBS->LocateProtocol(&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  ASSERT_EFI_ERROR(Status);
  
  Status = Cpu->FlushDataCache(Cpu, Address, Size, EfiCpuFlushTypeWriteBackInvalidate);
  ASSERT_EFI_ERROR(Status);
}

VOID *
UncachedInternalAllocatePages (
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
    Memory = 0;
  }
  
  if (Memory != 0) {
    FlushCache(Memory, EFI_PAGES_TO_SIZE(Pages));
    Memory = (EFI_PHYSICAL_ADDRESS)(UINTN)ConvertToUncachedAddress((VOID *)(UINTN)Memory);
  }
  
  return (VOID *) (UINTN) Memory;
}

VOID *
EFIAPI
UncachedAllocatePages (
  IN UINTN  Pages
  )
{
  return UncachedInternalAllocatePages (EfiBootServicesData, Pages);
}

VOID *
EFIAPI
UncachedAllocateRuntimePages (
  IN UINTN  Pages
  )
{
  return UncachedInternalAllocatePages (EfiRuntimeServicesData, Pages);
}

VOID *
EFIAPI
UncachedAllocateReservedPages (
  IN UINTN  Pages
  )
{
  return UncachedInternalAllocatePages (EfiReservedMemoryType, Pages);
}

VOID
EFIAPI
UncachedFreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  EFI_STATUS  Status;

  ASSERT (Pages != 0);
  
  Buffer = ConvertToCachedAddress(Buffer);
  
  Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) Buffer, Pages);
  ASSERT_EFI_ERROR (Status);
}

VOID *
UncachedInternalAllocateAlignedPages (
  IN EFI_MEMORY_TYPE  MemoryType,  
  IN UINTN            Pages,
  IN UINTN            Alignment
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;
  UINTN                 AlignedMemory;
  UINTN                 AlignmentMask;
  UINTN                 UnalignedPages;
  UINTN                 RealPages;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);
 
  if (Pages == 0) {
    return NULL;
  }
  if (Alignment > EFI_PAGE_SIZE) {
    //
    // Caculate the total number of pages since alignment is larger than page size.
    //
    AlignmentMask  = Alignment - 1;
    RealPages      = Pages + EFI_SIZE_TO_PAGES (Alignment);
    //
    // Make sure that Pages plus EFI_SIZE_TO_PAGES (Alignment) does not overflow.
    //
    ASSERT (RealPages > Pages);
 
    Status         = gBS->AllocatePages (AllocateAnyPages, MemoryType, RealPages, &Memory);
    if (EFI_ERROR (Status)) {
      return NULL;
    }
    AlignedMemory  = ((UINTN) Memory + AlignmentMask) & ~AlignmentMask;
    UnalignedPages = EFI_SIZE_TO_PAGES (AlignedMemory - (UINTN) Memory);
    if (UnalignedPages > 0) {
      //
      // Free first unaligned page(s).
      //
      Status = gBS->FreePages (Memory, UnalignedPages);
      ASSERT_EFI_ERROR (Status);
    }
    Memory         = (EFI_PHYSICAL_ADDRESS) (AlignedMemory + EFI_PAGES_TO_SIZE (Pages));
    UnalignedPages = RealPages - Pages - UnalignedPages;
    if (UnalignedPages > 0) {
      //
      // Free last unaligned page(s).
      //
      Status = gBS->FreePages (Memory, UnalignedPages);
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    //
    // Do not over-allocate pages in this case.
    //
    Status = gBS->AllocatePages (AllocateAnyPages, MemoryType, Pages, &Memory);
    if (EFI_ERROR (Status)) {
      return NULL;
    }
    AlignedMemory  = (UINTN) Memory;
  }
  
  if (AlignedMemory != 0) {
    FlushCache (AlignedMemory, EFI_PAGES_TO_SIZE(Pages));
    AlignedMemory = (UINTN)ConvertToUncachedAddress((VOID *)AlignedMemory);
  }
  
  return (VOID *) AlignedMemory;
}

VOID *
EFIAPI
UncachedAllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedPages (EfiBootServicesData, Pages, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedRuntimePages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedPages (EfiRuntimeServicesData, Pages, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedReservedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedPages (EfiReservedMemoryType, Pages, Alignment);
}

VOID
EFIAPI
UncachedFreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  EFI_STATUS  Status;

  ASSERT (Pages != 0);
  
  Buffer = ConvertToCachedAddress(Buffer);
  
  Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) Buffer, Pages);
  ASSERT_EFI_ERROR (Status);
}

VOID *
UncachedInternalAllocateAlignedPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN UINTN            Alignment
  )
{
  VOID        *RawAddress;
  UINTN       AlignedAddress;
  UINTN       AlignmentMask;
  UINTN       OverAllocationSize;
  UINTN       RealAllocationSize;
  VOID        **FreePointer;
  UINTN       DataCacheLineLength;
  EFI_STATUS  Status;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);

  DataCacheLineLength = ArmDataCacheLineLength();
  
  // Alignment must be at least cache-line aligned
  if (Alignment < DataCacheLineLength) {
    Alignment = DataCacheLineLength;
  }
  
  if (Alignment == 0) {
    AlignmentMask = Alignment;
  } else {
    AlignmentMask = Alignment - 1;
  }
    
  //
  // Calculate the extra memory size, over-allocate memory pool and get the aligned memory address. 
  //
  OverAllocationSize  = sizeof (RawAddress) + AlignmentMask;
  RealAllocationSize  = AllocationSize + OverAllocationSize;
  //
  // Make sure that AllocationSize plus OverAllocationSize does not overflow. 
  //
  ASSERT (RealAllocationSize > AllocationSize); 

  Status = gBS->AllocatePool (PoolType, RealAllocationSize, &RawAddress);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  AlignedAddress      = ((UINTN) RawAddress + OverAllocationSize) & ~AlignmentMask;
  //
  // Save the original memory address just before the aligned address.
  //
  FreePointer         = (VOID **)(AlignedAddress - sizeof (RawAddress));
  *FreePointer        = RawAddress;

  if (AlignedAddress != 0) {
    FlushCache (AlignedAddress, AllocationSize);
    AlignedAddress = (UINTN)ConvertToUncachedAddress((VOID *)AlignedAddress);
  }

  return (VOID *) AlignedAddress;
}

VOID *
EFIAPI
UncachedAllocateAlignedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedPool (EfiBootServicesData, AllocationSize, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedRuntimePool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedPool (EfiRuntimeServicesData, AllocationSize, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedReservedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedPool (EfiReservedMemoryType, AllocationSize, Alignment);
}

VOID *
UncachedInternalAllocateAlignedZeroPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN UINTN            Alignment
  )
{
  VOID    *Memory;
  Memory = UncachedInternalAllocateAlignedPool (PoolType, AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

VOID *
EFIAPI
UncachedAllocateAlignedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedZeroPool (EfiBootServicesData, AllocationSize, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedRuntimeZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedZeroPool (EfiRuntimeServicesData, AllocationSize, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedReservedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedZeroPool (EfiReservedMemoryType, AllocationSize, Alignment);
}

VOID *
UncachedInternalAllocateAlignedCopyPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN CONST VOID       *Buffer,
  IN UINTN            Alignment
  )
{
  VOID  *Memory;
  
  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = UncachedInternalAllocateAlignedPool (PoolType, AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

VOID *
EFIAPI
UncachedAllocateAlignedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return UncachedInternalAllocateAlignedCopyPool (EfiBootServicesData, AllocationSize, Buffer, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return UncachedInternalAllocateAlignedCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return UncachedInternalAllocateAlignedCopyPool (EfiReservedMemoryType, AllocationSize, Buffer, Alignment);
}

VOID
EFIAPI
UncachedFreeAlignedPool (
  IN VOID   *Buffer
  )
{
  VOID        *RawAddress;
  VOID        **FreePointer;
  EFI_STATUS  Status;

  Buffer = ConvertToCachedAddress(Buffer);
  
  //
  // Get the pre-saved original address in the over-allocate pool.
  //
  FreePointer = (VOID **)((UINTN) Buffer - sizeof (RawAddress));
  RawAddress  = *FreePointer;

  Status = gBS->FreePool (RawAddress);
  ASSERT_EFI_ERROR (Status);
}

VOID *
UncachedInternalAllocatePool (
  IN EFI_MEMORY_TYPE  MemoryType,  
  IN UINTN            AllocationSize
  )
{
  UINTN CacheLineLength = ArmDataCacheLineLength();
  return UncachedInternalAllocateAlignedPool(MemoryType, AllocationSize, CacheLineLength);
}

VOID *
EFIAPI
UncachedAllocatePool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocatePool (EfiBootServicesData, AllocationSize);
}

VOID *
EFIAPI
UncachedAllocateRuntimePool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocatePool (EfiRuntimeServicesData, AllocationSize);
}

VOID *
EFIAPI
UncachedAllocateReservedPool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocatePool (EfiReservedMemoryType, AllocationSize);
}

VOID *
UncachedInternalAllocateZeroPool (
  IN EFI_MEMORY_TYPE  PoolType,  
  IN UINTN            AllocationSize
  ) 
{
  VOID  *Memory;

  Memory = UncachedInternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

VOID *
EFIAPI
UncachedAllocateZeroPool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocateZeroPool (EfiBootServicesData, AllocationSize);
}

VOID *
EFIAPI
UncachedAllocateRuntimeZeroPool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocateZeroPool (EfiRuntimeServicesData, AllocationSize);
}

VOID *
EFIAPI
UncachedAllocateReservedZeroPool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocateZeroPool (EfiReservedMemoryType, AllocationSize);
}

VOID *
UncachedInternalAllocateCopyPool (
  IN EFI_MEMORY_TYPE  PoolType,  
  IN UINTN            AllocationSize,
  IN CONST VOID       *Buffer
  ) 
{
  VOID  *Memory;

  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = UncachedInternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
     Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
} 

VOID *
EFIAPI
UncachedAllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return UncachedInternalAllocateCopyPool (EfiBootServicesData, AllocationSize, Buffer);
}

VOID *
EFIAPI
UncachedAllocateRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return UncachedInternalAllocateCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer);
}

VOID *
EFIAPI
UncachedAllocateReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return UncachedInternalAllocateCopyPool (EfiReservedMemoryType, AllocationSize, Buffer);
}

VOID
EFIAPI
UncachedFreePool (
  IN VOID   *Buffer
  )
{
  UncachedFreeAlignedPool(Buffer);
}

VOID
EFIAPI
UncachedSafeFreePool (
  IN VOID   *Buffer
  )
{
  if (Buffer != NULL) {
    UncachedFreePool (Buffer);
    Buffer = NULL;
  }
}

