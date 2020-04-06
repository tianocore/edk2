/** @file -- VariablePolicyExtraInitNull.c
This file contains extra init and deinit routines that don't do anything
extra.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiRuntimeServicesTableLib.h>


/**
  An extra init hook that enables the RuntimeDxe library instance to
  register VirtualAddress change callbacks. Among other things.

  @retval     EFI_SUCCESS   Everything is good. Continue with init.
  @retval     Others        Uh... don't continue.

**/
EFI_STATUS
VariablePolicyExtraInit (
  VOID
  )
{
  // NULL implementation.
  return EFI_SUCCESS;
}


/**
  An extra deinit hook that enables the RuntimeDxe library instance to
  register VirtualAddress change callbacks. Among other things.

  @retval     EFI_SUCCESS   Everything is good. Continue with deinit.
  @retval     Others        Uh... don't continue.

**/
EFI_STATUS
VariablePolicyExtraDeinit (
  VOID
  )
{
  // NULL implementation.
  return EFI_SUCCESS;
}
