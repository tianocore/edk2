/** @file
    Wide character classification and conversion functions.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1989 The Regents of the University of California.
    All rights reserved.
    (c) UNIX System Laboratories, Inc.
    All or some portions of this file are derived from material licensed
    to the University of California by American Telephone and Telegraph
    Co. or Unix System Laboratories, Inc. and are reproduced herein with
    the permission of UNIX System Laboratories, Inc.

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

    NetBSD: iswctype_sb.c,v 1.4 2004/07/21 20:27:46 tshiozak Exp
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: iswctype_sb.c,v 1.4 2004/07/21 20:27:46 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include  <ctype.h>
#include  <string.h>
#include  <wchar.h>
#include  <wctype.h>

#undef iswalnum
int
iswalnum(wint_t c)
{
  return isalnum((int)c);
}

#undef iswalpha
int
iswalpha(wint_t c)
{
  return isalpha((int)c);
}

#undef iswblank
int
iswblank(wint_t c)
{
  return isblank((int)c);
}

#undef iswcntrl
int
iswcntrl(wint_t c)
{
  return iscntrl((int)c);
}

#undef iswdigit
int
iswdigit(wint_t c)
{
  return isdigit((int)c);
}

#undef iswgraph
int
iswgraph(wint_t c)
{
  return isgraph((int)c);
}

#undef iswlower
int
iswlower(wint_t c)
{
  return islower((int)c);
}

#undef iswprint
int
iswprint(wint_t c)
{
  return isprint((int)c);
}

#undef iswpunct
int
iswpunct(wint_t c)
{
  return ispunct((int)c);
}

#undef iswspace
int
iswspace(wint_t c)
{
  return isspace((int)c);
}

#undef iswupper
int
iswupper(wint_t c)
{
  return isupper((int)c);
}

#undef iswxdigit
int
iswxdigit(wint_t c)
{
  return isxdigit((int)c);
}

#undef towupper
wint_t
towupper(wint_t c)
{
  return toupper((int)c);
}

#undef towlower
wint_t
towlower(wint_t c)
{
  return tolower((int)c);
}

#undef wcwidth
int
/*ARGSUSED*/
wcwidth(wchar_t c)
{
  return 1;
}

#undef iswctype
int
iswctype(wint_t c, wctype_t charclass)
{
  /*
  * SUSv3: If charclass is 0, iswctype() shall return 0.
  */
  return (__isCClass((int)c, (unsigned int)charclass));
}

// Additional functions in <wctype.h> but not in NetBSD _sb code.
static
struct _typestrval {
  char    *name;
  wctype_t  value;
} typestrval[] = {
  { "alnum",    (_CD | _CU | _CL | _XA) },
  { "alpha",    (_CU | _CL | _XA)       },
  { "blank",    (_CB)                   },
  { "cntrl",    (_CC)                   },
  { "digit",    (_CD)                   },
  { "graph",    (_CG)                   },
  { "lower",    (_CL)                   },
  { "print",    (_CS | _CG)             },
  { "punct",    (_CP)                   },
  { "space",    (_CW)                   },
  { "upper",    (_CU)                   },
  { "xdigit",   (_CD | _CX)             }
};

#define NUM_PROPVAL   (sizeof(typestrval) / sizeof(struct _typestrval))

static
struct _transtrval {
  char       *name;
  wctrans_t   function;
} transtrval[] = {
  { "tolower",  towlower },
  { "toupper",  towupper }
};

#define NUM_TRANSVAL   (sizeof(transtrval) / sizeof(struct _transtrval))

wctype_t wctype(const char *property)
{
  int i;

  for(i = 0; i < NUM_PROPVAL; ++i) {
    if( strcmp(typestrval[i].name, property) == 0) {
      return typestrval[i].value;
    }
  }
  return 0;
}

wint_t  towctrans(wint_t p1, wctrans_t tranfunc)
{
  return tranfunc(p1);
}

wctrans_t wctrans(const char *property)
{
  int i;

  for(i = 0; i < NUM_TRANSVAL; ++i) {
    if( strcmp(transtrval[i].name, property) == 0) {
      return transtrval[i].function;
    }
  }
  return NULL;
}
