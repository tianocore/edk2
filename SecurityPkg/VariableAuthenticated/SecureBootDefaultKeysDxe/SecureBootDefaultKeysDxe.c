/** @file
  This driver init default Secure Boot variables

Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
Copyright (c) 2021, Semihalf All rights reserved.<BR>
Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/ImageAuthentication.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SecureBootVariableLib.h>
#include <Library/SecureBootVariableProvisionLib.h>

/**
  The entry point for SecureBootDefaultKeys driver.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_SUCCESS            The secure default keys are initialized successfully.
  @retval EFI_UNSUPPORTED        One of the secure default keys already exists.
  @retval EFI_NOT_FOUND          One of the PK, KEK, or DB default keys is not found.
  @retval Others                 Fail to initialize the secure default keys.

**/
EFI_STATUS
EFIAPI
SecureBootDefaultKeysEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = SecureBootInitPKDefault ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize PKDefault: %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = SecureBootInitKEKDefault ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize KEKDefault: %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = SecureBootInitDbDefault ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize dbDefault: %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = SecureBootInitDbtDefault ();
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_INFO, "%a: dbtDefault not initialized\n", __FUNCTION__));
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize dbtDefault: %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = SecureBootInitDbxDefault ();
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_INFO, "%a: dbxDefault not initialized\n", __FUNCTION__));
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize dbxDefault: %r\n", __FUNCTION__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}
