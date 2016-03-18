/** @file

    Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

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
 *  @(#)paths.h 8.1 (Berkeley) 6/2/93
 *  $NetBSD: paths.h,v 1.30 2004/12/11 06:01:33 christos Exp
 */

#ifndef _PATHS_H_
#define _PATHS_H_

/* Provide trailing slash, since mostly used for building pathnames.
 * see the __CONCAT() macro from <sys/EfiCdefs.h> for cpp examples.
 */
#define _PATH_DEV         "/dev/"
#define _PATH_STDLIB      "/Efi/StdLib/"
#define _PATH_ETC         _PATH_STDLIB "etc/"
#define _PATH_TMP         _PATH_STDLIB "tmp/"
#define _PATH_LIB         _PATH_STDLIB "lib/"
#define _PATH_BIN         "/Efi/Tools/"

/* DOS style device paths */
#define _PATH_TTYDEV      "tty:"
#define _PATH_NULLDEV     "null:"
#define _PATH_CONSOLE     "console:"
#define _PATH_CONSTTY     "constty:"
#define _PATH_STDIN       "stdin:"
#define _PATH_STDOUT      "stdout:"
#define _PATH_STDERR      "stderr:"
#define _PATH_SOCKET      "socket:"

/* *nix style device paths */
#define _PATH_DEVTTY      _PATH_DEV "tty"
#define _PATH_DEVNULL     _PATH_DEV "null"
#define _PATH_DEVCONSOLE  _PATH_DEV "console"
#define _PATH_DEVCONSTTY  _PATH_DEV "constty"
#define _PATH_DEVSTDIN    _PATH_DEV "stdin"
#define _PATH_DEVSTDOUT   _PATH_DEV "stdout"
#define _PATH_DEVSTDERR   _PATH_DEV "stderr"
#define _PATH_DEVSOCKET   _PATH_DEV "socket"

/* Special files and locations */
#define _PATH_FSTAB       _PATH_ETC "fstab"
#define _PATH_HOSTNAME    _PATH_ETC "hostname"
#define _PATH_HOSTS       _PATH_ETC "hosts"
#define _PATH_HOSTCONF    _PATH_ETC "host.conf"
#define _PATH_LOCALE      _PATH_ETC "Locale"
#define _PATH_NETCONF     _PATH_ETC "host.conf"
#define _PATH_NETWORKS    _PATH_ETC "networks"
#define _PATH_PROTOCOLS   _PATH_ETC "protocols"

/* Resolver configuration file.
 * Normally not present, but may contain the address of the
 * inital name server(s) to query and the domain search list.
 */
#define _PATH_RESCONF     _PATH_ETC "resolv.conf"
#define _PATH_SERVICES    _PATH_ETC "services"

#endif /* !_PATHS_H_ */
