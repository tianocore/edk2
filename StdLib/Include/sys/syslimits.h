/*  $NetBSD: syslimits.h,v 1.23 2005/12/11 12:25:21 christos Exp $  */

/*
 * Copyright (c) 1988, 1993
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
 *  @(#)syslimits.h 8.1 (Berkeley) 6/2/93
 */

#ifndef _SYS_SYSLIMITS_H_
#define _SYS_SYSLIMITS_H_

#include <sys/featuretest.h>

#define ARG_MAX    (2 * 1024) /* max bytes for an exec function */
#define ARGC_MAX    (ARG_MAX / 2)   /* Maximum value for argc */

#ifndef CHILD_MAX
  #define CHILD_MAX     128 /* max simultaneous processes */
#endif
#define MAX_INPUT     255 /* max bytes in terminal input */
#define NAME_MAX      255 /* max bytes in a file name */
#ifndef OPEN_MAX
  #define OPEN_MAX       20 /* max open files per process */
#endif
#define PATH_MAX     1024 /* max bytes in pathname */
#define PIPE_BUF      512 /* max bytes for atomic pipe writes */

#define LOGIN_NAME_MAX       17 /* max login name length incl. NUL */

#endif /* !_SYS_SYSLIMITS_H_ */
