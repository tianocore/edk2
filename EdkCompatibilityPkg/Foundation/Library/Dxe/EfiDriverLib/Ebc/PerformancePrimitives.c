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

#include "Tiano.h"  // for ASSERT macro
#include "TianoCommon.h"

EFI_STATUS
GetTimerValue (
  OUT UINT64    *TimerValue
  )
/*++

Routine Description:

  Set TimerValue to 0, which is not expected to be run.

Arguments:

  TimerValue  - Timer value for output

Returns:

  EFI_SUCCESS - Should not be reached.

--*/
{
  //
  // Should not be used for EBC, so assert.
  //
  *TimerValue = 0;
  ASSERT (FALSE);

  return EFI_SUCCESS;
}
