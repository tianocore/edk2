/** @file

  Copyright (c) 2011, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARMCPU_LIB__
#define __ARMCPU_LIB__

// These are #define and not enum to be used in assembly files
#define ARM_CPU_EVENT_DEFAULT         0
#define ARM_CPU_EVENT_BOOT_MEM_INIT   1
#define ARM_CPU_EVENT_SECURE_INIT     2

typedef UINTN ARM_CPU_SYNCHRONIZE_EVENT;


VOID
ArmCpuSynchronizeWait (
  IN ARM_CPU_SYNCHRONIZE_EVENT   Event
  );

VOID
ArmCpuSynchronizeSignal (
  IN ARM_CPU_SYNCHRONIZE_EVENT   Event
  );

VOID
ArmCpuSetup (
  IN  UINTN         MpId
  );

VOID
ArmCpuSetupSmpNonSecure (
  IN  UINTN         MpId
  );

#endif // __ARMCPU_LIB__
