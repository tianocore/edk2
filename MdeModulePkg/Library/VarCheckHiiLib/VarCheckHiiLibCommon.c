/** @file
  Var Check Hii Lib Common logic
Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <Library/DebugLib.h>

#include "VarCheckHii.h"
#include "VarCheckHiiLibCommon.h"
EFI_HANDLE  mEfiVariableCheckHiiHandle = NULL;

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
  )
{
  VAR_CHECK_HII_VARIABLE_HEADER  *HiiVariable;
  VAR_CHECK_HII_QUESTION_HEADER  *HiiQuestion;

  if (HiiVariableBin == NULL) {
    return EFI_SUCCESS;
  }

  if ((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) || (Attributes == 0)) {
    //
    // Do not check delete variable.
    //
  }

  //
  // For Hii Variable header align.
  //
  HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *)HEADER_ALIGN (HiiVariableBin);
  while ((UINTN)HiiVariable < ((UINTN)HiiVariableBin + HiiVariableBinSize)) {
    if ((StrCmp ((CHAR16 *)(HiiVariable + 1), VariableName) == 0) &&
        (CompareGuid (&HiiVariable->Guid, VendorGuid)))
    {
      //
      // Found the Hii Variable that could be used to do check.
      //
      DEBUG ((DEBUG_INFO, "VarCheckHiiVariable - %s:%g with Attributes = 0x%08x Size = 0x%x\n", VariableName, VendorGuid, Attributes, DataSize));
      if (HiiVariable->Attributes != Attributes) {
        DEBUG ((DEBUG_INFO, "VarCheckHiiVariable fail for Attributes - 0x%08x\n", HiiVariable->Attributes));
        return EFI_SECURITY_VIOLATION;
      }

      if (DataSize == 0) {
        DEBUG ((DEBUG_INFO, "VarCheckHiiVariable - CHECK PASS with DataSize == 0 !\n"));
        return EFI_SUCCESS;
      }

      if (HiiVariable->Size != DataSize) {
        DEBUG ((DEBUG_INFO, "VarCheckHiiVariable fail for Size - 0x%x\n", HiiVariable->Size));
        return EFI_SECURITY_VIOLATION;
      }

      //
      // Do the check.
      // For Hii Question header align.
      //
      HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *)HEADER_ALIGN (((UINTN)HiiVariable + HiiVariable->HeaderLength));
      while ((UINTN)HiiQuestion < ((UINTN)HiiVariable + HiiVariable->Length)) {
        if (!VarCheckHiiQuestion (HiiQuestion, Data, DataSize)) {
          return EFI_SECURITY_VIOLATION;
        }

        //
        // For Hii Question header align.
        //
        HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *)HEADER_ALIGN (((UINTN)HiiQuestion + HiiQuestion->Length));
      }

      DEBUG ((DEBUG_INFO, "VarCheckHiiVariable - ALL CHECK PASS!\n"));
      return EFI_SUCCESS;
    }

    //
    // For Hii Variable header align.
    //
    HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *)HEADER_ALIGN (((UINTN)HiiVariable + HiiVariable->Length));
  }

  // Not found, so pass.
  return EFI_SUCCESS;
}
