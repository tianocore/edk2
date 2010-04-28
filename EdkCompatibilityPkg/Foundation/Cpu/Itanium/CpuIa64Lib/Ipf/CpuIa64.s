//****************************************************************************
//
//   Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
//   This program and the accompanying materials                          
//   are licensed and made available under the terms and conditions of the BSD License         
//   which accompanies this distribution.  The full text of the license may be found at        
//   http://opensource.org/licenses/bsd-license.php                                            
//                                                                                             
//   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
//   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
//   
//   Module Name:
//
//     CpuIA64.s
//  
//   Abstract: 
//   
//     Contains basic assembly procedures to support IPF CPU.
//  
//****************************************************************************

.file  "CpuIA64.s"

#include  "IpfMacro.i"
#include  "IpfDefines.h"


PROCEDURE_ENTRY (EfiReadTsc)
  
  mov r8 = ar.itc   
  br.ret.dpnt  b0;;

PROCEDURE_EXIT (EfiReadTsc)