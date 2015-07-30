/* $NetBSD: nexf2.c,v 1.2 2004/09/27 10:16:24 he Exp $ */
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
__RCSID("$NetBSD: nexf2.c,v 1.2 2004/09/27 10:16:24 he Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef FLOATX80

flag __nexf2(floatx80, floatx80);

flag
__nexf2(floatx80 a, floatx80 b)
{

    /* libgcc1.c says a != b */
    return !floatx80_eq(a, b);
}
#endif /* FLOATX80 */
