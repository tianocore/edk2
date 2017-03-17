/** @file
  Decide whether the firmware should expose an ACPI- and/or a Device Tree-based
  hardware description to the operating system.

  Copyright (c) 2017, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Guid/PlatformHasAcpi.h>
#include <Guid/PlatformHasDeviceTree.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
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
