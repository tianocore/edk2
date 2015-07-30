/* $NetBSD: unordtf2.c,v 1.2 2014/01/30 19:11:41 matt Exp $ */
/** @file
*
*  Copyright (c) 2013 - 2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/
/*
 * Written by Richard Earnshaw, 2003.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: unordtf2.c,v 1.2 2014/01/30 19:11:41 matt Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef FLOAT128

flag __unordtf2(float128, float128);

flag
__unordtf2(float128 a, float128 b)
{
    /*
     * The comparison is unordered if either input is a NaN.
     * Test for this by comparing each operand with itself.
     * We must perform both comparisons to correctly check for
     * signalling NaNs.
     */
    return 1 ^ (float128_eq(a, a) & float128_eq(b, b));
}

#endif /* FLOAT128 */
