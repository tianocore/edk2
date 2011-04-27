/** @file
  The header <stdbool.h> defines four macros: bool, true, false,
  and __bool_true_false_are_defined.

  The macro bool expands to _Bool.

  The remaining three macros are suitable for use in #if preprocessing
  directives. They are true, which expands to the integer constant 1,
  false, which expands to the integer constant 0, and
  __bool_true_false_are_defined which expands to the integer constant 1.

  A program may undefine and perhaps then redefine the
  macros bool, true, and false.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _STDBOOL_H
#define _STDBOOL_H
#include  <sys/EfiCdefs.h>

#define bool  _Bool
#define true  1
#define false 0
#define __bool_true_false_are_defined 1

#endif    /* _STDBOOL_H */
