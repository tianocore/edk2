/** @file
  SMBASE relocation for hot-plugged CPUs.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>                             // BASE_1MB
#include <Library/BaseMemoryLib.h>            // CopyMem()
#include <Library/DebugLib.h>                 // DEBUG()

#include "Smbase.h"

extern CONST UINT8 mPostSmmPen[];
extern CONST UINT16 mPostSmmPenSize;

/**
  Allocate a non-SMRAM reserved memory page for the Post-SMM Pen for hot-added
  CPUs.

  This function may only be called from the entry point function of the driver.

  @param[out] PenAddress   The address of the allocated (normal RAM) reserved
                           page.

  @param[in] BootServices  Pointer to the UEFI boot services table. Used for
                           allocating the normal RAM (not SMRAM) reserved page.

  @retval EFI_SUCCESS          Allocation successful.

  @retval EFI_BAD_BUFFER_SIZE  The Post-SMM Pen template is not smaller than
                               EFI_PAGE_SIZE.

  @return                      Error codes propagated from underlying services.
                               DEBUG_ERROR messages have been logged. No
                               resources have been allocated.
**/
EFI_STATUS
SmbaseAllocatePostSmmPen (
  OUT UINT32                  *PenAddress,
  IN  CONST EFI_BOOT_SERVICES *BootServices
  )
{
  EFI_STATUS           Status;
  EFI_PHYSICAL_ADDRESS Address;

  //
  // The pen code must fit in one page, and the last byte must remain free for
  // signaling the SMM Monarch.
  //
  if (mPostSmmPenSize >= EFI_PAGE_SIZE) {
    Status = EFI_BAD_BUFFER_SIZE;
    DEBUG ((DEBUG_ERROR, "%a: mPostSmmPenSize=%u: %r\n", __FUNCTION__,
      mPostSmmPenSize, Status));
    return Status;
  }

  Address = BASE_1MB - 1;
  Status = BootServices->AllocatePages (AllocateMaxAddress,
                           EfiReservedMemoryType, 1, &Address);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AllocatePages(): %r\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: Post-SMM Pen at 0x%Lx\n", __FUNCTION__, Address));
  *PenAddress = (UINT32)Address;
  return EFI_SUCCESS;
}

/**
  Copy the Post-SMM Pen template code into the reserved page allocated with
  SmbaseAllocatePostSmmPen().

  Note that this effects an "SMRAM to normal RAM" copy.

  The SMM Monarch is supposed to call this function from the root MMI handler.

  @param[in] PenAddress  The allocation address returned by
                         SmbaseAllocatePostSmmPen().
**/
VOID
SmbaseReinstallPostSmmPen (
  IN UINT32 PenAddress
  )
{
  CopyMem ((VOID *)(UINTN)PenAddress, mPostSmmPen, mPostSmmPenSize);
}

/**
  Release the reserved page allocated with SmbaseAllocatePostSmmPen().

  This function may only be called from the entry point function of the driver,
  on the error path.

  @param[in] PenAddress    The allocation address returned by
                           SmbaseAllocatePostSmmPen().

  @param[in] BootServices  Pointer to the UEFI boot services table. Used for
                           releasing the normal RAM (not SMRAM) reserved page.
**/
VOID
SmbaseReleasePostSmmPen (
  IN UINT32                  PenAddress,
  IN CONST EFI_BOOT_SERVICES *BootServices
  )
{
  BootServices->FreePages (PenAddress, 1);
}
