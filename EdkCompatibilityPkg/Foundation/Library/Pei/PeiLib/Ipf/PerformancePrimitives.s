//++
// Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
// This program and the accompanying materials                          
// are licensed and made available under the terms and conditions of the BSD License         
// which accompanies this distribution.  The full text of the license may be found at        
// http://opensource.org/licenses/bsd-license.php                                            
//                                                                                           
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
// 
//  Module Name:
//
//    PerformancePrimitives.s
//
//  Abstract:
//
//
// Revision History:
//
//--

.file  "PerformancePrimitives.s"

#include  "IpfMacro.i"

//-----------------------------------------------------------------------------
//++
// GetTimerValue
//
// Implementation of CPU-based time service
//
// On Entry :
//    EFI_STATUS
//    GetTimerValue (
//      OUT UINT64    *TimerValue
//    )
//
// Return Value: 
//        r8  = Status
//        r9  = 0
//        r10 = 0
//        r11 = 0
// 
// As per static calling conventions. 
// 
//--
//---------------------------------------------------------------------------
PROCEDURE_ENTRY (GetTimerValue)

      NESTED_SETUP (1,8,0,0)
      mov               r8 = ar.itc;;
      st8               [r32]= r8
      mov               r8 = r0
      mov               r9 = r0
      mov               r10 = r0
      mov               r11 = r0
      NESTED_RETURN

PROCEDURE_EXIT (GetTimerValue)
//---------------------------------------------------------------------------

