/** @file
*
*  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
*  Copyright (c) 2012-2023, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
*  @par Reference(s):
*   - SMC Calling Convention (SMCCC), ARM DEN 0028E, EAC0, 1.4
*     (https://developer.arm.com/documentation/den0028/e/)
**/

#ifndef ARM_SMC_LIB_H_
#define ARM_SMC_LIB_H_

/**
 * The size of the SMC arguments are different between AArch64 and AArch32.
 * The native size is used for the arguments.
 * According to the SMCCC Section 2.6 SMC32/HVC32 argument passing
 * When an SMC32/HVC32 call is made from AArch32:
 *  - Arguments are passed in registers R1-R7.
 *  - Results are returned in R0-R7.
 * When an SMC32/HVC32 call is made from AArch64:
 *  - Arguments are passed in registers W1-W7.
 *  - Results are returned in W0-W7.
 *
 * According to SMCCC Section 2.7 SMC64/HVC64 argument passing
 * When an SMC64/HVC64 call is made from AArch64:
 *  - Arguments are passed in registers X1-X17.
 *  - Results are returned in X0-X17.
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
 #ifdef MDE_CPU_AARCH64
  UINTN    Arg8;
  UINTN    Arg9;
  UINTN    Arg10;
  UINTN    Arg11;
 #endif
} ARM_SMC_ARGS;

/**
  Trigger an SMC call

  According to the SMCCC Section 2.6 SMC32/HVC32 argument passing
  When an SMC32/HVC32 call is made from AArch32:
   - Arguments are passed in registers R1-R7.
   - Results are returned in R0-R7.
  When an SMC32/HVC32 call is made from AArch64:
   - Arguments are passed in registers W1-W7.
   - Results are returned in W0-W7.

  According to SMCCC Section 2.7 SMC64/HVC64 argument passing
  When an SMC64/HVC64 call is made from AArch64:
   - Arguments are passed in registers X1-X17.
   - Results are returned in X0-X17.

  This means SMC calls can take up to 7/17 arguments and return up
  to 7/17 return values.

  However, the current use-case:
  - For SMC32/HVC32 calls made from AArch32/AArch64 up to 7 arguments
    and 4 return values are required. Therefore, limit the maximum
    arguments to 7 and return values to 4.
  - For AMC64/HVC64 calls made from AArch64 up to 11 arguments and
    return values are required. Therefore, limit the maximum arguments
    and return values to 11.

  The fields in the ARM_SMC_ARGS structure are used
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
