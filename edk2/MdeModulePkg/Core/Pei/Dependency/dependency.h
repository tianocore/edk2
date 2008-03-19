/** @file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  dependency.h

Abstract:
   
  This module contains data specific to dependency expressions
  and local function prototypes.
        
**/

#ifndef _PEI_DEPENDENCY_H_
#define _PEI_DEPENDENCY_H_


#define MAX_GRAMMAR_SIZE  256

//
// type definitions
//
typedef UINT8 DEPENDENCY_EXPRESSION_OPERAND;

typedef struct {
  BOOLEAN                 Result;
  VOID                    *Operator;
} EVAL_STACK_ENTRY;

#endif
