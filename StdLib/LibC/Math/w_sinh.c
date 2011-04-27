/* @(#)w_sinh.c 5.1 93/09/24 */
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
__RCSID("$NetBSD: w_sinh.c,v 1.9 2002/05/26 22:02:03 wiz Exp $");
#endif

/*
 * wrapper sinh(x)
 */

#include "math.h"
#include "math_private.h"

double
sinh(double x)    /* wrapper sinh */
{
#ifdef _IEEE_LIBM
  return __ieee754_sinh(x);
#else
  double z;
  z = __ieee754_sinh(x);
  if(_LIB_VERSION == _IEEE_) return z;
  if(!finite(z)&&finite(x)) {
      return __kernel_standard(x,x,25); /* sinh overflow */
  } else
      return z;
#endif
}
