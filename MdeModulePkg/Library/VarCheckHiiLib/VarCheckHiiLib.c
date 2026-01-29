/** @file
  Var Check Hii handler.

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VarCheckHii.h"
#include "VarCheckHiiGen.h"
#include "VarCheckHiiLibCommon.h"

/**
  Sets the variable check handler for HII.
  @param[in] VariableName               Name of Variable to set.
  @param[in] VendorGuid                 Variable vendor GUID.
  @param[in] Attributes                 Attribute value of the variable.
  @param[in] DataSize                   Size of Data to set.
  @param[in] Data                       Data pointer.
  @retval EFI_SUCCESS               The SetVariable check result was success.
  @retval EFI_SECURITY_VIOLATION    Check fail.
**/
EFI_STATUS
EFIAPI
SetVariableCheckHandlerHii (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  return CheckHiiVariableCommon (mVarCheckHiiBin, mVarCheckHiiBinSize, VariableName, VendorGuid, Attributes, DataSize, Data);
}

/**
  Constructor function of VarCheckHiiLib to register var check HII handler.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckHiiLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VarCheckLibRegisterEndOfDxeCallback (VarCheckHiiGen);
  VarCheckLibRegisterAddressPointer ((VOID **)&mVarCheckHiiBin);
  VarCheckLibRegisterSetVariableCheckHandler (SetVariableCheckHandlerHii);

  return EFI_SUCCESS;
}
