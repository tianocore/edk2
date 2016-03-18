/** @file
    System-specific declarations and macros related to time.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1982, 1986, 1993
    The Regents of the University of California.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    3. Neither the name of the University nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.

    time.h  8.5 (Berkeley) 5/4/95
    NetBSD: time.h,v 1.56 2006/06/18 21:09:24 uwe Exp
 */
#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include  <Uefi.h>
#include  <sys/featuretest.h>
#include  <sys/types.h>

/*
 * Traditional *nix structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
struct timeval {
  LONG32    tv_sec;   /* seconds */
  LONG32    tv_usec;  /* and microseconds */
};

/*
 * Structure defined by POSIX.1b to be like a timeval.
 * This works within EFI since the times really are time_t.
 */
struct timespec {
  time_t  tv_sec;   /* seconds */
  LONG32  tv_nsec;  /* and nanoseconds */
};

#define TIMEVAL_TO_TIMESPEC(tv, ts) do {        \
  (ts)->tv_sec = (tv)->tv_sec;          \
  (ts)->tv_nsec = (tv)->tv_usec * 1000;       \
} while (/*CONSTCOND*/0)

#define TIMESPEC_TO_TIMEVAL(tv, ts) do {        \
  (tv)->tv_sec = (ts)->tv_sec;          \
  (tv)->tv_usec = (ts)->tv_nsec / 1000;       \
} while (/*CONSTCOND*/0)

/* Operations on timevals. */
#define timerclear(tvp)   (tvp)->tv_sec = (tvp)->tv_usec = 0
#define timerisset(tvp)   ((tvp)->tv_sec || (tvp)->tv_usec)

#define timercmp(tvp, uvp, cmp)           \
  (((tvp)->tv_sec == (uvp)->tv_sec) ?       \
      ((tvp)->tv_usec cmp (uvp)->tv_usec) :     \
      ((tvp)->tv_sec cmp (uvp)->tv_sec))

#define timeradd(tvp, uvp, vvp)           \
  do {                \
    (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;    \
    (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec; \
    if ((vvp)->tv_usec >= 1000000) {      \
      (vvp)->tv_sec++;        \
      (vvp)->tv_usec -= 1000000;      \
    }             \
  } while (/* CONSTCOND */ 0)

#define timersub(tvp, uvp, vvp)           \
  do {                \
    (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;    \
    (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec; \
    if ((vvp)->tv_usec < 0) {       \
      (vvp)->tv_sec--;        \
      (vvp)->tv_usec += 1000000;      \
    }             \
  } while (/* CONSTCOND */ 0)

/* Operations on timespecs. */
#define timespecclear(tsp)    (tsp)->tv_sec = (tsp)->tv_nsec = 0
#define timespecisset(tsp)    ((tsp)->tv_sec || (tsp)->tv_nsec)

#define timespeccmp(tsp, usp, cmp)          \
  (((tsp)->tv_sec == (usp)->tv_sec) ?       \
      ((tsp)->tv_nsec cmp (usp)->tv_nsec) :     \
      ((tsp)->tv_sec cmp (usp)->tv_sec))

#define timespecadd(tsp, usp, vsp)          \
  do {                \
    (vsp)->tv_sec = (tsp)->tv_sec + (usp)->tv_sec;    \
    (vsp)->tv_nsec = (tsp)->tv_nsec + (usp)->tv_nsec; \
    if ((vsp)->tv_nsec >= 1000000000L) {      \
      (vsp)->tv_sec++;        \
      (vsp)->tv_nsec -= 1000000000L;      \
    }             \
  } while (/* CONSTCOND */ 0)

#define timespecsub(tsp, usp, vsp)          \
  do {                \
    (vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;    \
    (vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec; \
    if ((vsp)->tv_nsec < 0) {       \
      (vsp)->tv_sec--;        \
      (vsp)->tv_nsec += 1000000000L;      \
    }             \
  } while (/* CONSTCOND */ 0)

/*
 * Names of the interval timers, and structure
 * defining a timer setting.
 */
#define ITIMER_REAL     0
#define ITIMER_VIRTUAL  1
#define ITIMER_PROF     2

struct  itimerval {
  struct  timeval it_interval;  /* timer interval */
  struct  timeval it_value; /* current value */
};

/*
 * Structure defined by POSIX.1b to be like a itimerval, but with
 * timespecs. Used in the timer_*() system calls.
 */
struct  itimerspec {
  struct  timespec it_interval;
  struct  timespec it_value;
};

#define CLOCK_REALTIME  0
#define CLOCK_VIRTUAL   1
#define CLOCK_PROF      2
#define CLOCK_MONOTONIC 3

#define TIMER_RELTIME   0x0 /* relative timer */
#define TIMER_ABSTIME   0x1 /* absolute timer */

#if 0
  #if (_POSIX_C_SOURCE - 0) >= 200112L || \
      (defined(_XOPEN_SOURCE) && defined(_XOPEN_SOURCE_EXTENDED)) || \
      (_XOPEN_SOURCE - 0) >= 500 || defined(_NETBSD_SOURCE)
    #include  <sys/select.h>
  #endif
#endif  /* if 0 */

#include  <sys/EfiCdefs.h>
#include  <time.h>

/* Functions useful for dealing with EFI */
__BEGIN_DECLS

/* Convert an EFI_TIME structure into a time_t value. */
time_t  Efi2Time( EFI_TIME *EfiBDtime);

/* Convert a time_t value into an EFI_TIME structure.
    It is the caller's responsibility to free the returned structure.
*/
EFI_TIME *  Time2Efi(time_t OTime);

/* Convert an EFI_TIME structure into a C Standard tm structure. */
void    Efi2Tm( EFI_TIME *EfiBDtime, struct tm *NewTime);
void    Tm2Efi( struct tm *BdTime, EFI_TIME *ETime);

/* BSD compatibility functions */
int gettimeofday (struct timeval *tp, void *ignore);
/* POSIX compatibility functions */
int getitimer (int which, struct itimerval *value);
int setitimer (int which, const struct itimerval *value, struct itimerval *ovalue);

__END_DECLS

#endif /* !_SYS_TIME_H_ */
