/** @file

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1989, 1993
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
 *  @(#)unistd.h  8.2 (Berkeley) 1/7/94
    NetBSD: unistd.h,v 1.35 2006/08/14 18:17:48 rpaulo Exp
**/
#ifndef _SYS_UNISTD_H_
#define _SYS_UNISTD_H_

#include <sys/featuretest.h>

/* compile-time symbolic constants */

/*
 * According to POSIX 1003.1:
 * "The saved set-user-ID capability allows a program to regain the
 * effective user ID established at the last exec call."
 * However, the setuid/setgid function as specified by POSIX 1003.1 does
 * not allow changing the effective ID from the super-user without also
 * changed the saved ID, so it is impossible to get super-user privileges
 * back later.  Instead we provide this feature independent of the current
 * effective ID through the seteuid/setegid function.  In addition, we do
 * not use the saved ID as specified by POSIX 1003.1 in setuid/setgid,
 * because this would make it impossible for a set-user-ID executable
 * owned by a user other than the super-user to permanently revoke its
 * extra privileges.
 */
#ifdef  _NOT_AVAILABLE
#define _POSIX_SAVED_IDS  /* saved set-user-ID and set-group-ID */
#endif

#define _POSIX_VERSION    199009L
#define _POSIX2_VERSION   199212L

/* execution-time symbolic constants */
        /* timers */
#define _POSIX_TIMERS   200112L

/* Always ensure that these are consistent with <fcntl.h>!
   whence values for lseek(2).
*/
#ifndef SEEK_SET
#define SEEK_SET  0 /**< set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR  1 /**< set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END  2 /**< set file offset to EOF plus offset */
#endif

/* whence values for lseek(2); renamed by POSIX 1003.1 */
#define L_SET   SEEK_SET
#define L_INCR    SEEK_CUR
#define L_XTND    SEEK_END

/* configurable system strings */
#define _CS_PATH     1

#endif /* !_SYS_UNISTD_H_ */
