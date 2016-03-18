/** @file
    Get next entry in a directory.

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

    NetBSD: readdir.c,v 1.24 2008/05/04 18:53:26 tonnerre Exp
    readdir.c 8.3 (Berkeley) 9/29/94
 */
#include <sys/cdefs.h>

#include  <namespace.h>
#include  <reentrant.h>
#include  <extern.h>
#include <sys/param.h>
#include  <sys/stdint.h>

#include  <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/*
 * get next entry in a directory.
 */
struct dirent *
_readdir_unlocked(DIR *dirp, int skipdeleted)
{
  struct dirent *dp;


  for (;;) {
    if (dirp->dd_loc >= dirp->dd_size) {
      if (dirp->dd_flags & __DTF_READALL)
        return (NULL);
      dirp->dd_loc = 0;
    }
    if (dirp->dd_loc == 0 && !(dirp->dd_flags & __DTF_READALL)) {
      dirp->dd_size = (long)read(dirp->dd_fd, dirp->dd_buf, (size_t)dirp->dd_len);
      if (dirp->dd_size <= 0)
        return (NULL);
    }
    dp = (struct dirent *) (void *)(dirp->dd_buf + (size_t)dirp->dd_loc);
    if ((intptr_t)dp & _DIRENT_ALIGN(dp))/* bogus pointer check */
      return (NULL);
    dirp->dd_loc += (long)dp->Size;
    if ((dp->Attribute & DT_HIDDEN) && (dirp->dd_flags & DTF_HIDEW))
      continue;
    return (dp);
  }
}

struct dirent *
readdir(DIR *dirp)
{
  struct dirent *dp;

#ifdef _REENTRANT
  if (__isthreaded) {
    mutex_lock((mutex_t *)dirp->dd_lock);
    dp = _readdir_unlocked(dirp, 1);
    mutex_unlock((mutex_t *)dirp->dd_lock);
  }
  else
#endif
    dp = _readdir_unlocked(dirp, 1);
  return (dp);
}

int
readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
  struct dirent *dp;
  int saved_errno;

  saved_errno = errno;
  errno = 0;
#ifdef _REENTRANT
  if (__isthreaded) {
    mutex_lock((mutex_t *)dirp->dd_lock);
    if ((dp = _readdir_unlocked(dirp, 1)) != NULL)
      memcpy(entry, dp, (size_t)_DIRENT_SIZE(dp));
    mutex_unlock((mutex_t *)dirp->dd_lock);
  }
  else
#endif
    if ((dp = _readdir_unlocked(dirp, 1)) != NULL)
      memcpy(entry, dp, (size_t)_DIRENT_SIZE(dp));

  if (errno != 0) {
    if (dp == NULL)
      return (errno);
  } else
    errno = saved_errno;

  if (dp != NULL)
    *result = entry;
  else
    *result = NULL;

  return (0);
}
