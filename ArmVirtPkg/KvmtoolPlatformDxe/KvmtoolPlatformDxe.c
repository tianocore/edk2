/** @file

  The KvmtoolPlatformDxe performs the platform specific initialization like:
  - It decides if the firmware should expose ACPI or Device Tree-based
    hardware description to the operating system.

  Copyright (c) 2018 - 2023, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/VariableFormat.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FdtClient.h>

/** Decide if the firmware should expose ACPI tables or Device Tree and
    install the appropriate protocol interface.

  Note: This function is derived from "ArmVirtPkg/PlatformHasAcpiDtDxe",
        by dropping the word size check, and the fw_cfg check.

  @param [in]  ImageHandle  Handle for this image.

  @retval EFI_SUCCESS             Success.
  @retval EFI_OUT_OF_RESOURCES    There was not enough memory to install the
                                  protocols.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.

**/
STATIC
EFI_STATUS
PlatformHasAcpiDt (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (!PcdGetBool (PcdForceNoAcpi)) {
    // Expose ACPI tables
    return gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkiiPlatformHasAcpiGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  }

  // Expose the Device Tree.
  return gBS->InstallProtocolInterface (
                &ImageHandle,
                &gEdkiiPlatformHasDeviceTreeGuid,
                EFI_NATIVE_INTERFACE,
                NULL
                );
}

/** Entry point for Kvmtool Platform Dxe

  @param [in]  ImageHandle  Handle for this image.
  @param [in]  SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS             Success.
  @retval EFI_OUT_OF_RESOURCES    There was not enough memory to install the
                                  protocols.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.

**/
EFI_STATUS
EFIAPI
KvmtoolPlatformDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (PcdGetBool (PcdEmuVariableNvModeEnable)) {
    // The driver implementing the variable service can now be dispatched.
    Status = gBS->InstallProtocolInterface (
                    &gImageHandle,
                    &gEdkiiNvVarStoreFormattedGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  Status = PlatformHasAcpiDt (ImageHandle);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
