/** @file
  Var Check Hii Lib Common logic
Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef VAR_CHECK_HII_LIB_GUID_H_
#define VAR_CHECK_HII_LIB_GUID_H_

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/MmCommunication.h>
#include <Library/VarCheckLib.h>

#include "VarCheckHii.h"

/**
  SetVariable check handler HII.
  @param[in] HiiVariableBin             Variable BIN.
  @param[in] HiiVariableBinSize         The size of Variable BIN.
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
CheckHiiVariableCommon (
  IN VAR_CHECK_HII_VARIABLE_HEADER  *HiiVariableBin,
  IN UINTN                          HiiVariableBinSize,
  IN CHAR16                         *VariableName,
  IN EFI_GUID                       *VendorGuid,
  IN UINT32                         Attributes,
  IN UINTN                          DataSize,
  IN VOID                           *Data
  );

#endif
