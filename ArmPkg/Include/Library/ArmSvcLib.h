/** @file
*
*  Copyright (c) 2016 - 2017, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARM_SVC_LIB_H_
#define ARM_SVC_LIB_H_

/**
 * The size of the SVC arguments are different between AArch64 and AArch32.
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
} ARM_SVC_ARGS;

/**
  Trigger an SVC call

  SVC calls can take up to 8 arguments and return up to 8 return values.
  Therefore, the 8 first fields in the ARM_SVC_ARGS structure are used
  for both input and output values.

  @param[in, out]    Args Arguments to be passed as part of the SVC call
                     The return values of the SVC call are also placed
                     in the same structure

  @retval None

**/
VOID
ArmCallSvc (
  IN OUT ARM_SVC_ARGS  *Args
  );

#endif // ARM_SVC_LIB_H_
