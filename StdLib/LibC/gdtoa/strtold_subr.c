/* $NetBSD: strtold_subr.c,v 1.1 2006/03/15 17:35:18 kleink Exp $ */

/*
 * Written by Klaus Klein <kleink@NetBSD.org>, November 16, 2005.
 * Public domain.
 */

/*
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * the format-specific strtold_*.c, like so:
 *
 *  #define GDTOA_LD_FMT  <gdtoa extended-precision format code>
 *  #include "strtold_subr.c"
 */
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  "namespace.h"
#include  <math.h>
#include  <sys/stdint.h>
#include  <stdlib.h>
#include  "gdtoa.h"

#ifdef __weak_alias
__weak_alias(strtold, _strtold)
#endif

#ifndef __HAVE_LONG_DOUBLE
#error no extended-precision long double type
#endif

#ifndef GDTOA_LD_FMT
#error GDTOA_LD_FMT must be defined by format-specific source file
#endif

#define STRTOP(x) __CONCAT(strtop, x)

long double
strtold(const char *nptr, char **endptr)
{
  long double ld;

  (void)STRTOP(GDTOA_LD_FMT)(nptr, endptr, &ld);
  return ld;
}
