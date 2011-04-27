/* @(#)s_ldexp.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: s_ldexp.c,v 1.9 2002/05/26 22:01:56 wiz Exp $");
#endif

#include <errno.h>
#include "math.h"
#include "math_private.h"

double
ldexp(double value, int exp)
{
  if(!finite(value)||value==0.0) return value;
  value = scalbn(value,exp);
  if(!finite(value)||value==0.0) errno = ERANGE;
  return value;
}
