/*  $NetBSD: fd_set.h,v 1.2 2005/12/11 12:25:20 christos Exp $  */

/*-
 * Copyright (c) 1992, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  from: @(#)types.h 8.4 (Berkeley) 1/21/94
 */

#ifndef _SYS_FD_SET_H_
#define _SYS_FD_SET_H_

#include  <sys/EfiCdefs.h>
#include  <machine/int_types.h>

/*
 * Implementation dependent defines, hidden from user space. X/Open does not
 * specify them.
 */
#define __NBBY    8   /* number of bits in a byte */
typedef __int32_t __fd_mask;

/* bits per mask */
#define __NFDBITS ((unsigned int)sizeof(__fd_mask) * __NBBY)

#define __howmany(x, y) (((x) + ((y) - 1)) / (y))

/*
 * Select uses bit masks of file descriptors in longs.  These macros
 * manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here should
 * be enough for most uses.
 */
#ifndef FD_SETSIZE
#define FD_SETSIZE  256
#endif

typedef struct fd_set {
  __fd_mask fds_bits[__howmany(FD_SETSIZE, __NFDBITS)];
} fd_set;

#define FD_SET(n, p)  \
    ((p)->fds_bits[(n)/__NFDBITS] |= (1 << ((n) % __NFDBITS)))
#define FD_CLR(n, p)  \
    ((p)->fds_bits[(n)/__NFDBITS] &= ~(1 << ((n) % __NFDBITS)))
#define FD_ISSET(n, p)  \
    ((p)->fds_bits[(n)/__NFDBITS] & (1 << ((n) % __NFDBITS)))
#if __GNUC_PREREQ__(2, 95)
#define FD_ZERO(p)  (void)__builtin_memset((p), 0, sizeof(*(p)))
#else
#define FD_ZERO(p)  do {            \
  fd_set *__fds = (p);            \
  unsigned int __i;           \
  for (__i = 0; __i < __howmany(FD_SETSIZE, __NFDBITS); __i++)  \
    __fds->fds_bits[__i] = 0;       \
  } while (/* CONSTCOND */ 0)
#endif /* GCC 2.95 */

/*
 * Expose our internals if we are not required to hide them.
 */
#if defined(_NETBSD_SOURCE)

#define fd_mask __fd_mask
#define NFDBITS __NFDBITS
#ifndef howmany
#define howmany(a, b) __howmany(a, b)
#endif

#if __GNUC_PREREQ__(2, 95)
#define FD_COPY(f, t) (void)__builtin_memcpy((t), (f), sizeof(*(f)))
#else
#define FD_COPY(f, t) do {            \
  fd_set *__f = (f), *__t = (t);          \
  unsigned int __i;           \
  for (__i = 0; __i < __howmany(FD_SETSIZE, __NFDBITS); __i++)  \
    __t->fds_bits[__i] = __f->fds_bits[__i];    \
  } while (/* CONSTCOND */ 0)
#endif /* GCC 2.95 */

#endif /* _NETBSD_SOURCE */

#endif /* _SYS_FD_SET_H_ */
