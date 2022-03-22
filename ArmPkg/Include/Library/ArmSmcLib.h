/** @file
*
*  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARM_SMC_LIB_H_
#define ARM_SMC_LIB_H_

/**
 * The size of the SMC arguments are different between AArch64 and AArch32.
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
} ARM_SMC_ARGS;

/**
  Trigger an SMC call

  SMC calls can take up to 7 arguments and return up to 4 return values.
  Therefore, the 4 first fields in the ARM_SMC_ARGS structure are used
  for both input and output values.

**/
VOID
ArmCallSmc (
  IN OUT ARM_SMC_ARGS  *Args
  );

/** Trigger an SMC call with 3 arguments.

  @param Function The SMC function.
  @param Arg1     Argument/result.
  @param Arg2     Argument/result.
  @param Arg3     Argument/result.

  @return The SMC error code.

**/
UINTN
ArmCallSmc3 (
  IN     UINTN  Function,
  IN OUT UINTN  *Arg1 OPTIONAL,
  IN OUT UINTN  *Arg2 OPTIONAL,
  IN OUT UINTN  *Arg3 OPTIONAL
  );

/** Trigger an SMC call with 2 arguments.

  @param Function The SMC function.
  @param Arg1     Argument/result.
  @param Arg2     Argument/result.
  @param Arg3     Result.

  @return The SMC error code.

**/
UINTN
ArmCallSmc2 (
  IN     UINTN  Function,
  IN OUT UINTN  *Arg1 OPTIONAL,
  IN OUT UINTN  *Arg2 OPTIONAL,
  OUT UINTN     *Arg3 OPTIONAL
  );

/** Trigger an SMC call with 1 argument.

  @param Function The SMC function.
  @param Arg1     Argument/result.
  @param Arg2     Result.
  @param Arg3     Result.

  @return The SMC error code.

**/
UINTN
ArmCallSmc1 (
  IN     UINTN  Function,
  IN OUT UINTN  *Arg1 OPTIONAL,
  OUT UINTN     *Arg2 OPTIONAL,
  OUT UINTN     *Arg3 OPTIONAL
  );

/** Trigger an SMC call with 0 arguments.

  @param Function The SMC function.
  @param Arg1     Result.
  @param Arg2     Result.
  @param Arg3     Result.

  @return The SMC error code.

**/
UINTN
ArmCallSmc0 (
  IN     UINTN  Function,
  OUT UINTN     *Arg1 OPTIONAL,
  OUT UINTN     *Arg2 OPTIONAL,
  OUT UINTN     *Arg3 OPTIONAL
  );

#endif // ARM_SMC_LIB_H_
