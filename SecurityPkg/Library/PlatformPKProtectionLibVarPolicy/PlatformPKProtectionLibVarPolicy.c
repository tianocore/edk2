/** @file
  Provides an abstracted interface for configuring PK related variable protection.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Protocol/VariablePolicy.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Disable any applicable protection against variable 'PK'. The implementation
  of this interface is platform specific, depending on the protection techniques
  used per platform.

  Note: It is the platform's responsibility to conduct cautious operation after
        disabling this protection.

  @retval     EFI_SUCCESS             State has been successfully updated.
  @retval     Others                  Error returned from implementation specific
                                      underying APIs.

**/
EFI_STATUS
EFIAPI
DisablePKProtection (
  VOID
  )
{
  EFI_STATUS                      Status;
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy;

  DEBUG ((DEBUG_INFO, "%a() Entry...\n", __FUNCTION__));

  // IMPORTANT NOTE: This operation is sticky and leaves variable protections disabled.
  //                  The system *MUST* be reset after performing this operation.
  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (!EFI_ERROR (Status)) {
    Status = VariablePolicy->DisableVariablePolicy ();
    // EFI_ALREADY_STARTED means that everything is currently disabled.
    // This should be considered SUCCESS.
    if (Status == EFI_ALREADY_STARTED) {
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}
