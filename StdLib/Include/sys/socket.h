/*  $NetBSD: socket.h,v 1.82 2006/06/27 03:49:08 mrg Exp $  */

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
 * Copyright (c) 1982, 1985, 1986, 1988, 1993, 1994
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
 *  @(#)socket.h  8.6 (Berkeley) 5/3/95
 */

#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#include <sys/featuretest.h>

/*
 * Definitions related to sockets: types, address families, options.
 */

/*
 * Data types.
 */
#include <sys/ansi.h>

#ifndef sa_family_t
typedef __sa_family_t sa_family_t;
#define sa_family_t __sa_family_t
#endif

#ifndef socklen_t
typedef __socklen_t socklen_t;
#define socklen_t __socklen_t
#endif

#include <machine/ansi.h>

#ifdef  _EFI_SIZE_T_
typedef _EFI_SIZE_T_  size_t;
#undef  _EFI_SIZE_T_
#undef  _BSD_SIZE_T_
#endif

#ifdef  _BSD_SSIZE_T_
typedef _BSD_SSIZE_T_ ssize_t;
#undef  _BSD_SSIZE_T_
#endif

#include <sys/uio.h>

/*
 * Socket types.
 */
#define SOCK_STREAM 1   /* stream socket */
#define SOCK_DGRAM  2   /* datagram socket */
#define SOCK_RAW  3   /* raw-protocol interface */
#define SOCK_RDM  4   /* reliably-delivered message */
#define SOCK_SEQPACKET  5   /* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define SO_DEBUG  0x0001    /* turn on debugging info recording */
#define SO_ACCEPTCONN 0x0002    /* socket has had listen() */
#define SO_REUSEADDR  0x0004    /* allow local address reuse */
#define SO_KEEPALIVE  0x0008    /* keep connections alive */
#define SO_DONTROUTE  0x0010    /* just use interface addresses */
#define SO_BROADCAST  0x0020    /* permit sending of broadcast msgs */
#define SO_USELOOPBACK  0x0040    /* bypass hardware when possible */
#define SO_LINGER 0x0080    /* linger on close if data present */
#define SO_OOBINLINE  0x0100    /* leave received OOB data in line */
#define SO_REUSEPORT  0x0200    /* allow local address & port reuse */
#define SO_TIMESTAMP  0x0400    /* timestamp received dgram traffic */


/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF 0x1001    /* send buffer size */
#define SO_RCVBUF 0x1002    /* receive buffer size */
#define SO_SNDLOWAT 0x1003    /* send low-water mark */
#define SO_RCVLOWAT 0x1004    /* receive low-water mark */
#define SO_SNDTIMEO 0x1005    /* send timeout */
#define SO_RCVTIMEO 0x1006    /* receive timeout */
#define SO_ERROR  0x1007    /* get error status and clear */
#define SO_TYPE   0x1008    /* get socket type */
#define SO_OVERFLOWED 0x1009    /* datagrams: return packets dropped */

/*
 * Structure used for manipulating linger option.
 */
struct  linger {
  int l_onoff;    /* option on/off */
  int l_linger;   /* linger time in seconds */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define SOL_SOCKET  0xffff    /* options for socket level */

/*
 * Address families.
 */
#define AF_UNSPEC 0   /* unspecified */
#define AF_LOCAL  1   /* local to host (pipes, portals) */
#define AF_UNIX   AF_LOCAL  /* backward compatibility */
#define AF_INET   2   /* internetwork: UDP, TCP, etc. */
#define AF_IMPLINK  3   /* arpanet imp addresses */
#define AF_PUP    4   /* pup protocols: e.g. BSP */
#define AF_CHAOS  5   /* mit CHAOS protocols */
#define AF_NS   6   /* XEROX NS protocols */
#define AF_ISO    7   /* ISO protocols */
#define AF_OSI    AF_ISO
#define AF_ECMA   8   /* european computer manufacturers */
#define AF_DATAKIT  9   /* datakit protocols */
#define AF_CCITT  10    /* CCITT protocols, X.25 etc */
#define AF_SNA    11    /* IBM SNA */
#define AF_DECnet 12    /* DECnet */
#define AF_DLI    13    /* DEC Direct data link interface */
#define AF_LAT    14    /* LAT */
#define AF_HYLINK 15    /* NSC Hyperchannel */
#define AF_APPLETALK  16    /* Apple Talk */
#define AF_ROUTE  17    /* Internal Routing Protocol */
#define AF_LINK   18    /* Link layer interface */
#if defined(_NETBSD_SOURCE)
#define pseudo_AF_XTP 19    /* eXpress Transfer Protocol (no AF) */
#endif
#define AF_COIP   20    /* connection-oriented IP, aka ST II */
#define AF_CNT    21    /* Computer Network Technology */
#if defined(_NETBSD_SOURCE)
#define pseudo_AF_RTIP  22    /* Help Identify RTIP packets */
#endif
#define AF_IPX    23    /* Novell Internet Protocol */
#define AF_INET6  24    /* IP version 6 */
#if defined(_NETBSD_SOURCE)
#define pseudo_AF_PIP 25    /* Help Identify PIP packets */
#endif
#define AF_ISDN   26    /* Integrated Services Digital Network*/
#define AF_E164   AF_ISDN   /* CCITT E.164 recommendation */
#define AF_NATM   27    /* native ATM access */
#define AF_ARP    28    /* (rev.) addr. res. prot. (RFC 826) */
#if defined(_NETBSD_SOURCE)
#define pseudo_AF_KEY 29    /* Internal key management protocol  */
#define pseudo_AF_HDRCMPLT 30   /* Used by BPF to not rewrite hdrs
             in interface output routine */
#endif
#define AF_BLUETOOTH  31

#define AF_MAX    32

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
  __uint8_t sa_len;   /* total length */
  sa_family_t sa_family;  /* address family */
  char    sa_data[14];  /* actually longer; address value */
};

#if defined(_KERNEL)
/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
  u_short sp_family;    /* address family */
  u_short sp_protocol;    /* protocol */
};
#endif /* _KERNEL */

#if 1
/*
 * RFC 2553: protocol-independent placeholder for socket addresses
 */
#define _SS_MAXSIZE 128
#define _SS_ALIGNSIZE (sizeof(__int64_t))
#define _SS_PAD1SIZE  (_SS_ALIGNSIZE - 2)
#define _SS_PAD2SIZE  (_SS_MAXSIZE - 2 - \
        _SS_PAD1SIZE - _SS_ALIGNSIZE)

#if (_XOPEN_SOURCE - 0) >= 500 || defined(_NETBSD_SOURCE)
struct sockaddr_storage {
  __uint8_t ss_len;   /* address length */
  sa_family_t ss_family;  /* address family */
  char    __ss_pad1[_SS_PAD1SIZE];
  __int64_t     __ss_align;/* force desired structure storage alignment */
  char    __ss_pad2[_SS_PAD2SIZE];
};
#endif /* _XOPEN_SOURCE >= 500 || _NETBSD_SOURCE */
#endif /* 1 */

/*
 * Protocol families, same as address families for now.
 */
#define PF_UNSPEC AF_UNSPEC
#define PF_LOCAL  AF_LOCAL
#define PF_UNIX   PF_LOCAL  /* backward compatibility */
#define PF_INET   AF_INET
#define PF_IMPLINK  AF_IMPLINK
#define PF_PUP    AF_PUP
#define PF_CHAOS  AF_CHAOS
#define PF_NS   AF_NS
#define PF_ISO    AF_ISO
#define PF_OSI    AF_ISO
#define PF_ECMA   AF_ECMA
#define PF_DATAKIT  AF_DATAKIT
#define PF_CCITT  AF_CCITT
#define PF_SNA    AF_SNA
#define PF_DECnet AF_DECnet
#define PF_DLI    AF_DLI
#define PF_LAT    AF_LAT
#define PF_HYLINK AF_HYLINK
#define PF_APPLETALK  AF_APPLETALK
#define PF_ROUTE  AF_ROUTE
#define PF_LINK   AF_LINK
#if defined(_NETBSD_SOURCE)
#define PF_XTP    pseudo_AF_XTP /* really just proto family, no AF */
#endif
#define PF_COIP   AF_COIP
#define PF_CNT    AF_CNT
#define PF_INET6  AF_INET6
#define PF_IPX    AF_IPX    /* same format as AF_NS */
#if defined(_NETBSD_SOURCE)
#define PF_RTIP   pseudo_AF_RTIP  /* same format as AF_INET */
#define PF_PIP    pseudo_AF_PIP
#endif
#define PF_ISDN   AF_ISDN   /* same as E164 */
#define PF_E164   AF_E164
#define PF_NATM   AF_NATM
#define PF_ARP    AF_ARP
#if defined(_NETBSD_SOURCE)
#define PF_KEY    pseudo_AF_KEY /* like PF_ROUTE, only for key mgmt */
#endif
#define PF_BLUETOOTH  AF_BLUETOOTH

#define PF_MAX    AF_MAX

#if defined(_NETBSD_SOURCE)

#ifndef gid_t
typedef __gid_t   gid_t;    /* group id */
#define gid_t   __gid_t
#endif

#ifndef uid_t
typedef __uid_t   uid_t;    /* user id */
#define uid_t   __uid_t
#endif

/*
 * Socket credentials.
 */
struct sockcred {
  uid_t sc_uid;     /* real user id */
  uid_t sc_euid;    /* effective user id */
  gid_t sc_gid;     /* real group id */
  gid_t sc_egid;    /* effective group id */
  int sc_ngroups;   /* number of supplemental groups */
  gid_t sc_groups[1];   /* variable length */
};

/*
 * Compute size of a sockcred structure with groups.
 */
#define SOCKCREDSIZE(ngrps) \
  (sizeof(struct sockcred) + (sizeof(gid_t) * ((ngrps) - 1)))
#endif /* _NETBSD_SOURCE */


#if defined(_NETBSD_SOURCE)
/*
 * Definitions for network related sysctl, CTL_NET.
 *
 * Second level is protocol family.
 * Third level is protocol number.
 *
 * Further levels are defined by the individual families below.
 */
#define NET_MAXID AF_MAX

#define CTL_NET_NAMES { \
  { 0, 0 }, \
  { "local", CTLTYPE_NODE }, \
  { "inet", CTLTYPE_NODE }, \
  { "implink", CTLTYPE_NODE }, \
  { "pup", CTLTYPE_NODE }, \
  { "chaos", CTLTYPE_NODE }, \
  { "xerox_ns", CTLTYPE_NODE }, \
  { "iso", CTLTYPE_NODE }, \
  { "emca", CTLTYPE_NODE }, \
  { "datakit", CTLTYPE_NODE }, \
  { "ccitt", CTLTYPE_NODE }, \
  { "ibm_sna", CTLTYPE_NODE }, \
  { "decnet", CTLTYPE_NODE }, \
  { "dec_dli", CTLTYPE_NODE }, \
  { "lat", CTLTYPE_NODE }, \
  { "hylink", CTLTYPE_NODE }, \
  { "appletalk", CTLTYPE_NODE }, \
  { "route", CTLTYPE_NODE }, \
  { "link_layer", CTLTYPE_NODE }, \
  { "xtp", CTLTYPE_NODE }, \
  { "coip", CTLTYPE_NODE }, \
  { "cnt", CTLTYPE_NODE }, \
  { "rtip", CTLTYPE_NODE }, \
  { "ipx", CTLTYPE_NODE }, \
  { "inet6", CTLTYPE_NODE }, \
  { "pip", CTLTYPE_NODE }, \
  { "isdn", CTLTYPE_NODE }, \
  { "natm", CTLTYPE_NODE }, \
  { "arp", CTLTYPE_NODE }, \
  { "key", CTLTYPE_NODE }, \
}

struct kinfo_pcb {
  __uint64_t  ki_pcbaddr; /* PTR: pcb addr */
  __uint64_t  ki_ppcbaddr;  /* PTR: ppcb addr */
  __uint64_t  ki_sockaddr;  /* PTR: socket addr */

  __uint32_t  ki_family;  /* INT: protocol family */
  __uint32_t  ki_type;  /* INT: socket type */
  __uint32_t  ki_protocol;  /* INT: protocol */
  __uint32_t  ki_pflags;  /* INT: generic protocol flags */

  __uint32_t  ki_sostate; /* INT: socket state */
  __uint32_t  ki_prstate; /* INT: protocol state */
  __int32_t ki_tstate;  /* INT: tcp state */
  __uint32_t  ki_tflags;  /* INT: tcp flags */

  __uint64_t  ki_rcvq;  /* U_LONG: receive queue len */
  __uint64_t  ki_sndq;  /* U_LONG: send queue len */

  union {
    struct sockaddr _kis_src; /* STRUCT: local address */
    char _kis_pad[256 + 8];   /* pad to max addr length */
  } ki_s;
  union {
    struct sockaddr _kid_dst; /* STRUCT: remote address */
    char _kid_pad[256 + 8];   /* pad to max addr length */
  } ki_d;

  __uint64_t  ki_inode; /* INO_T: fake inode number */
  __uint64_t  ki_vnode; /* PTR: if associated with file */
  __uint64_t  ki_conn;  /* PTR: control block of peer */
  __uint64_t  ki_refs;  /* PTR: referencing socket */
  __uint64_t  ki_nextref; /* PTR: link in refs list */
};

#define ki_src ki_s._kis_src
#define ki_dst ki_d._kid_dst

#define PCB_SLOP    20
#define PCB_ALL     0

#endif /* _NETBSD_SOURCE */

#if defined(_NETBSD_SOURCE)
/*
 * PF_ROUTE - Routing table
 *
 * Three additional levels are defined:
 *  Fourth: address family, 0 is wildcard
 *  Fifth: type of info, defined below
 *  Sixth: flag(s) to mask with for NET_RT_FLAGS
 */
#define NET_RT_DUMP 1   /* dump; may limit to a.f. */
#define NET_RT_FLAGS  2   /* by flags, e.g. RESOLVING */
#define NET_RT_OIFLIST  3   /* old NET_RT_IFLIST (pre 1.5) */
#define NET_RT_IFLIST 4   /* survey interface list */
#define NET_RT_MAXID  5

#define CTL_NET_RT_NAMES { \
  { 0, 0 }, \
  { "dump", CTLTYPE_STRUCT }, \
  { "flags", CTLTYPE_STRUCT }, \
  { 0, 0 }, \
  { "iflist", CTLTYPE_STRUCT }, \
}
#endif /* _NETBSD_SOURCE */

/*
 * Maximum queue length specifiable by listen(2).
 */
#ifndef SOMAXCONN
#define SOMAXCONN 128
#endif

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 */
struct msghdr {
  void    *msg_name;  /* optional address */
  socklen_t msg_namelen;  /* size of address */
  struct iovec  *msg_iov; /* scatter/gather array */
  int   msg_iovlen; /* # elements in msg_iov */
  void    *msg_control; /* ancillary data, see below */
  socklen_t msg_controllen; /* ancillary data buffer len */
  int   msg_flags;  /* flags on received message */
};

#define MSG_OOB   0x0001    /* process out-of-band data */
#define MSG_PEEK  0x0002    /* peek at incoming message */
#define MSG_DONTROUTE 0x0004    /* send without using routing tables */
#define MSG_EOR   0x0008    /* data completes record */
#define MSG_TRUNC 0x0010    /* data discarded before delivery */
#define MSG_CTRUNC  0x0020    /* control data lost before delivery */
#define MSG_WAITALL 0x0040    /* wait for full request or error */
#define MSG_DONTWAIT  0x0080    /* this message should be nonblocking */
#define MSG_BCAST 0x0100    /* this message was rcvd using link-level brdcst */
#define MSG_MCAST 0x0200    /* this message was rcvd using link-level mcast */
#define MSG_NOSIGNAL  0x0400    /* do not generate SIGPIPE on EOF */

/*
 * Header for ancillary data objects in msg_control buffer.
 * Used for additional information with/about a datagram
 * not expressible by flags.  The format is a sequence
 * of message elements headed by cmsghdr structures.
 */
struct cmsghdr {
  socklen_t cmsg_len; /* data byte count, including hdr */
  int   cmsg_level; /* originating protocol */
  int   cmsg_type;  /* protocol-specific type */
/* followed by  u_char  cmsg_data[]; */
};

/* given pointer to struct cmsghdr, return pointer to data */
#define CMSG_DATA(cmsg) \
  ((u_char *)(void *)(cmsg) + __CMSG_ALIGN(sizeof(struct cmsghdr)))
#define CCMSG_DATA(cmsg) \
  ((const u_char *)(const void *)(cmsg) + \
  __CMSG_ALIGN(sizeof(struct cmsghdr)))

/*
 * Alignment requirement for CMSG struct manipulation.
 * This basically behaves the same as ALIGN() ARCH/include/param.h.
 * We declare it separately for two reasons:
 * (1) avoid dependency between machine/param.h, and (2) to sync with kernel's
 * idea of ALIGNBYTES at runtime.
 * without (2), we can't guarantee binary compatibility in case of future
 * changes in ALIGNBYTES.
 */
#define __CMSG_ALIGN(n) (((n) + __cmsg_alignbytes()) & ~__cmsg_alignbytes())
#ifdef _KERNEL
#define CMSG_ALIGN(n) __CMSG_ALIGN(n)
#endif

/* given pointer to struct cmsghdr, return pointer to next cmsghdr */
#define CMSG_NXTHDR(mhdr, cmsg) \
  (((__caddr_t)(cmsg) + __CMSG_ALIGN((cmsg)->cmsg_len) + \
          __CMSG_ALIGN(sizeof(struct cmsghdr)) > \
      (((__caddr_t)(mhdr)->msg_control) + (mhdr)->msg_controllen)) ? \
      (struct cmsghdr *)0 : \
      (struct cmsghdr *)((__caddr_t)(cmsg) + \
          __CMSG_ALIGN((cmsg)->cmsg_len)))

/*
 * RFC 2292 requires to check msg_controllen, in case that the kernel returns
 * an empty list for some reasons.
 */
#define CMSG_FIRSTHDR(mhdr) \
  ((mhdr)->msg_controllen >= sizeof(struct cmsghdr) ? \
   (struct cmsghdr *)(mhdr)->msg_control : \
   (struct cmsghdr *)0)

#define CMSG_SPACE(l) (__CMSG_ALIGN(sizeof(struct cmsghdr)) + __CMSG_ALIGN(l))
#define CMSG_LEN(l) (__CMSG_ALIGN(sizeof(struct cmsghdr)) + (l))

/* "Socket"-level control message types: */
#define SCM_RIGHTS  0x01    /* access rights (array of int) */
#if defined(_NETBSD_SOURCE)
#define SCM_TIMESTAMP 0x02    /* timestamp (struct timeval) */
#define SCM_CREDS 0x04    /* credentials (struct sockcred) */
#endif

/*
 * Types of socket shutdown(2).
 */
#define SHUT_RD   0   /* Disallow further receives. */
#define SHUT_WR   1   /* Disallow further sends. */
#define SHUT_RDWR 2   /* Disallow further sends/receives. */

#include <sys/EfiCdefs.h>

__BEGIN_DECLS
int __cmsg_alignbytes(void);
__END_DECLS

#ifndef _KERNEL

__BEGIN_DECLS
int accept(int, struct sockaddr * __restrict, socklen_t * __restrict);
int bind(int, const struct sockaddr *, socklen_t);
int connect(int, const struct sockaddr *, socklen_t);
int getpeername(int, struct sockaddr * __restrict, socklen_t * __restrict);
int getsockname(int, struct sockaddr * __restrict, socklen_t * __restrict);
int getsockopt(int, int, int, void * __restrict, socklen_t * __restrict);
int listen(int, int);
ssize_t recv(int, void *, size_t, int);
ssize_t recvfrom(int, void * __restrict, size_t, int,
      struct sockaddr * __restrict, socklen_t * __restrict);
ssize_t recvmsg(int, struct msghdr *, int);
ssize_t send(int, const void *, size_t, int);
ssize_t sendto(int, const void *,
      size_t, int, const struct sockaddr *, socklen_t);
ssize_t sendmsg(int, const struct msghdr *, int);
int setsockopt(int, int, int, const void *, socklen_t);
int shutdown(int, int);
int sockatmark(int);
int socket(int, int, int)
#if !defined(__LIBC12_SOURCE__) && !defined(_STANDALONE)
__RENAME(__socket30)
#endif
           ;
int socketpair(int, int, int, int *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* !_SYS_SOCKET_H_ */
