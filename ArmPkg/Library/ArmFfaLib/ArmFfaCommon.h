/** @file
  Arm FF-A ns common library Header file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile
     - spmc - Secure Partition Manager Core
     - spmd - Secure Partition Manager Dispatcher

  @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/

#ifndef ARM_FFA_COMMON_LIB_H_
#define ARM_FFA_COMMON_LIB_H_

/**
 * Arguments to call FF-A request via SMC/SVC.
 */
typedef struct ArmFfaArgs {
  UINTN    Arg0;
  UINTN    Arg1;
  UINTN    Arg2;
  UINTN    Arg3;
  UINTN    Arg4;
  UINTN    Arg5;
  UINTN    Arg6;
  UINTN    Arg7;
} ARM_FFA_ARGS;

extern BOOLEAN  gFfaSupported;
extern UINT16   gPartId;

/**
  Convert FfArgs to EFI_STATUS.

  @param [in] FfaArgs            Ffa arguments

  @retval EFI_STATUS             return value correspond EFI_STATUS to FfaStatus

**/
EFI_STATUS
EFIAPI
FfaArgsToEfiStatus (
  IN ARM_FFA_ARGS  *FfaArgs
  );

/**
  Trigger FF-A ABI call according to PcdFfaLibConduitSmc.

  @param [in out]  FfaArgs        Ffa arguments

**/
VOID
EFIAPI
ArmCallFfa (
  IN OUT ARM_FFA_ARGS  *FfaArgs
  );

/**
  Common ArmFfaLib Constructor.

  @retval EFI_SUCCESS
  @retval Others                  Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibCommonInit (
  IN VOID
  );

#endif
