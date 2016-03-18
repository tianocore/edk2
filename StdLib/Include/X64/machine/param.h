/** @file
    Machine dependent constants for the Intel64 Architecture(X64).

    Copyright (c) 2010-2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 *  $NetBSD: param.h,v 1.3 2006/08/28 13:43:35 yamt Exp
**/
#ifndef _X64_PARAM_H_
#define _X64_PARAM_H_

#define _MACHINE      amd64
#define MACHINE       "amd64"
#define _MACHINE_ARCH x86_64
#define MACHINE_ARCH  "x86_64"
#define MID_MACHINE   MID_X86_64

/*
 * Round p (pointer or byte index) up to a correctly-aligned value
 * for all data types (int, long, ...).   The result is u_int and
 * must be cast to any desired pointer type.
 *
 * ALIGNED_POINTER is a boolean macro that checks whether an address
 * is valid to fetch data elements of type t from on this architecture.
 * This does not reflect the optimal alignment, just the possibility
 * (within reasonable limits).
 *
 */
#define ALIGNBYTES            (sizeof(INT64) - 1)
#define ALIGN(p)              (((UINT64)(p) + ALIGNBYTES) &~ALIGNBYTES)
#define ALIGNED_POINTER(p,t)  1

#define ALIGNBYTES32          (sizeof(INT32) - 1)
#define ALIGN32(p)            (((UINT64)(p) + ALIGNBYTES32) &~ALIGNBYTES32)

#define PGSHIFT       12    /* LOG2(NBPG) */
#define NBPG          (1 << PGSHIFT)  /* bytes/page */
#define PGOFSET       (NBPG-1)  /* byte offset into page */
#define NPTEPG        (NBPG/(sizeof (pt_entry_t)))

#define DEV_BSHIFT    9   /* log2(DEV_BSIZE) */
#define DEV_BSIZE     (1 << DEV_BSHIFT)
#define BLKDEV_IOSIZE 2048
#ifndef MAXPHYS
  #define MAXPHYS     (64 * 1024) /* max raw I/O transfer size */
#endif

#define SSIZE         1                 /* initial stack size/NBPG */
#define SINCR         1                 /* increment of stack/NBPG */
#define UPAGES        5                 /* pages of u-area */
#define USPACE        (UPAGES * NBPG)   /* total size of u-area */

#ifndef MSGBUFSIZE
  #define MSGBUFSIZE  4*NBPG          /* default message buffer size */
#endif

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define MSIZE         512   /* size of an mbuf */

#ifndef MCLSHIFT
  #define MCLSHIFT    11    /* convert bytes to m_buf clusters */
            /* 2K cluster can hold Ether frame */
#endif  /* MCLSHIFT */

#define MCLBYTES      (1 << MCLSHIFT) /* size of a m_buf cluster */

#ifndef NMBCLUSTERS
  #ifdef GATEWAY
    #define NMBCLUSTERS 4096    /* map size, max cluster allocation */
  #else
    #define NMBCLUSTERS 2048    /* map size, max cluster allocation */
  #endif
#endif

#ifndef NFS_RSIZE
  #define NFS_RSIZE     32768
#endif
#ifndef NFS_WSIZE
  #define NFS_WSIZE     32768
#endif

#define x86_round_page(x) ((((EFI_ULONG_T)(x)) + PGOFSET) & ~PGOFSET)
#define x86_trunc_page(x)   ((EFI_ULONG_T)(x) & ~PGOFSET)
#define x86_btop(x)         ((EFI_ULONG_T)(x) >> PGSHIFT)
#define x86_ptob(x)         ((EFI_ULONG_T)(x) << PGSHIFT)

#define btop(x)           x86_btop(x)
#define ptob(x)           x86_ptob(x)
#define round_pdr(x)      x86_round_pdr(x)

#define mstohz(ms) ((ms + 0UL) * hz / 1000)

#endif  /* _X64_PARAM_H_ */
