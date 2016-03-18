/** @file
    Wrapper for strtold so that it just calls strtod().  This is because the IPF implementation doesn't have
    long double.  (actually MS VC++ makes long double a distinct type that is identical to double.)  VC++
    also doesn't support the {strong, weak}_alias feature so we actually have to have an object.

**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  "namespace.h"
#include  "../gdtoaimp.h"
#include  "../gdtoa.h"

long double
strtold(const char * __restrict nptr, char ** __restrict endptr)
{
  return (long double)strtod( nptr, endptr);
}
