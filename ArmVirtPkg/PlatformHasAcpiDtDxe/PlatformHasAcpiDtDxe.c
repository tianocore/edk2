/** @file
  Decide whether the firmware should expose an ACPI- and/or a Device Tree-based
  hardware description to the operating system.

  Copyright (c) 2017, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/PlatformHasAcpi.h>
#include <Guid/PlatformHasDeviceTree.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
PlatformHasAcpiDt (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;

  //
  // If we fail to install any of the necessary protocols below, the OS will be
  // unbootable anyway (due to lacking hardware description), so tolerate no
  // errors here.
  //
  if (MAX_UINTN == MAX_UINT64 &&
      !PcdGetBool (PcdForceNoAcpi) &&
      !EFI_ERROR (
         QemuFwCfgFindFile (
           "etc/table-loader",
           &FwCfgItem,
           &FwCfgSize
           )
         )) {
    //
    // Only make ACPI available on 64-bit systems, and only if QEMU generates
    // (a subset of) the ACPI tables.
    //
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
