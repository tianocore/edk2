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
    if (SelectItem == FwCfgCachedItem->FwCfgItem ) {
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
  Clear the QEMU_FW_CFG_CACHE_WORK_AREA.
**/
VOID
InternalQemuFwCfgCacheResetWorkArea (
  VOID
  )
{
  QEMU_FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWorkArea;

  FwCfgCacheWorkArea = InternalQemuFwCfgCacheGetWorkArea ();
  if (FwCfgCacheWorkArea != NULL) {
    FwCfgCacheWorkArea->FwCfgItem = 0;
    FwCfgCacheWorkArea->Offset    = 0;
    FwCfgCacheWorkArea->Reading   = FALSE;
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
  BOOLEAN                      Reading;
  QEMU_FW_CFG_CACHE_WORK_AREA  *QemuFwCfgCacheWorkArea;

  Reading                = FALSE;
  QemuFwCfgCacheWorkArea = InternalQemuFwCfgCacheGetWorkArea ();
  if (QemuFwCfgCacheWorkArea != NULL) {
    Reading = QemuFwCfgCacheWorkArea->Reading;
  }

  return Reading;
}

BOOLEAN
InternalQemuFwCfgCacheSelectItem (
  IN  FIRMWARE_CONFIG_ITEM  Item
  )
{
  QEMU_FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWorkArea;

  // Walk thru cached fw_items to see if Item is cached.
  if (InternalQemuFwCfgItemCached (Item) == NULL) {
    return FALSE;
  }

  FwCfgCacheWorkArea = InternalQemuFwCfgCacheGetWorkArea ();
  if (FwCfgCacheWorkArea == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid FwCfg Cache Work Area\n", __func__));
    return FALSE;
  }

  FwCfgCacheWorkArea->FwCfgItem = (UINT16)Item;
  FwCfgCacheWorkArea->Offset    = 0;
  FwCfgCacheWorkArea->Reading   = TRUE;

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
  QEMU_FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWorkArea;
  FW_CFG_CACHED_ITEM           *CachedItem;
  UINTN                        ReadSize;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FwCfgCacheWorkArea = InternalQemuFwCfgCacheGetWorkArea ();
  if (FwCfgCacheWorkArea == NULL) {
    return RETURN_NOT_FOUND;
  }

  if (!FwCfgCacheWorkArea->Reading) {
    return RETURN_NOT_READY;
  }

  CachedItem = InternalQemuFwCfgItemCached (FwCfgCacheWorkArea->FwCfgItem);
  if (CachedItem == NULL) {
    return RETURN_NOT_FOUND;
  }

  if (FwCfgCacheWorkArea->Offset >= CachedItem->DataSize) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Item Offset(0x%x) in FwCfg Cache\n", __func__, FwCfgCacheWorkArea->Offset));
    return RETURN_ABORTED;
  }

  if (CachedItem->DataSize - FwCfgCacheWorkArea->Offset > Size) {
    ReadSize = Size;
  } else {
    ReadSize = CachedItem->DataSize - FwCfgCacheWorkArea->Offset;
  }

  CopyMem (Buffer, (UINT8 *)CachedItem + sizeof (FW_CFG_CACHED_ITEM) + FwCfgCacheWorkArea->Offset, ReadSize);
  FwCfgCacheWorkArea->Offset += (UINT32)ReadSize;

  DEBUG ((DEBUG_INFO, "%a: found Item 0x%x in FwCfg Cache\n", __func__, FwCfgCacheWorkArea->FwCfgItem));
  return RETURN_SUCCESS;
}

RETURN_STATUS
InternalQemuFwCfgItemInCacheList (
  IN   CONST CHAR8           *Name,
  OUT  FIRMWARE_CONFIG_ITEM  *Item,
  OUT  UINTN                 *Size
  )
{
  UINT16             HobSize;
  UINT16             Offset;
  EFI_HOB_GUID_TYPE  *GuidHob;

 #ifdef TDX_PEI_LESS_BOOT
  if (InternalQemuFwCfgCheckOvmfWorkArea () == FALSE) {
    return RETURN_UNSUPPORTED;
  }

 #endif

  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  if (GuidHob == NULL) {
    return RETURN_UNSUPPORTED;
  }

  HobSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
  Offset  = sizeof (EFI_HOB_GUID_TYPE) + sizeof (FW_CFG_FILE_DIR_HOB_HEADER) + sizeof (FWCFG_FILE);
  if (Offset > HobSize) {
    return RETURN_UNSUPPORTED;
  }

  UINT32                      Count;
  UINT32                      Idx;
  FW_CFG_FILE_DIR_HOB_HEADER  *FwCfgFileDirHobHeader;

  FwCfgFileDirHobHeader = (FW_CFG_FILE_DIR_HOB_HEADER *)(GuidHob + 1);

  if (FwCfgFileDirHobHeader->FwCfgItem != QemuFwCfgItemFileDir) {
    DEBUG ((
      DEBUG_INFO,
      "%a: Expect item: 0x%x but found 0x%x in FwCfg FileDir Hob\n",
      __func__,
      QemuFwCfgItemFileDir,
      FwCfgFileDirHobHeader->FwCfgItem
      ));
    return RETURN_UNSUPPORTED;
  }

  Count = FwCfgFileDirHobHeader->ItemCount;

  FWCFG_FILE  *ItemFile;

  ItemFile = (FWCFG_FILE *)(FwCfgFileDirHobHeader + 1);
  for (Idx = 0; Idx < Count; ++Idx) {
    UINT32  FileSize;
    UINT16  FileSelect;
    CHAR8   FName[QEMU_FW_CFG_FNAME_SIZE];
    FileSize   = ItemFile->Size;
    FileSelect = ItemFile->Select;
    CopyMem (FName, ItemFile->Name, sizeof (FName));
    if (AsciiStrCmp (Name, FName) == 0) {
      *Item = SwapBytes16 (FileSelect);
      *Size = SwapBytes32 (FileSize);
      DEBUG ((DEBUG_INFO, "%a: Found %a(item: 0x%x) in FwCfg FileDir Hob\n", __func__, Name, SwapBytes16 (FileSelect)));
      return RETURN_SUCCESS;
    }

    ItemFile = ItemFile + 1;
  }

  return RETURN_NOT_FOUND;
}
