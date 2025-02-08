/** @file
  QemuFwCfg cached feature related functions.
  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgLib.h>
#include "QemuFwCfgLibInternal.h"

/**
  Get the pointer to the cached fw_cfg item.
  @param[in] Item   The fw_cfg item to be retrieved.
  @retval    FW_CFG_CACHED_ITEM   Pointer to the cached fw_cfg item.
  @retval    NULL                The fw_cfg item is not cached.
**/
FW_CFG_CACHED_ITEM *
InternalQemuFwCfgItemCached (
  IN FIRMWARE_CONFIG_ITEM  Item
  )
{
  BOOLEAN               Cached;
  FW_CFG_CACHED_ITEM    *CachedItem;
  UINT16                SelectItem;
  EFI_PEI_HOB_POINTERS  Hob;
  FW_CFG_CACHED_ITEM    *FwCfgCachedItem;

 #ifdef TDX_PEI_LESS_BOOT
  if (InternalQemuFwCfgCheckOvmfWorkArea () == FALSE) {
    return NULL;
  }

 #endif

  SelectItem = (UINT16)(UINTN)Item;

  Hob.Raw    = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  Cached     = FALSE;
  CachedItem = NULL;

  while (Hob.Raw != NULL) {
    FwCfgCachedItem = GET_GUID_HOB_DATA (Hob);
    if ((SelectItem == FwCfgCachedItem->FwCfgItem) && (FwCfgCachedItem->DataSize != 0)) {
      Cached     = TRUE;
      CachedItem = FwCfgCachedItem;
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextGuidHob (&gOvmfFwCfgInfoHobGuid, Hob.Raw);
  }

  return Cached ? CachedItem : NULL;
}

/**
  Clear the QEMU_FW_CFG_WORK_AREA.
**/
VOID
InternalQemuFwCfgCacheResetWorkArea (
  VOID
  )
{
  QEMU_FW_CFG_WORK_AREA  *QemuFwCfgWorkArea;

  QemuFwCfgWorkArea = InternalQemuFwCfgCacheGetWorkArea ();
  if (QemuFwCfgWorkArea != NULL) {
    QemuFwCfgWorkArea->FwCfgItem = 0;
    QemuFwCfgWorkArea->Offset    = 0;
    QemuFwCfgWorkArea->Reading   = FALSE;
  }
}

/**
  Check if reading from FwCfgCache is ongoing.
  @retval   TRUE  Reading from FwCfgCache is ongoing.
  @retval   FALSE Reading from FwCfgCache is not ongoing.
**/
BOOLEAN
InternalQemuFwCfgCacheReading (
  VOID
  )
{
  BOOLEAN                Reading;
  QEMU_FW_CFG_WORK_AREA  *QemuFwCfgWorkArea;

  Reading           = FALSE;
  QemuFwCfgWorkArea = InternalQemuFwCfgCacheGetWorkArea ();
  if (QemuFwCfgWorkArea != NULL) {
    Reading = QemuFwCfgWorkArea->Reading;
  }

  return Reading;
}

BOOLEAN
InternalQemuFwCfgCacheSelectItem (
  IN  FIRMWARE_CONFIG_ITEM  Item
  )
{
  QEMU_FW_CFG_WORK_AREA  *QemuFwCfgWorkArea;

  // Walk thru cached fw_items to see if Item is cached.
  if (InternalQemuFwCfgItemCached (Item) == NULL) {
    return FALSE;
  }

  QemuFwCfgWorkArea = InternalQemuFwCfgCacheGetWorkArea ();
  if (QemuFwCfgWorkArea == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid FwCfg Cache Work Area\n", __func__));
    return FALSE;
  }

  QemuFwCfgWorkArea->FwCfgItem = (UINT16)Item;
  QemuFwCfgWorkArea->Offset    = 0;
  QemuFwCfgWorkArea->Reading   = TRUE;

  return TRUE;
}

/**
  Read the fw_cfg data from Cache.
  @param[in]  Size    Data size to be read
  @param[in]  Buffer  Pointer to the buffer to which data is written
  @retval  EFI_SUCCESS   - Successfully
  @retval  Others        - As the error code indicates
**/
EFI_STATUS
InternalQemuFwCfgCacheReadBytes (
  IN     UINTN  Size,
  IN OUT VOID   *Buffer
  )
{
  QEMU_FW_CFG_WORK_AREA  *QemuFwCfgWorkArea;
  FW_CFG_CACHED_ITEM     *CachedItem;
  UINTN                  ReadSize;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  QemuFwCfgWorkArea = InternalQemuFwCfgCacheGetWorkArea ();
  if (QemuFwCfgWorkArea == NULL) {
    return RETURN_NOT_FOUND;
  }

  if (!QemuFwCfgWorkArea->Reading) {
    return RETURN_NOT_READY;
  }

  CachedItem = InternalQemuFwCfgItemCached (QemuFwCfgWorkArea->FwCfgItem);
  if (CachedItem == NULL) {
    return RETURN_NOT_FOUND;
  }

  if (QemuFwCfgWorkArea->Offset >= CachedItem->DataSize) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Item Offset(0x%x) in FwCfg Cache\n", __func__, QemuFwCfgWorkArea->Offset));
    ASSERT (FALSE);
    return RETURN_ABORTED;
  }

  if (CachedItem->DataSize - QemuFwCfgWorkArea->Offset > Size) {
    ReadSize = Size;
  } else {
    ReadSize = CachedItem->DataSize - QemuFwCfgWorkArea->Offset;
  }

  CopyMem (Buffer, (UINT8 *)CachedItem + sizeof (FW_CFG_CACHED_ITEM) + QemuFwCfgWorkArea->Offset, ReadSize);
  QemuFwCfgWorkArea->Offset += (UINT32)ReadSize;

  DEBUG ((DEBUG_VERBOSE, "%a: found Item 0x%x in FwCfg Cache\n", __func__, QemuFwCfgWorkArea->FwCfgItem));
  return RETURN_SUCCESS;
}
