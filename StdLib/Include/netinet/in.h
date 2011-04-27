/*	$NetBSD: in.h,v 1.77 2006/11/13 05:13:41 dyoung Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)in.h	8.3 (Berkeley) 1/3/94
 */

/*
 * Constants and structures defined by the internet system,
 * Per RFC 790, September 1981, and numerous additions.
 */

#ifndef _NETINET_IN_H_
#define	_NETINET_IN_H_

#include <machine/int_types.h>

#ifndef uint8_t
typedef __uint8_t	uint8_t;
#define	uint8_t		__uint8_t
#endif

#ifndef uint32_t
typedef __uint32_t	uint32_t;
#define	uint32_t	__uint32_t
#endif

#include <sys/ansi.h>

#ifndef in_addr_t
typedef __in_addr_t	in_addr_t;
#define	in_addr_t	__in_addr_t
#endif

#ifndef in_port_t
typedef __in_port_t	in_port_t;
#define	in_port_t	__in_port_t
#endif

#ifndef sa_family_t
typedef __sa_family_t	sa_family_t;
#define	sa_family_t	__sa_family_t
#endif

/*
 * Protocols
 */
#define	IPPROTO_IP		0		/* dummy for IP */
#define	IPPROTO_HOPOPTS		0		/* IP6 hop-by-hop options */
#define	IPPROTO_ICMP		1		/* control message protocol */
#define	IPPROTO_IGMP		2		/* group mgmt protocol */
#define	IPPROTO_GGP		3		/* gateway^2 (deprecated) */
#define	IPPROTO_IPV4		4 		/* IP header */
#define	IPPROTO_IPIP		4		/* IP inside IP */
#define	IPPROTO_TCP		6		/* tcp */
#define	IPPROTO_EGP		8		/* exterior gateway protocol */
#define	IPPROTO_PUP		12		/* pup */
#define	IPPROTO_UDP		17		/* user datagram protocol */
#define	IPPROTO_IDP		22		/* xns idp */
#define	IPPROTO_TP		29 		/* tp-4 w/ class negotiation */
#define	IPPROTO_IPV6		41		/* IP6 header */
#define	IPPROTO_ROUTING		43		/* IP6 routing header */
#define	IPPROTO_FRAGMENT	44		/* IP6 fragmentation header */
#define	IPPROTO_RSVP		46		/* resource reservation */
#define	IPPROTO_GRE		47		/* GRE encaps RFC 1701 */
#define	IPPROTO_ESP		50 		/* encap. security payload */
#define	IPPROTO_AH		51 		/* authentication header */
#define	IPPROTO_MOBILE		55		/* IP Mobility RFC 2004 */
#define	IPPROTO_IPV6_ICMP	58		/* IPv6 ICMP */
#define	IPPROTO_ICMPV6		58		/* ICMP6 */
#define	IPPROTO_NONE		59		/* IP6 no next header */
#define	IPPROTO_DSTOPTS		60		/* IP6 destination option */
#define	IPPROTO_EON		80		/* ISO cnlp */
#define	IPPROTO_ETHERIP		97		/* Ethernet-in-IP */
#define	IPPROTO_ENCAP		98		/* encapsulation header */
#define	IPPROTO_PIM		103		/* Protocol indep. multicast */
#define	IPPROTO_IPCOMP		108		/* IP Payload Comp. Protocol */
#define	IPPROTO_VRRP		112		/* VRRP RFC 2338 */
#define	IPPROTO_CARP		112		/* Common Address Resolution Protocol */
#define	IPPROTO_RAW		255		/* raw IP packet */
#define	IPPROTO_MAX		256

/* last return value of *_input(), meaning "all job for this pkt is done".  */
#define	IPPROTO_DONE		257

/* sysctl placeholder for (FAST_)IPSEC */
#define CTL_IPPROTO_IPSEC	258


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

#define	IPPORT_RESERVED		1024
#define	IPPORT_ANONMIN		49152
#define	IPPORT_ANONMAX		65535
#define	IPPORT_RESERVEDMIN	600
#define	IPPORT_RESERVEDMAX	(IPPORT_RESERVED-1)

/*
 * Internet address (a structure for historical reasons)
 */
struct in_addr {
	in_addr_t s_addr;
} __attribute__((__packed__));

/*
 * Definitions of bits in internet address integers.
 * On subnets, the decomposition of addresses to host and net parts
 * is done according to subnet mask, not the masks here.
 *
 * By byte-swapping the constants, we avoid ever having to byte-swap IP
 * addresses inside the kernel.  Unfortunately, user-level programs rely
 * on these macros not doing byte-swapping.
 */
#ifdef _KERNEL
#define	__IPADDR(x)	((uint32_t) htonl((uint32_t)(x)))
#else
#define	__IPADDR(x)	((uint32_t)(x))
#endif

#define	IN_CLASSA(i)		(((uint32_t)(i) & __IPADDR(0x80000000)) == \
				 __IPADDR(0x00000000))
#define	IN_CLASSA_NET		__IPADDR(0xff000000)
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		__IPADDR(0x00ffffff)
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(i)		(((uint32_t)(i) & __IPADDR(0xc0000000)) == \
				 __IPADDR(0x80000000))
#define	IN_CLASSB_NET		__IPADDR(0xffff0000)
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		__IPADDR(0x0000ffff)
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(i)		(((uint32_t)(i) & __IPADDR(0xe0000000)) == \
				 __IPADDR(0xc0000000))
#define	IN_CLASSC_NET		__IPADDR(0xffffff00)
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		__IPADDR(0x000000ff)

#define	IN_CLASSD(i)		(((uint32_t)(i) & __IPADDR(0xf0000000)) == \
				 __IPADDR(0xe0000000))
/* These ones aren't really net and host fields, but routing needn't know. */
#define	IN_CLASSD_NET		__IPADDR(0xf0000000)
#define	IN_CLASSD_NSHIFT	28
#define	IN_CLASSD_HOST		__IPADDR(0x0fffffff)
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#define	IN_EXPERIMENTAL(i)	(((uint32_t)(i) & __IPADDR(0xf0000000)) == \
				 __IPADDR(0xf0000000))
#define	IN_BADCLASS(i)		(((uint32_t)(i) & __IPADDR(0xf0000000)) == \
				 __IPADDR(0xf0000000))

#define IN_LINKLOCAL(i)	(((uint32_t)(i) & __IPADDR(0xffff0000)) == \
			 __IPADDR(0xa9fe0000))

#define	IN_PRIVATE(i)	((((uint32_t)(i) & __IPADDR(0xff000000)) ==	\
			  __IPADDR(0x0a000000))	||			\
			 (((uint32_t)(i) & __IPADDR(0xfff00000)) ==	\
			  __IPADDR(0xac100000))	||			\
			 (((uint32_t)(i) & __IPADDR(0xffff0000)) ==	\
			  __IPADDR(0xc0a80000)))

#define	IN_LOCAL_GROUP(i)	(((uint32_t)(i) & __IPADDR(0xffffff00)) == \
				 __IPADDR(0xe0000000))

#define	IN_ANY_LOCAL(i)		(IN_LINKLOCAL(i) || IN_LOCAL_GROUP(i))

#define	INADDR_ANY		__IPADDR(0x00000000)
#define	INADDR_LOOPBACK		__IPADDR(0x7f000001)
#define	INADDR_BROADCAST	__IPADDR(0xffffffff)	/* must be masked */
#define	INADDR_NONE		__IPADDR(0xffffffff)	/* -1 return */

#define	INADDR_UNSPEC_GROUP	__IPADDR(0xe0000000)	/* 224.0.0.0 */
#define	INADDR_ALLHOSTS_GROUP	__IPADDR(0xe0000001)	/* 224.0.0.1 */
#define	INADDR_ALLRTRS_GROUP	__IPADDR(0xe0000002)	/* 224.0.0.2 */
#define	INADDR_CARP_GROUP	__IPADDR(0xe0000012)	/* 224.0.0.18 */
#define	INADDR_MAX_LOCAL_GROUP	__IPADDR(0xe00000ff)	/* 224.0.0.255 */

#define	IN_LOOPBACKNET		127			/* official! */

/*
 * Socket address, internet style.
 */
struct sockaddr_in {
	uint8_t		sin_len;
	sa_family_t	sin_family;
	in_port_t	sin_port;
	struct in_addr	sin_addr;
	__int8_t	sin_zero[8];
};

#define	INET_ADDRSTRLEN                 16

/*
 * Structure used to describe IP options.
 * Used to store options internally, to pass them to a process,
 * or to restore options retrieved earlier.
 * The ip_dst is used for the first-hop gateway when using a source route
 * (this gets put into the header proper).
 */
struct ip_opts {
	struct in_addr	ip_dst;		/* first hop, 0 w/o src rt */
#if defined(__cplusplus)
	__int8_t	Ip_opts[40];	/* actually variable in size */
#else
	__int8_t	ip_opts[40];	/* actually variable in size */
#endif
};

/*
 * Options for use with [gs]etsockopt at the IP level.
 * First word of comment is data type; bool is stored in int.
 */
#define	IP_OPTIONS		1    /* buf/ip_opts; set/get IP options */
#define	IP_HDRINCL		2    /* int; header is included with data */
#define	IP_TOS			3    /* int; IP type of service and preced. */
#define	IP_TTL			4    /* int; IP time to live */
#define	IP_RECVOPTS		5    /* bool; receive all IP opts w/dgram */
#define	IP_RECVRETOPTS		6    /* bool; receive IP opts for response */
#define	IP_RECVDSTADDR		7    /* bool; receive IP dst addr w/dgram */
#define	IP_RETOPTS		8    /* ip_opts; set/get IP options */
#define	IP_MULTICAST_IF		9    /* in_addr; set/get IP multicast i/f  */
#define	IP_MULTICAST_TTL	10   /* u_char; set/get IP multicast ttl */
#define	IP_MULTICAST_LOOP	11   /* u_char; set/get IP multicast loopback */
#define	IP_ADD_MEMBERSHIP	12   /* ip_mreq; add an IP group membership */
#define	IP_DROP_MEMBERSHIP	13   /* ip_mreq; drop an IP group membership */
#define	IP_PORTRANGE		19   /* int; range to use for ephemeral port */
#define	IP_RECVIF		20   /* bool; receive reception if w/dgram */
#define	IP_ERRORMTU		21   /* int; get MTU of last xmit = EMSGSIZE */
#if 1 /*IPSEC*/
#define	IP_IPSEC_POLICY		22 /* struct; get/set security policy */
#endif

/*
 * Defaults and limits for options
 */
#define	IP_DEFAULT_MULTICAST_TTL  1	/* normally limit m'casts to 1 hop  */
#define	IP_DEFAULT_MULTICAST_LOOP 1	/* normally hear sends if a member  */
#define	IP_MAX_MEMBERSHIPS	20	/* per socket; must fit in one mbuf */

/*
 * Argument structure for IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP.
 */
struct ip_mreq {
	struct	in_addr imr_multiaddr;	/* IP multicast address of group */
	struct	in_addr imr_interface;	/* local IP address of interface */
};

/*
 * Argument for IP_PORTRANGE:
 * - which range to search when port is unspecified at bind() or connect()
 */
#define	IP_PORTRANGE_DEFAULT	0	/* default range */
#define	IP_PORTRANGE_HIGH	1	/* same as DEFAULT (FreeBSD compat) */
#define	IP_PORTRANGE_LOW	2	/* use privileged range */

#if defined(_NETBSD_SOURCE)
/*
 * Definitions for inet sysctl operations.
 *
 * Third level is protocol number.
 * Fourth level is desired variable within that protocol.
 */
#define	IPPROTO_MAXID	(IPPROTO_AH + 1)	/* don't list to IPPROTO_MAX */

#define	CTL_IPPROTO_NAMES { \
	{ "ip", CTLTYPE_NODE }, \
	{ "icmp", CTLTYPE_NODE }, \
	{ "igmp", CTLTYPE_NODE }, \
	{ "ggp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "tcp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ "egp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "pup", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "udp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "idp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "ipsec", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "pim", CTLTYPE_NODE }, \
}

/*
 * Names for IP sysctl objects
 */
#define	IPCTL_FORWARDING	1	/* act as router */
#define	IPCTL_SENDREDIRECTS	2	/* may send redirects when forwarding */
#define	IPCTL_DEFTTL		3	/* default TTL */
#ifdef notyet
#define	IPCTL_DEFMTU		4	/* default MTU */
#endif
#define	IPCTL_FORWSRCRT		5	/* forward source-routed packets */
#define	IPCTL_DIRECTEDBCAST	6	/* default broadcast behavior */
#define	IPCTL_ALLOWSRCRT	7	/* allow/drop all source-routed pkts */
#define	IPCTL_SUBNETSARELOCAL	8	/* treat subnets as local addresses */
#define	IPCTL_MTUDISC		9	/* allow path MTU discovery */
#define	IPCTL_ANONPORTMIN      10	/* minimum ephemeral port */
#define	IPCTL_ANONPORTMAX      11	/* maximum ephemeral port */
#define	IPCTL_MTUDISCTIMEOUT   12	/* allow path MTU discovery */
#define	IPCTL_MAXFLOWS         13	/* maximum ip flows allowed */
#define	IPCTL_HOSTZEROBROADCAST 14	/* is host zero a broadcast addr? */
#define	IPCTL_GIF_TTL 	       15	/* default TTL for gif encap packet */
#define	IPCTL_LOWPORTMIN       16	/* minimum reserved port */
#define	IPCTL_LOWPORTMAX       17	/* maximum reserved port */
#define	IPCTL_MAXFRAGPACKETS   18	/* max packets reassembly queue */
#define	IPCTL_GRE_TTL          19	/* default TTL for gre encap packet */
#define	IPCTL_CHECKINTERFACE   20	/* drop pkts in from 'wrong' iface */
#define	IPCTL_IFQ	       21	/* ipintrq node */
#define	IPCTL_RANDOMID	       22	/* use random IP ids (if configured) */
#define	IPCTL_LOOPBACKCKSUM    23	/* do IP checksum on loopback */
#define	IPCTL_STATS		24	/* IP statistics */
#define	IPCTL_MAXID	       25

#define	IPCTL_NAMES { \
	{ 0, 0 }, \
	{ "forwarding", CTLTYPE_INT }, \
	{ "redirect", CTLTYPE_INT }, \
	{ "ttl", CTLTYPE_INT }, \
	{ "mtu", CTLTYPE_INT }, \
	{ "forwsrcrt", CTLTYPE_INT }, \
	{ "directed-broadcast", CTLTYPE_INT }, \
	{ "allowsrcrt", CTLTYPE_INT }, \
	{ "subnetsarelocal", CTLTYPE_INT }, \
	{ "mtudisc", CTLTYPE_INT }, \
	{ "anonportmin", CTLTYPE_INT }, \
	{ "anonportmax", CTLTYPE_INT }, \
	{ "mtudisctimeout", CTLTYPE_INT }, \
	{ "maxflows", CTLTYPE_INT }, \
	{ "hostzerobroadcast", CTLTYPE_INT }, \
	{ "gifttl", CTLTYPE_INT }, \
	{ "lowportmin", CTLTYPE_INT }, \
	{ "lowportmax", CTLTYPE_INT }, \
	{ "maxfragpackets", CTLTYPE_INT }, \
	{ "grettl", CTLTYPE_INT }, \
	{ "checkinterface", CTLTYPE_INT }, \
	{ "ifq", CTLTYPE_NODE }, \
	{ "random_id", CTLTYPE_INT }, \
	{ "do_loopback_cksum", CTLTYPE_INT }, \
	{ "stats", CTLTYPE_STRUCT }, \
}
#endif /* _NETBSD_SOURCE */

/* INET6 stuff */
#define	__KAME_NETINET_IN_H_INCLUDED_
#include <netinet6/in6.h>
#undef __KAME_NETINET_IN_H_INCLUDED_

#ifdef _KERNEL
/*
 * in_cksum_phdr:
 *
 *	Compute significant parts of the IPv4 checksum pseudo-header
 *	for use in a delayed TCP/UDP checksum calculation.
 *
 *	Args:
 *
 *		src		Source IP address
 *		dst		Destination IP address
 *		lenproto	htons(proto-hdr-len + proto-number)
 */
static __inline u_int16_t __attribute__((__unused__))
in_cksum_phdr(u_int32_t src, u_int32_t dst, u_int32_t lenproto)
{
	u_int32_t sum;

	sum = lenproto +
	      (u_int16_t)(src >> 16) +
	      (u_int16_t)(src /*& 0xffff*/) +
	      (u_int16_t)(dst >> 16) +
	      (u_int16_t)(dst /*& 0xffff*/);

	sum = (u_int16_t)(sum >> 16) + (u_int16_t)(sum /*& 0xffff*/);

	if (sum > 0xffff)
		sum -= 0xffff;

	return (sum);
}

/*
 * in_cksum_addword:
 *
 *	Add the two 16-bit network-order values, carry, and return.
 */
static __inline u_int16_t __attribute__((__unused__))
in_cksum_addword(u_int16_t a, u_int16_t b)
{
	u_int32_t sum = a + b;

	if (sum > 0xffff)
		sum -= 0xffff;

	return (sum);
}

extern	struct in_addr zeroin_addr;
extern	u_char	ip_protox[];

int	in_broadcast(struct in_addr, struct ifnet *);
int	in_canforward(struct in_addr);
int	in_cksum(struct mbuf *, int);
int	in4_cksum(struct mbuf *, u_int8_t, int, int);
void	in_delayed_cksum(struct mbuf *);
int	in_localaddr(struct in_addr);
void	in_socktrim(struct sockaddr_in *);

#define	in_hosteq(s,t)	((s).s_addr == (t).s_addr)
#define	in_nullhost(x)	((x).s_addr == INADDR_ANY)

#define	satosin(sa)	((struct sockaddr_in *)(sa))
#define	sintosa(sin)	((struct sockaddr *)(sin))
#define	ifatoia(ifa)	((struct in_ifaddr *)(ifa))
#endif /* _KERNEL */

#endif /* !_NETINET_IN_H_ */
