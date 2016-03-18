/** @file
    Close an open directory.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1983, 1993
     Regents of the University of California.  All rights reserved.

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

    NetBSD: closedir.c,v 1.15 2006/05/17 20:36:50 christos Exp
    closedir.c  8.1 (Berkeley) 6/10/93
**/
#include <sys/cdefs.h>

#include  <namespace.h>
#include  <reentrant.h>
#include  <extern.h>
#include <sys/types.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(closedir,_closedir)
#endif

/*
 * close a directory.
 */
int
closedir(DIR *dirp)
{
  int fd;

  _DIAGASSERT(dirp != NULL);

#ifdef _REENTRANT
  if (__isthreaded)
    mutex_lock((mutex_t *)dirp->dd_lock);
#endif
  fd = dirp->dd_fd;
  dirp->dd_fd = -1;
  dirp->dd_loc = 0;
  free(dirp->dd_buf);

#ifdef _REENTRANT
  if (__isthreaded) {
    mutex_unlock((mutex_t *)dirp->dd_lock);
    mutex_destroy((mutex_t *)dirp->dd_lock);
    free(dirp->dd_lock);
  }
#endif
  free((void *)dirp);
  return(close(fd));
}
