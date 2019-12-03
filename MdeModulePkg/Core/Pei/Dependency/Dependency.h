/** @file
  This module contains data specific to dependency expressions
  and local function prototypes.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PEI_DEPENDENCY_H_
#define _PEI_DEPENDENCY_H_


#define MAX_GRAMMAR_SIZE  64

//
// type definitions
//
typedef UINT8 DEPENDENCY_EXPRESSION_OPERAND;

typedef struct {
  BOOLEAN                 Result;
  VOID                    *Operator;
} EVAL_STACK_ENTRY;

#endif
