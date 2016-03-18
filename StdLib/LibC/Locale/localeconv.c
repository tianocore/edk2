/*  $NetBSD: localeconv.c,v 1.13 2005/11/29 03:11:59 christos Exp $ */

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: localeconv.c,v 1.13 2005/11/29 03:11:59 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/localedef.h>
#include <locale.h>

/*
 * The localeconv() function constructs a struct lconv from the current
 * monetary and numeric locales.
 *
 * Because localeconv() may be called many times (especially by library
 * routines like printf() & strtod()), the approprate members of the
 * lconv structure are computed only when the monetary or numeric
 * locale has been changed.
 */
int __mlocale_changed = 1;
int __nlocale_changed = 1;

/*
 * Return the current locale conversion.
 */
struct lconv *
localeconv()
{
  static struct lconv ret;

  if (__mlocale_changed) {
    /* LC_MONETARY */
    ret.int_curr_symbol =
        __UNCONST(_CurrentMonetaryLocale->int_curr_symbol);
    ret.currency_symbol =
        __UNCONST(_CurrentMonetaryLocale->currency_symbol);
    ret.mon_decimal_point =
        __UNCONST(_CurrentMonetaryLocale->mon_decimal_point);
    ret.mon_thousands_sep =
        __UNCONST(_CurrentMonetaryLocale->mon_thousands_sep);
    ret.mon_grouping =
        __UNCONST(_CurrentMonetaryLocale->mon_grouping);
    ret.positive_sign =
        __UNCONST(_CurrentMonetaryLocale->positive_sign);
    ret.negative_sign =
        __UNCONST(_CurrentMonetaryLocale->negative_sign);
    ret.int_frac_digits = _CurrentMonetaryLocale->int_frac_digits;
    ret.frac_digits = _CurrentMonetaryLocale->frac_digits;
    ret.p_cs_precedes = _CurrentMonetaryLocale->p_cs_precedes;
    ret.p_sep_by_space = _CurrentMonetaryLocale->p_sep_by_space;
    ret.n_cs_precedes = _CurrentMonetaryLocale->n_cs_precedes;
    ret.n_sep_by_space = _CurrentMonetaryLocale->n_sep_by_space;
    ret.p_sign_posn = _CurrentMonetaryLocale->p_sign_posn;
    ret.n_sign_posn = _CurrentMonetaryLocale->n_sign_posn;
    ret.int_p_cs_precedes =
        _CurrentMonetaryLocale->int_p_cs_precedes;
    ret.int_n_cs_precedes =
        _CurrentMonetaryLocale->int_n_cs_precedes;
    ret.int_p_sep_by_space =
        _CurrentMonetaryLocale->int_p_sep_by_space;
    ret.int_n_sep_by_space =
        _CurrentMonetaryLocale->int_n_sep_by_space;
    ret.int_p_sign_posn = _CurrentMonetaryLocale->int_p_sign_posn;
    ret.int_n_sign_posn = _CurrentMonetaryLocale->int_n_sign_posn;
    __mlocale_changed = 0;
  }

  if (__nlocale_changed) {
    /* LC_NUMERIC */
    ret.decimal_point =
        __UNCONST(_CurrentNumericLocale->decimal_point);
    ret.thousands_sep =
        __UNCONST(_CurrentNumericLocale->thousands_sep);
    ret.grouping =
        __UNCONST(_CurrentNumericLocale->grouping);
    __nlocale_changed = 0;
  }

  return (&ret);
}
