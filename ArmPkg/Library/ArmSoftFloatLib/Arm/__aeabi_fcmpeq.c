/* $NetBSD: __aeabi_fcmpeq.c,v 1.1 2013/04/16 10:37:39 matt Exp $ */

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

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_fcmpeq.c,v 1.1 2013/04/16 10:37:39 matt Exp $");
#endif /* LIBC_SCCS and not lint */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

int __aeabi_fcmpeq(float32, float32);

int
__aeabi_fcmpeq(float32 a, float32 b)
{

    return float32_eq(a, b);
}
