/** @file

  Copyright (c) 2011-2024, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/ArmPlatformLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Guid/TlHob.h>
#include <Library/ArmTransferListLib.h>
#include <Library/DebugLib.h>
#include <Guid/FdtHob.h>
#include <Library/MemoryAllocationLib.h>
#include <libfdt.h>

EFI_STATUS
EFIAPI
PlatformPeim (
  VOID
  )
{
  VOID                   *TransferListBase;
  VOID                   *DeviceTreeBase;
  TRANSFER_ENTRY_HEADER  *Te;
  UINT64                 *TlHobData;
  UINTN                  FdtSize;
  UINTN                  FdtPages;
  UINT64                 *FdtHobData;
  VOID                   *NewBase;

  TransferListBase = (VOID *)PcdGet64 (PcdTransferListBaseAddress);
  if (TlCheckHeader (TransferListBase) != TL_OPS_INVALID) {
    TlDump (TransferListBase);
    TlHobData = BuildGuidHob (&gTlHobGuid, sizeof (*TlHobData));
    if (TlHobData == NULL) {
      ASSERT (0);
      return EFI_OUT_OF_RESOURCES;
    }

    *TlHobData = (UINTN)TransferListBase;
    DEBUG ((DEBUG_INFO|DEBUG_INIT|DEBUG_LOAD, "Transfer List GUID HOB built\n"));
    // Find the device tree entry
    Te = TlFindEntry (TransferListBase, TRANSFER_ENTRY_TAG_ID_FDT);
    if (Te == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Device Tree entry not found\n", __func__));
    } else {
      DeviceTreeBase = TlGetEntryData (Te);
      if ((DeviceTreeBase == NULL) || (fdt_check_header (DeviceTreeBase) != 0)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to get Device Tree data\n", __func__));
        return EFI_NOT_FOUND;
      }

      FdtSize  = fdt_totalsize ((VOID *)DeviceTreeBase) + 0x6f;
      FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
      NewBase  = AllocatePages (FdtPages);
      if (NewBase == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: Could not allocate memory for DTB\n", __func__));
        return EFI_OUT_OF_RESOURCES;
      }

      fdt_open_into ((VOID *)DeviceTreeBase, NewBase, EFI_PAGES_TO_SIZE (FdtPages));
      // Building FDT GuidHob
      FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof (*FdtHobData));
      if (FdtHobData == NULL) {
        DEBUG ((DEBUG_INFO, "%a: BuildGuidHob failed\n", __func__));
        return EFI_OUT_OF_RESOURCES;
      }

      *FdtHobData = (UINTN)NewBase;
    }
  } else {
    DEBUG (
           (
            DEBUG_ERROR,
            "%a: No Transfer List found @ 0x%p\n",
            __func__,
            TransferListBase
           )
           );
  }

  BuildFvHob (PcdGet64 (PcdFvBaseAddress), PcdGet32 (PcdFvSize));

  return EFI_SUCCESS;
}
