/** @file
  Macros to simplify boolean expressions and operations.

  This header is not specified by the C95 standard but is included here for
  operational convenience.

  The macro bool expands to _Bool, as required by the C99 specification.
  This subsequently expands to BOOLEAN, is a UEFI data type which is automatically
  defined correctly for the target CPU architecture.

  The remaining three macros are suitable for use in #if preprocessing
  directives. They are true, which expands to the integer constant 1,
  false, which expands to the integer constant 0, and
  __bool_true_false_are_defined which expands to the integer constant 1.

  A program may undefine and perhaps then redefine the
  macros bool, true, and false.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _STDBOOL_H
#define _STDBOOL_H
#include  <sys/EfiCdefs.h>

#define bool    _Bool
#define true        1
#define false       0
#define __bool_true_false_are_defined 1

#endif    /* _STDBOOL_H */
