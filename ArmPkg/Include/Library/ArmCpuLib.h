/** @file

  Copyright (c) 2011-2012, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARMCPU_LIB__
#define __ARMCPU_LIB__

VOID
ArmCpuSetup (
  IN  UINTN         MpId
  );

VOID
ArmCpuSetupSmpNonSecure (
  IN  UINTN         MpId
  );

#endif // __ARMCPU_LIB__
