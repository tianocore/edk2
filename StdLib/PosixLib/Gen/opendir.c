/** @file
    Open a directory.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1983, 1993
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

    NetBSD: opendir.c,v 1.33 2008/01/10 09:49:04 elad Exp
    opendir.c 8.7 (Berkeley) 12/10/94
**/
#include <sys/cdefs.h>

#include  <namespace.h>
#include  <reentrant.h>
#include  <extern.h>
#include <sys/param.h>
//#include <sys/mount.h>
#include <sys/stat.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXITERATIONS 100

/*
 * Open a directory.
 */
DIR *
opendir(const char *name)
{
  _DIAGASSERT(name != NULL);

  return (__opendir2(name, DTF_HIDEW|DTF_NODUP));
}

DIR *
__opendir2(const char *name, int flags)
{
  DIR *dirp = NULL;
  int fd;
  int serrno;
  struct stat sb;
  int incr;

  _DIAGASSERT(name != NULL);

  if ((fd = open(name, O_RDONLY | O_NONBLOCK, 0)) == -1 ||
      fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
    goto error;
  if (fstat(fd, &sb) || !S_ISDIR(sb.st_mode)) {
    errno = ENOTDIR;
    goto error;
  }
  if ((dirp = (DIR *)malloc(sizeof(DIR))) == NULL)
    goto error;
  dirp->dd_buf = NULL;

  /*
   * If the machine's page size is an exact multiple of DIRBLKSIZ,
   * use a buffer that is cluster boundary aligned.
   * Hopefully this can be a big win someday by allowing page trades
   * to user space to be done by getdirentries()
   */
  incr = DIRBLKSIZ;

  dirp->dd_len = incr;
  dirp->dd_buf = malloc((size_t)dirp->dd_len);
  if (dirp->dd_buf == NULL)
    goto error;
  dirp->dd_seek = 0;
  flags &= ~DTF_REWIND;

  dirp->dd_loc = 0;
  dirp->dd_fd = fd;
  dirp->dd_flags = flags;

  /*
   * Set up seek point for rewinddir.
   */
#ifdef _REENTRANT
  if (__isthreaded) {
    if ((dirp->dd_lock = malloc(sizeof(mutex_t))) == NULL)
      goto error;
    mutex_init((mutex_t *)dirp->dd_lock, NULL);
  }
#endif
  dirp->dd_internal = NULL;
  return (dirp);
error:
  serrno = errno;
  if (dirp && dirp->dd_buf)
    free(dirp->dd_buf);
  if (dirp)
    free(dirp);
  if (fd != -1)
    (void)close(fd);
  errno = serrno;
  return NULL;
}
