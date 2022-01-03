/** @file
   Enroll default PK, KEK, DB and DBX

   Copyright (C) 2014, Red Hat, Inc.

   This program and the accompanying materials are licensed and made available
   under the terms and conditions of the BSD License which accompanies this
   distribution. The full text of the license may be found at
   http://opensource.org/licenses/bsd-license.

   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
   WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 **/

#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/SecureBootVariableProvisionLib.h>
#include <Library/SecureBootVariableLib.h>

STATIC
EFI_STATUS
EFIAPI
GetExact (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  OUT VOID     *Data,
  IN UINTN     DataSize,
  IN BOOLEAN   AllowMissing
  )
{
  UINTN       Size;
  EFI_STATUS  Status;

  Size   = DataSize;
  Status = gRT->GetVariable (VariableName, VendorGuid, NULL, &Size, Data);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) && AllowMissing) {
      ZeroMem (Data, DataSize);
      return EFI_SUCCESS;
    }

    DEBUG ((
      EFI_D_ERROR,
      "SecureBootSetup: GetVariable(\"%s\", %g): %r\n",
      VariableName,
      VendorGuid,
      Status
      ));
    return Status;
  }

  if (Size != DataSize) {
    DEBUG ((
      EFI_D_INFO,
      "SecureBootSetup: GetVariable(\"%s\", %g): expected size 0x%Lx, "
      "got 0x%Lx\n",
      VariableName,
      VendorGuid,
      (UINT64)DataSize,
      (UINT64)Size
      ));
    return EFI_PROTOCOL_ERROR;
  }

  return EFI_SUCCESS;
}

typedef struct {
  UINT8    SetupMode;
  UINT8    SecureBoot;
  UINT8    SecureBootEnable;
  UINT8    CustomMode;
  UINT8    VendorKeys;
} SETTINGS;

STATIC
EFI_STATUS
EFIAPI
GetSettings (
  OUT SETTINGS  *Settings,
  BOOLEAN       AllowMissing
  )
{
  EFI_STATUS  Status;

  ZeroMem (Settings, sizeof (SETTINGS));

  Status = GetExact (
             EFI_SETUP_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &Settings->SetupMode,
             sizeof Settings->SetupMode,
             AllowMissing
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (
             EFI_SECURE_BOOT_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &Settings->SecureBoot,
             sizeof Settings->SecureBoot,
             AllowMissing
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (
             EFI_SECURE_BOOT_ENABLE_NAME,
             &gEfiSecureBootEnableDisableGuid,
             &Settings->SecureBootEnable,
             sizeof Settings->SecureBootEnable,
             AllowMissing
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (
             EFI_CUSTOM_MODE_NAME,
             &gEfiCustomModeEnableGuid,
             &Settings->CustomMode,
             sizeof Settings->CustomMode,
             AllowMissing
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (
             EFI_VENDOR_KEYS_VARIABLE_NAME,
             &gEfiGlobalVariableGuid,
             &Settings->VendorKeys,
             sizeof Settings->VendorKeys,
             AllowMissing
             );
  return Status;
}

STATIC
VOID
EFIAPI
PrintSettings (
  IN CONST SETTINGS  *Settings
  )
{
  DEBUG ((
    EFI_D_INFO,
    "SecureBootSetup: SetupMode=%d SecureBoot=%d SecureBootEnable=%d "
    "CustomMode=%d VendorKeys=%d\n",
    Settings->SetupMode,
    Settings->SecureBoot,
    Settings->SecureBootEnable,
    Settings->CustomMode,
    Settings->VendorKeys
    ));
}

/**
  Install SecureBoot certificates once the VariableDriver is running.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
InstallSecureBootHook (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Protocol;
  SETTINGS    Settings;

  Status = gBS->LocateProtocol (&gEfiVariableWriteArchProtocolGuid, NULL, (VOID **)&Protocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = GetSettings (&Settings, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: Failed to get current settings\n"));
    return;
  }

  if (Settings.SetupMode != SETUP_MODE) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: already in User Mode\n"));
    return;
  }

  if (Settings.SecureBootEnable != SECURE_BOOT_MODE_ENABLE) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: SecureBootEnable is disabled.\n"));
    return;
  }

  PrintSettings (&Settings);

  if (Settings.CustomMode != CUSTOM_SECURE_BOOT_MODE) {
    Settings.CustomMode = CUSTOM_SECURE_BOOT_MODE;
    Status              = gRT->SetVariable (
                                 EFI_CUSTOM_MODE_NAME,
                                 &gEfiCustomModeEnableGuid,
                                 (EFI_VARIABLE_NON_VOLATILE |
                                  EFI_VARIABLE_BOOTSERVICE_ACCESS),
                                 sizeof Settings.CustomMode,
                                 &Settings.CustomMode
                                 );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_ERROR,
        "SecureBootSetup: SetVariable(\"%s\", %g): %r\n",
        EFI_CUSTOM_MODE_NAME,
        &gEfiCustomModeEnableGuid,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
    }
  }

  // Enroll all the keys from default variables
  Status = EnrollDbFromDefault ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot enroll db: %r\n", Status));
    goto error;
  }

  Status = EnrollDbxFromDefault ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot enroll dbx: %r\n", Status));
  }

  Status = EnrollDbtFromDefault ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot enroll dbt: %r\n", Status));
  }

  Status = EnrollKEKFromDefault ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot enroll KEK: %r\n", Status));
    goto cleardbs;
  }

  Status = EnrollPKFromDefault ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot enroll PK: %r\n", Status));
    goto clearKEK;
  }

  Status = SetSecureBootMode (STANDARD_SECURE_BOOT_MODE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Cannot set CustomMode to STANDARD_SECURE_BOOT_MODE\n"
      "Please do it manually, otherwise system can be easily compromised\n"
      ));
  }

  // FIXME: Force SecureBoot to ON. The AuthService will do this if authenticated variables
  // are supported, which aren't as the SMM handler isn't able to verify them.

  Settings.SecureBootEnable = SECURE_BOOT_ENABLE;
  Status                    = gRT->SetVariable (
                                     EFI_SECURE_BOOT_ENABLE_NAME,
                                     &gEfiSecureBootEnableDisableGuid,
                                     EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                                     sizeof Settings.SecureBootEnable,
                                     &Settings.SecureBootEnable
                                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      EFI_D_ERROR,
      "SecureBootSetup: SetVariable(\"%s\", %g): %r\n",
      EFI_SECURE_BOOT_ENABLE_NAME,
      &gEfiSecureBootEnableDisableGuid,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
  }

  Settings.SecureBoot = SECURE_BOOT_ENABLE;
  Status              = gRT->SetVariable (
                               EFI_SECURE_BOOT_MODE_NAME,
                               &gEfiGlobalVariableGuid,
                               EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                               sizeof Settings.SecureBoot,
                               &Settings.SecureBoot
                               );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      EFI_D_ERROR,
      "SecureBootSetup: SetVariable(\"%s\", %g): %r\n",
      EFI_SECURE_BOOT_MODE_NAME,
      &gEfiGlobalVariableGuid,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
  }

  Status = GetSettings (&Settings, FALSE);
  ASSERT_EFI_ERROR (Status);

  //
  // Final sanity check:
  //
  //                                 [SetupMode]
  //                        (read-only, standardized by UEFI)
  //                                /                \_
  //                               0               1, default
  //                              /                    \_
  //                      PK enrolled                   no PK enrolled yet,
  //              (this is called "User Mode")          PK enrollment possible
  //                             |
  //                             |
  //                     [SecureBootEnable]
  //         (read-write, edk2-specific, boot service only)
  //                /                           \_
  //               0                         1, default
  //              /                               \_
  //       [SecureBoot]=0                     [SecureBoot]=1
  // (read-only, standardized by UEFI)  (read-only, standardized by UEFI)
  //     images are not verified         images are verified, platform is
  //                                      operating in Secure Boot mode
  //                                                 |
  //                                                 |
  //                                           [CustomMode]
  //                          (read-write, edk2-specific, boot service only)
  //                                /                           \_
  //                          0, default                         1
  //                              /                               \_
  //                      PK, KEK, db, dbx                PK, KEK, db, dbx
  //                    updates are verified          updates are not verified
  //

  PrintSettings (&Settings);

  if ((Settings.SetupMode != 0) || (Settings.SecureBoot != 1) ||
      (Settings.SecureBootEnable != 1) || (Settings.CustomMode != 0) ||
      (Settings.VendorKeys != 0))
  {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: disabled\n"));
    return;
  }

  DEBUG ((EFI_D_INFO, "SecureBootSetup: SecureBoot enabled\n"));
  return;

clearKEK:
  DeleteKEK ();

cleardbs:
  DeleteDbt ();
  DeleteDbx ();
  DeleteDb ();

error:
  if (SetSecureBootMode (STANDARD_SECURE_BOOT_MODE) != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Cannot set mode to Secure: %r\n", Status));
  }

  DEBUG ((EFI_D_ERROR, "SecureBootSetup: disabled\n"));
}

EFI_STATUS
EFIAPI
DriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  VOID  *TcgProtocol;
  VOID  *Registration;

  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **)&TcgProtocol);
  if (!EFI_ERROR (Status)) {
    DEBUG ((
      EFI_D_ERROR,
      "SecureBootSetup: Started too late."
      "TPM is already running!\n"
      ));
    return EFI_DEVICE_ERROR;
  }

  //
  // Create event callback, because we need access variable on SecureBootPolicyVariable
  // We should use VariableWriteArch instead of VariableArch, because Variable driver
  // may update SecureBoot value based on last setting.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiVariableWriteArchProtocolGuid,
    TPL_CALLBACK,
    InstallSecureBootHook,
    NULL,
    &Registration
    );

  return EFI_SUCCESS;
}
