/* $NetBSD: __aeabi_dcmpun.c,v 1.1 2013/04/16 10:37:39 matt Exp $ */

/** @file
*
*  Copyright (c) 2013 - 2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

/*
 * Written by Richard Earnshaw, 2003.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_dcmpun.c,v 1.1 2013/04/16 10:37:39 matt Exp $");
#endif /* LIBC_SCCS and not lint */

int __aeabi_dcmpun(float64, float64);

int
__aeabi_dcmpun(float64 a, float64 b)
{
    /*
     * The comparison is unordered if either input is a NaN.
     * Test for this by comparing each operand with itself.
     * We must perform both comparisons to correctly check for
     * signalling NaNs.
     */
    return !float64_eq(a, a) || !float64_eq(b, b);
}
