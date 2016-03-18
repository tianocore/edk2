/*  $NetBSD: _def_monetary.c,v 1.8 2005/06/12 05:21:27 lukem Exp $  */

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _def_monetary.c,v 1.8 2005/06/12 05:21:27 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/localedef.h>
#include <limits.h>
#include <locale.h>

const _MonetaryLocale _DefaultMonetaryLocale =
{
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX,
  (char)CHAR_MAX
};

const _MonetaryLocale *_CurrentMonetaryLocale = &_DefaultMonetaryLocale;
