/** @file
  Internal Function and Macro defintions for IFR Expression evaluation used in Ifr Parsing. This header file should only
  be included by UefiIfrParserExpression.c

  Copyright (c) 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_UEFI_IFR_PARSER_EXPRESSION_INTERNAL_
#define _HII_THUNK_UEFI_IFR_PARSER_EXPRESSION_INTERNAL_

//
// Incremental size of stack for expression
//
#define EXPRESSION_STACK_SIZE_INCREMENT    0x100


FORM_BROWSER_STATEMENT *
IdToQuestion (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN FORM_BROWSER_FORM     *Form,
  IN UINT16                QuestionId
  )
;


FORM_EXPRESSION *
IdToExpression (
  IN FORM_BROWSER_FORM  *Form,
  IN UINT8              RuleId
  )
;

 
#endif
