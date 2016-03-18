/*  $NetBSD: bswap64.c,v 1.1 2005/12/20 19:28:51 christos Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@NetBSD.org>.
 * Public domain.
 */

//#include <sys/cdefs.h>
//#if defined(LIBC_SCCS) && !defined(lint)
//__RCSID("$NetBSD: bswap64.c,v 1.1 2005/12/20 19:28:51 christos Exp $");
//#endif /* LIBC_SCCS and not lint */

//#include <sys/types.h>
//#include <machine/bswap.h>

#undef bswap64

UINT64
bswap64(UINT64 x)
{
#ifndef _LP64
  /*
   * Assume we have wide enough registers to do it without touching
   * memory.
   */
  return  ( (x << 56) & 0xff00000000000000UL ) |
    ( (x << 40) & 0x00ff000000000000UL ) |
    ( (x << 24) & 0x0000ff0000000000UL ) |
    ( (x <<  8) & 0x000000ff00000000UL ) |
    ( (x >>  8) & 0x00000000ff000000UL ) |
    ( (x >> 24) & 0x0000000000ff0000UL ) |
    ( (x >> 40) & 0x000000000000ff00UL ) |
    ( (x >> 56) & 0x00000000000000ffUL );
#else
  /*
   * Split the operation in two 32bit steps.
   */
  u_int32_t tl, th;

  th = bswap32((u_int32_t)(x & 0x00000000ffffffffULL));
  tl = bswap32((u_int32_t)((x >> 32) & 0x00000000ffffffffULL));
  return ((u_int64_t)th << 32) | tl;
#endif
}
