//++
// Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
// This program and the accompanying materials                          
// are licensed and made available under the terms and conditions of the BSD License         
// which accompanies this distribution.  The full text of the license may be found at        
// http://opensource.org/licenses/bsd-license.php                                            
//                                                                                           
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
// 
// Module Name:
//
//  HWAccess.s
//
// Abstract:
//
//  Contains an implementation of Read/Write Kr7 for the Itanium-based architecture.
//
//
//
// Revision History:
//
//--

  .file  "HWAccess.s"
#include  "IpfMacro.i"


//----------------------------------------------------------------------------------
//++ 
//VOID
//AsmWriteKr7 (
//  UINT64
//  );
//
// This routine saves the given input value into the kernel register 7
//
// Arguments :
//
// On Entry :  64 bit value to be saved.
//
// Return Value: None
//
//--
//----------------------------------------------------------------------------------
PROCEDURE_ENTRY (AsmWriteKr7)
        NESTED_SETUP (1,2,0,0)
        mov ar.k7 = in0;;
        NESTED_RETURN

PROCEDURE_EXIT (AsmWriteKr7)

//---------------------------------------------------------------------------------
//++
//UINT64
//AsmReadKr7 (
//  VOID
//  );
//
// This routine returns the value of the kernel register 7
//
// Arguments :
//
// On Entry :  None
//
// Return Value: 64bit Value of the register.
//
//--
//----------------------------------------------------------------------------------
PROCEDURE_ENTRY (AsmReadKr7)
        NESTED_SETUP (0,2,0,0)
        mov r8 = ar.k7;;
        NESTED_RETURN
PROCEDURE_EXIT (AsmReadKr7)
//----------------------------------------------------------------------------------


