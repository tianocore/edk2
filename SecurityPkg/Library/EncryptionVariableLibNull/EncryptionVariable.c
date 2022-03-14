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

EFI_STATUS
EFIAPI
EncryptVariable (
  IN OUT VARIABLE_ENCRYPTION_INFO     *VarEncInfo
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
DecryptVariable (
  IN OUT VARIABLE_ENCRYPTION_INFO     *VarEncInfo
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
GetCipherDataInfo (
  IN VARIABLE_ENCRYPTION_INFO     *VarEncInfo
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
SetCipherDataInfo (
  IN VARIABLE_ENCRYPTION_INFO     *VarEncInfo
  )
{
  return EFI_UNSUPPORTED;
}


