/** @file
 * Cache FW_CFG data in OVMF and measure it in TDVF.
 *
 * Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/
#include <IndustryStandard/E820.h>
#include <Base.h>
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <Library/PeiServicesLib.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>
#include <Library/TdxHelperLib.h>

#define EV_POSTCODE_INFO_QEMU_FW_CFG_DATA  "QEMU FW CFG"
#define QEMU_FW_CFG_SIZE                   sizeof (EV_POSTCODE_INFO_QEMU_FW_CFG_DATA)

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

#pragma pack(1)

typedef struct {
  CHAR8                   FileName[QEMU_FW_CFG_FNAME_SIZE];
  BOOLEAN                 NeedMeasure;
  FIRMWARE_CONFIG_ITEM    FwCfgItem;
  UINTN                   FwCfgSize;
} CACHE_FW_CFG_STRCUT;

typedef struct {
  UINT8    FwCfg[QEMU_FW_CFG_SIZE];
  UINT8    FwCfgFileName[QEMU_FW_CFG_FNAME_SIZE];
} FW_CFG_EVENT;

#pragma pack()

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
CacheFwCfgInfoHobWithOptionalMeasurement (
  IN CHAR8                 *FileName,
  IN BOOLEAN               Measurement,
  IN FIRMWARE_CONFIG_ITEM  Item,
  IN UINTN                 Size,
  OUT UINT8                *Buffer
  )
{
  EFI_STATUS          Status;
  FW_CFG_CACHED_ITEM  *CachedItem;
  UINT8               *ItemData;

  if ((FileName == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  CachedItem            = (FW_CFG_CACHED_ITEM *)Buffer;
  CachedItem->FwCfgItem = Item;
  CachedItem->DataSize  = Size;
  CopyMem (CachedItem->FileName, FileName, QEMU_FW_CFG_FNAME_SIZE);

  ItemData = Buffer + sizeof (FW_CFG_CACHED_ITEM);

  QemuFwCfgSelectItem (Item);
  QemuFwCfgReadBytes (Size, ItemData);

  if (Measurement == FALSE) {
    return Status;
  }

  if (TdIsEnabled ()) {
    FW_CFG_EVENT  FwCfgEvent;
    ZeroMem (&FwCfgEvent, sizeof (FW_CFG_EVENT));
    CopyMem (&FwCfgEvent.FwCfg, EV_POSTCODE_INFO_QEMU_FW_CFG_DATA, sizeof (EV_POSTCODE_INFO_QEMU_FW_CFG_DATA));
    CopyMem (&FwCfgEvent.FwCfgFileName, FileName, QEMU_FW_CFG_FNAME_SIZE);

    Status = TdxHelperMeasureFwCfgData (
                                        (VOID *)&FwCfgEvent,
                                        sizeof (FwCfgEvent),
                                        (VOID *)ItemData,
                                        Size
                                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "TdxHelperMeasureFwCfgData failed with %r\n", Status));
    }
  }

  return Status;
}

/**
 OVMF reads configuration data from QEMU via fw_cfg.
 For Td-Guest VMM is out of TCB and the configuration data is untrusted.
 From the security perpective the configuration data shall be measured
 before it is consumed.

 This function reads the fw_cfg items and cached them. In the meanwhile these
 fw_cfg items are measured as well. This is to avoid changing the order when
 reading the fw_cfg process, which depends on multiple factors(depex, order in
 the Firmware volume).

 @retval  EFI_SUCCESS   - Successfully cache with measurement
 @retval  Others        - As the error code indicates
 */
EFI_STATUS
EFIAPI
PlatformInitFwCfgCachedItems (
  VOID
  )
{
  UINT8       *FwCfhHobData;
  UINTN       TotalSize;
  EFI_STATUS  Status;
  UINT32      Index;
  UINT32      Count;

  UINTN                 FwCfgSize;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  FW_CFG_CACHED_ITEM    *CachedItem;
  CACHE_FW_CFG_STRCUT   CacheFwCfgList[CACHE_FW_CFG_COUNT];

  if (!QemuFwCfgIsAvailable ()) {
    DEBUG ((DEBUG_ERROR, "%a: Qemu Fw_Cfg is not Available! \n", __func__));
    return EFI_UNSUPPORTED;
  }

  CopyMem (CacheFwCfgList, mCacheFwCfgList, sizeof (mCacheFwCfgList));
  Count     = CACHE_FW_CFG_COUNT;
  TotalSize = 0;

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

    CacheFwCfgList[Index].FwCfgItem = FwCfgItem;
    CacheFwCfgList[Index].FwCfgSize = FwCfgSize;
    TotalSize                      += (FwCfgSize + sizeof (FW_CFG_CACHED_ITEM));
  }

  DEBUG ((DEBUG_INFO, "%a: FW_CFG TotalSize is %d (Bytes)\n", __func__, TotalSize));

  FwCfhHobData = BuildGuidHob (&gOvmfFwCfgInfoHobGuid, TotalSize + sizeof (FW_CFG_CACHE_WORK_AREA));
  if (FwCfhHobData == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: BuildGuidHob Failed with TotalSize(0x%x)\n", __func__, TotalSize));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (FwCfhHobData, TotalSize);

  FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWrokArea;

  FwCfgCacheWrokArea             = (FW_CFG_CACHE_WORK_AREA *)FwCfhHobData;
  FwCfgCacheWrokArea->CacheReady = FALSE;

  CachedItem = (FW_CFG_CACHED_ITEM *)(FwCfhHobData + sizeof (FW_CFG_CACHE_WORK_AREA));

  CachedItem = (FW_CFG_CACHED_ITEM *)(FwCfhHobData + sizeof (FW_CFG_CACHE_WORK_AREA));

  for (Index = 0; Index < Count; Index++) {
    if (CacheFwCfgList[Index].FwCfgItem == INVALID_FW_CFG_ITEM) {
      continue;
    }

    DEBUG (
           (
            DEBUG_INFO,
            "%a: Name: %a Item:0x%x Size: 0x%x \n",
            __func__,
            CacheFwCfgList[Index].FileName,
            CacheFwCfgList[Index].FwCfgItem,
            CacheFwCfgList[Index].FwCfgSize
           )
           );

    Status = CacheFwCfgInfoHobWithOptionalMeasurement (
                                                       CacheFwCfgList[Index].FileName,
                                                       CacheFwCfgList[Index].NeedMeasure,
                                                       CacheFwCfgList[Index].FwCfgItem,
                                                       CacheFwCfgList[Index].FwCfgSize,
                                                       (UINT8 *)CachedItem
                                                       );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    CachedItem = (FW_CFG_CACHED_ITEM *)((UINT8 *)CachedItem +  sizeof (FW_CFG_CACHED_ITEM) + CachedItem->DataSize);
  }

  FwCfgCacheWrokArea->CacheReady = TRUE;
  DEBUG ((DEBUG_INFO, "PlatformInitFwCfgInfo Pass!!!\n"));
  return EFI_SUCCESS;
}
