/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARM_HVC_LIB_H_
#define ARM_HVC_LIB_H_

/**
 * The size of the HVC arguments are different between AArch64 and AArch32.
 * The native size is used for the arguments.
 */
typedef struct {
  UINTN    Arg0;
  UINTN    Arg1;
  UINTN    Arg2;
  UINTN    Arg3;
  UINTN    Arg4;
  UINTN    Arg5;
  UINTN    Arg6;
  UINTN    Arg7;
} ARM_HVC_ARGS;

/**
  Trigger an HVC call

  HVC calls can take up to 8 arguments and return up to 4 return values.
  Therefore, the 4 first fields in the ARM_HVC_ARGS structure are used
  for both input and output values.

**/
VOID
ArmCallHvc (
  IN OUT ARM_HVC_ARGS  *Args
  );

#endif // ARM_HVC_LIB_H_
