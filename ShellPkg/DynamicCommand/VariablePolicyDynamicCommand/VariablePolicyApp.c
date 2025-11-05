/** @file
  Functionality specific for standalone UEFI application support.

  This application can provide detailed UEFI variable policy configuration
  information in the UEFI shell.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariablePolicy.h"

#include <Library/BaseLib.h>
#include <Library/HiiLib.h>

extern EFI_HII_HANDLE  mVarPolicyShellCommandHiiHandle;

//
// String token ID of help message text.
// Shell supports finding the help message in the resource section of an
// application image if a .MAN file is not found. This global variable is added
// to make the build tool recognize that the help string is consumed by the user and
// then the build tool will add the string into the resource section. Thus the
// application can use '-?' option to show help message in Shell.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_STRING_ID  mStringHelpTokenId = STRING_TOKEN (STR_GET_HELP_VAR_POLICY);

/**
  Entry of the UEFI variable policy application.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           The application successfully initialized.
  @retval EFI_ABORTED           The application failed to initialize.
  @retval Others                A different error occurred.

**/
EFI_STATUS
EFIAPI
VariablePolicyAppInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mVarPolicyShellCommandHiiHandle = InitializeHiiPackage (ImageHandle);
  if (mVarPolicyShellCommandHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = (EFI_STATUS)RunVarPolicy (ImageHandle, SystemTable);

  HiiRemovePackages (mVarPolicyShellCommandHiiHandle);

  return Status;
}
