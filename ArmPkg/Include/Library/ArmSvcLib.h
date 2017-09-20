/** @file
*
*  Copyright (c) 2016 - 2017, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __ARM_SVC_LIB__
#define __ARM_SVC_LIB__

/**
 * The size of the SVC arguments are different between AArch64 and AArch32.
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
} ARM_SVC_ARGS;

/**
  Trigger an SVC call

  SVC calls can take up to 7 arguments and return up to 4 return values.
  Therefore, the 4 first fields in the ARM_SVC_ARGS structure are used
  for both input and output values.

**/
VOID
ArmCallSvc (
  IN OUT ARM_SVC_ARGS *Args
  );

#endif
