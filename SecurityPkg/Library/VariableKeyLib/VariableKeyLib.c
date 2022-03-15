/** @file
  VariableKeyLib implementation.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/VariableKeyLib.h>

#include <Ppi/KeyServicePpi.h>

#define VAR_KEY_SALT      L"Key for RPMC Variable"
#define VAR_KEY_SALT_SIZE sizeof (VAR_KEY_SALT)

/**
  Retrieves the key for integrity and/or confidentiality of variables.

  @param[out]     VariableKey         A pointer to pointer for the variable key buffer.
  @param[in,out]  VariableKeySize     The size in bytes of the variable key.

  @retval       EFI_SUCCESS             The variable key was returned.
  @retval       EFI_DEVICE_ERROR        An error occurred while attempting to get the variable key.
  @retval       EFI_ACCESS_DENIED       The function was invoked after locking the key interface.
  @retval       EFI_UNSUPPORTED         The variable key is not supported in the current boot configuration.
**/
EFI_STATUS
EFIAPI
GetVariableKey (
  OUT VOID      *VariableKey,
  IN  UINTN     VariableKeySize
  )
{
  EFI_STATUS            Status;
  KEY_SERVICE_PPI       *KeyService;

  Status = PeiServicesLocatePpi (
            &gKeyServicePpiGuid,
            0,
            NULL,
            &KeyService
            );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = KeyService->GenerateKey (
                        (UINT8 *)VAR_KEY_SALT,
                        VAR_KEY_SALT_SIZE,
                        VariableKey,
                        VariableKeySize
                        );
  return Status;
}

