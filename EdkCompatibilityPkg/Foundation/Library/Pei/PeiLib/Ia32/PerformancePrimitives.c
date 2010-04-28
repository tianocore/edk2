/*++

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>                                                         
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

  Get timer value.

Arguments:

  TimerValue  - Pointer to the returned timer value

Returns:

  EFI_SUCCESS - Successfully got timer value

--*/
{
  *TimerValue = EfiReadTsc ();
  return EFI_SUCCESS;
}
