/** @file

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "VirtHstiDxe.h"

VOID
VirtHstiQemuCommonInit (
  VIRT_ADAPTER_INFO_PLATFORM_SECURITY  *VirtHsti
  )
{
  VirtHstiSetSupported (VirtHsti, 0, VIRT_HSTI_BYTE0_READONLY_CODE_FLASH);
}

VOID
VirtHstiQemuCommonVerify (
  VOID
  )
{
  CHAR16  *ErrorMsg;

  switch (VirtHstiQemuFirmwareFlashCheck (PcdGet32 (PcdBfvBase))) {
    case QEMU_FIRMWARE_FLASH_WRITABLE:
      ErrorMsg = L"qemu code pflash is writable";
      break;
    default:
      ErrorMsg = NULL;
  }

  VirtHstiTestResult (ErrorMsg, 0, VIRT_HSTI_BYTE0_READONLY_CODE_FLASH);
}
