/** @file
    Internal C-type locale functions.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution. The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.
    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1997 Christos Zoulas.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
      1. Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
      2. Redistributions in binary form must reproduce the above copyright
         notice, this list of conditions and the following disclaimer in the
         documentation and/or other materials provided with the distribution.
      3. All advertising materials mentioning features or use of this software
         must display the following acknowledgement:
       This product includes software developed by Christos Zoulas.
      4. The name of the author may not be used to endorse or promote products
         derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    NetBSD: ctypeio.c,v 1.7 2005/11/29 03:11:59 christos Exp
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: ctypeio.c,v 1.7 2005/11/29 03:11:59 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _CTYPE_PRIVATE
#include <ctype.h>
#include "ctypeio.h"

int
__loadctype(const char *name)
{
  FILE *fp;
  char id[sizeof(_CTYPE_ID) - 1];
  u_int32_t i, len;
  unsigned short *new_ctype = NULL;
  unsigned char *new_toupper = NULL, *new_tolower = NULL;

  _DIAGASSERT(name != NULL);

  if ((fp = fopen(name, "r")) == NULL)
    return 0;

  if (fread(id, sizeof(id), 1, fp) != 1)
    goto bad;

  if (memcmp(id, _CTYPE_ID, sizeof(id)) != 0)
    goto bad;

  if (fread(&i, sizeof(u_int32_t), 1, fp) != 1)
    goto bad;

  if ((i = ntohl(i)) != _CTYPE_REV)
    goto bad;

  if (fread(&len, sizeof(u_int32_t), 1, fp) != 1)
    goto bad;

  if ((len = ntohl(len)) != _CTYPE_NUM_CHARS)
    goto bad;

  if ((new_ctype = malloc(sizeof(UINT16) * (1 + len))) == NULL)
    goto bad;

  new_ctype[0] = 0;
  if (fread(&new_ctype[1], sizeof(UINT16), len, fp) != len)
    goto bad;

  if ((new_toupper = malloc(sizeof(UINT8) * (1 + len))) == NULL)
    goto bad;

  new_toupper[0] = (UINT8)EOF;
  if (fread(&new_toupper[1], sizeof(UINT8), len, fp) != len)
    goto bad;

  if ((new_tolower = malloc(sizeof(UINT8) * (1 + len))) == NULL)
    goto bad;

  new_tolower[0] = (UINT8)EOF;
  if (fread(&new_tolower[1], sizeof(UINT8), len, fp) != len)
    goto bad;

#if BYTE_ORDER == LITTLE_ENDIAN
  for (i = 1; i <= len; i++) {
    new_ctype[i] = ntohs(new_ctype[i]);
  }
#endif

  (void) fclose(fp);
  if (_cClass != _C_CharClassTable)
    free(__UNCONST(_cClass));
  _cClass = new_ctype;
  if (_uConvT != _C_ToUpperTable)
    free(__UNCONST(_uConvT));
  _uConvT = new_toupper;
  if (_lConvT != _C_ToLowerTable)
    free(__UNCONST(_lConvT));
  _lConvT = new_tolower;

  return 1;
bad:
  free(new_tolower);
  free(new_toupper);
  free(new_ctype);
  (void) fclose(fp);
  return 0;
}

int
__savectype(
  const char     *name,
  unsigned short *new_ctype,
  unsigned char  *new_toupper,
  unsigned char  *new_tolower
  )
{
  FILE *fp;
  u_int32_t i, len = _CTYPE_NUM_CHARS;

  _DIAGASSERT(name != NULL);
  _DIAGASSERT(new_ctype != NULL);
  _DIAGASSERT(new_toupper != NULL);
  _DIAGASSERT(new_tolower != NULL);

  if ((fp = fopen(name, "w")) == NULL)
    return 0;

  if (fwrite(_CTYPE_ID, sizeof(_CTYPE_ID) - 1, 1, fp) != 1)
    goto bad;

  i = htonl(_CTYPE_REV);
  if (fwrite(&i, sizeof(u_int32_t), 1, fp) != 1)
    goto bad;

  i = htonl(len);
  if (fwrite(&i, sizeof(u_int32_t), 1, fp) != 1)
    goto bad;

#if BYTE_ORDER == LITTLE_ENDIAN
  for (i = 1; i <= len; i++) {
    new_ctype[i] = htons(new_ctype[i]);
  }
#endif
  if (fwrite(&new_ctype[1], sizeof(UINT16), len, fp) != len)
    goto bad;

  if (fwrite(&new_toupper[1], sizeof(UINT8), len, fp) != len)
    goto bad;

  if (fwrite(&new_tolower[1], sizeof(UINT8), len, fp) != len)
    goto bad;

  (void) fclose(fp);
  return 1;
bad:
  (void) fclose(fp);
  return 0;
}
