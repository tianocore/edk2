/*  $NetBSD: infinityf_ieee754.c,v 1.2 2005/06/12 05:21:27 lukem Exp $  */

/*
 * IEEE-compatible infinityf.c -- public domain.
 */
#include  <LibConfig.h>
#include <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: infinityf_ieee754.c,v 1.2 2005/06/12 05:21:27 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>
#include <machine/endian.h>

const union __float_u __infinityf =
#if BYTE_ORDER == BIG_ENDIAN
  { { 0x7f, 0x80,     0,    0 } };
#else
  { {    0,    0,  0x80, 0x7f } };
#endif
