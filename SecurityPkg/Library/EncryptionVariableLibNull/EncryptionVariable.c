/** @file
  NULL implementation of EncryptionVariableLib.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/EncryptionVariableLib.h>
#include <Library/DebugLib.h>

/**
  Encrypt variable data.

  Null version.

  @param[in, out]   VarEncInfo   Pointer to structure containing detailed
                                 information about a variable.

  @retval EFI_UNSUPPORTED         Unsupported to encrypt variable.

**/
EFI_STATUS
EFIAPI
EncryptVariable (
  IN OUT VARIABLE_ENCRYPTION_INFO  *VarEncInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Decrypt variable data.

  Null version.

  @param[in, out]   VarEncInfo   Pointer to structure containing detailed
                                 information about a variable.

  @retval EFI_UNSUPPORTED         Unsupported to encrypt variable.

**/
EFI_STATUS
EFIAPI
DecryptVariable (
  IN OUT VARIABLE_ENCRYPTION_INFO  *VarEncInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get cipher information.

  Null version.

  @param[in]   VarEncInfo   Pointer to structure containing detailed
                            information about a variable.

  @retval EFI_UNSUPPORTED         Unsupported interface.

**/
EFI_STATUS
EFIAPI
GetCipherDataInfo (
  IN VARIABLE_ENCRYPTION_INFO  *VarEncInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Set cipher information for a variable.

  Null version.

  @param[in]   VarEncInfo   Pointer to structure containing detailed
                            information about a variable.

  @retval EFI_UNSUPPORTED         If this method is not supported.

**/
EFI_STATUS
EFIAPI
SetCipherDataInfo (
  IN VARIABLE_ENCRYPTION_INFO  *VarEncInfo
  )
{
  return EFI_UNSUPPORTED;
}
