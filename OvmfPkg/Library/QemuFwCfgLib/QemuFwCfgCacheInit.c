/** @file
  QemuFwCfg cached feature related functions.
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgLib.h>
#include "QemuFwCfgLibInternal.h"
#include <Library/TdxHelperLib.h>
#include <Library/PrintLib.h>

#define EV_POSTCODE_INFO_QEMU_FW_CFG_DATA  "QEMU FW CFG"
#define QEMU_FW_CFG_SIZE                   sizeof (EV_POSTCODE_INFO_QEMU_FW_CFG_DATA)

#pragma pack(1)
typedef struct {
  CHAR8      FileName[QEMU_FW_CFG_FNAME_SIZE];
  BOOLEAN    NeedMeasure;
  UINT16     FwCfgItem;
  UINT32     FwCfgSize;
} CACHE_FW_CFG_STRCUT;

typedef struct {
  UINT8    FwCfg[QEMU_FW_CFG_SIZE];
  UINT8    FwCfgFileName[QEMU_FW_CFG_FNAME_SIZE];
} FW_CFG_EVENT;

#pragma pack()

#define E820_FWCFG_FILE \
        "etc/e820"

#define SYSTEM_STATES_FWCFG_FILE \
        "etc/system-states"

#define EXTRA_PCI_ROOTS_FWCFG_FILE \
        "etc/extra-pci-roots"

#define EXTRA_PCI_ROOTS_FWCFG_SIZE  sizeof (UINT64)

#define BOOT_MENU_WAIT_TIME_FWCFG_FILE \
        "etc/boot-menu-wait"

#define BOOT_MENU_WAIT_TIME_FWCFG_SIZE  sizeof (UINT16)

#define RESERVED_MEMORY_END_FWCFG_FILE \
        "etc/reserved-memory-end"

#define RESERVED_MEMORY_END_FWCFG_SIZE  sizeof (UINT64)

#define PCI_MMIO_FWCFG_FILE \
        "opt/ovmf/X-PciMmio64Mb"

#define BOOTORDER_FWCFG_FILE \
        "bootorder"

#define INVALID_FW_CFG_ITEM  0xFFFF

STATIC CONST CACHE_FW_CFG_STRCUT  mCacheFwCfgList[] = {
  { E820_FWCFG_FILE,                FALSE, INVALID_FW_CFG_ITEM, 0                              },
  { SYSTEM_STATES_FWCFG_FILE,       FALSE, INVALID_FW_CFG_ITEM, 0                              },
  { EXTRA_PCI_ROOTS_FWCFG_FILE,     TRUE,  INVALID_FW_CFG_ITEM, EXTRA_PCI_ROOTS_FWCFG_SIZE     },
  { BOOT_MENU_WAIT_TIME_FWCFG_FILE, TRUE,  INVALID_FW_CFG_ITEM, BOOT_MENU_WAIT_TIME_FWCFG_SIZE },
  { RESERVED_MEMORY_END_FWCFG_FILE, TRUE,  INVALID_FW_CFG_ITEM, RESERVED_MEMORY_END_FWCFG_SIZE },
  { PCI_MMIO_FWCFG_FILE,            TRUE,  INVALID_FW_CFG_ITEM, 0                              },
  { BOOTORDER_FWCFG_FILE,           TRUE,  INVALID_FW_CFG_ITEM, 0                              },
};

#define CACHE_FW_CFG_COUNT  sizeof (mCacheFwCfgList)/sizeof (mCacheFwCfgList[0])

EFI_STATUS
InternalQemuFwCfgInitCache (
  VOID
  )
{
  UINTN                 TotalSize;
  UINT32                Index;
  UINT32                Count;
  UINTN                 FwCfgSize;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  CACHE_FW_CFG_STRCUT   CacheFwCfgList[CACHE_FW_CFG_COUNT];
  FW_CFG_CACHED_ITEM    *CachedItem;
  UINT8                 *ItemData;
  UINT8                 *FwCfhHobData;

  CopyMem (CacheFwCfgList, mCacheFwCfgList, sizeof (mCacheFwCfgList));
  Count     = CACHE_FW_CFG_COUNT;
  TotalSize = sizeof (FW_CFG_CACHE_WORK_AREA);

  for (Index = 0; Index < Count; Index++) {
    if (EFI_ERROR (QemuFwCfgFindFile (CacheFwCfgList[Index].FileName, &FwCfgItem, &FwCfgSize))) {
      continue;
    }

    if (FwCfgSize == 0) {
      continue;
    }

    if ((CacheFwCfgList[Index].FwCfgSize != 0) && (FwCfgSize != CacheFwCfgList[Index].FwCfgSize)) {
      continue;
    }

    CacheFwCfgList[Index].FwCfgItem = (UINT16)FwCfgItem;
    CacheFwCfgList[Index].FwCfgSize = (UINT32)FwCfgSize;
    TotalSize                      += (FwCfgSize + sizeof (FW_CFG_CACHED_ITEM));
  }

  DEBUG ((DEBUG_INFO, "%a: FW_CFG TotalSize is %d (Bytes)\n", __func__, TotalSize));

  if (TotalSize < (sizeof (FW_CFG_CACHED_ITEM) + sizeof (FW_CFG_CACHE_WORK_AREA))) {
    return EFI_ABORTED;
  }

  FwCfhHobData = BuildGuidHob (&gOvmfFwCfgInfoHobGuid, TotalSize);
  if (FwCfhHobData == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: BuildGuidHob Failed with TotalSize(0x%x)\n", __func__, TotalSize));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (FwCfhHobData, TotalSize);

  FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWorkArea;

  FwCfgCacheWorkArea             = (FW_CFG_CACHE_WORK_AREA *)FwCfhHobData;
  FwCfgCacheWorkArea->CacheReady = FALSE;

  CachedItem = (FW_CFG_CACHED_ITEM *)(FwCfhHobData + sizeof (FW_CFG_CACHE_WORK_AREA));

  for (Index = 0; Index < Count; Index++) {
    if (CacheFwCfgList[Index].FwCfgItem == INVALID_FW_CFG_ITEM) {
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: Name: %a Item:0x%x Size: 0x%x \n",
      __func__,
      CacheFwCfgList[Index].FileName,
      CacheFwCfgList[Index].FwCfgItem,
      CacheFwCfgList[Index].FwCfgSize
      ));
    CachedItem->FwCfgItem = CacheFwCfgList[Index].FwCfgItem;
    CachedItem->DataSize  = CacheFwCfgList[Index].FwCfgSize;
    CopyMem (CachedItem->FileName, CacheFwCfgList[Index].FileName, QEMU_FW_CFG_FNAME_SIZE);

    ItemData = (UINT8 *)CachedItem + sizeof (FW_CFG_CACHED_ITEM);

    QemuFwCfgSelectItem (CacheFwCfgList[Index].FwCfgItem);
    QemuFwCfgReadBytes (CacheFwCfgList[Index].FwCfgSize, ItemData);

    if (CacheFwCfgList[Index].NeedMeasure == FALSE) {
      CachedItem = (FW_CFG_CACHED_ITEM *)((UINT8 *)CachedItem +  sizeof (FW_CFG_CACHED_ITEM) + CachedItem->DataSize);
      continue;
    }

    if (TdIsEnabled ()) {
      FW_CFG_EVENT  FwCfgEvent;
      EFI_STATUS    Status;

      ZeroMem (&FwCfgEvent, sizeof (FW_CFG_EVENT));
      CopyMem (&FwCfgEvent.FwCfg, EV_POSTCODE_INFO_QEMU_FW_CFG_DATA, sizeof (EV_POSTCODE_INFO_QEMU_FW_CFG_DATA));
      CopyMem (&FwCfgEvent.FwCfgFileName, CacheFwCfgList[Index].FileName, QEMU_FW_CFG_FNAME_SIZE);

      Status = TdxHelperMeasureFwCfgData (
                 (VOID *)&FwCfgEvent,
                 sizeof (FwCfgEvent),
                 (VOID *)ItemData,
                 CacheFwCfgList[Index].FwCfgSize
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "TdxHelperMeasureFwCfgData failed with %r\n", Status));
      }
    }

    CachedItem = (FW_CFG_CACHED_ITEM *)((UINT8 *)CachedItem +  sizeof (FW_CFG_CACHED_ITEM) + CachedItem->DataSize);
  }

  FwCfgCacheWorkArea->CacheReady = TRUE;
  return EFI_SUCCESS;
}
