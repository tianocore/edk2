/** @file
  check for rdrand instruction support via cpuid

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define RDRAND_MASK  BIT30

STATIC
BOOLEAN
RdrandSupported (
  VOID
  )
{
  UINT32  RegEcx;

  AsmCpuid (1, 0, 0, &RegEcx, 0);
  if ((RegEcx & RDRAND_MASK) != RDRAND_MASK) {
    return FALSE;
  }

  return TRUE;
}

EFI_STATUS
EFIAPI
CheckRdrandX64DxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (!RdrandSupported ()) {
    DEBUG ((DEBUG_INFO, "%a: rdrand instruction not available\n", __func__));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "%a: rdrand instruction is supported\n", __func__));
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gOvmfCheckRdrandX64Guid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  return Status;
}
