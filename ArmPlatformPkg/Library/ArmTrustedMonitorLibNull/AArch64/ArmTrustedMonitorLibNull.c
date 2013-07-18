/** @file
*  Main file supporting the Monitor World on ARM PLatforms
*
*  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
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

VOID
ArmSecureMonitorWorldInitialize (
  VOID
  )
{
  // Do not touch the EL3 Exception Vector Table Register.
  // The default default DebugAgentLib could have already set its own vector
  // into EL3 to catch abort exceptions.
}
