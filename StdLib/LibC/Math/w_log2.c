/* @(#)w_log10.c 5.1 93/09/24 */
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
__RCSID("$NetBSD: w_log2.c,v 1.1 2005/07/21 16:58:39 christos Exp $");
#endif

/*
 * wrapper log2(X)
 */

#include "math.h"
#include "math_private.h"


double
log2(double x)    /* wrapper log10 */
{
#ifdef _IEEE_LIBM
  return __ieee754_log2(x);
#else
  double z;
  z = __ieee754_log2(x);
  if(_LIB_VERSION == _IEEE_ || isnan(x)) return z;
  if(x<=0.0) {
      if(x==0.0)
          return __kernel_standard(x,x,48); /* log2(0) */
      else
          return __kernel_standard(x,x,49); /* log2(x<0) */
  } else
      return z;
#endif
}
