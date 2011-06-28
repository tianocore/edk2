/*  $NetBSD: in6.h,v 1.57 2006/10/31 00:29:30 cbiere Exp $  */
/*  $KAME: in6.h,v 1.83 2001/03/29 02:55:07 jinmei Exp $  */

/*
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
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
 */

/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *  @(#)in.h  8.3 (Berkeley) 1/3/94
 */

#ifndef _NETINET6_IN6_H_
#define _NETINET6_IN6_H_

#ifndef __KAME_NETINET_IN_H_INCLUDED_
#error "do not include netinet6/in6.h directly, include netinet/in.h.  see RFC2553"
#endif

#include <sys/socket.h>

/*
 * Identification of the network protocol stack
 * for *BSD-current/release: http://www.kame.net/dev/cvsweb.cgi/kame/COVERAGE
 * has the table of implementation/integration differences.
 */
#define __KAME__
#define __KAME_VERSION    "NetBSD-current"

/*
 * Local port number conventions:
 *
 * Ports < IPPORT_RESERVED are reserved for privileged processes (e.g. root),
 * unless a kernel is compiled with IPNOPRIVPORTS defined.
 *
 * When a user does a bind(2) or connect(2) with a port number of zero,
 * a non-conflicting local port address is chosen.
 *
 * The default range is IPPORT_ANONMIN to IPPORT_ANONMAX, although
 * that is settable by sysctl(3); net.inet.ip.anonportmin and
 * net.inet.ip.anonportmax respectively.
 *
 * A user may set the IPPROTO_IP option IP_PORTRANGE to change this
 * default assignment range.
 *
 * The value IP_PORTRANGE_DEFAULT causes the default behavior.
 *
 * The value IP_PORTRANGE_HIGH is the same as IP_PORTRANGE_DEFAULT,
 * and exists only for FreeBSD compatibility purposes.
 *
 * The value IP_PORTRANGE_LOW changes the range to the "low" are
 * that is (by convention) restricted to privileged processes.
 * This convention is based on "vouchsafe" principles only.
 * It is only secure if you trust the remote host to restrict these ports.
 * The range is IPPORT_RESERVEDMIN to IPPORT_RESERVEDMAX.
 */

#if defined(_NETBSD_SOURCE)
#define IPV6PORT_RESERVED 1024
#define IPV6PORT_ANONMIN  49152
#define IPV6PORT_ANONMAX  65535
#define IPV6PORT_RESERVEDMIN  600
#define IPV6PORT_RESERVEDMAX  (IPV6PORT_RESERVED-1)
#endif

/*
 * IPv6 address
 */
struct in6_addr {
  union {
    __uint8_t   __u6_addr8[16];
    __uint16_t  __u6_addr16[8];
    uint32_t  __u6_addr32[4];
  } __u6_addr;      /* 128-bit IP6 address */
};

#define s6_addr   __u6_addr.__u6_addr8
#ifdef _KERNEL  /* XXX nonstandard */
#define s6_addr8  __u6_addr.__u6_addr8
#define s6_addr16 __u6_addr.__u6_addr16
#define s6_addr32 __u6_addr.__u6_addr32
#endif

#define INET6_ADDRSTRLEN  46

/*
 * Socket address for IPv6
 */
#if defined(_NETBSD_SOURCE)
#define SIN6_LEN
#endif
struct sockaddr_in6 {
  uint8_t   sin6_len; /* length of this struct(socklen_t)*/
  sa_family_t sin6_family;  /* AF_INET6 (sa_family_t) */
  in_port_t sin6_port;  /* Transport layer port */
  uint32_t  sin6_flowinfo;  /* IP6 flow information */
  struct in6_addr sin6_addr;  /* IP6 address */
  uint32_t  sin6_scope_id;  /* scope zone index */
};

/*
 * Local definition for masks
 */
#ifdef _KERNEL  /* XXX nonstandard */
#define IN6MASK0  {{{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }}}
#define IN6MASK32 {{{ 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, \
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6MASK64 {{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6MASK96 {{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
          0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6MASK128  {{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }}}
#endif

#ifdef _KERNEL
extern const struct sockaddr_in6 sa6_any;

extern const struct in6_addr in6mask0;
extern const struct in6_addr in6mask32;
extern const struct in6_addr in6mask64;
extern const struct in6_addr in6mask96;
extern const struct in6_addr in6mask128;
#endif /* _KERNEL */

/*
 * Macros started with IPV6_ADDR is KAME local
 */
#ifdef _KERNEL  /* XXX nonstandard */
#if BYTE_ORDER == BIG_ENDIAN
#define IPV6_ADDR_INT32_ONE 1
#define IPV6_ADDR_INT32_TWO 2
#define IPV6_ADDR_INT32_MNL 0xff010000
#define IPV6_ADDR_INT32_MLL 0xff020000
#define IPV6_ADDR_INT32_SMP 0x0000ffff
#define IPV6_ADDR_INT16_ULL 0xfe80
#define IPV6_ADDR_INT16_USL 0xfec0
#define IPV6_ADDR_INT16_MLL 0xff02
#elif BYTE_ORDER == LITTLE_ENDIAN
#define IPV6_ADDR_INT32_ONE 0x01000000
#define IPV6_ADDR_INT32_TWO 0x02000000
#define IPV6_ADDR_INT32_MNL 0x000001ff
#define IPV6_ADDR_INT32_MLL 0x000002ff
#define IPV6_ADDR_INT32_SMP 0xffff0000
#define IPV6_ADDR_INT16_ULL 0x80fe
#define IPV6_ADDR_INT16_USL 0xc0fe
#define IPV6_ADDR_INT16_MLL 0x02ff
#endif
#endif

/*
 * Definition of some useful macros to handle IP6 addresses
 */
#define IN6ADDR_ANY_INIT \
  {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6ADDR_LOOPBACK_INIT \
  {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#define IN6ADDR_NODELOCAL_ALLNODES_INIT \
  {{{ 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#define IN6ADDR_LINKLOCAL_ALLNODES_INIT \
  {{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#define IN6ADDR_LINKLOCAL_ALLROUTERS_INIT \
  {{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 }}}

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;
extern const struct in6_addr in6addr_nodelocal_allnodes;
extern const struct in6_addr in6addr_linklocal_allnodes;
extern const struct in6_addr in6addr_linklocal_allrouters;

/*
 * Equality
 * NOTE: Some of kernel programming environment (for example, openbsd/sparc)
 * does not supply memcmp().  For userland memcmp() is preferred as it is
 * in ANSI standard.
 */
#ifdef _KERNEL
#define IN6_ARE_ADDR_EQUAL(a, b)      \
    (bcmp(&(a)->s6_addr[0], &(b)->s6_addr[0], sizeof(struct in6_addr)) == 0)
#else
#define IN6_ARE_ADDR_EQUAL(a, b)      \
    (memcmp(&(a)->s6_addr[0], &(b)->s6_addr[0], sizeof(struct in6_addr)) == 0)
#endif

/*
 * Unspecified
 */
#define IN6_IS_ADDR_UNSPECIFIED(a)  \
  ((*(const uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[8]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[12]) == 0))

/*
 * Loopback
 */
#define IN6_IS_ADDR_LOOPBACK(a)   \
  ((*(const uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[8]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[12]) == ntohl(1)))

/*
 * IPv4 compatible
 */
#define IN6_IS_ADDR_V4COMPAT(a)   \
  ((*(const uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[8]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[12]) != 0) && \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[12]) != ntohl(1)))

/*
 * Mapped
 */
#define IN6_IS_ADDR_V4MAPPED(a)         \
  ((*(const uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) &&  \
   (*(const uint32_t *)(const void *)(&(a)->s6_addr[8]) == ntohl(0x0000ffff)))

/*
 * KAME Scope Values
 */

#ifdef _KERNEL  /* XXX nonstandard */
#define IPV6_ADDR_SCOPE_NODELOCAL 0x01
#define IPV6_ADDR_SCOPE_INTFACELOCAL  0x01
#define IPV6_ADDR_SCOPE_LINKLOCAL 0x02
#define IPV6_ADDR_SCOPE_SITELOCAL 0x05
#define IPV6_ADDR_SCOPE_ORGLOCAL  0x08  /* just used in this file */
#define IPV6_ADDR_SCOPE_GLOBAL    0x0e
#else
#define __IPV6_ADDR_SCOPE_NODELOCAL 0x01
#define __IPV6_ADDR_SCOPE_LINKLOCAL 0x02
#define __IPV6_ADDR_SCOPE_SITELOCAL 0x05
#define __IPV6_ADDR_SCOPE_ORGLOCAL  0x08  /* just used in this file */
#define __IPV6_ADDR_SCOPE_GLOBAL  0x0e
#endif

/*
 * Unicast Scope
 * Note that we must check topmost 10 bits only, not 16 bits (see RFC2373).
 */
#define IN6_IS_ADDR_LINKLOCAL(a)  \
  (((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0x80))
#define IN6_IS_ADDR_SITELOCAL(a)  \
  (((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0xc0))

/*
 * Multicast
 */
#define IN6_IS_ADDR_MULTICAST(a)  ((a)->s6_addr[0] == 0xff)

#ifdef _KERNEL  /* XXX nonstandard */
#define IPV6_ADDR_MC_SCOPE(a)   ((a)->s6_addr[1] & 0x0f)
#else
#define __IPV6_ADDR_MC_SCOPE(a)   ((a)->s6_addr[1] & 0x0f)
#endif

/*
 * Multicast Scope
 */
#ifdef _KERNEL  /* refers nonstandard items */
#define IN6_IS_ADDR_MC_NODELOCAL(a) \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_NODELOCAL))
#define IN6_IS_ADDR_MC_INTFACELOCAL(a)  \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_INTFACELOCAL))
#define IN6_IS_ADDR_MC_LINKLOCAL(a) \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_LINKLOCAL))
#define IN6_IS_ADDR_MC_SITELOCAL(a) \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_SITELOCAL))
#define IN6_IS_ADDR_MC_ORGLOCAL(a)  \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_ORGLOCAL))
#define IN6_IS_ADDR_MC_GLOBAL(a)  \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_GLOBAL))
#else
#define IN6_IS_ADDR_MC_NODELOCAL(a) \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_NODELOCAL))
#define IN6_IS_ADDR_MC_LINKLOCAL(a) \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_LINKLOCAL))
#define IN6_IS_ADDR_MC_SITELOCAL(a) \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_SITELOCAL))
#define IN6_IS_ADDR_MC_ORGLOCAL(a)  \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_ORGLOCAL))
#define IN6_IS_ADDR_MC_GLOBAL(a)  \
  (IN6_IS_ADDR_MULTICAST(a) &&  \
   (__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_GLOBAL))
#endif

#ifdef _KERNEL  /* nonstandard */
/*
 * KAME Scope
 */
#define IN6_IS_SCOPE_LINKLOCAL(a) \
  ((IN6_IS_ADDR_LINKLOCAL(a)) ||  \
   (IN6_IS_ADDR_MC_LINKLOCAL(a)))

#define IN6_IS_SCOPE_EMBEDDABLE(__a)  \
    (IN6_IS_SCOPE_LINKLOCAL(__a) || IN6_IS_ADDR_MC_INTFACELOCAL(__a))

#define IFA6_IS_DEPRECATED(a) \
  ((a)->ia6_lifetime.ia6t_pltime != ND6_INFINITE_LIFETIME && \
   (u_int32_t)((time_second - (a)->ia6_updatetime)) > \
   (a)->ia6_lifetime.ia6t_pltime)
#define IFA6_IS_INVALID(a) \
  ((a)->ia6_lifetime.ia6t_vltime != ND6_INFINITE_LIFETIME && \
   (u_int32_t)((time_second - (a)->ia6_updatetime)) > \
   (a)->ia6_lifetime.ia6t_vltime)
#endif

/*
 * IP6 route structure
 */
#if defined(_NETBSD_SOURCE)
struct route_in6 {
  struct  rtentry *ro_rt;
  struct  sockaddr_in6 ro_dst;
};
#endif

/*
 * Options for use with [gs]etsockopt at the IPV6 level.
 * First word of comment is data type; bool is stored in int.
 */
/* no hdrincl */
#if 0
/* These are deprecated non-standard options which are no longer supported. */
#define IPV6_OPTIONS    1  /* buf/ip6_opts; set/get IP6 options */
#define IPV6_RECVOPTS   5  /* bool; receive all IP6 opts w/dgram */
#define IPV6_RECVRETOPTS  6  /* bool; receive IP6 opts for response */
#define IPV6_RECVDSTADDR  7  /* bool; receive IP6 dst addr w/dgram */
#define IPV6_RETOPTS    8  /* ip6_opts; set/get IP6 options */
#endif
#define IPV6_SOCKOPT_RESERVED1  3  /* reserved for future use */
#define IPV6_UNICAST_HOPS 4  /* int; IP6 hops */
#define IPV6_MULTICAST_IF 9  /* u_int; set/get IP6 multicast i/f  */
#define IPV6_MULTICAST_HOPS 10 /* int; set/get IP6 multicast hops */
#define IPV6_MULTICAST_LOOP 11 /* u_int; set/get IP6 multicast loopback */
#define IPV6_JOIN_GROUP   12 /* ip6_mreq; join a group membership */
#define IPV6_LEAVE_GROUP  13 /* ip6_mreq; leave a group membership */
#define IPV6_PORTRANGE    14 /* int; range to choose for unspec port */
#if defined(_NETBSD_SOURCE)
#define ICMP6_FILTER    18 /* icmp6_filter; icmp6 filter */
#endif
/* RFC2292 options */
#ifdef _KERNEL
#define IPV6_2292PKTINFO  19 /* bool; send/recv if, src/dst addr */
#define IPV6_2292HOPLIMIT 20 /* bool; hop limit */
#define IPV6_2292NEXTHOP  21 /* bool; next hop addr */
#define IPV6_2292HOPOPTS  22 /* bool; hop-by-hop option */
#define IPV6_2292DSTOPTS  23 /* bool; destinaion option */
#define IPV6_2292RTHDR    24 /* bool; routing header */
#define IPV6_2292PKTOPTIONS 25 /* buf/cmsghdr; set/get IPv6 options */
#endif
#define IPV6_CHECKSUM   26 /* int; checksum offset for raw socket */
#define IPV6_V6ONLY   27 /* bool; make AF_INET6 sockets v6 only */

#if 1 /* IPSEC */
#define IPV6_IPSEC_POLICY 28 /* struct; get/set security policy */
#endif
#define IPV6_FAITH    29 /* bool; accept FAITH'ed connections */

/* new socket options introduced in RFC3542 */
#define IPV6_RTHDRDSTOPTS       35 /* ip6_dest; send dst option before rthdr */

#define IPV6_RECVPKTINFO        36 /* bool; recv if, dst addr */
#define IPV6_RECVHOPLIMIT       37 /* bool; recv hop limit */
#define IPV6_RECVRTHDR          38 /* bool; recv routing header */
#define IPV6_RECVHOPOPTS        39 /* bool; recv hop-by-hop option */
#define IPV6_RECVDSTOPTS        40 /* bool; recv dst option after rthdr */
#ifdef _KERNEL
#define IPV6_RECVRTHDRDSTOPTS   41 /* bool; recv dst option before rthdr */
#endif
#define IPV6_USE_MIN_MTU  42 /* bool; send packets at the minimum MTU */
#define IPV6_RECVPATHMTU  43 /* bool; notify an according MTU */
#define IPV6_PATHMTU    44 /* mtuinfo; get the current path MTU (sopt),
              4 bytes int; MTU notification (cmsg) */

/* more new socket options introduced in RFC3542 */
#define IPV6_PKTINFO    46 /* in6_pktinfo; send if, src addr */
#define IPV6_HOPLIMIT   47 /* int; send hop limit */
#define IPV6_NEXTHOP    48 /* sockaddr; next hop addr */
#define IPV6_HOPOPTS    49 /* ip6_hbh; send hop-by-hop option */
#define IPV6_DSTOPTS    50 /* ip6_dest; send dst option befor rthdr */
#define IPV6_RTHDR    51 /* ip6_rthdr; send routing header */

#define IPV6_RECVTCLASS   57 /* bool; recv traffic class values */
#ifdef _KERNEL
#define IPV6_OTCLASS    58 /* u_int8_t; send traffic class value */
#endif

#define IPV6_TCLASS   61 /* int; send traffic class value */
#define IPV6_DONTFRAG   62 /* bool; disable IPv6 fragmentation */
/* to define items, should talk with KAME guys first, for *BSD compatibility */

#define IPV6_RTHDR_LOOSE     0 /* this hop need not be a neighbor. XXX old spec */
#define IPV6_RTHDR_STRICT    1 /* this hop must be a neighbor. XXX old spec */
#define IPV6_RTHDR_TYPE_0    0 /* IPv6 routing header type 0 */

/*
 * Defaults and limits for options
 */
#define IPV6_DEFAULT_MULTICAST_HOPS 1 /* normally limit m'casts to 1 hop  */
#define IPV6_DEFAULT_MULTICAST_LOOP 1 /* normally hear sends if a member  */

/*
 * Argument structure for IPV6_JOIN_GROUP and IPV6_LEAVE_GROUP.
 */
struct ipv6_mreq {
  struct in6_addr ipv6mr_multiaddr;
  unsigned int  ipv6mr_interface;
};

/*
 * IPV6_PKTINFO: Packet information(RFC2292 sec 5)
 */
struct in6_pktinfo {
  struct in6_addr ipi6_addr;  /* src/dst IPv6 address */
  unsigned int  ipi6_ifindex; /* send/recv interface index */
};

/*
 * Control structure for IPV6_RECVPATHMTU socket option.
 */
struct ip6_mtuinfo {
  struct sockaddr_in6 ip6m_addr;  /* or sockaddr_storage? */
  uint32_t ip6m_mtu;
};

/*
 * Argument for IPV6_PORTRANGE:
 * - which range to search when port is unspecified at bind() or connect()
 */
#define IPV6_PORTRANGE_DEFAULT  0 /* default range */
#define IPV6_PORTRANGE_HIGH 1 /* "high" - request firewall bypass */
#define IPV6_PORTRANGE_LOW  2 /* "low" - vouchsafe security */

#if defined(_NETBSD_SOURCE)
/*
 * Definitions for inet6 sysctl operations.
 *
 * Third level is protocol number.
 * Fourth level is desired variable within that protocol.
 */
#define IPV6PROTO_MAXID (IPPROTO_PIM + 1) /* don't list to IPV6PROTO_MAX */

#define CTL_IPV6PROTO_NAMES { \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, \
  { "tcp6", CTLTYPE_NODE }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { "udp6", CTLTYPE_NODE }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, \
  { "ip6", CTLTYPE_NODE }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, \
  { "ipsec6", CTLTYPE_NODE }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { "icmp6", CTLTYPE_NODE }, \
  { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { "pim6", CTLTYPE_NODE }, \
}

/*
 * Names for IP sysctl objects
 */
#define IPV6CTL_FORWARDING  1 /* act as router */
#define IPV6CTL_SENDREDIRECTS 2 /* may send redirects when forwarding*/
#define IPV6CTL_DEFHLIM   3 /* default Hop-Limit */
#ifdef notyet
#define IPV6CTL_DEFMTU    4 /* default MTU */
#endif
#define IPV6CTL_FORWSRCRT 5 /* forward source-routed dgrams */
#define IPV6CTL_STATS   6 /* stats */
#define IPV6CTL_MRTSTATS  7 /* multicast forwarding stats */
#define IPV6CTL_MRTPROTO  8 /* multicast routing protocol */
#define IPV6CTL_MAXFRAGPACKETS  9 /* max packets reassembly queue */
#define IPV6CTL_SOURCECHECK 10  /* verify source route and intf */
#define IPV6CTL_SOURCECHECK_LOGINT 11 /* minimume logging interval */
#define IPV6CTL_ACCEPT_RTADV  12
#define IPV6CTL_KEEPFAITH 13
#define IPV6CTL_LOG_INTERVAL  14
#define IPV6CTL_HDRNESTLIMIT  15
#define IPV6CTL_DAD_COUNT 16
#define IPV6CTL_AUTO_FLOWLABEL  17
#define IPV6CTL_DEFMCASTHLIM  18
#define IPV6CTL_GIF_HLIM  19  /* default HLIM for gif encap packet */
#define IPV6CTL_KAME_VERSION  20
#define IPV6CTL_USE_DEPRECATED  21  /* use deprecated addr (RFC2462 5.5.4) */
#define IPV6CTL_RR_PRUNE  22  /* walk timer for router renumbering */
/* 23: reserved */
#define IPV6CTL_V6ONLY    24
/* 25 to 27: reserved */
#define IPV6CTL_ANONPORTMIN 28  /* minimum ephemeral port */
#define IPV6CTL_ANONPORTMAX 29  /* maximum ephemeral port */
#define IPV6CTL_LOWPORTMIN  30  /* minimum reserved port */
#define IPV6CTL_LOWPORTMAX  31  /* maximum reserved port */
/* 32 to 38: reserved */
#define IPV6CTL_USE_DEFAULTZONE 39  /* use default scope zone */
/* 40: reserved */
#define IPV6CTL_MAXFRAGS  41  /* max fragments */
#define IPV6CTL_IFQ   42  /* ip6intrq node */
/* New entries should be added here from current IPV6CTL_MAXID value. */
/* to define items, should talk with KAME guys first, for *BSD compatibility */
#define IPV6CTL_MAXID   43

#define IPV6CTL_NAMES { \
  { 0, 0 }, \
  { "forwarding", CTLTYPE_INT }, \
  { "redirect", CTLTYPE_INT }, \
  { "hlim", CTLTYPE_INT }, \
  { "mtu", CTLTYPE_INT }, \
  { "forwsrcrt", CTLTYPE_INT }, \
  { "stats", CTLTYPE_STRUCT }, \
  { 0, 0 }, \
  { "mrtproto", CTLTYPE_INT }, \
  { "maxfragpackets", CTLTYPE_INT }, \
  { "sourcecheck", CTLTYPE_INT }, \
  { "sourcecheck_logint", CTLTYPE_INT }, \
  { "accept_rtadv", CTLTYPE_INT }, \
  { "keepfaith", CTLTYPE_INT }, \
  { "log_interval", CTLTYPE_INT }, \
  { "hdrnestlimit", CTLTYPE_INT }, \
  { "dad_count", CTLTYPE_INT }, \
  { "auto_flowlabel", CTLTYPE_INT }, \
  { "defmcasthlim", CTLTYPE_INT }, \
  { "gifhlim", CTLTYPE_INT }, \
  { "kame_version", CTLTYPE_STRING }, \
  { "use_deprecated", CTLTYPE_INT }, \
  { "rr_prune", CTLTYPE_INT }, \
  { 0, 0 }, \
  { "v6only", CTLTYPE_INT }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { "anonportmin", CTLTYPE_INT }, \
  { "anonportmax", CTLTYPE_INT }, \
  { "lowportmin", CTLTYPE_INT }, \
  { "lowportmax", CTLTYPE_INT }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { 0, 0 }, \
  { "maxfrags", CTLTYPE_INT }, \
  { "ifq", CTLTYPE_NODE }, \
}

#endif /* _NETBSD_SOURCE */

#ifdef _KERNEL
struct cmsghdr;

/*
 * in6_cksum_phdr:
 *
 *  Compute significant parts of the IPv6 checksum pseudo-header
 *  for use in a delayed TCP/UDP checksum calculation.
 *
 *  Args:
 *
 *    src   Source IPv6 address
 *    dst   Destination IPv6 address
 *    len   htonl(proto-hdr-len)
 *    nxt   htonl(next-proto-number)
 *
 *  NOTE: We expect the src and dst addresses to be 16-bit
 *  aligned!
 */
static __inline u_int16_t __attribute__((__unused__))
in6_cksum_phdr(const struct in6_addr *src, const struct in6_addr *dst,
    u_int32_t len, u_int32_t nxt)
{
  u_int32_t sum = 0;
  const u_int16_t *w;

  /*LINTED*/
  w = (const u_int16_t *) src;
  sum += w[0];
  if (!IN6_IS_SCOPE_LINKLOCAL(src))
    sum += w[1];
  sum += w[2]; sum += w[3]; sum += w[4]; sum += w[5];
  sum += w[6]; sum += w[7];

  /*LINTED*/
  w = (const u_int16_t *) dst;
  sum += w[0];
  if (!IN6_IS_SCOPE_LINKLOCAL(dst))
    sum += w[1];
  sum += w[2]; sum += w[3]; sum += w[4]; sum += w[5];
  sum += w[6]; sum += w[7];

  sum += (u_int16_t)(len >> 16) + (u_int16_t)(len /*& 0xffff*/);

  sum += (u_int16_t)(nxt >> 16) + (u_int16_t)(nxt /*& 0xffff*/);

  sum = (u_int16_t)(sum >> 16) + (u_int16_t)(sum /*& 0xffff*/);

  if (sum > 0xffff)
    sum -= 0xffff;

  return (sum);
}

struct mbuf;
struct ifnet;
int in6_cksum __P((struct mbuf *, u_int8_t, u_int32_t, u_int32_t));
void  in6_delayed_cksum __P((struct mbuf *));
int in6_localaddr __P((struct in6_addr *));
int in6_addrscope __P((struct in6_addr *));
struct  in6_ifaddr *in6_ifawithifp __P((struct ifnet *, struct in6_addr *));
extern void in6_if_up __P((struct ifnet *));
#ifndef __FreeBSD__
extern int in6_src_sysctl __P((void *, size_t *, void *, size_t));
#endif
extern void addrsel_policy_init __P((void));
extern  u_char  ip6_protox[];

#define satosin6(sa)  ((struct sockaddr_in6 *)(sa))
#define sin6tosa(sin6)  ((struct sockaddr *)(sin6))
#define ifatoia6(ifa) ((struct in6_ifaddr *)(ifa))
#endif /* _KERNEL */

#if defined(_NETBSD_SOURCE)

#include <machine/ansi.h>

#ifdef  _EFI_SIZE_T_
typedef _EFI_SIZE_T_    size_t;
#define _SIZE_T
#undef  _EFI_SIZE_T_
#undef  _BSD_SIZE_T_
#endif

#include <sys/EfiCdefs.h>

__BEGIN_DECLS
struct cmsghdr;

extern int inet6_option_space (int);
extern int inet6_option_init (void *, struct cmsghdr **, int);
extern int inet6_option_append (struct cmsghdr *, const uint8_t *,
  int, int);
extern uint8_t *inet6_option_alloc (struct cmsghdr *, int, int, int);
extern int inet6_option_next (const struct cmsghdr *, uint8_t **);
extern int inet6_option_find (const struct cmsghdr *, uint8_t **, int);

extern size_t inet6_rthdr_space (int, int);
extern struct cmsghdr *inet6_rthdr_init (void *, int);
extern int inet6_rthdr_add (struct cmsghdr *, const struct in6_addr *,
    unsigned int);
extern int inet6_rthdr_lasthop (struct cmsghdr *, unsigned int);
#if 0 /* not implemented yet */
extern int inet6_rthdr_reverse (const struct cmsghdr *, struct cmsghdr *);
#endif
extern int inet6_rthdr_segments (const struct cmsghdr *);
extern struct in6_addr *inet6_rthdr_getaddr (struct cmsghdr *, int);
extern int inet6_rthdr_getflags (const struct cmsghdr *, int);

extern int inet6_opt_init (void *, socklen_t);
extern int inet6_opt_append (void *, socklen_t, int, uint8_t,
    socklen_t, uint8_t, void **);
extern int inet6_opt_finish (void *, socklen_t, int);
extern int inet6_opt_set_val (void *, int, void *, socklen_t);

extern int inet6_opt_next (void *, socklen_t, int, uint8_t *,
    socklen_t *, void **);
extern int inet6_opt_find (void *, socklen_t, int, uint8_t,
    socklen_t *, void **);
extern int inet6_opt_get_val (void *, int, void *, socklen_t);
extern socklen_t inet6_rth_space (int, int);
extern void *inet6_rth_init (void *, socklen_t, int, int);
extern int inet6_rth_add (void *, const struct in6_addr *);
extern int inet6_rth_reverse (const void *, void *);
extern int inet6_rth_segments (const void *);
extern struct in6_addr *inet6_rth_getaddr (const void *, int);
__END_DECLS
#endif /* _NETBSD_SOURCE */

#endif /* !_NETINET6_IN6_H_ */
