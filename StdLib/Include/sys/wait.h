/*  $NetBSD: wait.h,v 1.24 2005/12/11 12:25:21 christos Exp $ */

/*
 * Copyright (c) 1982, 1986, 1989, 1993, 1994
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
 *  @(#)wait.h  8.2 (Berkeley) 7/10/94
 */
#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

#include <sys/featuretest.h>
#include <sys/types.h>

/*
 * This file holds definitions relevent to the wait4 system call
 * and the alternate interfaces that use it (wait, wait3, waitpid).
 */

/*
 * Macros to test the exit status returned by wait
 * and extract the relevant values.
 */
#define _W_INT(w) (*(int *)(void *)&(w))  /* convert union wait to int */

#define _WSTATUS(x) (_W_INT(x) & 0177)
#define _WSTOPPED 0177    /* _WSTATUS if process is stopped */
#define WIFSTOPPED(x) (_WSTATUS(x) == _WSTOPPED)
#define WSTOPSIG(x) ((int)(((unsigned int)_W_INT(x)) >> 8) & 0xff)
#define WIFSIGNALED(x)  (_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0)
#define WTERMSIG(x) (_WSTATUS(x))
#define WIFEXITED(x)  (_WSTATUS(x) == 0)
#define WEXITSTATUS(x)  ((int)(((unsigned int)_W_INT(x)) >> 8) & 0xff)
#define WCOREFLAG 0200
#define WCOREDUMP(x)  (_W_INT(x) & WCOREFLAG)

#define W_EXITCODE(ret, sig)  ((ret) << 8 | (sig))
#define W_STOPCODE(sig)   ((sig) << 8 | _WSTOPPED)

/*
 * Option bits for the third argument of wait4.  WNOHANG causes the
 * wait to not hang if there are no stopped or terminated processes, rather
 * returning an error indication in this case (pid==0).  WUNTRACED
 * indicates that the caller should receive status about untraced children
 * which stop due to signals.  If children are stopped and a wait without
 * this option is done, it is as though they were still running... nothing
 * about them is returned.
 */
#define WNOHANG   0x00000001  /* don't hang in wait */
#define WUNTRACED 0x00000002  /* tell about stopped,
             untraced children */
#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
#define WALTSIG   0x00000004  /* wait for processes that exit
             with an alternate signal (i.e.
             not SIGCHLD) */
#define WALLSIG   0x00000008  /* wait for processes that exit
             with any signal, i.e. SIGCHLD
             and alternates */

/*
 * These are the Linux names of some of the above flags, for compatibility
 * with Linux's clone(2) API.
 */
#define __WCLONE  WALTSIG
#define __WALL    WALLSIG

/*
 * These bits are used in order to support SVR4 (etc) functionality
 * without replicating sys_wait4 5 times.
 */
#define WNOWAIT   0x00010000  /* Don't mark child 'P_WAITED' */
#define WNOZOMBIE 0x00020000  /* Ignore zombies */
#endif /* _XOPEN_SOURCE || _NETBSD_SOURCE */

/* POSIX extensions and 4.2/4.3 compatibility: */

/*
 * Tokens for special values of the "pid" parameter to wait4.
 */
#define WAIT_ANY  (-1)  /* any process */
#define WAIT_MYPGRP 0 /* any process in my process group */

/*
 * Deprecated:
 * Structure of the information in the status word returned by wait4.
 * If w_stopval==WSTOPPED, then the second structure describes
 * the information returned, else the first.
 */
union wait {
  int w_status;   /* used in syscall */
  /*
   * Terminated process status.
   */
  struct {
#if BYTE_ORDER == LITTLE_ENDIAN
    unsigned int  w_Termsig:7,  /* termination signal */
        w_Coredump:1, /* core dump indicator */
        w_Retcode:8,  /* exit code if w_termsig==0 */
        w_Filler:16;  /* upper bits filler */
#endif
#if BYTE_ORDER == BIG_ENDIAN
    unsigned int  w_Filler:16,  /* upper bits filler */
        w_Retcode:8,  /* exit code if w_termsig==0 */
        w_Coredump:1, /* core dump indicator */
        w_Termsig:7;  /* termination signal */
#endif
  } w_T;
  /*
   * Stopped process status.  Returned
   * only for traced children unless requested
   * with the WUNTRACED option bit.
   */
  struct {
#if BYTE_ORDER == LITTLE_ENDIAN
    unsigned int  w_Stopval:8,  /* == W_STOPPED if stopped */
        w_Stopsig:8,  /* signal that stopped us */
        w_Filler:16;  /* upper bits filler */
#endif
#if BYTE_ORDER == BIG_ENDIAN
    unsigned int  w_Filler:16,  /* upper bits filler */
        w_Stopsig:8,  /* signal that stopped us */
        w_Stopval:8;  /* == W_STOPPED if stopped */
#endif
  } w_S;
};
#define w_termsig w_T.w_Termsig
#define w_coredump  w_T.w_Coredump
#define w_retcode w_T.w_Retcode
#define w_stopval w_S.w_Stopval
#define w_stopsig w_S.w_Stopsig

#define WSTOPPED  _WSTOPPED

__BEGIN_DECLS
pid_t wait(int *);

#if 0   /* Normally declared here but not implemented for UEFI. */
struct rusage;  /* forward declaration */

pid_t waitpid(pid_t, int *, int);
pid_t wait3(int *, int, struct rusage *);
pid_t wait4(pid_t, int *, int, struct rusage *);
#endif
__END_DECLS

#endif /* !_SYS_WAIT_H_ */
