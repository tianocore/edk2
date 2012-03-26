/** @file
*
*  Copyright (c) 2012, ARM Limited. All rights reserved.
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

#ifndef __ARM_SMC_LIB__
#define __ARM_SMC_LIB__

VOID
ArmCallSmc (
  IN OUT UINTN *Rx
  );

VOID
ArmCallSmcArg1 (
  IN OUT UINTN *Rx,
  IN OUT UINTN *Arg1
  );

VOID
ArmCallSmcArg2 (
  IN OUT UINTN *Rx,
  IN OUT UINTN *Arg1,
  IN OUT UINTN *Arg2
  );

VOID
ArmCallSmcArg3 (
  IN OUT UINTN *Rx,
  IN OUT UINTN *Arg1,
  IN OUT UINTN *Arg2,
  IN OUT UINTN *Arg3
  );

#endif
