/** @file

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <LockBoxLib.h>

/**
  Allocate memory below 4G memory address.

  This function allocates memory below 4G memory address.

  @param  MemoryType   Memory type of memory to allocate.
  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
STATIC
VOID *
AllocateMemoryBelow4G (
  IN EFI_MEMORY_TYPE    MemoryType,
  IN UINTN              Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;
  UINTN                 AllocRemaining;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  //
  // Since we need to use gBS->AllocatePages to get a buffer below
  // 4GB, there is a good chance that space will be wasted for very
  // small allocation. We keep track of unused portions of the page
  // allocations, and use these to allocate memory for small buffers.
  //
  ASSERT (mLockBoxGlobal->Signature == LOCK_BOX_GLOBAL_SIGNATURE);
  if ((UINTN) mLockBoxGlobal->SubPageRemaining >= Size) {
    Buffer = (VOID*)(UINTN) mLockBoxGlobal->SubPageBuffer;
    mLockBoxGlobal->SubPageBuffer += (UINT32) Size;
    mLockBoxGlobal->SubPageRemaining -= (UINT32) Size;
    return Buffer;
  }

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   MemoryType,
                   Pages,
                   &Address
                   );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, EFI_PAGES_TO_SIZE (Pages));

  AllocRemaining = EFI_PAGES_TO_SIZE (Pages) - Size;
  if (AllocRemaining > (UINTN) mLockBoxGlobal->SubPageRemaining) {
    mLockBoxGlobal->SubPageBuffer = (UINT32) (Address + Size);
    mLockBoxGlobal->SubPageRemaining = (UINT32) AllocRemaining;
  }

  return Buffer;
}


/**
  Allocates a buffer of type EfiACPIMemoryNVS.

  Allocates the number bytes specified by AllocationSize of type
  EfiACPIMemoryNVS and returns a pointer to the allocated buffer.
  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy
  the request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAcpiNvsPool (
  IN UINTN  AllocationSize
  )
{
  return AllocateMemoryBelow4G (EfiACPIMemoryNVS, AllocationSize);
}


EFI_STATUS
EFIAPI
LockBoxDxeLibInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return LockBoxLibInitialize ();
}
