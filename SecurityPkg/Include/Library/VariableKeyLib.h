/** @file
  Public definitions for Variable Key Library.

Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_KEY_LIB_H_
#define _VARIABLE_KEY_LIB_H_

#include <Uefi/UefiBaseType.h>

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
  OUT VOID    *VariableKey,
  IN  UINTN   VariableKeySize
  );

#endif
