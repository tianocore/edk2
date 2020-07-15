/** @file
  Implements NULL authenticated variable services.

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AuthVariableLib.h>
#include <Library/DebugLib.h>

/**
  Initialization for authenticated varibale services.
  If this initialization returns error status, other APIs will not work
  and expect to be not called then.

  @param[in]  AuthVarLibContextIn   Pointer to input auth variable lib context.
  @param[out] AuthVarLibContextOut  Pointer to output auth variable lib context.

  @retval EFI_SUCCESS               Function successfully executed.
  @retval EFI_INVALID_PARAMETER     If AuthVarLibContextIn == NULL or AuthVarLibContextOut == NULL.
  @retval EFI_OUT_OF_RESOURCES      Fail to allocate enough resource.
  @retval EFI_UNSUPPORTED           Unsupported to process authenticated variable.

**/
EFI_STATUS
EFIAPI
AuthVariableLibInitialize (
  IN  AUTH_VAR_LIB_CONTEXT_IN   *AuthVarLibContextIn,
  OUT AUTH_VAR_LIB_CONTEXT_OUT  *AuthVarLibContextOut
  )
{
  //
  // Do nothing, just return EFI_UNSUPPORTED.
  //
  return EFI_UNSUPPORTED;
}

/**
  Process variable with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set.

  @param[in] VariableName           Name of the variable.
  @param[in] VendorGuid             Variable vendor GUID.
  @param[in] Data                   Data pointer.
  @param[in] DataSize               Size of Data.
  @param[in] Attributes             Attribute value of the variable.

  @retval EFI_SUCCESS               The firmware has successfully stored the variable and its data as
                                    defined by the Attributes.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.
  @retval EFI_SECURITY_VIOLATION    The variable is with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS
                                    set, but the AuthInfo does NOT pass the validation
                                    check carried out by the firmware.
  @retval EFI_UNSUPPORTED           Unsupported to process authenticated variable.

**/
EFI_STATUS
EFIAPI
AuthVariableLibProcessVariable (
  IN CHAR16         *VariableName,
  IN EFI_GUID       *VendorGuid,
  IN VOID           *Data,
  IN UINTN          DataSize,
  IN UINT32         Attributes
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
