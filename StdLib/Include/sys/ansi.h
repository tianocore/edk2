/** @file
    ANSI type definitions.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1990, 1993
    The Regents of the University of California.  All rights reserved.

    This code is derived from software contributed to The NetBSD Foundation
    by Jun-ichiro itojun Hagino and by Klaus Klein.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
      - Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      - Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      - Neither the name of the University nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    NetBSD: ansi.h,v 1.11 2005/12/11 12:25:20 christos Exp
**/
#ifndef _SYS_ANSI_H_
#define _SYS_ANSI_H_

#include <machine/int_types.h>

typedef INT8 *      __caddr_t;      ///< core address
typedef __uint32_t  __gid_t;        ///< group id
typedef __uint32_t  __in_addr_t;    ///< IP(v4) address
typedef __uint16_t  __in_port_t;    ///< "Internet" port number
typedef __uint32_t  __mode_t;       ///< file permissions
typedef __int64_t   __off_t;        ///< file offset
typedef __int32_t   __pid_t;        ///< process id
typedef __uint8_t   __sa_family_t;  ///< socket address family
typedef UINT32      __socklen_t;    ///< socket-related datum length
typedef __uint32_t  __uid_t;        ///< user id
typedef __uint64_t  __fsblkcnt_t;   ///< fs block count (statvfs)
typedef __uint64_t  __fsfilcnt_t;   ///< fs file count

#endif  /* !_SYS_ANSI_H_ */
