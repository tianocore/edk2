/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TDX_PLATFORM_LIB_H_
#define TDX_PLATFORM_LIB_H_

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <IndustryStandard/IntelTdx.h>

#define FW_CFG_NX_STACK_ITEM      "opt/ovmf/PcdSetNxForStack"
#define FW_CFG_SYSTEM_STATE_ITEM  "etc/system-states"

/**
 * Perform Platform initialization.
 *
 * @param PlatformInfoHob       Pointer to the PlatformInfo Hob
 * @param CfgSysStateDefault    Indicate if using the default SysState
 * @param CfgNxForStackDefault  Indicate if using the default NxForStack
 * @return VOID
 */
VOID
EFIAPI
TdxPlatformInitialize (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob,
  OUT BOOLEAN                   *CfgSysStateDefault,
  OUT BOOLEAN                   *CfgNxForStackDefault
  );

#endif
