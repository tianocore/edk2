/*  $NetBSD: param.h,v 1.244.2.9.2.2 2008/10/05 08:44:03 bouyer Exp $ */

/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *  @(#)param.h 8.3 (Berkeley) 4/4/95
 */

#ifndef _SYS_PARAM_H_
#define _SYS_PARAM_H_

/*
 * Historic BSD #defines -- probably will remain untouched for all time.
 */
#define BSD 199506    /* System version (year & month). */
#define BSD4_3  1
#define BSD4_4  1

/*
 *  #define __NetBSD_Version__ MMmmrrpp00
 *
 *  M = major version
 *  m = minor version; a minor number of 99 indicates current.
 *  r = 0 (*)
 *  p = patchlevel
 *
 * When new releases are made, src/gnu/usr.bin/groff/tmac/mdoc.local
 * needs to be updated and the changes sent back to the groff maintainers.
 *
 * (*)  Up to 2.0I "release" used to be "",A-Z,Z[A-Z] but numeric
 *        e.g. NetBSD-1.2D  = 102040000 ('D' == 4)
 *  NetBSD-2.0H   (200080000) was changed on 20041001 to:
 *  2.99.9    (299000900)
 */

#define __NetBSD_Version__  400000003 /* NetBSD 4.0.1 */

#define __NetBSD_Prereq__(M,m,p) (((((M) * 100000000) + \
    (m) * 1000000) + (p) * 100) <= __NetBSD_Version__)

/*
 * Historical NetBSD #define
 *
 * NetBSD 1.4 was the last release for which this value was incremented.
 * The value is now permanently fixed at 199905. It will never be
 * changed again.
 *
 * New code must use __NetBSD_Version__ instead, and should not even
 * count on NetBSD being defined.
 *
 */

#define NetBSD  199905    /* NetBSD version (year & month). */

#ifndef _LOCORE
//#include <sys/inttypes.h>
#include <sys/types.h>
#endif

/*
 * Machine-independent constants (some used in following include files).
 * Redefined constants are from POSIX 1003.1 limits file.
 *
 * MAXCOMLEN should be >= sizeof(ac_comm) (see <acct.h>)
 * MAXLOGNAME should be >= UT_NAMESIZE (see <utmp.h>)
 */
#include <sys/syslimits.h>

#define MAXCOMLEN 16    /* max command name remembered */
#define MAXINTERP PATH_MAX  /* max interpreter file name length */
/* DEPRECATED: use LOGIN_NAME_MAX instead. */
#define MAXLOGNAME  (LOGIN_NAME_MAX - 1) /* max login name length */
#define NCARGS    ARG_MAX   /* max bytes for an exec function */
#define NGROUPS   NGROUPS_MAX /* max number groups */
#define NOGROUP   65535   /* marker for empty group set member */
#define MAXHOSTNAMELEN  256   /* max hostname size */

#ifndef NOFILE
#define NOFILE    OPEN_MAX  /* max open files per process */
#endif
#ifndef MAXUPRC       /* max simultaneous processes */
#define MAXUPRC   CHILD_MAX /* POSIX 1003.1-compliant default */
#else
#if (MAXUPRC - 0) < CHILD_MAX
#error MAXUPRC less than CHILD_MAX.  See options(4) for details.
#endif /* (MAXUPRC - 0) < CHILD_MAX */
#endif /* !defined(MAXUPRC) */

/* Signals. */
#include <sys/signal.h>

/* Machine type dependent parameters. */
#include <machine/param.h>
#include <machine/limits.h>

/* pages ("clicks") to disk blocks */
#define ctod(x)   ((x) << (PGSHIFT - DEV_BSHIFT))
#define dtoc(x)   ((x) >> (PGSHIFT - DEV_BSHIFT))

/* bytes to pages */
#define ctob(x)   ((x) << PGSHIFT)
#define btoc(x)   (((x) + PGOFSET) >> PGSHIFT)

/* bytes to disk blocks */
#define dbtob(x)  ((x) << DEV_BSHIFT)
#define btodb(x)  ((x) >> DEV_BSHIFT)

/*
 * Stack macros.  On most architectures, the stack grows down,
 * towards lower addresses; it is the rare architecture where
 * it grows up, towards higher addresses.
 *
 * STACK_GROW and STACK_SHRINK adjust a stack pointer by some
 * size, no questions asked.  STACK_ALIGN aligns a stack pointer.
 *
 * STACK_ALLOC returns a pointer to allocated stack space of
 * some size; given such a pointer and a size, STACK_MAX gives
 * the maximum (in the "maxsaddr" sense) stack address of the
 * allocated memory.
 */
#if defined(_KERNEL) || defined(__EXPOSE_STACK)
#ifdef __MACHINE_STACK_GROWS_UP
#define STACK_GROW(sp, _size)   (((caddr_t)(sp)) + (_size))
#define STACK_SHRINK(sp, _size)   (((caddr_t)(sp)) - (_size))
#define STACK_ALIGN(sp, bytes)  \
  ((caddr_t)((((unsigned long)(sp)) + (bytes)) & ~(bytes)))
#define STACK_ALLOC(sp, _size)    ((caddr_t)(sp))
#define STACK_MAX(p, _size)   (((caddr_t)(p)) + (_size))
#else
#define STACK_GROW(sp, _size)   (((caddr_t)(sp)) - (_size))
#define STACK_SHRINK(sp, _size)   (((caddr_t)(sp)) + (_size))
#define STACK_ALIGN(sp, bytes)  \
  ((caddr_t)(((unsigned long)(sp)) & ~(bytes)))
#define STACK_ALLOC(sp, _size)    (((caddr_t)(sp)) - (_size))
#define STACK_MAX(p, _size)   ((caddr_t)(p))
#endif
#endif /* defined(_KERNEL) || defined(__EXPOSE_STACK) */

/*
 * Priorities.  Note that with 32 run queues, differences less than 4 are
 * insignificant.
 */
#define PSWP  0
#define PVM 4
#define PINOD 8
#define PRIBIO  16
#define PVFS  20
#define PZERO 22    /* No longer magic, shouldn't be here.  XXX */
#define PSOCK 24
#define PWAIT 32
#define PLOCK 36
#define PPAUSE  40
#define PUSER 50
#define MAXPRI  127   /* Priorities range from 0 through MAXPRI. */

#define PRIMASK   0x0ff
#define PCATCH    0x100 /* OR'd with pri for tsleep to check signals */
#define PNORELOCK 0x200 /* OR'd with pri for cond_wait() to not relock
           the interlock */
#define PNOEXITERR      0x400   /* OR'd with pri for tsleep to not exit
           with an error code when LWPs are exiting */
#define NBPW  sizeof(int) /* number of bytes per word (integer) */

#define CMASK 022   /* default file mask: S_IWGRP|S_IWOTH */
#define NODEV (dev_t)(-1) /* non-existent device */

#define CBLOCK  64    /* Clist block size, must be a power of 2. */
#define CBQSIZE (CBLOCK/NBBY) /* Quote bytes/cblock - can do better. */
        /* Data chars/clist. */
#define CBSIZE  (CBLOCK - (int)sizeof(struct cblock *) - CBQSIZE)
#define CROUND  (CBLOCK - 1)  /* Clist rounding. */

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of at most MAXBSIZE units, with
 * smaller units (fragments) only in the last direct block.  MAXBSIZE
 * primarily determines the size of buffers in the buffer pool.  It may be
 * made larger without any effect on existing file systems; however making
 * it smaller may make some file systems unmountable.
 */
#ifndef MAXBSIZE        /* XXX */
#define MAXBSIZE  MAXPHYS
#endif
#define MAXFRAG   8

/*
 * MAXPATHLEN defines the longest permissible path length after expanding
 * symbolic links. It is used to allocate a temporary buffer from the buffer
 * pool in which to do the name expansion, hence should be a power of two,
 * and must be less than or equal to MAXBSIZE.  MAXSYMLINKS defines the
 * maximum number of symbolic links that may be expanded in a path name.
 * It should be set high enough to allow all legitimate uses, but halt
 * infinite loops reasonably quickly.
 */
#define MAXPATHLEN  PATH_MAX
#define MAXSYMLINKS 32

/* Bit map related macros. */
#define setbit(a,i) ((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define clrbit(a,i) ((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define isset(a,i)  ((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define isclr(a,i)  (((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

/* Macros for counting and rounding. */
#ifndef howmany
#define howmany(x, y) (((x)+((y)-1))/(y))
#endif
#define roundup(x, y) ((((x)+((y)-1))/(y))*(y))
#define rounddown(x,y)  (((x)/(y))*(y))
#define powerof2(x) ((((x)-1)&(x))==0)

///* Macros for min/max. */
//#define MIN(a,b)  (((a)<(b))?(a):(b))
//#define MAX(a,b)  (((a)>(b))?(a):(b))

/*
 * Constants for setting the parameters of the kernel memory allocator.
 *
 * 2 ** MINBUCKET is the smallest unit of memory that will be
 * allocated. It must be at least large enough to hold a pointer.
 *
 * Units of memory less or equal to MAXALLOCSAVE will permanently
 * allocate physical memory; requests for these size pieces of
 * memory are quite fast. Allocations greater than MAXALLOCSAVE must
 * always allocate and free physical memory; requests for these
 * size allocations should be done infrequently as they will be slow.
 *
 * Constraints: NBPG <= MAXALLOCSAVE <= 2 ** (MINBUCKET + 14), and
 * MAXALLOCSAVE must be a power of two.
 */
#ifdef _LP64
#define MINBUCKET 5   /* 5 => min allocation of 32 bytes */
#else
#define MINBUCKET 4   /* 4 => min allocation of 16 bytes */
#endif
#define MAXALLOCSAVE  (2 * NBPG)

/*
 * Scale factor for scaled integers used to count %cpu time and load avgs.
 *
 * The number of CPU `tick's that map to a unique `%age' can be expressed
 * by the formula (1 / (2 ^ (FSHIFT - 11))).  The maximum load average that
 * can be calculated (assuming 32 bits) can be closely approximated using
 * the formula (2 ^ (2 * (16 - FSHIFT))) for (FSHIFT < 15).
 *
 * For the scheduler to maintain a 1:1 mapping of CPU `tick' to `%age',
 * FSHIFT must be at least 11; this gives us a maximum load avg of ~1024.
 */
#define FSHIFT  11    /* bits to right of fixed binary point */
#define FSCALE  (1<<FSHIFT)

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define        MAXSLP          20

/*
 * Defaults for Unified Buffer Cache parameters.
 * These may be overridden in <machine/param.h>.
 */

#ifndef UBC_WINSHIFT
#define UBC_WINSHIFT  13
#endif
#ifndef UBC_NWINS
#define UBC_NWINS 1024
#endif

#ifdef _KERNEL
/*
 * macro to convert from milliseconds to hz without integer overflow
 * Default version using only 32bits arithmetics.
 * 64bit port can define 64bit version in their <machine/param.h>
 * 0x20000 is safe for hz < 20000
 */
#ifndef mstohz
#define mstohz(ms) \
  (__predict_false((ms) >= 0x20000) ? \
      ((ms +0u) / 1000u) * hz : \
      ((ms +0u) * hz) / 1000u)
#endif
#endif /* _KERNEL */

#endif /* !_SYS_PARAM_H_ */
