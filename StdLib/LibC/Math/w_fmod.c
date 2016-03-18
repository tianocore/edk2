/* @(#)w_fmod.c 5.1 93/09/24 */
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
__RCSID("$NetBSD: w_fmod.c,v 1.9 2002/05/26 22:02:00 wiz Exp $");
#endif

/*
 * wrapper fmod(x,y)
 */

#include "math.h"
#include "math_private.h"


double
fmod(double x, double y)  /* wrapper fmod */
{
#ifdef _IEEE_LIBM
  return __ieee754_fmod(x,y);
#else
  double z;
  z = __ieee754_fmod(x,y);
  if(_LIB_VERSION == _IEEE_ ||isnan(y)||isnan(x)) return z;
  if(y==0.0) {
          return __kernel_standard(x,y,27); /* fmod(x,0) */
  } else
      return z;
#endif
}
