/*  $NetBSD: bswap32.c,v 1.1 2005/12/20 19:28:51 christos Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@NetBSD.org>.
 * Public domain.
 */

//#include <sys/cdefs.h>
//#if defined(LIBC_SCCS) && !defined(lint)
//__RCSID("$NetBSD: bswap32.c,v 1.1 2005/12/20 19:28:51 christos Exp $");
//#endif /* LIBC_SCCS and not lint */

//#include <sys/types.h>
//#include <machine/bswap.h>

#undef bswap32

UINT32
bswap32(UINT32 x)
{
  return  ((x << 24) & 0xff000000 ) |
    ((x <<  8) & 0x00ff0000 ) |
    ((x >>  8) & 0x0000ff00 ) |
    ((x >> 24) & 0x000000ff );
}
