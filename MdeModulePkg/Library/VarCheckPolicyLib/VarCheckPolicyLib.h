/** @file -- VarCheckPolicyLib.h
This internal header file defines the common interface of constructor for
VarCheckPolicyLib.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VAR_CHECK_POLICY_LIB_H_
#define _VAR_CHECK_POLICY_LIB_H_

/**
  Common constructor function of VarCheckPolicyLib to register VarCheck handler
  and SW MMI handlers.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckPolicyLibCommonConstructor (
  VOID
  );

/**
  This function is wrapper function to validate the Primary Buffer (CommBuffer).

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid.
  @retval FALSE This buffer is not valid.
**/
BOOLEAN
EFIAPI
VarCheckPolicyIsPrimaryBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

#endif // _VAR_CHECK_POLICY_LIB_H_
