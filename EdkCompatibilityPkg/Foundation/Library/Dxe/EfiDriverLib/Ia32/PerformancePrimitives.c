/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PerformancePrimitives.c

Abstract:

  Support for Performance library

--*/

#include "TianoCommon.h"
#include "CpuIA32.h"

EFI_STATUS
GetTimerValue (
  OUT UINT64    *TimerValue
  )
/*++

Routine Description:

  Set TimerValue with current tick.

Arguments:

  TimerValue  - Timer value to be set

Returns:

  EFI_SUCCESS - TimerValue is set.

--*/
{
  *TimerValue = EfiReadTsc ();
  return EFI_SUCCESS;
}
