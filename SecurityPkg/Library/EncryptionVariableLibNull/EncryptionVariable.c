/** @file
  The common variable operation routines shared by DXE_RUNTIME variable
  module and DXE_SMM variable module.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data. They may be input in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  VariableServiceGetNextVariableName () and VariableServiceQueryVariableInfo() are external API.
  They need check input parameter.

  VariableServiceGetVariable() and VariableServiceSetVariable() are external API
  to receive datasize and data buffer. The size should be checked carefully.

  VariableServiceSetVariable() should also check authenticate data to avoid buffer overflow,
  integer overflow. It should also check attribute to avoid authentication bypass.

Copyright (c) 2019 - 2022, Intel Corporation. All rights reserved.<BR>
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
