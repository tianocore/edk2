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

STATIC VIRT_ADAPTER_INFO_PLATFORM_SECURITY  mHstiQ35 = {
  PLATFORM_SECURITY_VERSION_VNEXTCS,
  PLATFORM_SECURITY_ROLE_PLATFORM_REFERENCE,
  { L"OVMF (Qemu Q35)" },
  VIRT_HSTI_SECURITY_FEATURE_SIZE,
};

VIRT_ADAPTER_INFO_PLATFORM_SECURITY *
VirtHstiQemuQ35Init (
  VOID
  )
{
  if (FeaturePcdGet (PcdSmmSmramRequire)) {
    VirtHstiSetSupported (&mHstiQ35, 0, VIRT_HSTI_BYTE0_SMM_SMRAM_LOCK);
    VirtHstiSetSupported (&mHstiQ35, 0, VIRT_HSTI_BYTE0_SMM_SECURE_VARS_FLASH);
  }

  return &mHstiQ35;
}

VOID
VirtHstiQemuQ35Verify (
  VOID
  )
{
  if (VirtHstiIsSupported (&mHstiQ35, 0, VIRT_HSTI_BYTE0_SMM_SMRAM_LOCK)) {
    CHAR16  *ErrorMsg = NULL;
    UINT8   SmramVal;
    UINT8   EsmramcVal;

    SmramVal   = PciRead8 (DRAMC_REGISTER_Q35 (MCH_SMRAM));
    EsmramcVal = PciRead8 (DRAMC_REGISTER_Q35 (MCH_ESMRAMC));

    if (!(EsmramcVal & MCH_ESMRAMC_T_EN)) {
      ErrorMsg = L"q35 smram access is open";
    } else if (!(SmramVal & MCH_SMRAM_D_LCK)) {
      ErrorMsg = L"q35 smram config is not locked";
    }

    VirtHstiTestResult (ErrorMsg, 0, VIRT_HSTI_BYTE0_SMM_SMRAM_LOCK);
  }

  if (VirtHstiIsSupported (&mHstiQ35, 0, VIRT_HSTI_BYTE0_SMM_SECURE_VARS_FLASH)) {
    CHAR16  *ErrorMsg = NULL;

    switch (VirtHstiQemuFirmwareFlashCheck (PcdGet32 (PcdOvmfFlashNvStorageVariableBase))) {
      case QEMU_FIRMWARE_FLASH_WRITABLE:
        ErrorMsg = L"qemu vars pflash is not secure";
        break;
    }

    VirtHstiTestResult (ErrorMsg, 0, VIRT_HSTI_BYTE0_SMM_SECURE_VARS_FLASH);
  }
}
