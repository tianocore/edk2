/** @file
    Machine dependent constants for Intel IA32 Architecture.

    Copyright (c) 2010-2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *  @(#)param.h 5.8 (Berkeley) 6/28/91
 *  $NetBSD: param.h,v 1.61 2006/08/28 13:43:35 yamt Exp
 */
#ifndef _I386_PARAM_H_
#define _I386_PARAM_H_

#define _MACHINE      i386
#define MACHINE       "i386"
#define _MACHINE_ARCH i386
#define MACHINE_ARCH  "i386"
#define MID_MACHINE   MID_I386

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
#define ALIGNBYTES            (sizeof(int) - 1)
#define ALIGN(p)              (((EFI_ULONG_T)(p) + ALIGNBYTES) & ~ALIGNBYTES)
#define ALIGNED_POINTER(p,t)  1

#define PGSHIFT     12              /* LOG2(NBPG) */
#define NBPG        (1 << PGSHIFT)  /* bytes/page */
#define PGOFSET     (NBPG-1)        /* byte offset into page */
#define NPTEPG      (NBPG/(sizeof (pt_entry_t)))

#define DEV_BSHIFT      9           /* log2(DEV_BSIZE) */
#define DEV_BSIZE       (1 << DEV_BSHIFT)
#define BLKDEV_IOSIZE   2048
#ifndef MAXPHYS
  #define MAXPHYS       (64 * 1024) /* max raw I/O transfer size */
#endif

#define SSIZE   1   /* initial stack size/NBPG */
#define SINCR   1   /* increment of stack/NBPG */

#ifndef UPAGES
  #ifdef NOREDZONE
    #define UPAGES    2   /* pages of u-area */
  #else
    #define UPAGES    3
  #endif /*NOREDZONE */
#endif /* !defined(UPAGES) */
#define USPACE        (UPAGES * NBPG) /* total size of u-area */

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
#define MSIZE         256   /* size of an mbuf */

#ifndef MCLSHIFT
  #define MCLSHIFT    11    /* convert bytes to m_buf clusters */
            /* 2K cluster can hold Ether frame */
#endif  /* MCLSHIFT */

#define MCLBYTES      (1 << MCLSHIFT) /* size of a m_buf cluster */

#ifndef NMBCLUSTERS
  #ifdef GATEWAY
    #define NMBCLUSTERS 2048    /* map size, max cluster allocation */
  #else
    #define NMBCLUSTERS 1024    /* map size, max cluster allocation */
  #endif
#endif

#ifndef NFS_RSIZE
  #define NFS_RSIZE   32768
#endif
#ifndef NFS_WSIZE
  #define NFS_WSIZE   32768
#endif

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 */
#define NKMEMPAGES_MIN_DEFAULT  ((8 * 1024 * 1024) >> PAGE_SHIFT)
#define NKMEMPAGES_MAX_DEFAULT  ((128 * 1024 * 1024) >> PAGE_SHIFT)

/*
 * Mach derived conversion macros
 */
#define x86_round_pdr(x)  ((((EFI_ULONG_T)(x)) + PDOFSET) & ~PDOFSET)
#define x86_trunc_pdr(x)    ((EFI_ULONG_T)(x) & ~PDOFSET)
#define x86_btod(x)         ((EFI_ULONG_T)(x) >> PDSHIFT)
#define x86_dtob(x)         ((EFI_ULONG_T)(x) << PDSHIFT)
#define x86_round_page(x) ((((EFI_ULONG_T)(x)) + PGOFSET) & ~PGOFSET)
#define x86_trunc_page(x)   ((EFI_ULONG_T)(x) & ~PGOFSET)
#define x86_btop(x)         ((EFI_ULONG_T)(x) >> PGSHIFT)
#define x86_ptob(x)         ((EFI_ULONG_T)(x) << PGSHIFT)

#endif /* _I386_PARAM_H_ */
