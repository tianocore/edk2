//
//  Copyright (c) 2021, NUVIA Inc. All rights reserved.
//  Copyright (c) 2016, Linaro Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <Base.h>
#include <Library/ArmSmcLib.h>
#include <IndustryStandard/ArmStdSmc.h>

VOID
ArmCallSmc (
  IN OUT ARM_SMC_ARGS  *Args
  )
{
}

/** Triggers an SMC call with 3 arguments.

  @param Function The SMC function.
  @param Arg1      Argument/result.
  @param Arg2      Argument/result.
  @param Arg3      Argument/result.

  @return The SMC error code.
**/
UINTN
ArmCallSmc3 (
  IN     UINTN  Function,
  IN OUT UINTN  *Arg1 OPTIONAL,
  IN OUT UINTN  *Arg2 OPTIONAL,
  IN OUT UINTN  *Arg3 OPTIONAL
  )
{
  return SMC_ARCH_CALL_NOT_SUPPORTED;
}

/** Trigger an SMC call with 2 arguments.

  @param Function The SMC function.
  @param Arg1      Argument/result.
  @param Arg2      Argument/result.
  @param Arg3      Result.

  @return The SMC error code.

**/
UINTN
ArmCallSmc2 (
  IN     UINTN  Function,
  IN OUT UINTN  *Arg1 OPTIONAL,
  IN OUT UINTN  *Arg2 OPTIONAL,
  OUT UINTN     *Arg3 OPTIONAL
  )
{
  return SMC_ARCH_CALL_NOT_SUPPORTED;
}

/** Trigger an SMC call with 1 argument.

  @param Function The SMC function.
  @param Arg1      Argument/result.
  @param Arg2      Result.
  @param Arg3      Result.

  @return The SMC error code.

**/
UINTN
ArmCallSmc1 (
  IN     UINTN  Function,
  IN OUT UINTN  *Arg1 OPTIONAL,
  OUT UINTN     *Arg2 OPTIONAL,
  OUT UINTN     *Arg3 OPTIONAL
  )
{
  return SMC_ARCH_CALL_NOT_SUPPORTED;
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
  IN  UINTN  Function,
  OUT UINTN  *Arg1 OPTIONAL,
  OUT UINTN  *Arg2 OPTIONAL,
  OUT UINTN  *Arg3 OPTIONAL
  )
{
  return SMC_ARCH_CALL_NOT_SUPPORTED;
}
