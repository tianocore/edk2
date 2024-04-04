/** @file

  Stateless fw_cfg library implementation.

  Clients must call QemuFwCfgIsAvailable() first.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, Advanced Micro Devices. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgLib.h>
#include <PiPei.h>

#include "QemuFwCfgLibInternal.h"

/**
  Returns a boolean indicating if the firmware configuration interface
  is available or not.

  This function may change fw_cfg state.

  @retval    TRUE   The interface is available
  @retval    FALSE  The interface is not available

**/
BOOLEAN
EFIAPI
QemuFwCfgIsAvailable (
  VOID
  )
{
  UINT32  Signature;
  UINT32  Revision;

  QemuFwCfgSelectItem (QemuFwCfgItemSignature);
  Signature = QemuFwCfgRead32 ();
  DEBUG ((DEBUG_INFO, "FW CFG Signature: 0x%x\n", Signature));
  QemuFwCfgSelectItem (QemuFwCfgItemInterfaceVersion);
  Revision = QemuFwCfgRead32 ();
  DEBUG ((DEBUG_INFO, "FW CFG Revision: 0x%x\n", Revision));
  if ((Signature != SIGNATURE_32 ('Q', 'E', 'M', 'U')) ||
      (Revision < 1)
      )
  {
    DEBUG ((DEBUG_INFO, "QemuFwCfg interface not supported.\n"));
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "QemuFwCfg interface is supported.\n"));
  return TRUE;
}

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
  )
{
  //
  // We always return TRUE, because the consumer of this library ought to have
  // called QemuFwCfgIsAvailable before making other calls which would hit this
  // path.
  //
  return TRUE;
}

/**
  Returns a boolean indicating whether QEMU provides the DMA-like access method
  for fw_cfg.

  @retval    TRUE   The DMA-like access method is available.
  @retval    FALSE  The DMA-like access method is unavailable.
**/
BOOLEAN
InternalQemuFwCfgDmaIsAvailable (
  VOID
  )
{
  return FALSE;
}

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
  )
{
  //
  // We should never reach here
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
}

VOID
InternalQemuFwCfgSelectItem (
  IN     FIRMWARE_CONFIG_ITEM  Item
  )
{
 #ifdef TDX_PEI_LESS_BOOT

  EFI_HOB_GUID_TYPE  *GuidHob;
  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  if (GuidHob == NULL) {
    return;
  }

  FW_CFG_SELECT_INFO  *FwCfgSelectInfo;

  FwCfgSelectInfo = (FW_CFG_SELECT_INFO *)(VOID *)GET_GUID_HOB_DATA (GuidHob);

  if (Item == QemuFwCfgItemFileDir) {
    FwCfgSelectInfo->SkipCache = TRUE;
  } else {
    FwCfgSelectInfo->SkipCache = FALSE;
  }

  FwCfgSelectInfo->FwCfgItem = Item;
  FwCfgSelectInfo->Offset    = 0;
 #else
  DEBUG ((DEBUG_INFO, "%a: It's only support for Pei less startup %r\n", __func__));
  return;
 #endif
}

EFI_STATUS
InternalQemuFwCfgCacheBytes (
  IN     UINTN  Size,
  IN OUT VOID   *Buffer
  )
{
 #ifdef TDX_PEI_LESS_BOOT
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINT8              *FwCfgData;
  UINT16             HobSize;
  EFI_STATUS         Status;

  FW_CFG_SELECT_INFO  *FwCfgSelectInfo;

  if (Size == 0) {
    return EFI_INVALID_PARAMETER;
  }

  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  if (GuidHob == NULL) {
    return RETURN_NOT_READY;
  }

  FwCfgData = (UINT8 *)GET_GUID_HOB_DATA (GuidHob);
  HobSize   = GET_GUID_HOB_DATA_SIZE (GuidHob);

  if (HobSize < (sizeof (FW_CFG_CACHE_INFO) + sizeof (FW_CFG_SELECT_INFO))) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid HobSize. \n", __func__));
    return RETURN_NOT_READY;
  }

  HobSize        -= sizeof (FW_CFG_SELECT_INFO);
  FwCfgSelectInfo = (FW_CFG_SELECT_INFO *)FwCfgData;
  FwCfgData      += sizeof (FW_CFG_SELECT_INFO);

  Status = QemuFwCfgGetBytesFromCache (FwCfgData, HobSize, FwCfgSelectInfo->FwCfgItem, Size, FwCfgSelectInfo->Offset, Buffer);
  if (EFI_ERROR (Status)) {
    return RETURN_NOT_FOUND;
  }

  FwCfgSelectInfo->Offset = Size;

  DEBUG ((DEBUG_INFO, "%a: found in FwCfg Cache\n", __func__));
  return RETURN_SUCCESS;

 #else
  DEBUG ((DEBUG_INFO, "%a: It's only support for Pei less startup %r\n", __func__));
  return RETURN_UNSUPPORTED;
 #endif
}

#include <WorkArea.h>
BOOLEAN
QemuFwCfgCacheEnable (
  VOID
  )
{
 #ifdef TDX_PEI_LESS_BOOT
  TDX_WORK_AREA  *TdxWorkArea;

  TdxWorkArea = (TDX_WORK_AREA *)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaBase);

  if ((TdxWorkArea == NULL) || (TdxWorkArea->SecTdxWorkArea.HobList == 0)) {
    return FALSE;
  }

  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  if (GuidHob == NULL) {
    return FALSE;
  }

  FW_CFG_SELECT_INFO  *FwCfgSelectInfo;

  FwCfgSelectInfo = (FW_CFG_SELECT_INFO *)(VOID *)GET_GUID_HOB_DATA (GuidHob);
  if (FwCfgSelectInfo->CacheReady) {
    return TRUE;
  }

  return FALSE;
 #else
  DEBUG ((DEBUG_INFO, "%a: It's only support for Pei less startup %r\n", __func__));
  return FALSE;
 #endif
}

BOOLEAN
QemuFwCfgSkipCache (
  VOID
  )
{
 #ifdef TDX_PEI_LESS_BOOT
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  if (GuidHob == NULL) {
    return TRUE;
  }

  FW_CFG_SELECT_INFO  *FwCfgSelectInfo;

  FwCfgSelectInfo = (FW_CFG_SELECT_INFO *)(VOID *)GET_GUID_HOB_DATA (GuidHob);
  if (FwCfgSelectInfo->SkipCache) {
    return TRUE;
  }

  return FALSE;
 #else
  DEBUG ((DEBUG_INFO, "%a: It's only support for Pei less startup %r\n", __func__));
  return TRUE;
 #endif
}
