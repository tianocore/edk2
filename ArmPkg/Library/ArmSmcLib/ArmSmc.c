/** @file
  SMC helper functions.

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmSmcLib.h>

/** Triggers an SMC call with 3 arguments.

  @param Function The SMC function.
  @param Arg1     Argument/result.
  @param Arg2     Argument/result.
  @param Arg3     Argument/result.

  @return The SMC error code.
**/
UINTN
ArmCallSmc3 (
  IN     UINTN Function,
  IN OUT UINTN *Arg1,
  IN OUT UINTN *Arg2,
  IN OUT UINTN *Arg3
  )
{
  ARM_SMC_ARGS Args;
  UINTN        ErrorCode;

  Args.Arg0 = Function;

  if (Arg1 != NULL) {
    Args.Arg1 = *Arg1;
  }
  if (Arg2 != NULL) {
    Args.Arg2 = *Arg2;
  }
  if (Arg3 != NULL) {
    Args.Arg3 = *Arg3;
  }

  ArmCallSmc (&Args);

  ErrorCode = Args.Arg0;

  if (Arg1 != NULL) {
    *Arg1 = Args.Arg1;
  }
  if (Arg2 != NULL) {
    *Arg2 = Args.Arg2;
  }
  if (Arg3 != NULL) {
    *Arg3 = Args.Arg3;
  }

  return ErrorCode;
}

/** Trigger an SMC call with 2 arguments.

  @param Function The SMC function.
  @param Arg1     Argument/result.
  @param Arg2     Argument/result.
  @param Arg3     Result.

  @return The SMC error code.

**/
UINTN
ArmCallSmc2 (
  IN     UINTN Function,
  IN OUT UINTN *Arg1,
  IN OUT UINTN *Arg2,
     OUT UINTN *Arg3
  )
{
  return ArmCallSmc3 (Function, Arg1, Arg2, Arg3);
}

/** Trigger an SMC call with 1 argument.

  @param Function The SMC function.
  @param Arg1     Argument/result.
  @param Arg2     Result.
  @param Arg3     Result.

  @return The SMC error code.

**/
UINTN
ArmCallSmc1 (
  IN     UINTN Function,
  IN OUT UINTN *Arg1,
     OUT UINTN *Arg2,
     OUT UINTN *Arg3
  )
{
  return ArmCallSmc3 (Function, Arg1, Arg2, Arg3);
}

/** Trigger an SMC call with 0 arguments.

  @param Function The SMC function.
  @param Arg1     Result.
  @param Arg2     Result.
  @param Arg3     Result.

  @return The SMC error code.

**/
UINTN
ArmCallSmc0 (
  IN     UINTN Function,
     OUT UINTN *Arg1,
     OUT UINTN *Arg2,
     OUT UINTN *Arg3
  )
{
  return ArmCallSmc3 (Function, Arg1, Arg2, Arg3);
}
