/** @file

  The QemuPlatformDxe performs platform specific initialization.

  Copyright (c) 2018 - 2023, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/VariableFormat.h>
#include <Library/ArmCcaLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FdtClient.h>

/** Entrypoint for QemuPlatformDxeEntryPoint

  @param [in] ImageHandle  The image handle.
  @param [in] SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
QemuPlatformDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  INT32                Node;
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = FdtClient->FindCompatibleNode (
                        FdtClient,
                        "cfi-flash",
                        &Node
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "No Flash device found, falling back to Runtime Variable Emulation\n"
      ));

    Status = PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Failed to set PcdEmuVariableNvModeEnable, Status = %r\n",
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // The driver implementing the variable service can now be dispatched.
    Status = gBS->InstallProtocolInterface (
                    &gImageHandle,
                    &gEdkiiNvVarStoreFormattedGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  } else if (ArmCcaIsRealm ()) {
    // In a Confidential VM the CFI flash emulated by the VMM is outside
    // the trust boundary of the Realm. The VMM must not expose a CFI
    // flash device to a CCA Realm VM, as this is an invalid configuration.
    Status = EFI_SECURITY_VIOLATION;
    DEBUG ((
      DEBUG_ERROR,
      "Error: A Realm VM must not have a CFI Flash device, Status = %r.\n",
      Status
      ));
    // We must not proceed with the Realm Boot.
    CpuDeadLoop ();
  }

  return Status;
}
