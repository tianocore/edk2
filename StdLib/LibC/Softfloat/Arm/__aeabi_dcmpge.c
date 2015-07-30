/* $NetBSD: __aeabi_dcmpge.c,v 1.2 2013/04/16 13:38:34 matt Exp $ */
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
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_dcmpge.c,v 1.2 2013/04/16 13:38:34 matt Exp $");
#endif /* LIBC_SCCS and not lint */

int __aeabi_dcmpge(float64, float64);

int
__aeabi_dcmpge(float64 a, float64 b)
{

    return !float64_lt(a, b) && float64_eq(a, a) && float64_eq(b, b);
}
