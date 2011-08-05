/** @file
    Provides alternative "spellings" for several C operators.

    The header <iso646.h> defines the following eleven macros (on the left) that expand
    to the corresponding tokens (on the right).

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _ISO646_H
#define _ISO646_H
#include  <sys/EfiCdefs.h>

#define and     &&    ///< Logical AND of two boolean expressions
#define and_eq  &=    ///< Bitwise AND with assignment to lval
#define bitand  &     ///< Bitwise AND of two scalar expressions
#define bitor   |     ///< Bitwise OR of two scalar expressions
#define compl   ~     ///< Binary complement
#define not     !     ///< Logical complement of a boolean expression
#define not_eq  !=    ///< Not-equal comparison
#define or      ||    ///< Logical OR of two boolean expressions
#define or_eq   |=    ///< Bitwise OR with assignment to lval
#define xor     ^     ///< Exclusive OR
#define xor_eq  ^=    ///< Exclusive OR with assignment to lval

#endif  /* _ISO646_H */
