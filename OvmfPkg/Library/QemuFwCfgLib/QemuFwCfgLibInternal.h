/** @file
  Internal interfaces specific to the QemuFwCfgLib instances in OvmfPkg.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (C) 2017, Advanced Micro Devices. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __QEMU_FW_CFG_LIB_INTERNAL_H__
#define __QEMU_FW_CFG_LIB_INTERNAL_H__

#include <Base.h>
#include <Uefi/UefiMultiPhase.h>
#include <Uefi/UefiBaseType.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

#pragma pack(1)
typedef struct {
  UINT16    FwCfgItem;
  UINT32    DataSize;
  // UINT8  *data
} FW_CFG_CACHED_ITEM;

// Refer to https://www.qemu.org/docs/master/specs/fw_cfg.html
// The FwCfg File item struct in QemuFwCfgItemFileDir
typedef struct {
  UINT32    Size;       /* size of referenced fw_cfg item, big-endian */
  UINT16    Select;     /* selector key of fw_cfg item, big-endian */
  UINT16    Reserved;
  UINT8     Name[QEMU_FW_CFG_FNAME_SIZE];
} FWCFG_FILE;
#pragma pack()

/**
  Returns a boolean indicating if the firmware configuration interface is
  available for library-internal purposes.

  This function never changes fw_cfg state.

  @retval    TRUE   The interface is available internally.
  @retval    FALSE  The interface is not available internally.
**/
BOOLEAN
InternalQemuFwCfgIsAvailable (
  VOID
  );

/**
  Returns a boolean indicating whether QEMU provides the DMA-like access method
  for fw_cfg.

  @retval    TRUE   The DMA-like access method is available.
  @retval    FALSE  The DMA-like access method is unavailable.
**/
BOOLEAN
InternalQemuFwCfgDmaIsAvailable (
  VOID
  );

/**
  Transfer an array of bytes, or skip a number of bytes, using the DMA
  interface.

  @param[in]     Size     Size in bytes to transfer or skip.

  @param[in,out] Buffer   Buffer to read data into or write data from. Ignored,
                          and may be NULL, if Size is zero, or Control is
                          FW_CFG_DMA_CTL_SKIP.

  @param[in]     Control  One of the following:
                          FW_CFG_DMA_CTL_WRITE - write to fw_cfg from Buffer.
                          FW_CFG_DMA_CTL_READ  - read from fw_cfg into Buffer.
                          FW_CFG_DMA_CTL_SKIP  - skip bytes in fw_cfg.
**/
VOID
InternalQemuFwCfgDmaBytes (
  IN     UINT32  Size,
  IN OUT VOID    *Buffer OPTIONAL,
  IN     UINT32  Control
  );

/**
  Check if it is Tdx guest

  @retval    TRUE   It is Tdx guest
  @retval    FALSE  It is not Tdx guest
**/
BOOLEAN
QemuFwCfgIsTdxGuest (
  VOID
  );

/**
  Check if the Ovmf work area is built as HobList before invoking Hob services.

  @retval    TRUE   Ovmf work area is not NULL and it is built as HobList.
  @retval    FALSE   Ovmf work area is NULL or it is not built as HobList.
**/
BOOLEAN
InternalQemuFwCfgCheckOvmfWorkArea (
  VOID
  );

/**
  Read the fw_cfg data from Cache.
  @retval  EFI_SUCCESS   - Successfully
  @retval  Others        - As the error code indicates
**/
EFI_STATUS
InternalQemuFwCfgCacheReadBytes (
  IN     UINTN  Size,
  IN OUT VOID   *Buffer
  );

/**
  Select the fw_cfg item for reading from cache. If the fw_cfg item
  is not cached, then it returns FALSE.
  @param[in]    Item    The fw_cfg item to be selected
  @retval       TRUE    The fw_cfg item is selected.
  @retval       FALSE   The fw_cfg item is not selected.
**/
BOOLEAN
InternalQemuFwCfgCacheSelectItem (
  IN  FIRMWARE_CONFIG_ITEM  Item
  );

/**
  Get the pointer to the QEMU_FW_CFG_WORK_AREA. This data is used as the
  workarea to record the ongoing fw_cfg item and offset.
  @retval   QEMU_FW_CFG_WORK_AREA  Pointer to the QEMU_FW_CFG_WORK_AREA
  @retval   NULL                QEMU_FW_CFG_WORK_AREA doesn't exist
**/
QEMU_FW_CFG_WORK_AREA *
InternalQemuFwCfgCacheGetWorkArea (
  VOID
  );

/**
  Clear the QEMU_FW_CFG_WORK_AREA.
**/
VOID
InternalQemuFwCfgCacheResetWorkArea (
  VOID
  );

/**
  Get the pointer to the cached fw_cfg item.
  @param[in] Item   The fw_cfg item to be retrieved.
  @retval    FW_CFG_CACHED_ITEM   Pointer to the cached fw_cfg item.
  @retval    NULL                The fw_cfg item is not cached.
**/
FW_CFG_CACHED_ITEM *
InternalQemuFwCfgItemCached (
  IN FIRMWARE_CONFIG_ITEM  Item
  );

/**
  Check if reading from FwCfgCache is ongoing.
  @retval   TRUE  Reading from FwCfgCache is ongoing.
  @retval   FALSE Reading from FwCfgCache is not ongoing.
**/
BOOLEAN
InternalQemuFwCfgCacheReading (
  VOID
  );

/**
  init the fw_cfg info hob with optional measurement
**/
EFI_STATUS
InternalQemuFwCfgInitCache (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

#endif
