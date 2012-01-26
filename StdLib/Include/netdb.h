/** @file

    Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1980, 1983, 1988, 1993
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
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * Portions Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by WIDE Project and
 *    its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * -
 *      @(#)netdb.h 8.1 (Berkeley) 6/2/93
 *      netdb.h,v 1.15.18.6 2006/10/02 01:23:09 marka Exp
 *      netdb.h,v 1.55.2.1 2007/05/17 21:25:10 jdc Exp
 */
#ifndef _NETDB_H_
#define _NETDB_H_

#include <machine/ansi.h>
#include <machine/endian_machdep.h>
#include <sys/ansi.h>
#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <inttypes.h>
#include <paths.h>

/* Data types */
#ifndef socklen_t
typedef __socklen_t socklen_t;
#define socklen_t   __socklen_t
#endif

#ifdef _EFI_SIZE_T_
  typedef _EFI_SIZE_T_  size_t;
  #undef  _EFI_SIZE_T_
  #undef  _BSD_SIZE_T_
#endif

__BEGIN_DECLS
extern int h_errno;
__END_DECLS

/** Structures returned by network data base library.  All addresses are
 *  supplied in host order, and returned in network order (suitable for
 *  use in system calls).
 */
struct  hostent {
  char   *h_name;       /**< official name of host */
  char  **h_aliases;    /**< alias list */
  int     h_addrtype;   /**< host address type */
  int     h_length;     /**< length of address */
  char  **h_addr_list;  /**< list of addresses from name server */
};
#define h_addr  h_addr_list[0]  /**< address, for backward compatiblity */

/** Assumption here is that a network number
 *  fits in an unsigned long -- probably a poor one.
 */
struct  netent {
  char     *n_name;     /**< official name of net */
  char    **n_aliases;  /**< alias list */
  int       n_addrtype; /**< net address type */
  uint32_t  n_net;      /**< network # */
#if defined(__alpha__) || (defined(__i386__) && defined(_LP64)) || \
    (defined(__sh__) && defined(_LP64) && (_BYTE_ORDER == _LITTLE_ENDIAN))
  int       __n_pad0;   /* ABI compatibility */
#endif
};

struct  servent {
  char   *s_name;     /**< official service name */
  char  **s_aliases;  /**< alias list */
  int     s_port;     /**< port # */
  char   *s_proto;    /**< protocol to use */
};

struct  protoent {
  char   *p_name;     /**< official protocol name */
  char  **p_aliases;  /**< alias list */
  int     p_proto;    /**< protocol # */
};

/* Note: ai_addrlen used to be a size_t, per RFC 2553.
 * In XNS5.2, and subsequently in POSIX-2001 and
 * draft-ietf-ipngwg-rfc2553bis-02.txt it was changed to a socklen_t.
 * To accommodate for this while preserving binary compatibility with the
 * old interface, we prepend or append 32 bits of padding, depending on
 * the (LP64) architecture's endianness.
 *
 * This should be deleted the next time the libc major number is
 * incremented.
 */
struct addrinfo {
  int               ai_flags;     /**< AI_PASSIVE, AI_CANONNAME */
  int               ai_family;    /**< PF_xxx */
  int               ai_socktype;  /**< SOCK_xxx */
  int               ai_protocol;  /**< 0 or IPPROTO_xxx for IPv4 and IPv6 */
  socklen_t         ai_addrlen;   /**< length of ai_addr */
#if defined(__alpha__) || (defined(__i386__) && defined(_LP64))
  int               __ai_pad0;    /* ABI compatibility */
#endif
  char             *ai_canonname; /**< canonical name for hostname */
  struct sockaddr  *ai_addr;      /**< binary address */
  struct addrinfo  *ai_next;      /**< next structure in linked list */
};

/* Error return codes from gethostbyname() and gethostbyaddr()
 * (left in extern int h_errno).
*/

#define NETDB_INTERNAL   -1         /**< see errno */
#define NETDB_SUCCESS     0         /**< no problem */
#define NO_ADDRESS        NO_DATA   /* no address, look for MX record */
#define HOST_NOT_FOUND    1         /**< Authoritative Answer Host not found */
#define TRY_AGAIN         2         /**< Non-Authoritive Host not found, or SERVERFAIL */
#define NO_RECOVERY       3         /**< Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define NO_DATA           4         /**< Valid name, no data record of requested type */

/* Error return codes from getaddrinfo() */
#define EAI_ADDRFAMILY    1   /**< address family for hostname not supported */
#define EAI_AGAIN         2   /**< temporary failure in name resolution */
#define EAI_BADFLAGS      3   /**< invalid value for ai_flags */
#define EAI_FAIL          4   /**< non-recoverable failure in name resolution */
#define EAI_FAMILY        5   /**< ai_family not supported */
#define EAI_MEMORY        6   /**< memory allocation failure */
#define EAI_NODATA        7   /**< no address associated with hostname */
#define EAI_NONAME        8   /**< hostname nor servname provided, or not known */
#define EAI_SERVICE       9   /**< servname not supported for ai_socktype */
#define EAI_SOCKTYPE      10  /**< ai_socktype not supported */
#define EAI_SYSTEM        11  /**< system error returned in errno */
#define EAI_BADHINTS      12  /**< invalid value for hints */
#define EAI_PROTOCOL      13  /**< resolved protocol is unknown */
#define EAI_OVERFLOW      14  /**< argument buffer overflow */
#define EAI_MAX           15

/* Flag values for getaddrinfo() */
#define AI_PASSIVE      0x00000001  /* get address to use bind() */
#define AI_CANONNAME    0x00000002  /* fill ai_canonname */

#define AI_NUMERICHOST  0x00000004  /* prevent host name resolution */
#define AI_NUMERICSERV  0x00000008  /* prevent service name resolution */
/* valid flags for addrinfo (not a standard def, apps should not use it) */
#define AI_MASK         (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST | AI_NUMERICSERV)

/* Constants for getnameinfo() */
#define NI_MAXHOST      1025
#define NI_MAXSERV      32

/* Flag values for getnameinfo() */
#define NI_NOFQDN       0x00000001
#define NI_NUMERICHOST  0x00000002
#define NI_NAMEREQD     0x00000004
#define NI_NUMERICSERV  0x00000008
#define NI_DGRAM        0x00000010
#define NI_NUMERICSCOPE 0x00000040

/** Scope delimit character */
#define SCOPE_DELIMITER '%'

__BEGIN_DECLS
void              endhostent      (void);
void              endnetent       (void);
void              endprotoent     (void);
void              endservent      (void);
struct hostent   *gethostbyaddr   (const char *, socklen_t, int);
struct hostent   *gethostbyname   (const char *);
struct hostent   *gethostbyname2  (const char *, int);
struct hostent   *gethostent      (void);
struct netent    *getnetbyaddr    (uint32_t, int);
struct netent    *getnetbyname    (const char *);
struct netent    *getnetent       (void);
struct protoent  *getprotobyname  (const char *);
struct protoent  *getprotobynumber(int);
struct protoent  *getprotoent     (void);
struct servent   *getservbyname   (const char *, const char *);
struct servent   *getservbyport   (int, const char *);
struct servent   *getservent      (void);
void              sethostent      (int);
void              setnetent       (int);
void              setprotoent     (int);
void              setservent      (int);
int               getaddrinfo     ( const char * __restrict, const char * __restrict,
                                    const struct addrinfo * __restrict,
                                    struct addrinfo ** __restrict);
int               getnameinfo     ( const struct sockaddr * __restrict, socklen_t,
                                    char * __restrict, socklen_t,
                                    char * __restrict, socklen_t, int);
void              freeaddrinfo    (struct addrinfo *);
const char       *gai_strerror    (int);
void              setservent      (int);

__END_DECLS

#endif /* !_NETDB_H_ */
