/** @file
    This file includes the definitions for open and fcntl described by POSIX
    for <fcntl.h>; it also includes related kernel definitions.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made
    available under the terms and conditions of the BSD License which
    accompanies this distribution.  The full text of the license may be found
    at http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1983, 1990, 1993
     The Regents of the University of California.  All rights reserved.
    (c) UNIX System Laboratories, Inc.
    All or some portions of this file are derived from material licensed
    to the University of California by American Telephone and Telegraph
    Co. or Unix System Laboratories, Inc. and are reproduced herein with
    the permission of UNIX System Laboratories, Inc.

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

    fcntl.h 8.3 (Berkeley) 1/21/94
    NetBSD: fcntl.h,v 1.34 2006/10/05 14:48:33 chs Exp
 */
#ifndef _SYS_FCNTL_H_
#define _SYS_FCNTL_H_

#include <sys/featuretest.h>
#include <sys/types.h>

#include <sys/stat.h>

/*
 * File status flags: these are used by open(2), fcntl(2).
 * They are also used (indirectly) in the kernel file structure f_flags,
 * which is a superset of the open/fcntl flags.  Open flags and f_flags
 * are inter-convertible using OFLAGS(fflags) and FFLAGS(oflags).
 * Open/fcntl flags begin with O_; kernel-internal flags begin with F.
 */
/* open-only flags */
#define O_RDONLY    0x00000000  /* open for reading only */
#define O_WRONLY    0x00000001  /* open for writing only */
#define O_RDWR      0x00000002  /* open for reading and writing */
#define O_ACCMODE   0x00000003  /* mask for above modes */

/*
 * Kernel encoding of open mode; separate read and write bits that are
 * independently testable: 1 greater than the above.
 *
 * XXX
 * FREAD and FWRITE are excluded from the #ifdef _KERNEL so that TIOCFLUSH,
 * which was documented to use FREAD/FWRITE, continues to work.
 */
#define FREAD       0x00000001
#define FWRITE      0x00000002
#define O_NONBLOCK  0x00000004  /* no delay */
#define O_APPEND    0x00000008  /* set append mode */
#define O_CREAT     0x00000200  /* create if nonexistent */
#define O_TRUNC     0x00000400  /* truncate to zero length */
#define O_EXCL      0x00000800  /* error if already exists */

//#define O_DIRECT    0x00080000  /* direct I/O hint */

#define O_SETMASK     0x0000000F  /* Flags modifiable by F_SETFD (fcntl) */

/*
 * Constants used for fcntl(2)
 */

/* command values */
#define F_DUPFD      0  /* duplicate file descriptor */
#define F_GETFD      1  /* get file descriptor flags */
#define F_SETFD      2  /* set file descriptor flags */
#define F_GETFL      3  /* get file status flags */
#define F_SETFL      4  /* set file status flags */
#define F_GETOWN     5  /* get SIGIO/SIGURG proc/pgrp */
#define F_SETOWN     6  /* set SIGIO/SIGURG proc/pgrp */
#define F_GETLK      7  /* get record locking information */
#define F_SETLK      8  /* set record locking information */
#define F_SETLKW     9  /* F_SETLK; wait if blocked */
#define F_CLOSEM    10  /* close all fds >= to the one given */
#define F_MAXFD     11  /* return the max open fd */

/* file descriptor flags (F_GETFD, F_SETFD) */
#define FD_CLOEXEC  1   /* close-on-exec flag */

/* record locking flags (F_GETLK, F_SETLK, F_SETLKW) */
#define F_RDLCK     1   /* shared or read lock */
#define F_UNLCK     2   /* unlock */
#define F_WRLCK     3   /* exclusive or write lock */

/* Constants for fcntl's passed to the underlying fs - like ioctl's. */
#define F_PARAM_MASK    0xfff
#define F_PARAM_LEN(x)  (((x) >> 16) & F_PARAM_MASK)
#define F_PARAM_MAX     4095
#define F_FSCTL         (int)0x80000000   /* This fcntl goes to the fs */
#define F_FSVOID        (int)0x40000000   /* no parameters */
#define F_FSOUT         (int)0x20000000   /* copy out parameter */
#define F_FSIN          (int)0x10000000   /* copy in parameter */
#define F_FSINOUT       (F_FSIN | F_FSOUT)
#define F_FSDIRMASK     (int)0x70000000   /* mask for IN/OUT/VOID */
#define F_FSPRIV        (int)0x00008000   /* command is fs-specific */

/* Always ensure that these are consistent with <stdio.h> and <unistd.h>! */
#ifndef SEEK_SET
  #define SEEK_SET  0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
  #define SEEK_CUR  1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
  #define SEEK_END  2 /* set file offset to EOF plus offset */
#endif

#include  <sys/EfiCdefs.h>

__BEGIN_DECLS
#ifndef __FCNTL_SYSCALLS_DECLARED
  #define __FCNTL_SYSCALLS_DECLARED
  int open(const char *, int, int );
  int creat(const char *, mode_t);
  int fcntl(int, int, ...);
#endif  // __FCNTL_SYSCALLS_DECLARED
__END_DECLS

#endif /* !_SYS_FCNTL_H_ */
