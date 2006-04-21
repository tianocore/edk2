//++
// Copyright (c) 2006, Intel Corporation                                                         
// All rights reserved. This program and the accompanying materials                          
// are licensed and made available under the terms and conditions of the BSD License         
// which accompanies this distribution.  The full text of the license may be found at        
// http://opensource.org/licenses/bsd-license.php                                            
//                                                                                           
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
// 
// Module Name:
//  IpfMacro.i
//
// Abstract:
//  Contains the macros needed for calling procedures in Itanium-based assembly code.
//
//
// Revision History:
//
//--

#ifndef  __IA64PROC_I__
#define  __IA64PROC_I__


#define PROCEDURE_ENTRY(name)   .##text;            \
                                .##type name, @function;    \
                                .##proc name;           \
name::

#define PROCEDURE_EXIT(name)    .##endp name

// Note: use of NESTED_SETUP requires number of locals (l) >= 3

#define NESTED_SETUP(i,l,o,r) \
         alloc loc1=ar##.##pfs,i,l,o,r ;\
         mov loc0=b0

#define NESTED_RETURN \
         mov b0=loc0 ;\
         mov ar##.##pfs=loc1 ;;\
         br##.##ret##.##dpnt  b0;;


#define INTERRUPT_HANDLER_BEGIN(name) \
PROCEDURE_ENTRY(name##HandlerBegin) \
;; \
PROCEDURE_EXIT(name##HandlerBegin)

#define INTERRUPT_HANDLER_END(name) \
PROCEDURE_ENTRY(name##HandlerEnd) \
;; \
PROCEDURE_EXIT(name##HandlerEnd) 


#define INTERRUPT_HANDLER_BLOCK_BEGIN \
INTERRUPT_HANDLER_BEGIN(First)

#define INTERRUPT_HANDLER_BLOCK_END \
INTERRUPT_HANDLER_END(Last)



#endif
