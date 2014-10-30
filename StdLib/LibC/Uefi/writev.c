/** @file
 *
 * Copyright (c) 1999 - 2014, Intel Corporation. All rights reserved.<BR>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software must
 *    display the following acknowledgement:
 *
 *    This product includes software developed by Intel Corporation and its
 *    contributors.
 *
 * 4. Neither the name of Intel Corporation or its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL INTEL CORPORATION OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*++

Module Name:

    writev.c

Abstract:

    Functions implementing the standard "writev" system call interface


Revision History

--*/
#include  <LibConfig.h>

#ifdef foo
#include <efi_interface.h>
#include <unistd.h>
#include <fcntl.h>
#define KERNEL
#include <errno.h>
#undef KERNEL
#include "./filedesc.h"

#include <libc_debug.h>
#include <assert.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#ifndef KERNEL
#define KERNEL
#include <errno.h>
#undef KERNEL
#else
#include <errno.h>
#endif

//
//  Name:
//      writev
//
//  Description:
//      BSD writev interface for libc
//
//  Arguments:
//      File Descriptor (index into file descriptor table)
//      iovec pointer
//      size of iovec array
//
//  Returns:
//      number of bytes written
//

ssize_t
writev(
    int fd,
    const struct iovec *iov,
    int iovcnt
    )
{
  const struct iovec   *pVecTmp;
  char                 *pBuf;
  size_t                TotalBytes;
  size_t                i;
  size_t                ret;

  //
  //  See how much memory we'll need
  //

  for (i = 0, TotalBytes = 0, pVecTmp = iov; i < (size_t)iovcnt; i++, pVecTmp++) {
    TotalBytes += pVecTmp->iov_len;
  }

  //
  //  Allocate a contiguous buffer
  //

  pBuf = (char*)malloc (TotalBytes);
  if (pBuf == NULL) {
    errno = ENOMEM;
    return -1;
  }

  //
  //  Copy vectors to the buffer
  //

  for (; iovcnt; iovcnt--) {
    bcopy(iov->iov_base, pBuf, iov->iov_len);
    pBuf += iov->iov_len;
    iov++;
  }

  //
  //  Use standard write(2) then free buffer
  //

  ret = write (fd, pBuf, TotalBytes);
  free (pBuf);

  return (ret);
}
