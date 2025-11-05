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
#include <Library/TdxHelperLib.h>
#include <Library/PrintLib.h>
#include <Library/TpmMeasurementLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>

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

#define QEMU_FW_CFG_SIGNATURE \
        "QemuFwCfgSignature"

#define QEMU_FW_CFG_SIGNATURE_SIZE  sizeof (UINT32)

#define QEMU_FW_CFG_INTERFACE_VERSION \
        "QemuFwCfgInterfaceVersion"

#define QEMU_FW_CFG_INTERFACE_VERSION_SIZE  sizeof (UINT32)

#define QEMU_FW_CFG_FILE_DIR \
        "QemuFwCfgFileDri"

#define E820_FWCFG_FILE \
        "etc/e820"

#define SYSTEM_STATES_FWCFG_FILE \
        "etc/system-states"

#define EXTRA_PCI_ROOTS_FWCFG_FILE \
        "etc/extra-pci-roots"

#define EXTRA_PCI_ROOTS_FWCFG_SIZE  sizeof (UINT64)

#define BOOT_MENU_FWCFG_NAME  "BootMenu"

#define BOOT_MENU_FWCFG_SIZE  sizeof (UINT16)

#define BOOT_MENU_WAIT_TIME_FWCFG_FILE \
        "etc/boot-menu-wait"

#define BOOT_MENU_WAIT_TIME_FWCFG_SIZE  sizeof (UINT16)

#define RESERVED_MEMORY_END_FWCFG_FILE \
        "etc/reserved-memory-end"

#define RESERVED_MEMORY_END_FWCFG_SIZE  sizeof (UINT64)

#define PCI_MMIO64_FWCFG_FILE \
        "opt/ovmf/X-PciMmio64Mb"

#define BOOTORDER_FWCFG_FILE \
        "bootorder"

#define INVALID_FW_CFG_ITEM  0xFFFF

STATIC CONST CACHE_FW_CFG_STRCUT  mCacheFwCfgList[] = {
  { QEMU_FW_CFG_SIGNATURE,          FALSE, QemuFwCfgItemSignature,        QEMU_FW_CFG_SIGNATURE_SIZE         },
  { QEMU_FW_CFG_INTERFACE_VERSION,  FALSE, QemuFwCfgItemInterfaceVersion, QEMU_FW_CFG_INTERFACE_VERSION_SIZE },
  { QEMU_FW_CFG_FILE_DIR,           FALSE, QemuFwCfgItemFileDir,          0                                  },
  { E820_FWCFG_FILE,                FALSE, INVALID_FW_CFG_ITEM,           0                                  },
  { SYSTEM_STATES_FWCFG_FILE,       FALSE, INVALID_FW_CFG_ITEM,           0                                  },
  { EXTRA_PCI_ROOTS_FWCFG_FILE,     TRUE,  INVALID_FW_CFG_ITEM,           EXTRA_PCI_ROOTS_FWCFG_SIZE         },
  { BOOT_MENU_FWCFG_NAME,           TRUE,  QemuFwCfgItemBootMenu,         BOOT_MENU_FWCFG_SIZE               },
  { BOOT_MENU_WAIT_TIME_FWCFG_FILE, TRUE,  INVALID_FW_CFG_ITEM,           BOOT_MENU_WAIT_TIME_FWCFG_SIZE     },
  { RESERVED_MEMORY_END_FWCFG_FILE, TRUE,  INVALID_FW_CFG_ITEM,           RESERVED_MEMORY_END_FWCFG_SIZE     },
  { PCI_MMIO64_FWCFG_FILE,          TRUE,  INVALID_FW_CFG_ITEM,           0                                  },
  { BOOTORDER_FWCFG_FILE,           TRUE,  INVALID_FW_CFG_ITEM,           0                                  },
};

#define CACHE_FW_CFG_COUNT  sizeof (mCacheFwCfgList)/sizeof (mCacheFwCfgList[0])

STATIC
UINT32
CalcuateQemuFwCfgFileDirSize (
  IN FIRMWARE_CONFIG_ITEM  FwCfgItem
  )
{
  UINT32  FileCount;
  UINT32  FileDirSize;

  QemuFwCfgSelectItem (FwCfgItem);
  FileCount   = SwapBytes32 (QemuFwCfgRead32 ());
  FileDirSize = FileCount * sizeof (FWCFG_FILE) + sizeof (UINT32);
  return FileDirSize;
}

STATIC
EFI_STATUS
ConstructCacheFwCfgList (
  IN OUT CACHE_FW_CFG_STRCUT  *CacheFwCfgList
  )
{
  UINT32                Index;
  UINT32                Count;
  UINTN                 FwCfgSize;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;

  if (CacheFwCfgList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (CacheFwCfgList, mCacheFwCfgList, sizeof (mCacheFwCfgList));
  Count = CACHE_FW_CFG_COUNT;

  for (Index = 0; Index < Count; Index++) {
    if (CacheFwCfgList[Index].FwCfgItem == QemuFwCfgItemFileDir) {
      CacheFwCfgList[Index].FwCfgSize = CalcuateQemuFwCfgFileDirSize (QemuFwCfgItemFileDir);
      continue;
    }

    if (CacheFwCfgList[Index].FwCfgItem != INVALID_FW_CFG_ITEM) {
      continue;
    }

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
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CacheFwCfgInfoWithOptionalMeasurment (
  IN CACHE_FW_CFG_STRCUT  *CacheFwCfgList
  )
{
  UINT32              Index;
  UINT32              Count;
  UINT8               *FwCfginfoHobData;
  FW_CFG_CACHED_ITEM  *CachedItem;
  UINT8               *ItemData;
  UINT32              FwCfgItemHobSize;

  Count = CACHE_FW_CFG_COUNT;

  for (Index = 0; Index < Count; Index++) {
    if ((CacheFwCfgList[Index].FwCfgItem == INVALID_FW_CFG_ITEM) || (CacheFwCfgList[Index].FwCfgSize == 0)) {
      continue;
    }

    FwCfginfoHobData = NULL;
    FwCfgItemHobSize = sizeof (FW_CFG_CACHED_ITEM) + CacheFwCfgList[Index].FwCfgSize;
    FwCfginfoHobData = BuildGuidHob (&gOvmfFwCfgInfoHobGuid, FwCfgItemHobSize);
    if (FwCfginfoHobData == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: BuildGuidHob Failed with FwCfgItemHobSize(0x%x)\n", __func__, FwCfgItemHobSize));
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (FwCfginfoHobData, FwCfgItemHobSize);
    CachedItem = (FW_CFG_CACHED_ITEM *)FwCfginfoHobData;
    ItemData   = (UINT8 *)CachedItem + sizeof (FW_CFG_CACHED_ITEM);

    QemuFwCfgSelectItem (CacheFwCfgList[Index].FwCfgItem);
    QemuFwCfgReadBytes (CacheFwCfgList[Index].FwCfgSize, ItemData);

    CachedItem->FwCfgItem = CacheFwCfgList[Index].FwCfgItem;
    CachedItem->DataSize  = CacheFwCfgList[Index].FwCfgSize;
    DEBUG ((
      DEBUG_INFO,
      "Cache FwCfg Name: %a Item:0x%x Size: 0x%x \n",
      CacheFwCfgList[Index].FileName,
      CachedItem->FwCfgItem,
      CachedItem->DataSize
      ));

    if (CacheFwCfgList[Index].NeedMeasure == FALSE) {
      continue;
    }

    if (TdIsEnabled ()) {
      FW_CFG_EVENT  FwCfgEvent;
      EFI_STATUS    Status;

      ZeroMem (&FwCfgEvent, sizeof (FW_CFG_EVENT));
      CopyMem (&FwCfgEvent.FwCfg, EV_POSTCODE_INFO_QEMU_FW_CFG_DATA, sizeof (EV_POSTCODE_INFO_QEMU_FW_CFG_DATA));
      CopyMem (&FwCfgEvent.FwCfgFileName, CacheFwCfgList[Index].FileName, QEMU_FW_CFG_FNAME_SIZE);

      Status = TpmMeasureAndLogData (
                 1,
                 EV_PLATFORM_CONFIG_FLAGS,
                 (VOID *)&FwCfgEvent,
                 sizeof (FwCfgEvent),
                 (VOID *)ItemData,
                 CacheFwCfgList[Index].FwCfgSize
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "TpmMeasureAndLogData failed with %r\n", Status));
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InternalQemuFwCfgInitCache (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  if (PlatformInfoHob == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PlatformInfoHob->QemuFwCfgWorkArea.FwCfgItem = INVALID_FW_CFG_ITEM;
  PlatformInfoHob->QemuFwCfgWorkArea.Offset    = 0;
  PlatformInfoHob->QemuFwCfgWorkArea.Reading   = FALSE;

  if (!QemuFwCfgIsAvailable ()) {
    return EFI_UNSUPPORTED;
  }

  CACHE_FW_CFG_STRCUT  CacheFwCfgList[CACHE_FW_CFG_COUNT];

  if (EFI_ERROR (ConstructCacheFwCfgList (CacheFwCfgList))) {
    return EFI_ABORTED;
  }

  if (EFI_ERROR (CacheFwCfgInfoWithOptionalMeasurment (CacheFwCfgList))) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}
