/** @file

  QemuFwCfg cached feature related functions.

  Copyright (c) 224, Intel Corporation. All rights reserved.<BR>

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
QemuFwCfgItemCached (
  IN FIRMWARE_CONFIG_ITEM  Item
  )
{
  UINT16              HobSize;
  UINT16              Offset;
  BOOLEAN             Cached;
  FW_CFG_CACHED_ITEM  *CachedItem;
  EFI_HOB_GUID_TYPE   *GuidHob;
  UINT32              CachedItemSize;

  if (!QemuFwCfgCacheEnable ()) {
    return NULL;
  }

  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  ASSERT (GuidHob != NULL);

  HobSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
  Offset  = sizeof (EFI_HOB_GUID_TYPE) + sizeof (FW_CFG_CACHE_WORK_AREA);
  ASSERT (Offset < HobSize);

  CachedItem = QemuFwCfgCacheFirstItem ();

  Cached = FALSE;
  while (Offset < HobSize) {
    if (CachedItem->FwCfgItem == Item) {
      Cached = TRUE;
      break;
    }

    CachedItemSize = CachedItem->DataSize + sizeof (FW_CFG_CACHED_ITEM);
    Offset        += CachedItemSize;
    CachedItem     = (FW_CFG_CACHED_ITEM *)((UINT8 *)CachedItem + CachedItemSize);
  }

  return Cached ? CachedItem : NULL;
}

/**
  Clear the FW_CFG_CACHE_WORK_AREA.
**/
VOID
QemuFwCfgCacheResetWorkArea (
  VOID
  )
{
  FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWrokArea;

  FwCfgCacheWrokArea = QemuFwCfgCacheGetWorkArea ();
  if (FwCfgCacheWrokArea != NULL) {
    FwCfgCacheWrokArea->FwCfgItem = 0;
    FwCfgCacheWrokArea->Offset    = 0;
    FwCfgCacheWrokArea->Reading   = FALSE;
  }
}

/**
  Check if reading from FwCfgCache is ongoing.

  @retval   TRUE  Reading from FwCfgCache is ongoing.
  @retval   FALSE Reading from FwCfgCache is not ongoing.
**/
BOOLEAN
QemuFwCfgCacheReading (
  VOID
  )
{
  BOOLEAN                 Reading;
  FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWrokArea;

  Reading            = FALSE;
  FwCfgCacheWrokArea = QemuFwCfgCacheGetWorkArea ();
  if (FwCfgCacheWrokArea != NULL) {
    Reading = FwCfgCacheWrokArea->Reading;
  }

  return Reading;
}

BOOLEAN
QemuFwCfgCacheSelectItem (
  IN  FIRMWARE_CONFIG_ITEM  Item
  )
{
  FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWrokArea;

  // Walk thru cached fw_items to see if Item is cached.
  if (QemuFwCfgItemCached (Item) == NULL) {
    return FALSE;
  }

  FwCfgCacheWrokArea = QemuFwCfgCacheGetWorkArea ();
  ASSERT (FwCfgCacheWrokArea);

  FwCfgCacheWrokArea->FwCfgItem = Item;
  FwCfgCacheWrokArea->Offset    = 0;
  FwCfgCacheWrokArea->Reading   = TRUE;

  return TRUE;
}

/**
  Get the first cached item.

  @retval    FW_CFG_CACHED_ITEM  The first cached item.
  @retval    NULL               There is no cached item.
**/
FW_CFG_CACHED_ITEM *
QemuFwCfgCacheFirstItem (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINT16             HobSize;
  UINT16             Offset;

  if (!QemuFwCfgCacheEnable ()) {
    return NULL;
  }

  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  ASSERT (GuidHob != NULL);

  HobSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
  Offset  = sizeof (EFI_HOB_GUID_TYPE) + sizeof (FW_CFG_CACHE_WORK_AREA);
  ASSERT (Offset < HobSize);

  return (FW_CFG_CACHED_ITEM *)((UINT8 *)GuidHob + Offset);
}

/**
  Read the fw_cfg data from Cache.

  @param[in]  Size    Data size to be read
  @param[in]  Buffer  Pointer to the buffer to which data is written

  @retval  EFI_SUCCESS   - Successfully
  @retval  Others        - As the error code indicates
**/
EFI_STATUS
QemuFwCfgCacheReadBytes (
  IN     UINTN  Size,
  IN OUT VOID   *Buffer
  )
{
  FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWrokArea;
  FW_CFG_CACHED_ITEM      *CachedItem;
  UINT32                  ReadSize;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FwCfgCacheWrokArea = QemuFwCfgCacheGetWorkArea ();
  if (FwCfgCacheWrokArea == NULL) {
    return RETURN_NOT_FOUND;
  }

  if (!FwCfgCacheWrokArea->Reading) {
    return RETURN_NOT_READY;
  }

  CachedItem = QemuFwCfgItemCached (FwCfgCacheWrokArea->FwCfgItem);
  if (CachedItem == NULL) {
    return RETURN_NOT_FOUND;
  }

  if (CachedItem->DataSize - FwCfgCacheWrokArea->Offset > Size) {
    ReadSize = Size;
  } else {
    ReadSize = CachedItem->DataSize - FwCfgCacheWrokArea->Offset;
  }

  CopyMem (Buffer, (UINT8 *)CachedItem + sizeof (FW_CFG_CACHED_ITEM) + FwCfgCacheWrokArea->Offset, ReadSize);
  FwCfgCacheWrokArea->Offset += ReadSize;

  DEBUG ((DEBUG_INFO, "%a: found in FwCfg Cache\n", __func__));
  return RETURN_SUCCESS;
}

RETURN_STATUS
QemuFwCfgItemInCacheList (
  IN   CONST CHAR8           *Name,
  OUT  FIRMWARE_CONFIG_ITEM  *Item,
  OUT  UINTN                 *Size
  )
{
  UINT16              HobSize;
  UINT16              Offset;
  FW_CFG_CACHED_ITEM  *CachedItem;
  EFI_HOB_GUID_TYPE   *GuidHob;
  UINT32              CachedItemSize;

  CachedItem = QemuFwCfgCacheFirstItem ();
  if (CachedItem == NULL) {
    return RETURN_NOT_FOUND;
  }

  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  ASSERT (GuidHob != NULL);

  HobSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
  Offset  = sizeof (EFI_HOB_GUID_TYPE) + sizeof (FW_CFG_CACHE_WORK_AREA);
  ASSERT (Offset < HobSize);

  while (Offset < HobSize) {
    if (AsciiStrCmp (Name, CachedItem->FileName) == 0) {
      *Item = CachedItem->FwCfgItem;
      *Size = CachedItem->DataSize;
      return RETURN_SUCCESS;
    }

    CachedItemSize = CachedItem->DataSize + sizeof (FW_CFG_CACHED_ITEM);
    Offset        += CachedItemSize;
    CachedItem     = (FW_CFG_CACHED_ITEM *)((UINT8 *)CachedItem + CachedItemSize);
  }

  return RETURN_NOT_FOUND;
}
