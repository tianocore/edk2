/** @file

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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

    tmpfile.c 8.1 (Berkeley) 6/4/93
    NetBSD: tmpfile.c,v 1.11 2003/08/07 16:43:33 agc Exp
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  "namespace.h"
#include  <sys/types.h>
#include  <signal.h>
#include  <errno.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <paths.h>
#include  <unistd.h>

#define TRAILER "tmp.XXXX"

FILE *
tmpfile()
{
  //sigset_t set, oset;
  FILE *fp;
  int fd, sverrno;
  char buf[sizeof(_PATH_TMP) + sizeof(TRAILER)];

  (void)memcpy(buf, _PATH_TMP, sizeof(_PATH_TMP) - 1);
  (void)memcpy(buf + sizeof(_PATH_TMP) - 1, TRAILER, sizeof(TRAILER));

  //sigfillset(&set);
  //(void)sigprocmask(SIG_BLOCK, &set, &oset);

  fd = mkstemp(buf);
  if (fd != -1) {
    /*  Changed from unlink(buf) because of differences between the behavior
        of Unix and UEFI file systems.
    */
    (void)DeleteOnClose(fd);
  }

  //(void)sigprocmask(SIG_SETMASK, &oset, NULL);

  if (fd == -1)
    return (NULL);

  if ((fp = fdopen(fd, "w+")) == NULL) {
    sverrno = errno;
    (void)close(fd);
    errno = sverrno;
    return (NULL);
  }
  return (fp);
}
