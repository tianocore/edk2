/** @file
*  Main file supporting the Monitor World on ARM PLatforms
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
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

#include <Library/ArmLib.h>
#include <Library/ArmTrustedMonitorLib.h>
#include <Library/DebugLib.h>

#define IS_ALIGNED(Address, Align) (((UINTN)Address & (Align-1)) == 0)

VOID
MonitorVectorTable (
  VOID
  );

VOID
ArmSecureMonitorWorldInitialize (
  VOID
  )
{
  // Ensure the Monitor Table is 32bit aligned
  ASSERT (((UINTN)&MonitorVectorTable & ARM_VECTOR_TABLE_ALIGNMENT) == 0);

  // Write the Monitor Mode Vector Table Address
  ArmWriteMVBar ((UINTN) &MonitorVectorTable);
}

