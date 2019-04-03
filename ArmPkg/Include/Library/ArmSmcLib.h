/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef __ARM_SMC_LIB__
#define __ARM_SMC_LIB__

/**
 * The size of the SMC arguments are different between AArch64 and AArch32.
 * The native size is used for the arguments.
 */
typedef struct {
  UINTN  Arg0;
  UINTN  Arg1;
  UINTN  Arg2;
  UINTN  Arg3;
  UINTN  Arg4;
  UINTN  Arg5;
  UINTN  Arg6;
  UINTN  Arg7;
} ARM_SMC_ARGS;

/**
  Trigger an SMC call

  SMC calls can take up to 7 arguments and return up to 4 return values.
  Therefore, the 4 first fields in the ARM_SMC_ARGS structure are used
  for both input and output values.

**/
VOID
ArmCallSmc (
  IN OUT ARM_SMC_ARGS *Args
  );

#endif
