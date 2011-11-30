/** @file

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1997, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein and Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

    NetBSD: dirname.c,v 1.10 2008/05/10 22:39:40 christos Exp
 */
#include  <LibConfig.h>
#include <sys/cdefs.h>

#include <limits.h>
#include  <ctype.h>
#include <string.h>

#ifdef __weak_alias
__weak_alias(dirname,_dirname)
#endif

#ifdef HAVE_DIRNAME
char *
dirname(char *path)
{
  static char singledot[] = ".";
  static char result[PATH_MAX];
  const char *lastp;
  size_t len;

  /*
   * If `path' is a null pointer or points to an empty string,
   * return a pointer to the string ".".
   */
  if ((path == NULL) || (*path == '\0'))
    return (singledot);

  /* Strip trailing slashes, if any. */
  lastp = path + strlen(path) - 1;
  while (lastp != path && isDirSep(*lastp))
    lastp--;

  /* Terminate path at the last occurence of '/'. */
  do {
    if (isDirSep(*lastp)) {
      /* Strip trailing slashes, if any. */
      while (lastp != path && isDirSep(*lastp))
        lastp--;

      /* ...and copy the result into the result buffer.
        We make sure that there will be room for the terminating NUL
        and for a final '/', if necessary.
      */
      len = (lastp - path) + 1 /* last char */;
      if (len > (PATH_MAX - 2))
        len = PATH_MAX - 2;

      memcpy(result, path, len);
      if(*lastp == ':') {   /* Have we stripped off all except the Volume name? */
        if(isDirSep(lastp[1])) { /* Was ...":/"... so we want the root of the volume. */
          result[len++] = '/';
        }
        else {
          result[len++] = '.';
        }
      }
      result[len] = '\0';
      return (result);
    }
  } while (--lastp >= path);

  /* No /'s found, return a pointer to the string ".". */
  return (singledot);
}
#endif
