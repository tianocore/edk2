/** @file -- VarCheckPolicyLib.h
This internal header file defines the common interface of constructor for
VarCheckPolicyLib.

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
  This function is wrapper function to validate the buffer.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with SMRAM/MMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with SMRAM/MMRAM.
**/
BOOLEAN
EFIAPI
VarCheckPolicyIsBufferOutsideValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

#endif // _VAR_CHECK_POLICY_LIB_H_
