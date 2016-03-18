/*  $NetBSD: _def_time.c,v 1.8 2005/06/12 05:21:27 lukem Exp $  */

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _def_time.c,v 1.8 2005/06/12 05:21:27 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/localedef.h>
#include <locale.h>

const _TimeLocale _DefaultTimeLocale =
{
  {
    "Sun","Mon","Tue","Wed","Thu","Fri","Sat",
  },
  {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
    "Friday", "Saturday"
  },
  {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  },
  {
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"
  },
  {
    "AM", "PM"
  },
  "%a %b %e %H:%M:%S %Y",
  "%m/%d/%y",
  "%H:%M:%S",
  "%I:%M:%S %p"
};

const _TimeLocale *_CurrentTimeLocale = &_DefaultTimeLocale;
