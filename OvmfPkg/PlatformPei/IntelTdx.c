/** @file
  Initialize Intel TDX support.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <IndustryStandard/QemuFwCfg.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/TdxLib.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>
#include "Platform.h"

/**
  This Function checks if TDX is available, if present then it sets
  the dynamic PcdTdxIsEnabled and PcdIa32EferChangeAllowed.

  It relocates the td mailbox and create the PlatformInfo Hob which includes
  the TDX specific information which will be consumed in DXE phase.
  **/
VOID
IntelTdxInitialize (
  VOID
  )
{
#ifdef MDE_CPU_X64
  EFI_HOB_PLATFORM_INFO  PlatformInfoHob;
  RETURN_STATUS          PcdStatus;

  if (!TdIsEnabled ()) {
    return;
  }

  PcdStatus = PcdSet64S (PcdConfidentialComputingGuestAttr, CCAttrIntelTdx);
  ASSERT_RETURN_ERROR (PcdStatus);

  PcdStatus = PcdSetBoolS (PcdIa32EferChangeAllowed, FALSE);
  ASSERT_RETURN_ERROR (PcdStatus);

  PcdStatus = PcdSet64S (PcdTdxSharedBitMask, TdSharedPageMask ());
  ASSERT_RETURN_ERROR (PcdStatus);

  PcdStatus = PcdSetBoolS (PcdSetNxForStack, TRUE);
  ASSERT_RETURN_ERROR (PcdStatus);

  ZeroMem (&PlatformInfoHob, sizeof (PlatformInfoHob));
  PlatformInfoHob.HostBridgePciDevId = mHostBridgeDevId;

  BuildGuidDataHob (&gUefiOvmfPkgTdxPlatformGuid, &PlatformInfoHob, sizeof (EFI_HOB_PLATFORM_INFO));
#endif
}
