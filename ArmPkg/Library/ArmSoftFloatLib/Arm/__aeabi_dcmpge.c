/* $NetBSD: __aeabi_dcmpge.c,v 1.2 2013/04/16 13:38:34 matt Exp $ */
/** @file
*
*  Copyright (c) 2013 - 2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/
/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_dcmpge.c,v 1.2 2013/04/16 13:38:34 matt Exp $");
#endif /* LIBC_SCCS and not lint */

int __aeabi_dcmpge(float64, float64);

int
__aeabi_dcmpge(float64 a, float64 b)
{

    return !float64_lt(a, b) && float64_eq(a, a) && float64_eq(b, b);
}
