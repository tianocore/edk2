// Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
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
//   ReadKr1.s
//
// Abstract:
//
//   Contains assembly code for read Kr1.
//
//--

  .file  "ReadKr1.s"

#include  "IpfMacro.i"

//---------------------------------------------------------------------------------
//++
// AsmReadKr1
//
// This routine is used to get KR1. KR1 is used to store Pei Service Table
// Pointer in archeture.
//
// Arguments : 
//
// On Entry :  None.
//
// Return Value: Pei Services Table.
// 
//--
//----------------------------------------------------------------------------------
PROCEDURE_ENTRY (AsmReadKr1)
        
        mov             r8 = ar.k1;;  // Pei Services Table Pointer
        br.ret.dpnt     b0;;

PROCEDURE_EXIT (AsmReadKr1)
