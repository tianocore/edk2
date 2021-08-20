/** @file
  Decide whether the firmware should expose an ACPI- and/or a Device Tree-based
  hardware description to the operating system.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/PlatformHasAcpi.h>
#include <Guid/PlatformHasDeviceTree.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

/** Entry point for the Cloud Hypervisor PlatformHasAcpiDtDxe.

  @param [in]  ImageHandle  Handle for this image.
  @param [in]  SystemTable  Pointer to the EFI system table.

  @return EFI_SUCCESS             If ACPI or Device Tree based hardware
                                  description protocol was installed.
  @return EFI_INVALID_PARAMETER   A parameter was invalid.
  @return EFI_OUT_OF_RESOURCES    Insufficient resources exist to complete
                                  the request.
**/
EFI_STATUS
EFIAPI
PlatformHasAcpiDt (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS           Status;

  //
  // If we fail to install any of the necessary protocols below, the OS will be
  // unbootable anyway (due to lacking hardware description), so tolerate no
  // errors here.
  //
  if (MAX_UINTN == MAX_UINT64 &&
      !PcdGetBool (PcdForceNoAcpi)) {
    Status = gBS->InstallProtocolInterface (
                    &ImageHandle,
                    &gEdkiiPlatformHasAcpiGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto Failed;
    }

    return Status;
  }

  //
  // Expose the Device Tree otherwise.
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkiiPlatformHasDeviceTreeGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  return Status;

Failed:
  ASSERT_EFI_ERROR (Status);
  CpuDeadLoop ();
  //
  // Keep compilers happy.
  //
  return Status;
}
