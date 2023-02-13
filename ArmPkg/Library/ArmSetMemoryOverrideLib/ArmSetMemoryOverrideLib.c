/** @file
  Overlay implementation of DXE core gCpuSetMemoryAttributes for ARM.

  Copyright (c) 2023, Google LLC. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

#include <Library/ArmMmuLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Cpu.h>

extern EFI_CPU_SET_MEMORY_ATTRIBUTES  gCpuSetMemoryAttributes;

STATIC UINTN  mRecursionLevel;

/**
  Clone of CPU_ARCH_PROTOCOL::SetMemoryAttributes() which is made available to
  the DXE core by NULL library class resolution, so that it can manage page
  permissions right from the start.

  @param  This                  CPU arch protocol pointer, should be NULL.
  @param  BaseAddress           Start address of the region.
  @param  Length                Size of the region, in bytes.
  @param  Attributes            Attributes to set on the region.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Operation failed due to lack of memory.

**/
STATIC
EFI_STATUS
EFIAPI
EarlyArmSetMemoryAttributes (
  IN  EFI_CPU_ARCH_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN  UINT64                 Length,
  IN  UINT64                 Attributes
  )
{
  EFI_STATUS  Status;

  // There are cases where the use of strict memory permissions may trigger
  // unbounded recursion in the page table code. This happens when setting
  // memory permissions results in a page table split and therefore a page
  // allocation, which could trigger a recursive invocation of this function.
  ASSERT (mRecursionLevel < 2);

  mRecursionLevel++;

  Status = ArmSetMemoryAttributes (
             BaseAddress,
             Length,
             Attributes
             );

  mRecursionLevel--;
  return Status;
}

/**
  Library constructor.

  @retval RETURN_SUCCESS   Operation successful.

**/
RETURN_STATUS
EFIAPI
ArmSetMemoryOverrideLibConstructor (
  VOID
  )
{
  gCpuSetMemoryAttributes = EarlyArmSetMemoryAttributes;

  return RETURN_SUCCESS;
}
