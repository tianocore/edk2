/*  @file
    Misc. external declarations.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    NetBSD: extern.h,v 1.14 2006/11/22 17:23:25 christos Exp
 */
#include  <Uefi.h>
#include  <time.h>

__BEGIN_DECLS
extern char * __minbrk;
int           __getcwd(char *, size_t);
int           __getlogin(char *, size_t);
int           __setlogin(const char *);
void          _resumecontext(void);
const char  * __strerror(int , char *, size_t);
const char  * __strsignal(int , char *, size_t);
char        * __dtoa(double, int, int, int *, int *, char **);
void          __freedtoa(char *);
int           __sysctl(const int *, unsigned int, void *, size_t *, const void *, size_t);

#ifdef  HAVE_SIGACTION
  struct      sigaction;
  int         __sigaction_sigtramp (int, const struct sigaction *,
                                    struct sigaction *, const void *, int);
#endif  /* HAVE_SIGACTION */

#ifdef WIDE_DOUBLE
  char      * __hdtoa(double, const char *, int, int *, int *, char **);
  char      * __hldtoa(long double, const char *, int, int *, int *,  char **);
  char      * __ldtoa(long double *, int, int, int *, int *, char **);
#endif

#ifdef  HAVE_SYSLOG
  struct syslog_data;
  void  syslog_ss(int, struct syslog_data *, const char *, ...)
      __attribute__((__format__(__printf__,3,4)));
  void  vsyslog_ss(int, struct syslog_data *, const char *, _BSD_VA_LIST_);
#endif  /* HAVE_SYSLOG */

#ifdef  HAVE_SNPRINTF_SS
  int snprintf_ss(char * __restrict, size_t, const char * __restrict, ...)
                  __attribute__((__format__(__printf__, 3, 4)));
#endif  /* HAVE_SNPRINTF_SS */

#ifdef  HAVE_VSNPRINTF_SS
  int vsnprintf_ss(char * __restrict, size_t, const char * __restrict,
                    _BSD_VA_LIST_) __attribute__((__format__(__printf__, 3, 0)));
#endif  /* HAVE_VSNPRINTF_SS */

void    Efi2Tm( EFI_TIME *ET, struct tm *BT);
time_t  Efi2Time( EFI_TIME *EfiBDtime);

__END_DECLS
