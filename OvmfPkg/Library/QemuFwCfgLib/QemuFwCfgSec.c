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
#include <Library/PcdLib.h>
#include <Library/QemuFwCfgLib.h>
#include <PiPei.h>
#include <WorkArea.h>

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

/**
  Get the pointer to the FW_CFG_CACHE_WORK_AREA. This data is used as the
  workarea to record the onging fw_cfg item and offset.

  @retval   FW_CFG_CACHE_WORK_AREA  Pointer to the FW_CFG_CACHE_WORK_AREA
  @retval   NULL                FW_CFG_CACHE_WORK_AREA doesn't exist
**/
FW_CFG_CACHE_WORK_AREA *
InternalQemuFwCfgCacheGetWorkArea (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  if (!InternalQemuFwCfgCacheEnable ()) {
    return NULL;
  }

  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);

  return (FW_CFG_CACHE_WORK_AREA *)(VOID *)GET_GUID_HOB_DATA (GuidHob);
}

/**
  Check if fw_cfg cache is ready.

  @retval    TRUE   Cache is ready
  @retval    FALSE  Cache is not ready
**/
BOOLEAN
InternalQemuFwCfgCacheEnable (
  VOID
  )
{
  EFI_HOB_GUID_TYPE       *GuidHob;
  FW_CFG_CACHE_WORK_AREA  *FwCfgCacheWorkArea;
  TDX_WORK_AREA           *TdxWorkArea;

  // InternalQemuFwCfgCacheEnable in QemuFwCfgSecLib might be called in a very early
  // stage (at that moment HobList may not be set). So an additional check
  // to the HobList is needed.
  TdxWorkArea = (TDX_WORK_AREA *)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaBase);
  if (TdxWorkArea == NULL) {
    return FALSE;
  }

  if (TdxWorkArea->SecTdxWorkArea.HobList == 0) {
    return FALSE;
  }

  GuidHob = GetFirstGuidHob (&gOvmfFwCfgInfoHobGuid);
  if (GuidHob == NULL) {
    return FALSE;
  }

  FwCfgCacheWorkArea = (FW_CFG_CACHE_WORK_AREA *)GET_GUID_HOB_DATA (GuidHob);
  return FwCfgCacheWorkArea->CacheReady;
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

  @retval  RETURN_SUCCESS   - Successfully cache with measurement
  @retval  Others           - As the error code indicates
 */
RETURN_STATUS
EFIAPI
QemuFwCfgInitCache (
  VOID
  )
{
 #ifdef TDX_PEI_LESS_BOOT

  if (!QemuFwCfgIsAvailable ()) {
    DEBUG ((DEBUG_ERROR, "%a: Qemu Fw_Cfg is not Available! \n", __func__));
    return RETURN_UNSUPPORTED;
  }

  TDX_WORK_AREA  *TdxWorkArea;

  // InternalQemuFwCfgCacheEnable in QemuFwCfgSecLib might be called in a very early
  // stage (at that moment HobList may not be set). So an additional check
  // to the HobList is needed.
  TdxWorkArea = (TDX_WORK_AREA *)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaBase);
  if ((TdxWorkArea == NULL) || (TdxWorkArea->SecTdxWorkArea.HobList == 0)) {
    return RETURN_UNSUPPORTED;
  }

  if (EFI_ERROR (InternalQemuFwCfgInitCache ())) {
    return RETURN_ABORTED;
  }

  DEBUG ((DEBUG_INFO, "QemuFwCfgInitCache Pass!!!\n"));
  return RETURN_SUCCESS;
 #endif
}
