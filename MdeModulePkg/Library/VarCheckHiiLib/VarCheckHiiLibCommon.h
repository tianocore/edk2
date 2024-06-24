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
  Dump some hexadecimal data.
  @param[in] Indent     How many spaces to indent the output.
  @param[in] Offset     The offset of the dump.
  @param[in] DataSize   The size in bytes of UserData.
  @param[in] UserData   The data to dump.
**/
VOID
VarCheckHiiInternalDumpHex (
  IN UINTN  Indent,
  IN UINTN  Offset,
  IN UINTN  DataSize,
  IN VOID   *UserData
  );

/**
  Var Check Hii Question.
  @param[in] HiiQuestion    Pointer to Hii Question
  @param[in] Data           Data pointer.
  @param[in] DataSize       Size of Data to set.
  @retval TRUE  Check pass
  @retval FALSE Check fail.
**/
BOOLEAN
VarCheckHiiQuestion (
  IN VAR_CHECK_HII_QUESTION_HEADER  *HiiQuestion,
  IN VOID                           *Data,
  IN UINTN                          DataSize
  );

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
