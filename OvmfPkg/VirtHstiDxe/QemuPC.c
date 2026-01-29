/** @file

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HstiLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>

#include <IndustryStandard/Hsti.h>
#include <IndustryStandard/Q35MchIch9.h>

#include "VirtHstiDxe.h"

STATIC VIRT_ADAPTER_INFO_PLATFORM_SECURITY  mHstiPC = {
  PLATFORM_SECURITY_VERSION_VNEXTCS,
  PLATFORM_SECURITY_ROLE_PLATFORM_REFERENCE,
  { L"OVMF (Qemu PC)" },
  VIRT_HSTI_SECURITY_FEATURE_SIZE,
};

VIRT_ADAPTER_INFO_PLATFORM_SECURITY *
VirtHstiQemuPCInit (
  VOID
  )
{
  return &mHstiPC;
}

VOID
VirtHstiQemuPCVerify (
  VOID
  )
{
}
