/** @file

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_MONITOR_LIB_H_
#define ARM_MONITOR_LIB_H_

/** The size of the SMC arguments is different between AArch64 and AArch32.

  The native size is used for the arguments.
  It will be casted to either HVC or SMC args.
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
} ARM_MONITOR_ARGS;

/** Monitor call.

  An HyperVisor Call (HVC) or System Monitor Call (SMC) will be issued
  depending on the default conduit. PcdMonitorConduitHvc determines the type
  of the call: if true, do an HVC.

  @param [in,out]  Args    Arguments for the HVC/SMC.
**/
VOID
EFIAPI
ArmMonitorCall (
  IN OUT ARM_MONITOR_ARGS  *Args
  );

#endif // ARM_MONITOR_LIB_H_
